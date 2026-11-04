package service

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"sync"
	"time"

	"tebox-api/internal/constants"
	"tebox-api/internal/service/types"
	"tebox-api/internal/utils"
)

// inputHub owns, per running instance, the connection to QEMU's input
// monitor (QMP_INPUT_SOCKET). Clients reach it only via the shared control
// endpoint (TEBOX-CTRL / ServeInputUDP); no per-instance listen port.
type inputHub struct {
	mu        sync.Mutex
	instances map[string]*inputInstance
}

type inputInstance struct {
	qmpPath string
	done    chan struct{}

	mu  sync.Mutex
	qmp net.Conn

	touchDown  bool
	touchX     float64
	touchY     float64
	touchStamp time.Time

	subMu sync.Mutex
	subs  map[chan string]struct{}
	clip  string
}

func (h *inputHub) start(id, qmpPath, projectRoot, adbTarget string) {
	h.mu.Lock()
	defer h.mu.Unlock()
	if h.instances == nil {
		h.instances = make(map[string]*inputInstance)
	}
	if h.instances[id] != nil {
		return
	}
	_ = projectRoot
	_ = adbTarget
	inst := &inputInstance{
		qmpPath: qmpPath,
		subs:    make(map[chan string]struct{}),
		done:    make(chan struct{}),
	}
	go inst.touchWatchdog()
	h.instances[id] = inst
}

func (h *inputHub) stop(id string) {
	h.mu.Lock()
	inst := h.instances[id]
	delete(h.instances, id)
	h.mu.Unlock()
	if inst == nil {
		return
	}
	close(inst.done)
	inst.mu.Lock()
	if inst.qmp != nil {
		_ = inst.qmp.Close()
		inst.qmp = nil
	}
	inst.mu.Unlock()
	inst.subMu.Lock()
	for ch := range inst.subs {
		close(ch)
		delete(inst.subs, ch)
	}
	inst.subMu.Unlock()
}

func (h *inputHub) stopAll() {
	h.mu.Lock()
	ids := make([]string, 0, len(h.instances))
	for id := range h.instances {
		ids = append(ids, id)
	}
	h.mu.Unlock()
	for _, id := range ids {
		h.stop(id)
	}
}

func (h *inputHub) resolve(ev types.InputEvent) []*inputInstance {
	h.mu.Lock()
	defer h.mu.Unlock()
	ids := ev.IDs
	if ev.ID != "" {
		ids = append([]string{ev.ID}, ids...)
	}
	var out []*inputInstance
	seen := map[*inputInstance]bool{}
	for _, id := range ids {
		if id == constants.InstanceIDAll {
			for _, inst := range h.instances {
				if !seen[inst] {
					seen[inst] = true
					out = append(out, inst)
				}
			}
		} else if inst := h.instances[id]; inst != nil && !seen[inst] {
			seen[inst] = true
			out = append(out, inst)
		}
	}
	return out
}

func (h *inputHub) get(id string) *inputInstance {
	h.mu.Lock()
	defer h.mu.Unlock()
	return h.instances[id]
}

// ControlPort is always 0: input uses the shared control endpoint only.
func (m *Manager) ControlPort(id string) int {
	_ = id
	return 0
}

// SendInput injects one touch or key event into a running instance.
func (m *Manager) SendInput(id string, ev types.InputEvent) error {
	inst := m.input.get(id)
	if inst == nil {
		return constants.ErrInputNotRunning
	}
	return inst.send(ev)
}

// WatchClipboard streams clipboard texts published on this control session.
func (m *Manager) WatchClipboard(id string) (<-chan string, func(), error) {
	inst := m.input.get(id)
	if inst == nil {
		return nil, nil, constants.ErrInputNotRunning
	}
	ch, cancel := inst.subscribe()
	return ch, cancel, nil
}

// ServeInput reads JSON lines from r until EOF and injects each event.
func (m *Manager) ServeInput(id string, r *bufio.Reader, w io.Writer) {
	inst := m.input.get(id)
	if inst == nil {
		_ = utils.WriteJSONLine(w, utils.ReplyError(constants.ErrInputNotRunning.Error(), "", 0))
		return
	}
	inst.serve(r, w)
}

func (inst *inputInstance) serve(r *bufio.Reader, w io.Writer) {
	var wmu sync.Mutex
	write := func(v any) error {
		return utils.WriteJSONLineLocked(&wmu, w, v)
	}
	if write(utils.ReplyOK()) != nil {
		return
	}
	clips, cancel := inst.subscribe()
	defer cancel()
	go func() {
		for text := range clips {
			_ = write(utils.ReplyClipboard(text, ""))
		}
	}()
	for {
		line, err := r.ReadBytes('\n')
		if len(line) > 1 {
			var ev types.InputEvent
			sendErr := json.Unmarshal(line, &ev)
			if sendErr == nil {
				sendErr = inst.send(ev)
			}
			if sendErr != nil {
				if write(utils.ReplyError(sendErr.Error(), "", 0)) != nil {
					return
				}
			}
		}
		if err != nil {
			return
		}
	}
}

func (inst *inputInstance) subscribe() (chan string, func()) {
	ch := make(chan string, 8)
	inst.subMu.Lock()
	inst.subs[ch] = struct{}{}
	inst.subMu.Unlock()
	var once sync.Once
	return ch, func() {
		once.Do(func() {
			inst.subMu.Lock()
			if _, ok := inst.subs[ch]; ok {
				delete(inst.subs, ch)
				close(ch)
			}
			inst.subMu.Unlock()
		})
	}
}

func (inst *inputInstance) publishClipboard(text string) {
	inst.subMu.Lock()
	defer inst.subMu.Unlock()
	if text == inst.clip {
		return
	}
	inst.clip = text
	for ch := range inst.subs {
		select {
		case ch <- text:
		default:
		}
	}
}

func (inst *inputInstance) send(ev types.InputEvent) error {
	inst.mu.Lock()
	defer inst.mu.Unlock()
	switch ev.Type {
	case constants.EventTouch, constants.EventKey:
		cmd, err := inst.buildQMPCommand(ev)
		if err != nil {
			return err
		}
		if ev.Type == constants.EventTouch {
			inst.touchDown = ev.Action != constants.ActionUp
			inst.touchX, inst.touchY, inst.touchStamp = ev.X, ev.Y, time.Now()
		}
		return inst.writeQMPLocked(cmd)
	case constants.EventKeycode:
		return inst.sendKeycodeLocked(ev)
	case constants.EventText:
		return inst.sendTextLocked(ev.Text)
	case constants.EventClipboard:
		if err := inst.sendTextLocked(ev.Text); err != nil {
			return err
		}
		if ev.Paste {
			return inst.writeQMPKeysLocked([]string{"ret"}, true)
		}
		return nil
	default:
		return fmt.Errorf("unknown event type %q", ev.Type)
	}
}

func (inst *inputInstance) sendKeycodeLocked(ev types.InputEvent) error {
	if ev.Keycode <= 0 {
		return fmt.Errorf("invalid keycode %d", ev.Keycode)
	}
	if qnum, ok := utils.QNumForAndroidKey(ev.Keycode); ok {
		return inst.sendQNumActionLocked(qnum, ev.Action)
	}
	seq, err := utils.QCodesForAndroidKey(ev.Keycode, ev.Meta)
	if err != nil {
		return err
	}
	switch ev.Action {
	case constants.ActionDown:
		return inst.writeQMPKeySeqLocked(seq, true, false)
	case constants.ActionUp:
		return inst.writeQMPKeySeqLocked(seq, false, true)
	case "", constants.ActionPress:
		return inst.writeQMPKeysLocked(seq, true)
	default:
		return fmt.Errorf("unknown keycode action %q", ev.Action)
	}
}

func (inst *inputInstance) sendQNumActionLocked(qnum int, action string) error {
	switch action {
	case constants.ActionDown:
		return inst.writeQMPKeyNumLocked(qnum, true)
	case constants.ActionUp:
		return inst.writeQMPKeyNumLocked(qnum, false)
	case "", constants.ActionPress:
		if err := inst.writeQMPKeyNumLocked(qnum, true); err != nil {
			return err
		}
		return inst.writeQMPKeyNumLocked(qnum, false)
	default:
		return fmt.Errorf("unknown keycode action %q", action)
	}
}

func (inst *inputInstance) writeQMPKeyNumLocked(qnum int, down bool) error {
	cmd, err := utils.MarshalInputSendEvent([]types.QMPInputEvent{
		utils.KeyEvent(utils.KeyNumber(qnum), down),
	})
	if err != nil {
		return err
	}
	return inst.writeQMPLocked(cmd)
}

func (inst *inputInstance) sendTextLocked(text string) error {
	seqs, err := utils.QCodesForText(text)
	if err != nil {
		return err
	}
	for _, seq := range seqs {
		if err := inst.writeQMPKeysLocked(seq, true); err != nil {
			return err
		}
	}
	return nil
}

func (inst *inputInstance) writeQMPKeysLocked(qcodes []string, press bool) error {
	if len(qcodes) == 0 {
		return nil
	}
	if !press {
		return inst.writeQMPKeySeqLocked(qcodes, false, true)
	}
	if err := inst.writeQMPKeySeqLocked(qcodes, true, false); err != nil {
		return err
	}
	return inst.writeQMPKeySeqLocked(qcodes, false, true)
}

func (inst *inputInstance) writeQMPKeySeqLocked(qcodes []string, down, reverse bool) error {
	cmd, err := utils.MarshalInputSendEvent(utils.QCodeSequence(qcodes, down, reverse))
	if err != nil {
		return err
	}
	return inst.writeQMPLocked(cmd)
}

func (inst *inputInstance) touchWatchdog() {
	tick := time.NewTicker(constants.TouchWatchdogInterval)
	defer tick.Stop()
	for {
		select {
		case <-inst.done:
			return
		case <-tick.C:
		}
		inst.mu.Lock()
		if inst.touchDown && time.Since(inst.touchStamp) > constants.TouchHoldTimeout {
			inst.touchDown = false
			if cmd, err := inst.buildQMPCommand(types.InputEvent{
				Type: constants.EventTouch, Action: constants.ActionUp,
				X: inst.touchX, Y: inst.touchY,
			}); err == nil {
				_ = inst.writeQMPLocked(cmd)
			}
		}
		inst.mu.Unlock()
	}
}

func (inst *inputInstance) writeQMPLocked(cmd []byte) (err error) {
	for attempt := 0; attempt < constants.QMPWriteRetries; attempt++ {
		if inst.qmp == nil {
			if inst.qmp, err = utils.OpenInputQMP(inst.qmpPath); err != nil {
				return fmt.Errorf("input monitor: %w", err)
			}
		}
		if _, err = inst.qmp.Write(cmd); err == nil {
			return nil
		}
		_ = inst.qmp.Close()
		inst.qmp = nil
	}
	return fmt.Errorf("input monitor: %w", err)
}

func (inst *inputInstance) buildQMPCommand(ev types.InputEvent) ([]byte, error) {
	_ = inst
	var events []types.QMPInputEvent
	var err error
	switch ev.Type {
	case constants.EventTouch:
		events, err = utils.BuildTouchEvents(ev.Action, ev.X, ev.Y)
	case constants.EventKey:
		events, err = utils.BuildNavKeyEvents(ev.Key)
	default:
		return nil, fmt.Errorf("unknown event type %q", ev.Type)
	}
	if err != nil {
		return nil, err
	}
	return utils.MarshalInputSendEvent(events)
}

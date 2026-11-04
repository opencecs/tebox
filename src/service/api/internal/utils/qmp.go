package utils

import (
	"bufio"
	"encoding/json"
	"fmt"
	"io"
	"net"
	"strings"
	"time"

	"tebox-api/internal/constants"
	"tebox-api/internal/service/types"
)

// KeyQCode builds a qcode KeyValue.
func KeyQCode(code string) *types.QMPKeyValue {
	return &types.QMPKeyValue{Type: constants.QMPKeyQCode, Data: code}
}

// KeyNumber builds a number KeyValue (qnum).
func KeyNumber(qnum int) *types.QMPKeyValue {
	return &types.QMPKeyValue{Type: constants.QMPKeyNumber, Data: qnum}
}

// KeyEvent builds a key input-send-event entry.
func KeyEvent(key *types.QMPKeyValue, down bool) types.QMPInputEvent {
	return types.QMPInputEvent{
		Type: constants.QMPEventKey,
		Data: types.QMPEventData{Key: key, Down: BoolPtr(down)},
	}
}

// AbsEvent builds an abs input-send-event entry.
func AbsEvent(axis string, value int) types.QMPInputEvent {
	return types.QMPInputEvent{
		Type: constants.QMPEventAbs,
		Data: types.QMPEventData{Axis: axis, Value: IntPtr(value)},
	}
}

// BtnEvent builds a btn input-send-event entry.
func BtnEvent(button string, down bool) types.QMPInputEvent {
	return types.QMPInputEvent{
		Type: constants.QMPEventBtn,
		Data: types.QMPEventData{Button: button, Down: BoolPtr(down)},
	}
}

// MarshalInputSendEvent builds a QMP input-send-event line (with trailing \n).
func MarshalInputSendEvent(events []types.QMPInputEvent) ([]byte, error) {
	return jsonAppendNL(types.QMPRequest{
		Execute:   constants.QMPInputSendEvent,
		Arguments: types.InputSendEventArgs{Events: events},
	})
}

// KeyDownUp returns press then release events for one key value.
func KeyDownUp(key *types.QMPKeyValue) []types.QMPInputEvent {
	return []types.QMPInputEvent{KeyEvent(key, true), KeyEvent(key, false)}
}

// QCodeSequence builds key events for qcodes; reverse=true releases in reverse order.
func QCodeSequence(qcodes []string, down, reverse bool) []types.QMPInputEvent {
	order := qcodes
	if reverse {
		order = make([]string, len(qcodes))
		for i := range qcodes {
			order[i] = qcodes[len(qcodes)-1-i]
		}
	}
	events := make([]types.QMPInputEvent, 0, len(order))
	for _, code := range order {
		events = append(events, KeyEvent(KeyQCode(code), down))
	}
	return events
}

// ClampAbs maps a 0..1 coordinate to QEMU absolute axis units.
func ClampAbs(v float64) int {
	if v < 0 {
		v = 0
	} else if v > 1 {
		v = 1
	}
	return int(v*constants.QEMUAbsMax + 0.5)
}

// BuildTouchEvents builds abs (+ optional btn) events for a touch InputEvent.
func BuildTouchEvents(action string, x, y float64) ([]types.QMPInputEvent, error) {
	events := []types.QMPInputEvent{
		AbsEvent(constants.QMPAxisX, ClampAbs(x)),
		AbsEvent(constants.QMPAxisY, ClampAbs(y)),
	}
	switch action {
	case constants.ActionDown:
		events = append(events, BtnEvent(constants.QMPButtonLeft, true))
	case constants.ActionUp:
		events = append(events, BtnEvent(constants.QMPButtonLeft, false))
	case constants.ActionMove:
	default:
		return nil, fmt.Errorf("unknown touch action %q", action)
	}
	return events, nil
}

// BuildNavKeyEvents builds down+up events for a friendly / raw key name.
func BuildNavKeyEvents(name string) ([]types.QMPInputEvent, error) {
	qcode, qnum, useNum, err := ResolveNavKey(name)
	if err != nil {
		return nil, err
	}
	if useNum {
		return KeyDownUp(KeyNumber(qnum)), nil
	}
	return KeyDownUp(KeyQCode(qcode)), nil
}

// OpenInputQMP dials the input monitor, negotiates capabilities, and drains replies.
func OpenInputQMP(path string) (net.Conn, error) {
	conn, err := net.DialTimeout("unix", path, constants.QMPDialTimeout)
	if err != nil {
		return nil, err
	}
	r := bufio.NewReader(conn)
	_ = conn.SetDeadline(time.Now().Add(constants.QMPHandshakeTimeout))
	if _, err = r.ReadBytes('\n'); err == nil {
		capLine, _ := json.Marshal(types.QMPRequest{Execute: constants.QMPCapabilities})
		if _, err = conn.Write(append(capLine, '\n')); err == nil {
			_, err = r.ReadBytes('\n')
		}
	}
	if err != nil {
		conn.Close()
		return nil, err
	}
	_ = conn.SetDeadline(time.Time{})
	go func() { _, _ = io.Copy(io.Discard, r) }()
	return conn, nil
}

// ExecQMP dials a QMP socket, enables capabilities, sends request, checks error.
func ExecQMP(socketPath string, timeout time.Duration, request []byte) error {
	conn, err := net.DialTimeout("unix", socketPath, constants.QMPDialFastTimeout)
	if err != nil {
		return err
	}
	defer conn.Close()
	_ = conn.SetDeadline(time.Now().Add(timeout))

	buf := make([]byte, 64<<10)
	_, _ = conn.Read(buf)
	capLine, _ := json.Marshal(types.QMPRequest{Execute: constants.QMPCapabilities})
	if _, err := conn.Write(append(capLine, '\n')); err != nil {
		return err
	}
	_, _ = conn.Read(buf)
	if _, err := conn.Write(request); err != nil {
		return err
	}
	n, err := conn.Read(buf)
	if err != nil {
		return err
	}
	return ParseQMPError(buf[:n])
}

// ParseQMPError returns the first error object found in raw QMP reply bytes.
func ParseQMPError(raw []byte) error {
	for _, line := range strings.Split(string(raw), "\n") {
		line = strings.TrimSpace(line)
		if line == "" {
			continue
		}
		var msg types.QMPReply
		if json.Unmarshal([]byte(line), &msg) != nil || msg.Error == nil {
			continue
		}
		if msg.Error.Desc != "" {
			return fmt.Errorf("qmp: %s", msg.Error.Desc)
		}
		return fmt.Errorf("qmp: %s", msg.Error.Class)
	}
	return nil
}

// MarshalSendKey builds a send-key QMP line for one qcode.
func MarshalSendKey(qcode string) ([]byte, error) {
	return jsonAppendNL(types.QMPRequest{
		Execute: constants.QMPSendKey,
		Arguments: types.SendKeyArgs{
			Keys: []types.QMPKeyValueWrap{{Type: constants.QMPKeyQCode, Data: qcode}},
		},
	})
}

// MarshalExecute builds a no-argument QMP execute line.
func MarshalExecute(command string) ([]byte, error) {
	return jsonAppendNL(types.QMPRequest{Execute: command})
}

// MarshalScreendump builds a screendump QMP line.
func MarshalScreendump(filename, format string) ([]byte, error) {
	return jsonAppendNL(types.QMPRequest{
		Execute:   constants.QMPScreendump,
		Arguments: types.ScreendumpArgs{Filename: filename, Format: format},
	})
}

func jsonAppendNL(v any) ([]byte, error) {
	out, err := json.Marshal(v)
	if err != nil {
		return nil, err
	}
	return append(out, '\n'), nil
}

// QMPSocket is a unix QMP monitor path with bound helpers.
type QMPSocket struct {
	Path string
}

func (q QMPSocket) exec(timeout time.Duration, request []byte) error {
	return ExecQMP(q.Path, timeout, request)
}

// SendKey sends one qcode press via send-key.
func (q QMPSocket) SendKey(key string) error {
	req, err := MarshalSendKey(key)
	if err != nil {
		return err
	}
	return q.exec(constants.QMPCommandTimeout, req)
}

// PowerDown issues system_powerdown.
func (q QMPSocket) PowerDown() error {
	req, err := MarshalExecute(constants.QMPPowerdown)
	if err != nil {
		return err
	}
	return q.exec(constants.QMPCommandTimeout, req)
}

// SystemReset issues system_reset.
func (q QMPSocket) SystemReset() error {
	req, err := MarshalExecute(constants.QMPReset)
	if err != nil {
		return err
	}
	return q.exec(constants.QMPCommandTimeout, req)
}

// Screendump writes a PPM (or other format) framebuffer dump.
func (q QMPSocket) Screendump(filename, format string) error {
	req, err := MarshalScreendump(filename, format)
	if err != nil {
		return err
	}
	return q.exec(constants.QMPScreendumpTimeout, req)
}

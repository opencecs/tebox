package service

import (
	"bytes"
	"encoding/json"
	"fmt"
	"net"
	"sync"
	"time"

	"tebox-api/internal/constants"
	"tebox-api/internal/service/types"
	"tebox-api/internal/utils"
)

// UDP input on the shared endpoint: each datagram names targets with id/ids.

type udpSubscriber struct {
	cancel  func()
	lastSeq uint64
	seen    time.Time
}

type udpServer struct {
	conn    *net.UDPConn
	resolve func(types.InputEvent) []*inputInstance
	ids     func(*inputInstance) string

	mu   sync.Mutex
	subs map[string]*udpSubscriber
}

// ServeInputUDP serves the shared UDP input endpoint until the socket fails.
func (m *Manager) ServeInputUDP(addr string) error {
	udpAddr, err := net.ResolveUDPAddr("udp", addr)
	if err != nil {
		return err
	}
	conn, err := net.ListenUDP("udp", udpAddr)
	if err != nil {
		return err
	}
	srv := &udpServer{
		conn:    conn,
		resolve: m.input.resolve,
		ids:     m.input.idOf,
		subs:    map[string]*udpSubscriber{},
	}
	go srv.expire()
	go srv.loop()
	return nil
}

func (h *inputHub) idOf(inst *inputInstance) string {
	h.mu.Lock()
	defer h.mu.Unlock()
	for id, v := range h.instances {
		if v == inst {
			return id
		}
	}
	return ""
}

func (srv *udpServer) loop() {
	buf := make([]byte, constants.UDPMaxDatagram)
	for {
		n, addr, err := srv.conn.ReadFromUDP(buf)
		if err != nil {
			srv.closeAll()
			return
		}
		for _, line := range bytes.Split(buf[:n], []byte{'\n'}) {
			if len(bytes.TrimSpace(line)) > 0 {
				srv.handle(addr, line)
			}
		}
	}
}

func (srv *udpServer) reply(addr *net.UDPAddr, v any) {
	_ = utils.WriteJSONLine(udpAddrWriter{srv.conn, addr}, v)
}

type udpAddrWriter struct {
	conn *net.UDPConn
	addr *net.UDPAddr
}

func (w udpAddrWriter) Write(p []byte) (int, error) {
	return w.conn.WriteToUDP(p, w.addr)
}

func (srv *udpServer) handle(addr *net.UDPAddr, line []byte) {
	var ev types.InputEvent
	if err := json.Unmarshal(line, &ev); err != nil {
		srv.reply(addr, utils.ReplyError("bad json", "", 0))
		return
	}
	targets := srv.resolve(ev)
	if len(targets) == 0 {
		srv.reply(addr, utils.ReplyError(constants.ErrInputNotRunning.Error(), ev.ID, ev.Seq))
		return
	}
	for _, inst := range targets {
		if !srv.track(addr, inst, ev.Seq) {
			continue
		}
		if ev.Type == constants.EventPing || ev.Type == constants.EventSubscribe {
			continue
		}
		if err := inst.send(ev); err != nil {
			srv.reply(addr, utils.ReplyError(err.Error(), srv.idFor(inst), ev.Seq))
		}
	}
}

func (srv *udpServer) idFor(inst *inputInstance) string {
	if srv.ids == nil {
		return ""
	}
	return srv.ids(inst)
}

func (srv *udpServer) track(addr *net.UDPAddr, inst *inputInstance, seq uint64) bool {
	key := addr.String() + "|" + fmt.Sprintf("%p", inst)
	srv.mu.Lock()
	sub := srv.subs[key]
	if sub == nil {
		ch, cancel := inst.subscribe()
		sub = &udpSubscriber{cancel: cancel}
		srv.subs[key] = sub
		id := srv.idFor(inst)
		go func() {
			for text := range ch {
				srv.reply(addr, utils.ReplyClipboard(text, id))
			}
		}()
	}
	sub.seen = time.Now()
	ok := seq == 0 || seq > sub.lastSeq
	if ok && seq != 0 {
		sub.lastSeq = seq
	}
	srv.mu.Unlock()
	return ok
}

func (srv *udpServer) expire() {
	tick := time.NewTicker(constants.UDPExpireInterval)
	defer tick.Stop()
	for range tick.C {
		srv.mu.Lock()
		if srv.subs == nil {
			srv.mu.Unlock()
			return
		}
		for key, sub := range srv.subs {
			if time.Since(sub.seen) > constants.UDPSubscriberTTL {
				sub.cancel()
				delete(srv.subs, key)
			}
		}
		srv.mu.Unlock()
	}
}

func (srv *udpServer) closeAll() {
	srv.mu.Lock()
	defer srv.mu.Unlock()
	for _, sub := range srv.subs {
		sub.cancel()
	}
	srv.subs = nil
}

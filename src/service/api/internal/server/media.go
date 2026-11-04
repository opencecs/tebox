package server

import (
	"bufio"
	"fmt"
	"io"
	"net"
	"strings"
	"time"

	"tebox-api/internal/constants"
	"tebox-api/internal/service"
	"tebox-api/internal/utils"
)

// Media splits one tebox-cast stream across the LAN endpoints.
// Each connection names its instance on the first line:
//
//	cast    TEBOX-CAST/1 <id>    video records out; nothing is read back
//	audio   TEBOX-AUDIO/1 <id>   AAC records out; microphone PCM records in
//	control TEBOX-CTRL/1 <id>    input JSON lines
//	        TEBOX-CAMERA/1 <id>  camera JPEG records in
type Media struct {
	Mgr *service.Manager
}

const (
	laneCast = iota
	laneAudio
	laneControl
)

// ServeCast accepts video viewers.
func (m *Media) ServeCast(ln net.Listener) {
	m.serve(ln, laneCast)
}

// ServeAudio accepts audio and microphone connections.
func (m *Media) ServeAudio(ln net.Listener) {
	m.serve(ln, laneAudio)
}

// ServeControl accepts input JSON and camera JPEG connections.
func (m *Media) ServeControl(ln net.Listener) {
	m.serve(ln, laneControl)
}

func (m *Media) serve(ln net.Listener, lane int) {
	for {
		conn, err := ln.Accept()
		if err != nil {
			return
		}
		go func(conn net.Conn) {
			defer conn.Close()
			switch lane {
			case laneCast:
				m.cast(conn)
			case laneAudio:
				m.audio(conn)
			default:
				m.control(conn)
			}
		}(conn)
	}
}

func (m *Media) cast(conn net.Conn) {
	_, proto, id, ok := m.handshake(conn)
	if !ok || proto != constants.ProtoCast {
		return
	}
	backend, err := m.dial(id)
	if err != nil {
		return
	}
	defer backend.Close()
	// Video only. Anything the viewer writes is discarded so it cannot
	// reach the instance.
	done := make(chan struct{}, 2)
	go func() {
		utils.RelayVideo(conn, backend, constants.CastDownlinkMax)
		done <- struct{}{}
	}()
	go func() {
		_, _ = io.Copy(io.Discard, conn)
		done <- struct{}{}
	}()
	<-done
}

func (m *Media) audio(conn net.Conn) {
	reader, proto, id, ok := m.handshake(conn)
	if !ok || proto != constants.ProtoAudio {
		return
	}
	backend, err := m.dial(id)
	if err != nil {
		return
	}
	defer backend.Close()
	done := make(chan struct{}, 2)
	go func() {
		utils.RelayCodec(conn, backend, constants.CastDownlinkMax, constants.CodecAAC)
		done <- struct{}{}
	}()
	go func() {
		utils.RelayCodec(backend, reader, constants.CastUplinkMax, constants.CodecPCM)
		done <- struct{}{}
	}()
	<-done
}

func (m *Media) control(conn net.Conn) {
	reader, proto, id, ok := m.handshake(conn)
	if !ok {
		return
	}
	switch proto {
	case constants.ProtoCtrl:
		m.input(conn, reader, id)
	case constants.ProtoCamera:
		backend, err := m.dial(id)
		if err != nil {
			return
		}
		defer backend.Close()
		// tebox-cast streams video to every connection; drain it here.
		go func() { _, _ = io.Copy(io.Discard, backend) }()
		utils.RelayCodec(backend, reader, constants.CastUplinkMax, constants.CodecMJPEG)
	}
}

func (m *Media) input(conn net.Conn, reader *bufio.Reader, id string) {
	if tcp, ok := conn.(*net.TCPConn); ok {
		_ = tcp.SetNoDelay(true)
	}
	m.Mgr.ServeInput(id, reader, conn)
}

// handshake reads "<proto> <id>\n".
func (m *Media) handshake(conn net.Conn) (*bufio.Reader, string, string, bool) {
	_ = conn.SetReadDeadline(time.Now().Add(constants.CastHandshakeTimeout))
	reader := bufio.NewReader(conn)
	line, err := reader.ReadString('\n')
	if err != nil {
		return nil, "", "", false
	}
	_ = conn.SetReadDeadline(time.Time{})
	parts := strings.Fields(strings.TrimSpace(line))
	if len(parts) != 2 {
		return nil, "", "", false
	}
	id, err := utils.RequireID(parts[1])
	if err != nil {
		return nil, "", "", false
	}
	return reader, parts[0], id, true
}

func (m *Media) dial(id string) (net.Conn, error) {
	port, err := m.Mgr.CastPort(id)
	if err != nil {
		return nil, err
	}
	return net.DialTimeout("tcp", fmt.Sprintf("127.0.0.1:%d", port), constants.CastDialTimeout)
}

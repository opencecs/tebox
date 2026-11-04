// Package teboxctl is the wire protocol for the tebox control channel, the one
// guest <-> host link that carries every internal message between QEMU's guest
// Android and tebox-api. The guest exposes it as a single device/port; QEMU
// pipes that port to a host unix socket (ctl.sock); tebox-api reads framed
// messages here and dispatches them by channel. It replaces the old plain
// "quarter-turn digit per line" rotation port and the debug.tebox.rotation
// property bridge, which polluted Android's property space.
//
// Wire format (little-endian), a 12-byte header then payload:
//
//	off size field
//	0   2    magic   = {'T','B'} (0x54 0x42)
//	2   1    version = 1
//	3   1    flags   OPEN|DATA|CLOSE|ACK bitmask (stream lifecycle)
//	4   2    channel logical subsystem id (see Channel* below)
//	6   1    type    per-channel message type
//	7   1    _reserved (0)
//	8   4    length  payload byte count (capped at MaxPayload)
//	12  ...  payload
//
// The format is multiplexed (channel), streaming (many DATA frames over the
// life of a channel) and bidirectional (the same framing travels host->guest,
// used once a host->guest consumer exists). Adding a feature is adding a
// channel id, not touching QEMU or the kernel. The canonical C mirror of these
// constants lives in the guest kernel module and the composer forwarder.
package teboxctl

import (
	"encoding/binary"
	"fmt"
	"io"
)

// HeaderSize is the fixed frame header length in bytes.
const HeaderSize = 12

// MaxPayload caps a single frame's payload so a corrupt or hostile length
// cannot make the reader allocate without bound.
const MaxPayload = 1 << 20

// Magic0 / Magic1 are the frame's leading bytes ('T', 'B').
const (
	Magic0 = 'T'
	Magic1 = 'B'
)

// Version is the protocol version this build speaks.
const Version = 1

// Frame flags describe a channel's stream lifecycle. A one-shot value stream
// such as rotation only ever sets Data; longer-lived streams use Open/Close
// and may Ack.
const (
	FlagOpen  = 0x01
	FlagData  = 0x02
	FlagClose = 0x04
	FlagAck   = 0x08
)

// Channel ids. 0 is reserved for channel-level control; each real subsystem
// gets its own id and never reinterprets another's.
const (
	ChannelControl  = 0
	ChannelRotation = 1
)

// Rotation channel message types.
const (
	// RotationData carries one byte: the guest display rotation in
	// quarter-turns (0=0deg, 1=90, 2=180, 3=270).
	RotationData = 0
)

// Frame is one decoded protocol message.
type Frame struct {
	Flags   uint8
	Channel uint16
	Type    uint8
	Payload []byte
}

// ReadFrame reads and validates one frame from r. It returns io.EOF only when
// r is at a clean frame boundary; a short read mid-frame is io.ErrUnexpectedEOF.
func ReadFrame(r io.Reader) (Frame, error) {
	var h [HeaderSize]byte
	if _, err := io.ReadFull(r, h[:]); err != nil {
		return Frame{}, err
	}
	if h[0] != Magic0 || h[1] != Magic1 {
		return Frame{}, fmt.Errorf("teboxctl: bad magic %#x %#x", h[0], h[1])
	}
	if h[2] != Version {
		return Frame{}, fmt.Errorf("teboxctl: unsupported version %d", h[2])
	}
	length := binary.LittleEndian.Uint32(h[8:12])
	if length > MaxPayload {
		return Frame{}, fmt.Errorf("teboxctl: payload %d exceeds %d", length, MaxPayload)
	}
	f := Frame{
		Flags:   h[3],
		Channel: binary.LittleEndian.Uint16(h[4:6]),
		Type:    h[6],
	}
	if length > 0 {
		f.Payload = make([]byte, length)
		if _, err := io.ReadFull(r, f.Payload); err != nil {
			return Frame{}, err
		}
	}
	return f, nil
}

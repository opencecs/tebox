package utils

import (
	"tebox-api/internal/constants"
	"tebox-api/internal/service/types"
)

// ReplyOK is the control handshake / success ack.
func ReplyOK() types.ControlMessage {
	return types.ControlMessage{Ok: BoolPtr(true)}
}

// ReplyError is a rejected event (optional id/seq for UDP).
func ReplyError(err string, id string, seq uint64) types.ControlMessage {
	return types.ControlMessage{Ok: BoolPtr(false), Error: err, ID: id, Seq: seq}
}

// ReplyClipboard pushes a clipboard text change to the client.
func ReplyClipboard(text, id string) types.ControlMessage {
	return types.ControlMessage{Type: constants.EventClipboard, Text: text, ID: id}
}

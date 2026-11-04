package types

// QMPRequest is a generic QMP command envelope.
type QMPRequest struct {
	Execute   string `json:"execute"`
	Arguments any    `json:"arguments,omitempty"`
}

// InputSendEventArgs is arguments for execute=input-send-event.
type InputSendEventArgs struct {
	Events []QMPInputEvent `json:"events"`
}

// QMPInputEvent is one input-send-event entry (key / abs / btn).
type QMPInputEvent struct {
	Type string       `json:"type"`
	Data QMPEventData `json:"data"`
}

// QMPEventData carries fields for key, abs, and btn events (omitempty).
type QMPEventData struct {
	Key    *QMPKeyValue `json:"key,omitempty"`
	Down   *bool        `json:"down,omitempty"`
	Axis   string       `json:"axis,omitempty"`
	Value  *int         `json:"value,omitempty"`
	Button string       `json:"button,omitempty"`
}

// QMPKeyValue is a QMP KeyValue (qcode string or number qnum).
type QMPKeyValue struct {
	Type string `json:"type"`
	Data any    `json:"data"`
}

// SendKeyArgs is arguments for execute=send-key.
type SendKeyArgs struct {
	Keys []QMPKeyValueWrap `json:"keys"`
}

// QMPKeyValueWrap matches QMP KeyValue union wire form for send-key.
type QMPKeyValueWrap struct {
	Type string `json:"type"`
	Data string `json:"data"`
}

// ScreendumpArgs is arguments for execute=screendump.
type ScreendumpArgs struct {
	Filename string `json:"filename"`
	Format   string `json:"format"`
}

// QMPErrorBody is the error object inside a QMP reply line.
type QMPErrorBody struct {
	Class string `json:"class"`
	Desc  string `json:"desc"`
}

// QMPReply is a parsed QMP response line (success or error).
type QMPReply struct {
	Error *QMPErrorBody `json:"error"`
}

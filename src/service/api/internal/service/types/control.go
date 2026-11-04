package types

// InputEvent is one control-port input event for the guest (JSON line / UDP).
type InputEvent struct {
	ID  string   `json:"id,omitempty"`
	IDs []string `json:"ids,omitempty"`
	Seq uint64   `json:"seq,omitempty"`

	Type    string  `json:"type"`
	Action  string  `json:"action,omitempty"`
	X       float64 `json:"x,omitempty"`
	Y       float64 `json:"y,omitempty"`
	Key     string  `json:"key,omitempty"`
	Text    string  `json:"text,omitempty"`
	Keycode int     `json:"keycode,omitempty"`
	Meta    int     `json:"meta,omitempty"`
	Paste   bool    `json:"paste,omitempty"`
}

// ControlMessage is the shared control-port reply / push envelope.
// Handshake and errors set Ok; clipboard pushes set Type+Text (+ optional ID).
type ControlMessage struct {
	Ok    *bool  `json:"ok,omitempty"`
	Error string `json:"error,omitempty"`
	Type  string `json:"type,omitempty"`
	Text  string `json:"text,omitempty"`
	ID    string `json:"id,omitempty"`
	Seq   uint64 `json:"seq,omitempty"`
}

package constants

import (
	"errors"
	"time"
)

// Sentinel errors.
var (
	ErrInvalidOptions  = errors.New("invalid instance options")
	ErrInputNotRunning = errors.New("instance is not running or does not exist")
)

// Default CreateInstance resource sizes.
const (
	DefaultCPUCores = 4
	DefaultMemoryMB = 4096
	DefaultDiskMB   = 8192
	LegacyDiskMB    = 1024 // pre-size-field userdata
)

// Env keys.
const (
	EnvADB     = "TEBOX_ADB"
	EnvRoot    = "TEBOX_ROOT"
	EnvDataDir = "TEBOX_DATA_DIR"
)

// DefaultCmdline is the fixed kernel / androidboot cmdline the API passes as
// TEBOX_CMDLINE when an instance was created without one of its own.
// boot-qemu.sh layers GPU / debug / extra tokens on top of these.
var DefaultCmdline = []string{
	"console=ttyAMA0,115200",
	"earlycon=pl011,0x9000000",
	"ignore_loglevel",
	"androidboot.console=ttyAMA0",
	"androidboot.hardware=ranchu",
	"androidboot.selinux=permissive",
	"androidboot.zygote=zygote64",
	"androidboot.init_fatal_reboot_target=",
	"rdinit=/init",
	"module.sig_enforce=0",
}

// Control / input event types and actions (JSON wire values).
const (
	EventTouch     = "touch"
	EventKey       = "key"
	EventKeycode   = "keycode"
	EventText      = "text"
	EventClipboard = "clipboard"
	EventPing      = "ping"
	EventSubscribe = "subscribe"

	ActionDown  = "down"
	ActionUp    = "up"
	ActionMove  = "move"
	ActionPress = "press"

	InstanceIDAll = "*"
)

// QMP execute names and KeyValue kinds.
const (
	QMPCapabilities   = "qmp_capabilities"
	QMPInputSendEvent = "input-send-event"
	QMPSendKey        = "send-key"
	QMPPowerdown      = "system_powerdown"
	QMPReset          = "system_reset"
	QMPScreendump     = "screendump"

	QMPKeyQCode  = "qcode"
	QMPKeyNumber = "number"

	QMPEventKey = "key"
	QMPEventAbs = "abs"
	QMPEventBtn = "btn"

	QMPAxisX      = "x"
	QMPAxisY      = "y"
	QMPButtonLeft = "left"
)

// QEMU virtio / Android input constants.
const (
	QEMUAbsMax = 0x7fff // INPUT_EVENT_ABS_MAX

	AndroidMetaShift = 0x41
	AndroidMetaCtrl  = 0x3000

	AndroidKeycodeAppSwitch = 187
	QNumKeyScale            = 139 // → Linux KEY_SCALE → RECENT_APPS
)

// Timing.
const (
	TouchHoldTimeout      = time.Second
	TouchWatchdogInterval = 250 * time.Millisecond
	UDPSubscriberTTL      = 15 * time.Second
	UDPExpireInterval     = 5 * time.Second
	UDPMaxDatagram        = 64 * 1024
	QMPDialTimeout        = time.Second
	QMPHandshakeTimeout   = 2 * time.Second
	QMPCommandTimeout     = 500 * time.Millisecond
	QMPDialFastTimeout    = 200 * time.Millisecond
	QMPScreendumpTimeout  = 8 * time.Second
	QMPWriteRetries       = 2

	CastHandshakeTimeout = 5 * time.Second
	CastDialTimeout      = 2 * time.Second
)

// Paths / markers.
const (
	RunDirMarker      = ".tebox-instance"
	MaxInstanceDirLen = 103 - len("/qmp-input.sock")
	DefaultADBBinary  = "adb"
	// ConsoleLogName is the per-instance serial console capture under its dir.
	ConsoleLogName = "console.log"
	// ConsoleLogTailDefault / Max bound GetConsoleLog's tail read (bytes).
	ConsoleLogTailDefault = 64 * 1024
	ConsoleLogTailMax     = 4 * 1024 * 1024
)

// Cast framing. Codecs match cast/include/cast/protocol.h.
const (
	ProtoCast   = "TEBOX-CAST/1"
	ProtoAudio  = "TEBOX-AUDIO/1"
	ProtoCtrl   = "TEBOX-CTRL/1"
	ProtoCamera = "TEBOX-CAMERA/1"

	CodecAAC      = 16
	CodecMJPEG    = 32
	CodecPCM      = 33
	CodecRotation = 48 // server -> viewer: guest rotation, 1 byte quarter-turns

	CastRecordHeader = 10
	CastUplinkMax    = 4 << 20 // CAST_UPLINK_MAX
	CastDownlinkMax  = 32 << 20
)

// Preview defaults.
const (
	PreviewFormatJPEG     = "jpeg"
	PreviewFormatPNG      = "png"
	PreviewDefaultQuality = 80
)

// NavKeyToQCode maps friendly control-port key names to QEMU qcodes.
var NavKeyToQCode = map[string]string{
	"home": "ac_home", "back": "ac_back", "menu": "menu", "power": "power",
	"volume_up": "volumeup", "volume_down": "volumedown", "mute": "audiomute",
}

// NavKeyToQNum maps friendly names with no QKeyCode to QMP key numbers.
var NavKeyToQNum = map[string]int{
	"recent": QNumKeyScale, "app_switch": QNumKeyScale,
}

// AndroidKeyToQCode maps Android KEYCODE_* to QEMU qcodes.
var AndroidKeyToQCode = map[int]string{
	3: "ac_home", 4: "ac_back", 82: "menu", 26: "power",
	24: "volumeup", 25: "volumedown", 164: "audiomute",
	66: "ret", 67: "backspace", 61: "tab",
	19: "up", 20: "down", 21: "left", 22: "right",
	112: "delete", 123: "end",
	29: "a", 30: "b", 31: "c", 32: "d", 33: "e", 34: "f", 35: "g", 36: "h",
	37: "i", 38: "j", 39: "k", 40: "l", 41: "m", 42: "n", 43: "o", 44: "p",
	45: "q", 46: "r", 47: "s", 48: "t", 49: "u", 50: "v", 51: "w", 52: "x",
	53: "y", 54: "z",
	7: "0", 8: "1", 9: "2", 10: "3", 11: "4", 12: "5", 13: "6", 14: "7",
	15: "8", 16: "9",
	62: "spc", 69: "minus", 70: "equal", 71: "bracket_left", 72: "bracket_right",
	73: "backslash", 74: "semicolon", 75: "apostrophe", 68: "grave",
	55: "comma", 56: "dot", 76: "slash",
}

// AndroidKeyToQNum maps Android keys without a QKeyCode to QMP qnums.
var AndroidKeyToQNum = map[int]int{
	AndroidKeycodeAppSwitch: QNumKeyScale,
}

// ShiftedRunes / PlainRunes: ASCII → virtio qcode (with/without shift).
var ShiftedRunes = map[rune]string{
	'!': "1", '@': "2", '#': "3", '$': "4", '%': "5",
	'^': "6", '&': "7", '*': "8", '(': "9", ')': "0",
	'_': "minus", '+': "equal", '{': "bracket_left", '}': "bracket_right",
	'|': "backslash", ':': "semicolon", '"': "apostrophe", '~': "grave",
	'<': "comma", '>': "dot", '?': "slash",
}

var PlainRunes = map[rune]string{
	'-': "minus", '=': "equal", '[': "bracket_left", ']': "bracket_right",
	'\\': "backslash", ';': "semicolon", '\'': "apostrophe", '`': "grave",
	',': "comma", '.': "dot", '/': "slash",
}

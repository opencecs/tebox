package utils

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"net"
	"os"
	"path/filepath"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"tebox-api/internal/constants"
)

// EnvOr returns the environment value, or fallback when it is unset or empty.
func EnvOr(key, fallback string) string {
	if value := os.Getenv(key); value != "" {
		return value
	}
	return fallback
}

// ADBBinary returns TEBOX_ADB or the default adb executable name.
func ADBBinary() string {
	return EnvOr(constants.EnvADB, constants.DefaultADBBinary)
}

// ResolveProjectRoot returns TEBOX_ROOT or the process working directory.
func ResolveProjectRoot() (string, error) {
	if root := os.Getenv(constants.EnvRoot); root != "" {
		return root, nil
	}
	return os.Getwd()
}

// DefaultDataRoot returns TEBOX_DATA_DIR or ~/.tebox.
func DefaultDataRoot() string {
	if data := os.Getenv(constants.EnvDataDir); data != "" {
		return data
	}
	home, err := os.UserHomeDir()
	if err != nil || home == "" {
		return ".tebox"
	}
	return filepath.Join(home, ".tebox")
}

// TCPPortFree reports whether 127.0.0.1:port can be bound.
func TCPPortFree(port int) bool {
	ln, err := net.Listen("tcp", fmt.Sprintf("127.0.0.1:%d", port))
	if err != nil {
		return false
	}
	_ = ln.Close()
	return true
}

// AllocateTCPPort finds a free port at or after start, skipping values in used.
func AllocateTCPPort(start int, used map[string]int) (int, error) {
	busy := map[int]bool{}
	for _, p := range used {
		busy[p] = true
	}
	for port := start; port < 65535; port++ {
		if !busy[port] && TCPPortFree(port) {
			return port, nil
		}
	}
	return 0, fmt.Errorf("no free cast port available")
}

// RelayVideo copies cast records whose codec is below CodecAAC (H.264 and any earlier video codec).
func RelayVideo(dst io.Writer, src io.Reader, max uint32) {
	relayRecords(dst, src, max, true, 0)
}

// RelayCodec copies cast records of exactly one codec.
func RelayCodec(dst io.Writer, src io.Reader, max uint32, codec byte) {
	relayRecords(dst, src, max, false, codec)
}

// relayRecords copies whole cast records from src to dst.
// It stops on EOF, a write error or a malformed record.
func relayRecords(dst io.Writer, src io.Reader, max uint32, video bool, want byte) {
	var lenbuf [4]byte
	buf := make([]byte, 0, 64<<10)
	for {
		if _, err := io.ReadFull(src, lenbuf[:]); err != nil {
			return
		}
		n := binary.BigEndian.Uint32(lenbuf[:])
		if n < constants.CastRecordHeader || n > max {
			return
		}
		if cap(buf) < int(4+n) {
			buf = make([]byte, 0, 4+n)
		}
		buf = append(buf[:0], lenbuf[:]...)
		buf = buf[:4+n]
		if _, err := io.ReadFull(src, buf[4:]); err != nil {
			return
		}
		codec := buf[4]
		if video {
			if codec >= constants.CodecAAC {
				continue
			}
		} else if codec != want {
			continue
		}
		if _, err := dst.Write(buf); err != nil {
			return
		}
	}
}

// DiskFreeMB is max(0, diskMB-usedMB): userdata still free inside the allocated image.
func DiskFreeMB(diskMB, usedMB int) int32 {
	free := diskMB - usedMB
	if free < 0 {
		return 0
	}
	return int32(free)
}

// InputQMPPath is the per-instance QMP input monitor socket.
func InputQMPPath(dir string) string {
	return filepath.Join(dir, "qmp-input.sock")
}

// MonitorQMPPath is the per-instance primary QMP monitor socket.
func MonitorQMPPath(dir string) string {
	return filepath.Join(dir, "qmp.sock")
}

// RPCError maps a manager error onto a gRPC status.
func RPCError(err error) error {
	switch {
	case errors.Is(err, os.ErrNotExist):
		return status.Error(codes.NotFound, err.Error())
	case errors.Is(err, constants.ErrInvalidOptions):
		return status.Error(codes.InvalidArgument, err.Error())
	case errors.Is(err, constants.ErrInputNotRunning):
		return status.Error(codes.FailedPrecondition, err.Error())
	}
	return status.Error(codes.Internal, err.Error())
}

package utils

import (
	"fmt"
	"net"
	"os"
	"path/filepath"
	"strings"
)

// ResolveProjectRoot returns TEBOX_ROOT or the process working directory.
func ResolveProjectRoot() (string, error) {
	if root := os.Getenv("TEBOX_ROOT"); root != "" {
		return root, nil
	}
	return os.Getwd()
}

// DefaultDataRoot returns TEBOX_DATA_DIR or ~/.tebox.
func DefaultDataRoot() string {
	if data := os.Getenv("TEBOX_DATA_DIR"); data != "" {
		return data
	}
	home, err := os.UserHomeDir()
	if err != nil || home == "" {
		return ".tebox"
	}
	return filepath.Join(home, ".tebox")
}

// ChooseVariant picks TEBOX_VARIANT (if it has KERNEL) or the first src/aosp/* with KERNEL.
func ChooseVariant(root string) (string, error) {
	if preferred := os.Getenv("TEBOX_VARIANT"); preferred != "" {
		if _, err := os.Stat(filepath.Join(root, "src", "aosp", preferred, "KERNEL")); err == nil {
			return preferred, nil
		}
	}
	entries, err := os.ReadDir(filepath.Join(root, "src", "aosp"))
	if err != nil {
		return "", fmt.Errorf("no Android variant under %s/src/aosp", root)
	}
	for _, entry := range entries {
		if !entry.IsDir() || strings.HasPrefix(entry.Name(), ".") {
			continue
		}
		if _, err := os.Stat(filepath.Join(root, "src", "aosp", entry.Name(), "KERNEL")); err == nil {
			return entry.Name(), nil
		}
	}
	return "", fmt.Errorf("no Android variant under %s/src/aosp", root)
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

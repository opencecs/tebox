package service

import (
	"context"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"time"

	api "tebox-api/api/v1"
	"tebox-api/internal/constants"
	"tebox-api/internal/service/types"
	"tebox-api/internal/utils"
)

func (m *Manager) normalizeCreate(o *types.CreateOptions) error {
	if o.CPUCores == 0 {
		o.CPUCores = constants.DefaultCPUCores
	}
	if o.MemoryMB == 0 {
		o.MemoryMB = constants.DefaultMemoryMB
	}
	if o.DiskMB == 0 {
		o.DiskMB = constants.DefaultDiskMB
	}
	if o.CPUCores < 1 || o.CPUCores > runtime.NumCPU() {
		return fmt.Errorf("%w: cpu_cores must be 1..%d (host cores)", constants.ErrInvalidOptions, runtime.NumCPU())
	}
	if o.MemoryMB < 1024 || o.MemoryMB > 65536 {
		return fmt.Errorf("%w: memory_mb must be 1024..65536", constants.ErrInvalidOptions)
	}
	if o.DiskMB < 1024 || o.DiskMB > 1024*1024 {
		return fmt.Errorf("%w: disk_mb must be 1024..1048576", constants.ErrInvalidOptions)
	}
	o.Name = strings.TrimSpace(o.Name)
	if o.InstanceDir = strings.TrimSpace(o.InstanceDir); o.InstanceDir != "" {
		if err := m.checkInstanceDir(o); err != nil {
			return fmt.Errorf("%w: %v", constants.ErrInvalidOptions, err)
		}
	}
	for _, img := range []*string{&o.SystemImage, &o.VendorImage, &o.InitramfsImage} {
		if *img = strings.TrimSpace(*img); *img == "" {
			continue
		}
		if !filepath.IsAbs(*img) {
			return fmt.Errorf("%w: image path must be absolute: %s", constants.ErrInvalidOptions, *img)
		}
		*img = filepath.Clean(*img)
		if err := utils.CheckReadableImage(*img); err != nil {
			return fmt.Errorf("%w: %v", constants.ErrInvalidOptions, err)
		}
	}
	return nil
}

func (m *Manager) checkInstanceDir(o *types.CreateOptions) error {
	_ = m
	dir := &o.InstanceDir
	if !filepath.IsAbs(*dir) {
		return fmt.Errorf("instance_dir must be absolute: %s", *dir)
	}
	*dir = filepath.Clean(*dir)
	if *dir == "/" {
		return fmt.Errorf("instance_dir cannot be /")
	}
	if len(*dir) > constants.MaxInstanceDirLen {
		return fmt.Errorf("instance_dir is %d bytes; at most %d fit the instance's Unix sockets", len(*dir), constants.MaxInstanceDirLen)
	}
	entries, err := os.ReadDir(*dir)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return fmt.Errorf("instance_dir %s: %w", *dir, err)
	}
	if len(entries) > 0 {
		return fmt.Errorf("instance_dir %s is not empty", *dir)
	}
	return nil
}

func (m *Manager) fillResources(item *types.Instance) {
	_ = m
	if item.CPUCores <= 0 {
		item.CPUCores = constants.DefaultCPUCores
	}
	if item.MemoryMB <= 0 {
		item.MemoryMB = constants.DefaultMemoryMB
	}
	img := filepath.Join(item.Dir, "userdata.img")
	st, err := os.Stat(img)
	if item.DiskMB <= 0 {
		item.DiskMB = constants.LegacyDiskMB
		if err == nil {
			item.DiskMB = int(st.Size() >> 20)
		}
	}
	if err == nil {
		item.DiskUsedMB = utils.FileUsedMB(img)
	}
	if item.State == api.InstanceState_INSTANCE_STATE_UNSPECIFIED {
		item.State = api.InstanceState_INSTANCE_STATE_STOPPED
	}
}

func (m *Manager) watchBoot(id string, cmd *exec.Cmd, target string) {
	adb := utils.ADBBinary()
	for {
		time.Sleep(2 * time.Second)
		m.mu.Lock()
		current := m.procs[id] == cmd
		m.mu.Unlock()
		if !current {
			return
		}
		ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
		_ = exec.CommandContext(ctx, adb, "connect", target).Run()
		out, err := exec.CommandContext(ctx, adb, "-s", target, "shell", "getprop", "sys.boot_completed").Output()
		cancel()
		if err == nil && strings.TrimSpace(string(out)) == "1" {
			m.mu.Lock()
			if m.procs[id] == cmd {
				m.booted[id] = true
			}
			m.mu.Unlock()
			return
		}
	}
}

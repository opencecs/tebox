package service

import (
	"bufio"
	"encoding/json"
	"fmt"
	"hash/fnv"
	"io"
	"net"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"

	api "tebox-api/api/v1"
	"tebox-api/internal/constants"
	"tebox-api/internal/service/types"
	"tebox-api/internal/teboxctl"
	"tebox-api/internal/utils"
)

type Manager struct {
	ProjectRoot string
	DataRoot    string

	mu        sync.Mutex
	procs     map[string]*exec.Cmd
	castProcs map[string]*exec.Cmd
	castPorts map[string]int
	castFails map[string]int // consecutive short-lived tebox-cast runs
	input     inputHub
	booted    map[string]bool // sys.boot_completed seen for the running process
	stopping  map[string]bool
}

func NewManager(projectRoot, dataRoot string) (*Manager, error) {
	m := &Manager{
		ProjectRoot: projectRoot,
		DataRoot:    dataRoot,
		procs:       make(map[string]*exec.Cmd),
		castProcs:   make(map[string]*exec.Cmd),
		castPorts:   make(map[string]int),
		castFails:   make(map[string]int),
		booted:      make(map[string]bool),
		stopping:    make(map[string]bool),
	}
	if err := os.MkdirAll(m.instancesDir(), 0o755); err != nil {
		return nil, err
	}
	return m, nil
}

// watchControl reads the guest's framed control-channel messages from the
// instance's ctl.sock (QEMU virtio-serial tebox.ctl, guest -> host) and
// dispatches each frame by channel. This is the one internal guest<->host
// link (see internal/teboxctl); the API never asks the guest over adb. It
// reconnects until the process is replaced.
func (m *Manager) watchControl(id string, cmd *exec.Cmd, sock string) {
	dir := filepath.Dir(sock)
	for {
		m.mu.Lock()
		current := m.procs[id] == cmd
		m.mu.Unlock()
		if !current {
			return
		}
		conn, err := net.DialTimeout("unix", sock, 2*time.Second)
		if err != nil {
			time.Sleep(time.Second)
			continue
		}
		r := bufio.NewReader(conn)
		for {
			f, rerr := teboxctl.ReadFrame(r)
			if rerr != nil {
				break
			}
			m.dispatchControl(dir, f)
		}
		_ = conn.Close()
		time.Sleep(time.Second)
	}
}

// dispatchControl routes one decoded control frame to its subsystem. Unknown
// channels are ignored so a newer guest can add channels without breaking an
// older host.
func (m *Manager) dispatchControl(dir string, f teboxctl.Frame) {
	switch f.Channel {
	case teboxctl.ChannelRotation:
		if f.Type == teboxctl.RotationData && len(f.Payload) == 1 && f.Payload[0] <= 3 {
			// QEMU reads cast.rotation to rotate cast/screendump upright.
			_ = os.WriteFile(filepath.Join(dir, "cast.rotation"),
				[]byte(strconv.Itoa(int(f.Payload[0]))+"\n"), 0o644)
		}
	}
}

func (m *Manager) instancesDir() string {
	return filepath.Join(m.DataRoot, "instances")
}

func (m *Manager) List() ([]types.Instance, error) {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.reapLocked()

	entries, err := os.ReadDir(m.instancesDir())
	if err != nil {
		if os.IsNotExist(err) {
			return nil, nil
		}
		return nil, err
	}
	out := make([]types.Instance, 0, len(entries))
	for _, entry := range entries {
		if !entry.IsDir() || strings.HasPrefix(entry.Name(), ".") {
			continue
		}
		item, err := m.readMetadata(filepath.Join(m.instancesDir(), entry.Name()))
		if err != nil {
			continue
		}
		out = append(out, m.liveLocked(item))
	}
	return out, nil
}

func (m *Manager) Get(id string) (types.Instance, error) {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.reapLocked()
	item, err := m.readMetadata(filepath.Join(m.instancesDir(), id))
	if err != nil {
		return types.Instance{}, err
	}
	return m.liveLocked(item), nil
}

func (m *Manager) liveLocked(item types.Instance) types.Instance {
	item.State = api.InstanceState_INSTANCE_STATE_STOPPED
	if cmd := m.procs[item.ID]; cmd != nil && cmd.Process != nil {
		item.PID = cmd.Process.Pid
		item.Running = true
		switch {
		case m.stopping[item.ID]:
			item.State = api.InstanceState_INSTANCE_STATE_STOPPING
		case m.booted[item.ID]:
			item.State = api.InstanceState_INSTANCE_STATE_RUNNING
		default:
			item.State = api.InstanceState_INSTANCE_STATE_STARTING
		}
	}
	item.CastPort = m.castPorts[item.ID]
	item.ControlPort = m.ControlPort(item.ID)
	return item
}

func (m *Manager) Create(opts types.CreateOptions) (types.Instance, error) {
	if err := m.normalizeCreate(&opts); err != nil {
		return types.Instance{}, err
	}
	now := time.Now()
	item := types.Instance{
		ID:       fmt.Sprintf("android-%d-%d", now.Unix(), now.Nanosecond()/1e6),
		Name:     fmt.Sprintf("Android %d", now.Unix()),
		Profile:  types.DeviceProfile{Latitude: 39.9042, Longitude: 116.4074, Country: "CN", OperatorNumeric: "46000", OperatorName: "China Mobile", CellId: "0x1001", PhoneNumber: "+8613800138000"},
		CPUCores: opts.CPUCores,
		MemoryMB: opts.MemoryMB,
		DiskMB:   opts.DiskMB,
		State:    api.InstanceState_INSTANCE_STATE_STOPPED,

		SystemImage:    opts.SystemImage,
		VendorImage:    opts.VendorImage,
		InitramfsImage: opts.InitramfsImage,
		Cmdline:        opts.Cmdline,
		Gpu:            opts.Gpu,
		Kernel:         opts.Kernel,
		Append:         opts.Append,
		RunDir:         opts.InstanceDir,
	}
	if opts.Name != "" {
		item.Name = opts.Name
	}
	item.MetaDir = filepath.Join(m.instancesDir(), item.ID)
	item.Dir = item.MetaDir
	if item.RunDir != "" {
		item.Dir = item.RunDir
	}
	port, err := m.allocateAdbPort(item.ID)
	if err != nil {
		return types.Instance{}, err
	}
	item.AdbPort = port
	if err := os.MkdirAll(item.MetaDir, 0o755); err != nil {
		return types.Instance{}, err
	}
	if err := os.MkdirAll(item.Dir, 0o755); err != nil {
		_ = os.RemoveAll(item.MetaDir)
		return types.Instance{}, err
	}
	if item.RunDir != "" {
		// Claims the directory: it is no longer empty for another
		// CreateInstance, and Delete checks it before removing anything.
		if err := os.WriteFile(filepath.Join(item.Dir, constants.RunDirMarker), []byte(item.ID+"\n"), 0o644); err != nil {
			_ = os.RemoveAll(item.MetaDir)
			return types.Instance{}, err
		}
	}
	if err := m.writeMetadata(item); err != nil {
		return types.Instance{}, err
	}
	return item, nil
}

// imageEnv passes the instance's own images to ./run, after checking they
// are still there. Empty paths are left to ./run, which boots the sole
// variant's own images under src/aosp/<variant>/images.
func (m *Manager) imageEnv(item types.Instance) ([]string, error) {
	var env []string
	for _, img := range []struct{ key, path string }{
		{"SYSTEM_IMG", item.SystemImage},
		{"VENDOR_IMG", item.VendorImage},
		{"INITRD_IMG", item.InitramfsImage},
	} {
		if img.path == "" {
			continue
		}
		if err := utils.CheckReadableImage(img.path); err != nil {
			return nil, err
		}
		env = append(env, img.key+"="+img.path)
	}
	return env, nil
}

// Update changes a stopped instance's stored config; running instances keep
// their current config until the next Start. nil fields are left unchanged.
func (m *Manager) Update(id string, opts types.UpdateOptions) (types.Instance, error) {
	m.mu.Lock()
	defer m.mu.Unlock()
	item, err := m.readMetadata(filepath.Join(m.instancesDir(), id))
	if err != nil {
		return types.Instance{}, err
	}
	if opts.Name != nil {
		item.Name = strings.TrimSpace(*opts.Name)
	}
	if opts.CPUCores != nil {
		if *opts.CPUCores < 1 || *opts.CPUCores > runtime.NumCPU() {
			return types.Instance{}, fmt.Errorf("%w: cpu_cores must be 1..%d (host cores)", constants.ErrInvalidOptions, runtime.NumCPU())
		}
		item.CPUCores = *opts.CPUCores
	}
	if opts.MemoryMB != nil {
		if *opts.MemoryMB < 1024 || *opts.MemoryMB > 65536 {
			return types.Instance{}, fmt.Errorf("%w: memory_mb must be 1024..65536", constants.ErrInvalidOptions)
		}
		item.MemoryMB = *opts.MemoryMB
	}
	for _, f := range []struct{ in, out *string }{
		{opts.SystemImage, &item.SystemImage},
		{opts.VendorImage, &item.VendorImage},
		{opts.InitramfsImage, &item.InitramfsImage},
	} {
		if f.in == nil {
			continue
		}
		v := strings.TrimSpace(*f.in)
		if v != "" {
			if !filepath.IsAbs(v) {
				return types.Instance{}, fmt.Errorf("%w: image path must be absolute: %s", constants.ErrInvalidOptions, v)
			}
			v = filepath.Clean(v)
			if err := utils.CheckReadableImage(v); err != nil {
				return types.Instance{}, fmt.Errorf("%w: %v", constants.ErrInvalidOptions, err)
			}
		}
		*f.out = v
	}
	if opts.Gpu != nil {
		item.Gpu = strings.TrimSpace(*opts.Gpu)
	}
	if opts.Kernel != nil {
		item.Kernel = strings.TrimSpace(*opts.Kernel)
	}
	if opts.Cmdline != nil {
		item.Cmdline = *opts.Cmdline
	}
	if opts.Append != nil {
		item.Append = *opts.Append
	}
	if opts.Profile != nil {
		item.Profile = *opts.Profile
	}
	if err := m.writeMetadata(item); err != nil {
		return types.Instance{}, err
	}
	m.fillResources(&item)
	return item, nil
}

// ConsoleLogPath resolves an instance's serial console capture file.
func (m *Manager) ConsoleLogPath(id string) (string, error) {
	m.mu.Lock()
	defer m.mu.Unlock()
	item, err := m.readMetadata(filepath.Join(m.instancesDir(), id))
	if err != nil {
		return "", err
	}
	return filepath.Join(item.Dir, constants.ConsoleLogName), nil
}

// ConsoleLog returns up to maxBytes from the tail of the console log.
func (m *Manager) ConsoleLog(id string, maxBytes int) ([]byte, error) {
	path, err := m.ConsoleLogPath(id)
	if err != nil {
		return nil, err
	}
	if maxBytes <= 0 {
		maxBytes = constants.ConsoleLogTailDefault
	}
	if maxBytes > constants.ConsoleLogTailMax {
		maxBytes = constants.ConsoleLogTailMax
	}
	f, err := os.Open(path)
	if err != nil {
		if os.IsNotExist(err) {
			return nil, fmt.Errorf("%w: no console log yet (instance not started)", constants.ErrInputNotRunning)
		}
		return nil, err
	}
	defer f.Close()
	st, err := f.Stat()
	if err != nil {
		return nil, err
	}
	if off := st.Size() - int64(maxBytes); off > 0 {
		if _, err := f.Seek(off, io.SeekStart); err != nil {
			return nil, err
		}
	}
	return io.ReadAll(f)
}

func (m *Manager) Delete(id string) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	item, err := m.readMetadata(filepath.Join(m.instancesDir(), id))
	if err != nil {
		return err
	}
	if cmd := m.procs[id]; cmd != nil && cmd.Process != nil {
		_ = cmd.Process.Signal(os.Interrupt)
		delete(m.procs, id)
		time.Sleep(200 * time.Millisecond)
	}
	if cast := m.castProcs[id]; cast != nil && cast.Process != nil {
		_ = cast.Process.Kill()
		delete(m.castProcs, id)
		delete(m.castPorts, id)
	}
	m.input.stop(id)
	if item.Dir != item.MetaDir {
		owner, _ := os.ReadFile(filepath.Join(item.Dir, constants.RunDirMarker))
		if strings.TrimSpace(string(owner)) != item.ID {
			return fmt.Errorf("instance %s: %s is not marked as its directory; not removing it", id, item.Dir)
		}
		if err := os.RemoveAll(item.Dir); err != nil {
			return err
		}
	}
	return os.RemoveAll(item.MetaDir)
}

func (m *Manager) Start(id string) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.reapLocked()
	if cmd := m.procs[id]; cmd != nil && cmd.Process != nil {
		return nil
	}
	item, err := m.readMetadata(filepath.Join(m.instancesDir(), id))
	if err != nil {
		return err
	}
	images, err := m.imageEnv(item)
	if err != nil {
		return err
	}
	if item.AdbPort <= 0 {
		port, err := m.allocateAdbPortLocked(item.ID)
		if err != nil {
			return err
		}
		item.AdbPort = port
		if err := m.writeMetadata(item); err != nil {
			return err
		}
	}
	runPath := filepath.Join(m.ProjectRoot, "run")
	qmpPath := filepath.Join(item.Dir, "qmp.sock")
	_ = os.Remove(qmpPath)
	_ = os.Remove(utils.InputQMPPath(item.Dir))

	cmd := exec.Command(runPath)
	cmd.Dir = m.ProjectRoot
	cmd.Env = append(os.Environ(),
		"TEBOX_INSTANCE_DIR="+item.Dir,
		"QMP_SOCKET="+qmpPath,
		"QMP_INPUT_SOCKET="+utils.InputQMPPath(item.Dir),
		"SNAPSHOT=0",
		"FORCE_VIRGL=1",
		"QEMU_DEBUG=1",
		fmt.Sprintf("ADB_PORT=%d", item.AdbPort),
		"QEMU_NAME="+item.Name,
		fmt.Sprintf("SMP=%d", item.CPUCores),
		fmt.Sprintf("MEM=%d", item.MemoryMB),
		fmt.Sprintf("USERDATA_MB=%d", item.DiskMB),
		"TEBOX_CAST=1",
		"TEBOX_CAST_SOCK="+filepath.Join(item.Dir, "cast.sock"),
	)
	cmd.Env = append(cmd.Env, images...)
	// Always hand boot-qemu.sh a cmdline: the instance's own tokens when set
	// at creation, otherwise the fixed default. GPU / kernel stay empty here
	// and fall back to the variant's files inside boot-qemu.sh.
	cmdline := item.Cmdline
	if len(cmdline) == 0 {
		cmdline = constants.DefaultCmdline
	}
	cmd.Env = append(cmd.Env, "TEBOX_CMDLINE="+strings.Join(cmdline, " "))
	if item.Gpu != "" {
		cmd.Env = append(cmd.Env, "TEBOX_GPU="+item.Gpu)
	}
	if item.Kernel != "" {
		cmd.Env = append(cmd.Env, "KERNEL_IMG="+item.Kernel)
	}
	if len(item.Append) > 0 {
		// Append on top of any QEMU_EXTRA_APPEND the container already set
		// (e.g. cix-r1p-start.sh), rather than clobbering it.
		extra := strings.Join(item.Append, " ")
		if cur := os.Getenv("QEMU_EXTRA_APPEND"); cur != "" {
			extra = cur + " " + extra
		}
		cmd.Env = append(cmd.Env, "QEMU_EXTRA_APPEND="+extra)
	}
	// Capture this instance's serial console (-serial stdio) to its own file
	// so logs are not interleaved across instances; still mirror to the
	// container stdout. GetConsoleLog / StreamConsoleLog read this file.
	var logFile *os.File
	if f, ferr := os.Create(filepath.Join(item.Dir, constants.ConsoleLogName)); ferr == nil {
		logFile = f
		cmd.Stdout = io.MultiWriter(os.Stdout, f)
		cmd.Stderr = io.MultiWriter(os.Stderr, f)
	} else {
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
	}
	delete(m.castFails, id)
	m.startCastLocked(item)
	if err := cmd.Start(); err != nil {
		if logFile != nil {
			_ = logFile.Close()
		}
		if cast := m.castProcs[item.ID]; cast != nil && cast.Process != nil {
			_ = cast.Process.Kill()
			delete(m.castProcs, item.ID)
			delete(m.castPorts, item.ID)
		}
		return err
	}
	m.procs[id] = cmd
	m.input.start(id, utils.InputQMPPath(item.Dir), m.ProjectRoot, fmt.Sprintf("127.0.0.1:%d", item.AdbPort))
	delete(m.booted, id)
	delete(m.stopping, id)
	go m.watchBoot(id, cmd, fmt.Sprintf("127.0.0.1:%d", item.AdbPort))
	go m.watchControl(id, cmd, filepath.Join(item.Dir, "ctl.sock"))
	go func() {
		_ = cmd.Wait()
		if logFile != nil {
			_ = logFile.Close()
		}
		m.mu.Lock()
		if m.procs[id] == cmd {
			delete(m.procs, id)
			delete(m.booted, id)
			delete(m.stopping, id)
			m.input.stop(id)
		}
		m.mu.Unlock()
	}()
	return nil
}

func (m *Manager) Stop(id string) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.reapLocked()
	cmd := m.procs[id]
	if cast := m.castProcs[id]; cast != nil && cast.Process != nil {
		_ = cast.Process.Kill()
		delete(m.castProcs, id)
		delete(m.castPorts, id)
	}
	if cmd == nil || cmd.Process == nil {
		return nil
	}
	m.stopping[id] = true
	if path, err := m.QMPPath(id); err == nil {
		_ = utils.QMPSocket{Path: path}.PowerDown()
	}
	_ = cmd.Process.Signal(os.Interrupt)
	return nil
}

func (m *Manager) startCastLocked(item types.Instance) {
	if _, ok := m.castProcs[item.ID]; ok {
		return
	}
	binary := filepath.Join(m.ProjectRoot, "out", "service", "tebox-cast")
	if _, err := os.Stat(binary); err != nil {
		return
	}
	port, err := utils.AllocateTCPPort(12000, m.castPorts)
	if err != nil {
		return
	}
	sock := filepath.Join(item.Dir, "cast.sock")
	_ = os.Remove(sock)
	// Control port 0: every instance runs its own tebox-cast, and a fixed
	// default (9902) would make the second one fail to start.
	cmd := exec.Command(binary, "--capture-sock", sock, "--listen", fmt.Sprintf("127.0.0.1:%d", port),
		"--control", "127.0.0.1:0", "--fps", "60", "--bitrate", "8000000")
	cmd.Stdout, cmd.Stderr = os.Stdout, os.Stderr
	if err := cmd.Start(); err != nil {
		return
	}
	m.castProcs[item.ID], m.castPorts[item.ID] = cmd, port
	started := time.Now()
	go func() {
		err := cmd.Wait()
		m.mu.Lock()
		defer m.mu.Unlock()
		if m.castProcs[item.ID] != cmd {
			return // killed by Stop/Delete
		}
		delete(m.castProcs, item.ID)
		delete(m.castPorts, item.ID)
		if m.procs[item.ID] == nil || m.stopping[item.ID] {
			return
		}
		// The guest keeps running, so bring the cast back; QEMU's capture
		// hook reconnects to cast.sock on its own. Back off on crash loops.
		if time.Since(started) > time.Minute {
			m.castFails[item.ID] = 0
		}
		delay := time.Second << min(m.castFails[item.ID], 5)
		m.castFails[item.ID]++
		fmt.Fprintf(os.Stderr, "tebox-api: tebox-cast for %s exited (%v), restarting in %s\n", item.ID, err, delay)
		time.AfterFunc(delay, func() {
			m.mu.Lock()
			defer m.mu.Unlock()
			if m.procs[item.ID] != nil && !m.stopping[item.ID] {
				m.startCastLocked(item)
			}
		})
	}()
}

func (m *Manager) CastPort(id string) (int, error) {
	m.mu.Lock()
	defer m.mu.Unlock()
	if port := m.castPorts[id]; port > 0 {
		return port, nil
	}
	return 0, fmt.Errorf("cast is not running for instance %s", id)
}

// Reset reboots a running guest through QMP. With wipeData, it recreates the
// instance userdata image on the next boot, which provides a factory reset.
func (m *Manager) Reset(id string, wipeData bool) error {
	if wipeData {
		if _, err := m.stopAndWait(id, "factory reset"); err != nil {
			return err
		}
		item, err := m.readMetadata(filepath.Join(m.instancesDir(), id))
		if err != nil {
			return err
		}
		if err := os.Remove(filepath.Join(item.Dir, "userdata.img")); err != nil && !os.IsNotExist(err) {
			return fmt.Errorf("remove userdata: %w", err)
		}
		return m.Start(id)
	}

	path, err := m.QMPPath(id)
	if err != nil {
		return err
	}
	if err := (utils.QMPSocket{Path: path}).SystemReset(); err != nil {
		return err
	}
	// The guest reboots: back to "starting" until Android boots again.
	m.mu.Lock()
	if cmd := m.procs[id]; cmd != nil {
		delete(m.booted, id)
		if item, rerr := m.readMetadata(filepath.Join(m.instancesDir(), id)); rerr == nil {
			go m.watchBoot(id, cmd, fmt.Sprintf("127.0.0.1:%d", item.AdbPort))
		}
	}
	m.mu.Unlock()
	return nil
}

// stopAndWait stops a running instance and waits for QEMU to exit; it
// reports whether the instance was running.
func (m *Manager) stopAndWait(id, why string) (bool, error) {
	m.mu.Lock()
	cmd := m.procs[id]
	m.mu.Unlock()
	if cmd == nil || cmd.Process == nil {
		return false, nil
	}
	if err := m.Stop(id); err != nil {
		return true, err
	}
	deadline := time.Now().Add(5 * time.Second)
	for time.Now().Before(deadline) {
		m.mu.Lock()
		stopped := m.procs[id] == nil
		m.mu.Unlock()
		if stopped {
			return true, nil
		}
		time.Sleep(50 * time.Millisecond)
	}
	return true, fmt.Errorf("instance %s did not stop before %s", id, why)
}

// Configure applies instance-specific guest identity/location values over ADB.
// It deliberately leaves system.img and the kernel untouched.
func (m *Manager) Configure(id string, profile types.DeviceProfile) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	item, err := m.readMetadata(filepath.Join(m.instancesDir(), id))
	if err != nil {
		return err
	}
	item.Profile = profile
	if err := m.writeMetadata(item); err != nil {
		return err
	}
	cmd := m.procs[id]
	if cmd == nil || cmd.Process == nil {
		return nil
	}
	adb := utils.ADBBinary()
	target := fmt.Sprintf("127.0.0.1:%d", item.AdbPort)
	commands := [][]string{
		{"shell", "settings", "put", "secure", "location_providers_allowed", "+gps"},
		{"shell", "setprop", "gsm.operator.iso-country", strings.ToLower(profile.Country)},
		{"shell", "setprop", "gsm.sim.operator.iso-country", strings.ToLower(profile.Country)},
		{"shell", "setprop", "gsm.operator.numeric", profile.OperatorNumeric},
		{"shell", "setprop", "gsm.sim.operator.numeric", profile.OperatorNumeric},
		{"shell", "setprop", "gsm.operator.alpha", profile.OperatorName},
		{"shell", "setprop", "gsm.sim.operator.alpha", profile.OperatorName},
		{"shell", "setprop", "gsm.sim.line1.number", profile.PhoneNumber},
		{"shell", "setprop", "gsm.line1.number", profile.PhoneNumber},
	}
	for _, args := range commands {
		full := append([]string{"-s", target}, args...)
		if out, runErr := exec.Command(adb, full...).CombinedOutput(); runErr != nil {
			return fmt.Errorf("adb %v: %w (%s)", args, runErr, strings.TrimSpace(string(out)))
		}
	}
	if profile.Root {
		if out, runErr := exec.Command(adb, "-s", target, "root").CombinedOutput(); runErr != nil {
			return fmt.Errorf("adb root: %w (%s)", runErr, strings.TrimSpace(string(out)))
		}
	}
	return nil
}

func (m *Manager) QMPPath(id string) (string, error) {
	item, err := m.readMetadata(filepath.Join(m.instancesDir(), id))
	if err != nil {
		return "", err
	}
	return filepath.Join(item.Dir, "qmp.sock"), nil
}

// InputQMPPath is the QEMU monitor reserved for streamed cast input.
func (m *Manager) InputQMPPath(id string) (string, error) {
	item, err := m.readMetadata(filepath.Join(m.instancesDir(), id))
	if err != nil {
		return "", err
	}
	return utils.InputQMPPath(item.Dir), nil
}

func (m *Manager) ShutdownRunning() {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.input.stopAll()
	for id, cmd := range m.procs {
		if cmd == nil || cmd.Process == nil {
			continue
		}
		if path, err := m.QMPPath(id); err == nil {
			_ = utils.QMPSocket{Path: path}.PowerDown()
		}
	}
	for id, cmd := range m.castProcs {
		if cmd != nil && cmd.Process != nil {
			_ = cmd.Process.Kill()
			delete(m.castProcs, id)
		}
	}
}

func (m *Manager) reapLocked() {
	for id, cmd := range m.procs {
		if cmd == nil || cmd.Process == nil {
			delete(m.procs, id)
			continue
		}
		if cmd.ProcessState != nil && cmd.ProcessState.Exited() {
			delete(m.procs, id)
		}
	}
}

func (m *Manager) readMetadata(dir string) (types.Instance, error) {
	data, err := os.ReadFile(filepath.Join(dir, "instance.json"))
	if err != nil {
		return types.Instance{}, err
	}
	var item types.Instance
	if err := json.Unmarshal(data, &item); err != nil {
		return types.Instance{}, err
	}
	item.MetaDir, item.Dir = dir, dir
	if item.RunDir != "" {
		item.Dir = item.RunDir
	}
	if item.ID == "" {
		return types.Instance{}, fmt.Errorf("invalid metadata")
	}
	m.fillResources(&item)
	return item, nil
}

func (m *Manager) writeMetadata(item types.Instance) error {
	data, err := json.Marshal(item.Metadata())
	if err != nil {
		return err
	}
	return os.WriteFile(filepath.Join(item.MetaDir, "instance.json"), append(data, '\n'), 0o644)
}

func (m *Manager) allocateAdbPort(skipID string) (int, error) {
	m.mu.Lock()
	defer m.mu.Unlock()
	return m.allocateAdbPortLocked(skipID)
}

func (m *Manager) allocateAdbPortLocked(skipID string) (int, error) {
	used := map[int]bool{}
	entries, err := os.ReadDir(m.instancesDir())
	if err != nil && !os.IsNotExist(err) {
		return 0, err
	}
	for _, entry := range entries {
		if !entry.IsDir() || strings.HasPrefix(entry.Name(), ".") || entry.Name() == skipID {
			continue
		}
		item, err := m.readMetadata(filepath.Join(m.instancesDir(), entry.Name()))
		if err != nil || item.AdbPort <= 0 {
			continue
		}
		used[item.AdbPort] = true
	}

	h := fnv.New32a()
	_, _ = h.Write([]byte(skipID))
	base := 5555 + int(h.Sum32()%10000)
	for i := 0; i < 10000; i++ {
		port := 5555 + ((base - 5555 + i) % 10000)
		if used[port] {
			continue
		}
		if !utils.TCPPortFree(port) {
			continue
		}
		return port, nil
	}
	return 0, fmt.Errorf("no free ADB port available")
}

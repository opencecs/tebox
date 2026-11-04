package types

import api "tebox-api/api/v1"

// Instance is one Android guest managed by tebox-api.
type Instance struct {
	ID      string        `json:"id"`
	Name    string        `json:"name"`
	Dir     string        `json:"dir"`
	AdbPort int           `json:"adbPort"`
	PID     int           `json:"pid"`
	Running bool          `json:"running"`
	Profile DeviceProfile `json:"profile"`
	// CastPort is the per-instance tebox-cast port while casting (0 otherwise).
	CastPort int `json:"castPort"`
	// ControlPort is always 0 (shared TEBOX-CTRL / UDP endpoint only).
	ControlPort int `json:"controlPort"`
	// Resources fixed at creation; 0 in old metadata means the boot defaults.
	CPUCores int `json:"cpuCores"`
	MemoryMB int `json:"memoryMb"`
	DiskMB   int `json:"diskMb"`
	// DiskUsedMB is the space userdata.img occupies on the host.
	DiskUsedMB int `json:"diskUsedMb"`
	// State is the live lifecycle (stopped, starting, running, stopping).
	// It is derived from the QEMU process and is not stored in instance.json.
	State api.InstanceState `json:"-"`
	// Images set at creation; empty means the variant's own image.
	SystemImage    string `json:"systemImage,omitempty"`
	VendorImage    string `json:"vendorImage,omitempty"`
	InitramfsImage string `json:"initramfsImage,omitempty"`
	// Boot overrides set at creation. Empty Cmdline means the API passes its
	// fixed default (constants.DefaultCmdline); empty Gpu / Kernel fall back to
	// the variant's files inside boot-qemu.sh.
	Cmdline []string `json:"cmdline,omitempty"`
	Gpu     string   `json:"gpu,omitempty"`
	Kernel  string   `json:"kernel,omitempty"`
	// Append adds tokens on top of Cmdline (or the default), not replacing it.
	Append []string `json:"append,omitempty"`
	// RunDir is the instance directory chosen at creation (empty: default under data root).
	RunDir string `json:"runDir,omitempty"`

	// MetaDir holds instance.json under the data root.
	MetaDir string `json:"-"`
}

// Metadata is the instance.json document. Live fields (dir, pid, state) stay on Instance.
type Metadata struct {
	ID       string        `json:"id"`
	Name     string        `json:"name"`
	AdbPort  int           `json:"adbPort"`
	Profile  DeviceProfile `json:"profile"`
	CPUCores int           `json:"cpuCores"`
	MemoryMB int           `json:"memoryMb"`
	DiskMB   int           `json:"diskMb"`

	SystemImage    string   `json:"systemImage,omitempty"`
	VendorImage    string   `json:"vendorImage,omitempty"`
	InitramfsImage string   `json:"initramfsImage,omitempty"`
	Cmdline        []string `json:"cmdline,omitempty"`
	Gpu            string   `json:"gpu,omitempty"`
	Kernel         string   `json:"kernel,omitempty"`
	Append         []string `json:"append,omitempty"`
	RunDir         string   `json:"runDir,omitempty"`
}

// Metadata returns the fields stored in instance.json.
func (item Instance) Metadata() Metadata {
	return Metadata{
		ID:             item.ID,
		Name:           item.Name,
		AdbPort:        item.AdbPort,
		Profile:        item.Profile,
		CPUCores:       item.CPUCores,
		MemoryMB:       item.MemoryMB,
		DiskMB:         item.DiskMB,
		SystemImage:    item.SystemImage,
		VendorImage:    item.VendorImage,
		InitramfsImage: item.InitramfsImage,
		Cmdline:        item.Cmdline,
		Gpu:            item.Gpu,
		Kernel:         item.Kernel,
		Append:         item.Append,
		RunDir:         item.RunDir,
	}
}

// CreateOptions sizes a new instance; zero fields take the defaults.
type CreateOptions struct {
	Name           string
	CPUCores       int
	MemoryMB       int
	DiskMB         int
	SystemImage    string
	VendorImage    string
	InitramfsImage string
	InstanceDir    string
	Cmdline        []string
	Gpu            string
	Kernel         string
	Append         []string
}

// UpdateOptions changes a stopped instance's stored config; nil pointers and
// nil slices leave a field unchanged. Changes take effect on the next Start.
type UpdateOptions struct {
	Name           *string
	CPUCores       *int
	MemoryMB       *int
	SystemImage    *string
	VendorImage    *string
	InitramfsImage *string
	Cmdline        *[]string
	Gpu            *string
	Kernel         *string
	Append         *[]string
	Profile        *DeviceProfile
}

// DeviceProfile is guest identity / location applied over adb.
type DeviceProfile struct {
	Latitude        float64 `json:"latitude"`
	Longitude       float64 `json:"longitude"`
	Country         string  `json:"country"`
	OperatorNumeric string  `json:"operatorNumeric"`
	OperatorName    string  `json:"operatorName"`
	CellId          string  `json:"cellId"`
	PhoneNumber     string  `json:"phoneNumber"`
	Root            bool    `json:"root"`
}

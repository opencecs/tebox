package server

import (
	"context"
	"io"
	"os"
	"time"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	api "tebox-api/api/v1"
	"tebox-api/internal/service"
	"tebox-api/internal/service/types"
	"tebox-api/internal/utils"
)

// Service is the gRPC AndroidService implementation.
type Service struct {
	api.UnimplementedAndroidServiceServer
	mgr *service.Manager
}

// New returns the gRPC service backed by mgr.
func New(mgr *service.Manager) *Service {
	return &Service{mgr: mgr}
}

func (s *Service) ListInstances(context.Context, *api.ListInstancesRequest) (*api.ListInstancesResponse, error) {
	items, err := s.mgr.List()
	if err != nil {
		return nil, utils.RPCError(err)
	}
	result := &api.ListInstancesResponse{Instances: make([]*api.Instance, 0, len(items))}
	for _, item := range items {
		result.Instances = append(result.Instances, instanceMessage(item))
	}
	return result, nil
}

func (s *Service) GetInstance(_ context.Context, req *api.InstanceRequest) (*api.Instance, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	item, err := s.mgr.Get(id)
	if err != nil {
		return nil, utils.RPCError(err)
	}
	return instanceMessage(item), nil
}

func (s *Service) CreateInstance(_ context.Context, req *api.CreateInstanceRequest) (*api.Instance, error) {
	item, err := s.mgr.Create(types.CreateOptions{
		Name:           req.GetName(),
		CPUCores:       int(req.GetCpuCores()),
		MemoryMB:       int(req.GetMemoryMb()),
		DiskMB:         int(req.GetDiskMb()),
		SystemImage:    req.GetSystemImage(),
		VendorImage:    req.GetVendorImage(),
		InitramfsImage: req.GetInitramfsImage(),
		InstanceDir:    req.GetInstanceDir(),
		Cmdline:        req.GetCmdline(),
		Gpu:            req.GetGpu(),
		Kernel:         req.GetKernel(),
		Append:         req.GetAppend(),
	})
	if err != nil {
		return nil, utils.RPCError(err)
	}
	return instanceMessage(item), nil
}

func (s *Service) UpdateInstance(_ context.Context, req *api.UpdateInstanceRequest) (*api.Instance, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	var opts types.UpdateOptions
	if req.Name != nil {
		v := req.GetName()
		opts.Name = &v
	}
	if req.CpuCores != nil {
		v := int(req.GetCpuCores())
		opts.CPUCores = &v
	}
	if req.MemoryMb != nil {
		v := int(req.GetMemoryMb())
		opts.MemoryMB = &v
	}
	if req.SystemImage != nil {
		v := req.GetSystemImage()
		opts.SystemImage = &v
	}
	if req.VendorImage != nil {
		v := req.GetVendorImage()
		opts.VendorImage = &v
	}
	if req.InitramfsImage != nil {
		v := req.GetInitramfsImage()
		opts.InitramfsImage = &v
	}
	if req.GetCmdline() != nil {
		v := req.GetCmdline().GetValues()
		opts.Cmdline = &v
	}
	if req.Gpu != nil {
		v := req.GetGpu()
		opts.Gpu = &v
	}
	if req.Kernel != nil {
		v := req.GetKernel()
		opts.Kernel = &v
	}
	if req.GetAppend() != nil {
		v := req.GetAppend().GetValues()
		opts.Append = &v
	}
	if p := req.GetProfile(); p != nil {
		dp := deviceProfile(p)
		opts.Profile = &dp
	}
	item, err := s.mgr.Update(id, opts)
	if err != nil {
		return nil, utils.RPCError(err)
	}
	return instanceMessage(item), nil
}

func (s *Service) GetConsoleLog(_ context.Context, req *api.ConsoleLogRequest) (*api.ConsoleLogResponse, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	data, err := s.mgr.ConsoleLog(id, int(req.GetMaxBytes()))
	if err != nil {
		return nil, utils.RPCError(err)
	}
	return &api.ConsoleLogResponse{Data: data}, nil
}

func (s *Service) StreamConsoleLog(req *api.ConsoleLogRequest, stream api.AndroidService_StreamConsoleLogServer) error {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return err
	}
	path, err := s.mgr.ConsoleLogPath(id)
	if err != nil {
		return utils.RPCError(err)
	}
	f, err := os.Open(path)
	if err != nil {
		return utils.RPCError(err)
	}
	defer f.Close()
	// Start near the tail so the client gets recent context, then follow.
	tail := int64(req.GetMaxBytes())
	if tail <= 0 {
		tail = int64(64 * 1024)
	}
	if st, serr := f.Stat(); serr == nil {
		if off := st.Size() - tail; off > 0 {
			_, _ = f.Seek(off, io.SeekStart)
		}
	}
	ctx := stream.Context()
	buf := make([]byte, 32*1024)
	for {
		n, rerr := f.Read(buf)
		if n > 0 {
			if serr := stream.Send(&api.ConsoleLogChunk{Data: append([]byte(nil), buf[:n]...)}); serr != nil {
				return serr
			}
		}
		if rerr == io.EOF || (rerr == nil && n == 0) {
			// A restart recreates console.log (truncate to 0); our read
			// offset would then sit past EOF and miss the new boot. Detect
			// the shrink and rewind to the start of the fresh file.
			if st, serr := f.Stat(); serr == nil {
				if pos, perr := f.Seek(0, io.SeekCurrent); perr == nil && st.Size() < pos {
					_, _ = f.Seek(0, io.SeekStart)
					continue
				}
			}
			select {
			case <-ctx.Done():
				return ctx.Err()
			case <-time.After(150 * time.Millisecond):
			}
			continue
		}
		if rerr != nil {
			return utils.RPCError(rerr)
		}
		select {
		case <-ctx.Done():
			return ctx.Err()
		default:
		}
	}
}

func (s *Service) DeleteInstance(_ context.Context, req *api.InstanceRequest) (*api.Empty, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	if err := s.mgr.Delete(id); err != nil {
		return nil, utils.RPCError(err)
	}
	return &api.Empty{}, nil
}

func (s *Service) StartInstance(_ context.Context, req *api.InstanceRequest) (*api.Empty, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	if err := s.mgr.Start(id); err != nil {
		return nil, utils.RPCError(err)
	}
	return &api.Empty{}, nil
}

func (s *Service) StopInstance(_ context.Context, req *api.InstanceRequest) (*api.Empty, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	if err := s.mgr.Stop(id); err != nil {
		return nil, utils.RPCError(err)
	}
	return &api.Empty{}, nil
}

func (s *Service) ResetInstance(_ context.Context, req *api.ResetInstanceRequest) (*api.Empty, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	if err := s.mgr.Reset(id, req.GetWipeData()); err != nil {
		return nil, utils.RPCError(err)
	}
	return &api.Empty{}, nil
}

func (s *Service) ConfigureInstance(_ context.Context, req *api.ConfigureInstanceRequest) (*api.Empty, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	if req.GetProfile() == nil {
		return nil, status.Error(codes.InvalidArgument, "profile is required")
	}
	if err := s.mgr.Configure(id, deviceProfile(req.GetProfile())); err != nil {
		return nil, utils.RPCError(err)
	}
	return &api.Empty{}, nil
}

func (s *Service) SendKey(_ context.Context, req *api.KeyRequest) (*api.Empty, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	qmp, err := s.monitor(id)
	if err != nil {
		return nil, err
	}
	if err := qmp.SendKey(req.GetKey()); err != nil {
		return nil, utils.RPCError(err)
	}
	return &api.Empty{}, nil
}

func (s *Service) PowerDown(_ context.Context, req *api.InstanceRequest) (*api.Empty, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	qmp, err := s.monitor(id)
	if err != nil {
		return nil, err
	}
	if err := qmp.PowerDown(); err != nil {
		return nil, utils.RPCError(err)
	}
	return &api.Empty{}, nil
}

func (s *Service) GetPreview(_ context.Context, req *api.GetPreviewRequest) (*api.GetPreviewResponse, error) {
	id, err := utils.RequireID(req.GetId())
	if err != nil {
		return nil, err
	}
	preview, err := s.mgr.Preview(id, types.PreviewOptions{
		Size:    int(req.GetSize()),
		Format:  req.GetFormat(),
		Quality: int(req.GetQuality()),
	})
	if err != nil {
		return nil, utils.RPCError(err)
	}
	return &api.GetPreviewResponse{
		Image:    preview.Data,
		Format:   preview.Format,
		MimeType: preview.Mime,
		Width:    int32(preview.Width),
		Height:   int32(preview.Height),
	}, nil
}

func (s *Service) monitor(id string) (utils.QMPSocket, error) {
	path, err := s.mgr.QMPPath(id)
	if err != nil {
		return utils.QMPSocket{}, utils.RPCError(err)
	}
	return utils.QMPSocket{Path: path}, nil
}

func instanceMessage(item types.Instance) *api.Instance {
	return &api.Instance{
		Id:             item.ID,
		Name:           item.Name,
		Dir:            item.Dir,
		AdbPort:        int32(item.AdbPort),
		Pid:            int32(item.PID),
		Running:        item.Running,
		CastPort:       int32(item.CastPort),
		ControlPort:    int32(item.ControlPort),
		CpuCores:       int32(item.CPUCores),
		MemoryMb:       int32(item.MemoryMB),
		DiskMb:         int32(item.DiskMB),
		DiskUsedMb:     int32(item.DiskUsedMB),
		DiskFreeMb:     utils.DiskFreeMB(item.DiskMB, item.DiskUsedMB),
		State:          item.State,
		SystemImage:    item.SystemImage,
		VendorImage:    item.VendorImage,
		InitramfsImage: item.InitramfsImage,
		Cmdline:        item.Cmdline,
		Gpu:            item.Gpu,
		Kernel:         item.Kernel,
		Append:         item.Append,
		Profile:        profileMessage(item.Profile),
	}
}

func profileMessage(p types.DeviceProfile) *api.DeviceProfile {
	return &api.DeviceProfile{
		Latitude:        p.Latitude,
		Longitude:       p.Longitude,
		Country:         p.Country,
		OperatorNumeric: p.OperatorNumeric,
		OperatorName:    p.OperatorName,
		CellId:          p.CellId,
		PhoneNumber:     p.PhoneNumber,
		Root:            p.Root,
	}
}

func deviceProfile(p *api.DeviceProfile) types.DeviceProfile {
	return types.DeviceProfile{
		Latitude:        p.GetLatitude(),
		Longitude:       p.GetLongitude(),
		Country:         p.GetCountry(),
		OperatorNumeric: p.GetOperatorNumeric(),
		OperatorName:    p.GetOperatorName(),
		CellId:          p.GetCellId(),
		PhoneNumber:     p.GetPhoneNumber(),
		Root:            p.GetRoot(),
	}
}

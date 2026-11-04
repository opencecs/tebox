package app

import (
	"context"
	"fmt"
	"os"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
	api "tebox-client/api/v1"
	"tebox-client/internal/service/types"
)

// App is the Wails-bound desktop launcher.
type App struct {
	ctx  context.Context
	conn *grpc.ClientConn
	api  api.AndroidServiceClient
}

// Status is the launcher status shown in the UI.
type Status struct {
	Variant  string `json:"variant"`
	DataRoot string `json:"dataRoot"`
	Root     string `json:"root"`
}

// New returns an unbound launcher. Startup connects it to tebox-api.
func New() *App {
	return &App{}
}

// Startup dials TEBOX_API_ADDR (default 127.0.0.1:50051).
func (a *App) Startup(ctx context.Context) {
	a.ctx = ctx
	addr := os.Getenv("TEBOX_API_ADDR")
	if addr == "" {
		addr = "127.0.0.1:50051"
	}
	conn, err := grpc.Dial(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
	if err != nil {
		return
	}
	a.conn, a.api = conn, api.NewAndroidServiceClient(conn)
}

// Shutdown closes the API connection.
func (a *App) Shutdown(ctx context.Context) {
	_ = ctx
	if a.conn != nil {
		_ = a.conn.Close()
	}
}

func (a *App) ensure() error {
	if a.api == nil {
		return fmt.Errorf("tebox API is not connected (set TEBOX_API_ADDR and start tebox-api)")
	}
	return nil
}

func (a *App) GetStatus() (Status, error) {
	if err := a.ensure(); err != nil {
		return Status{}, err
	}
	return Status{
		Variant:  os.Getenv("TEBOX_VARIANT"),
		DataRoot: os.Getenv("TEBOX_DATA_DIR"),
		Root:     os.Getenv("TEBOX_ROOT"),
	}, nil
}

func (a *App) ListInstances() ([]types.Instance, error) {
	if err := a.ensure(); err != nil {
		return nil, err
	}
	res, err := a.api.ListInstances(a.ctx, &api.ListInstancesRequest{})
	if err != nil {
		return nil, err
	}
	return fromMessages(res.Instances), nil
}

func (a *App) CreateInstance() (types.Instance, error) {
	if err := a.ensure(); err != nil {
		return types.Instance{}, err
	}
	item, err := a.api.CreateInstance(a.ctx, &api.CreateInstanceRequest{})
	if err != nil {
		return types.Instance{}, err
	}
	return fromMessage(item), nil
}

func (a *App) DeleteInstance(id string) error {
	if err := a.ensure(); err != nil {
		return err
	}
	_, err := a.api.DeleteInstance(a.ctx, &api.InstanceRequest{Id: id})
	return err
}

func (a *App) StartInstance(id string) error {
	if err := a.ensure(); err != nil {
		return err
	}
	_, err := a.api.StartInstance(a.ctx, &api.InstanceRequest{Id: id})
	return err
}

func (a *App) StopInstance(id string) error {
	if err := a.ensure(); err != nil {
		return err
	}
	_, err := a.api.StopInstance(a.ctx, &api.InstanceRequest{Id: id})
	return err
}

func (a *App) ConfigureInstance(id string, profile types.DeviceProfile) error {
	if err := a.ensure(); err != nil {
		return err
	}
	_, err := a.api.ConfigureInstance(a.ctx, &api.ConfigureInstanceRequest{
		Id: id,
		Profile: &api.DeviceProfile{
			Latitude:        profile.Latitude,
			Longitude:       profile.Longitude,
			Country:         profile.Country,
			OperatorNumeric: profile.OperatorNumeric,
			OperatorName:    profile.OperatorName,
			CellId:          profile.CellId,
			PhoneNumber:     profile.PhoneNumber,
			Root:            profile.Root,
		},
	})
	return err
}

func (a *App) SendKey(id, key string) error {
	if err := a.ensure(); err != nil {
		return err
	}
	_, err := a.api.SendKey(a.ctx, &api.KeyRequest{Id: id, Key: key})
	return err
}

func (a *App) PowerDown(id string) error {
	if err := a.ensure(); err != nil {
		return err
	}
	_, err := a.api.PowerDown(a.ctx, &api.InstanceRequest{Id: id})
	return err
}

func (a *App) ResetInstance(id string, wipeData bool) error {
	if err := a.ensure(); err != nil {
		return err
	}
	_, err := a.api.ResetInstance(a.ctx, &api.ResetInstanceRequest{Id: id, WipeData: wipeData})
	return err
}

func fromMessages(items []*api.Instance) []types.Instance {
	out := make([]types.Instance, 0, len(items))
	for _, item := range items {
		out = append(out, fromMessage(item))
	}
	return out
}

func fromMessage(item *api.Instance) types.Instance {
	profile := types.DeviceProfile{}
	if item.Profile != nil {
		profile = types.DeviceProfile{
			Latitude:        item.Profile.Latitude,
			Longitude:       item.Profile.Longitude,
			Country:         item.Profile.Country,
			OperatorNumeric: item.Profile.OperatorNumeric,
			OperatorName:    item.Profile.OperatorName,
			CellId:          item.Profile.CellId,
			PhoneNumber:     item.Profile.PhoneNumber,
			Root:            item.Profile.Root,
		}
	}
	return types.Instance{
		ID:      item.Id,
		Name:    item.Name,
		Variant: item.Variant,
		Dir:     item.Dir,
		AdbPort: int(item.AdbPort),
		PID:     int(item.Pid),
		Running: item.Running,
		Profile: profile,
	}
}

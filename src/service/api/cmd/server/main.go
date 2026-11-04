package main

import (
	"context"
	"flag"
	"fmt"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
	api "tebox-api/api/v1"
	"tebox-api/internal/server"
	"tebox-api/internal/service"
	"tebox-api/internal/utils"
)

func main() {
	addr := flag.String("listen", utils.EnvOr("TEBOX_GRPC_ADDR", "127.0.0.1:50051"), "gRPC listen address")
	castAddr := flag.String("cast-listen", utils.EnvOr("TEBOX_CAST_ADDR", "127.0.0.1:9901"), "video listen address (TEBOX-CAST/1)")
	audioAddr := flag.String("audio-listen", utils.EnvOr("TEBOX_AUDIO_ADDR", "127.0.0.1:9904"), "audio + microphone listen address (TEBOX-AUDIO/1)")
	controlAddr := flag.String("control-listen", utils.EnvOr("TEBOX_CONTROL_ADDR", "127.0.0.1:9905"), "control address: TCP input + camera (TEBOX-CTRL/1, TEBOX-CAMERA/1) and UDP input datagrams")
	autostart := flag.String("start", utils.EnvOr("TEBOX_AUTOSTART", ""), "comma-separated instance ids to start once serving")
	flag.Parse()

	root, err := utils.ResolveProjectRoot()
	if err != nil {
		fatal(err)
	}
	mgr, err := service.NewManager(root, utils.DefaultDataRoot())
	if err != nil {
		fatal(err)
	}
	listener, err := net.Listen("tcp", *addr)
	if err != nil {
		fatal(err)
	}
	grpcServer := grpc.NewServer(
		grpc.MaxSendMsgSize(16<<20),
		grpc.MaxRecvMsgSize(16<<20),
	)
	api.RegisterAndroidServiceServer(grpcServer, server.New(mgr))
	// Lets grpcurl and other tools discover the service without tebox.proto.
	reflection.Register(grpcServer)

	media := &server.Media{Mgr: mgr}
	go media.ServeCast(listen(*castAddr))
	go media.ServeAudio(listen(*audioAddr))
	go media.ServeControl(listen(*controlAddr))
	// UDP input shares the control port number (events carry "id").
	if err := mgr.ServeInputUDP(*controlAddr); err != nil {
		fatal(err)
	}

	ctx, stop := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGTERM)
	defer stop()
	for _, id := range strings.Split(*autostart, ",") {
		id = strings.TrimSpace(id)
		if id == "" {
			continue
		}
		if err := mgr.Start(id); err != nil {
			fmt.Fprintf(os.Stderr, "autostart %s: %v\n", id, err)
		}
	}
	go func() {
		<-ctx.Done()
		grpcServer.GracefulStop()
		mgr.ShutdownRunning()
	}()
	fmt.Printf("tebox gRPC listening on %s, video %s, audio %s, control tcp+udp %s (root=%s, data=%s)\n",
		*addr, *castAddr, *audioAddr, *controlAddr, root, mgr.DataRoot)
	if err := grpcServer.Serve(listener); err != nil && ctx.Err() == nil {
		fatal(err)
	}
}

func listen(addr string) net.Listener {
	ln, err := net.Listen("tcp", addr)
	if err != nil {
		fatal(err)
	}
	return ln
}

func fatal(err error) {
	fmt.Fprintln(os.Stderr, err)
	os.Exit(1)
}

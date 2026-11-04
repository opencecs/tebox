package service

import (
	"fmt"
	"net"
	"time"
)

func qmpCommand(socketPath, command, key string) error {
	conn, err := net.DialTimeout("unix", socketPath, 200*time.Millisecond)
	if err != nil {
		return err
	}
	defer conn.Close()
	_ = conn.SetDeadline(time.Now().Add(500 * time.Millisecond))

	buf := make([]byte, 4096)
	_, _ = conn.Read(buf) // greeting
	if _, err := conn.Write([]byte("{\"execute\":\"qmp_capabilities\"}\n")); err != nil {
		return err
	}
	_, _ = conn.Read(buf)

	var request string
	if key != "" {
		request = fmt.Sprintf(
			"{\"execute\":\"send-key\",\"arguments\":{\"keys\":[{\"type\":\"qcode\",\"data\":\"%s\"}]}}\n",
			key,
		)
	} else {
		request = fmt.Sprintf("{\"execute\":\"%s\"}\n", command)
	}
	if _, err := conn.Write([]byte(request)); err != nil {
		return err
	}
	_, _ = conn.Read(buf)
	return nil
}

func SendKey(socketPath, key string) error {
	return qmpCommand(socketPath, "", key)
}

func PowerDown(socketPath string) error {
	return qmpCommand(socketPath, "system_powerdown", "")
}

func SystemReset(socketPath string) error {
	return qmpCommand(socketPath, "system_reset", "")
}

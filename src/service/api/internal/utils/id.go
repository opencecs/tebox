package utils

import (
	"strings"

	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
)

// RequireID rejects an empty id, "." / "..", and any id that contains a path separator.
func RequireID(raw string) (string, error) {
	id := strings.TrimSpace(raw)
	if id == "" || id == "." || id == ".." || strings.ContainsAny(id, `/\\`) {
		return "", status.Error(codes.InvalidArgument, "invalid instance id")
	}
	return id, nil
}

# Wails desktop launcher for Android instance management.
ROOT := $(abspath ../../..)
OUT := $(ROOT)/out/service
export PATH := $(shell go env GOPATH)/bin:$(PATH)
UNAME_S := $(shell uname -s)

.PHONY: all clean deps
all: $(OUT)/tebox-client

deps:
	cd web && npm install
	go mod tidy

$(OUT)/tebox-client: $(wildcard cmd/server/*.go) $(wildcard internal/app/*.go) $(wildcard internal/service/*.go) $(wildcard internal/service/types/*.go) $(wildcard internal/utils/*.go) $(shell find web/src -type f 2>/dev/null) web/index.html web/package.json cmd/server/wails.json
	@mkdir -p $(OUT)
	@test -d web/node_modules || (cd web && npm install)
	go mod tidy
	cd cmd/server && wails build -clean -nopackage -o tebox-client
	cp -f cmd/server/build/bin/tebox-client $(OUT)/tebox-client
	@chmod +x $(OUT)/tebox-client
ifeq ($(UNAME_S),Darwin)
	@# cp can leave a linker/adhoc signature that macOS AMFI SIGKILLs on launch.
	xattr -cr $(OUT)/tebox-client 2>/dev/null || true
	codesign --force --sign - $(OUT)/tebox-client
endif

clean:
	rm -rf cmd/server/build cmd/server/web/dist web/dist web/node_modules
	rm -f $(OUT)/tebox-client

ROOT := $(abspath ../../..)
OUT := $(ROOT)/out/service

.PHONY: all clean
all: $(OUT)/tebox-api

$(OUT)/tebox-api: $(wildcard cmd/server/*.go) $(wildcard internal/server/*.go) $(wildcard internal/service/*.go) $(wildcard internal/utils/*.go) $(wildcard api/v1/*.go)
	@mkdir -p $(OUT)
	go build -o $@ ./cmd/server
	@chmod +x $@

clean:
	rm -f $(OUT)/tebox-api

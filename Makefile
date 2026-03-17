WASI_SDK_PATH ?= /opt/wasi-sdk

CC     = $(WASI_SDK_PATH)/bin/clang
CFLAGS = --target=wasm32-wasi -Wall -Wextra -Werror -Os \
         -nostdlib -Wl,--no-entry -Wl,--export-dynamic \
         -Wl,--allow-undefined -I src

SRC    = src/main.c
OUT    = dist/chip.wasm

.PHONY: all clean

all: $(OUT) dist/chip.json

$(OUT): $(SRC) src/wokwi-api.h
	mkdir -p dist
	$(CC) $(CFLAGS) -o $@ $(SRC)

dist/chip.json: dist chip.json
	cp chip.json dist

dist:
	mkdir -p dist

clean:
	rm -rf dist

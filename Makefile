WASI_SDK_PATH ?= /opt/wasi-sdk

CC     = $(WASI_SDK_PATH)/bin/clang
CFLAGS = --target=wasm32-wasi -Wall -Wextra -Werror -Os \
         -nostdlib -Wl,--no-entry -Wl,--export-dynamic \
         -Wl,--allow-undefined -I src

SRC    = src/main.c
OUT    = ze03-nh3.wasm

.PHONY: all clean

all: $(OUT)

$(OUT): $(SRC) src/wokwi-api.h
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(OUT)

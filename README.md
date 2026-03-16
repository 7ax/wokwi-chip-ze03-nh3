# Wokwi ZE03-NH3 Custom Chip

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Wokwi custom chip simulating the **Winsen ZE03-NH3** electrochemical ammonia gas sensor (UART module).

## Usage

Add to your Wokwi project's `diagram.json`:

```json
{
  "type": "chip-ze03-nh3",
  "id": "nh3",
  "attrs": { "nh3_ppm": "5" }
}
```

In your project's `wokwi.toml`:

```toml
[chips.ze03-nh3]
url = "github:7ax/wokwi-chip-ze03-nh3@1.0.0"
```

## Pin Connections

| ZE03-NH3 Pin | Connect To    |
|--------------|---------------|
| VCC          | 5V            |
| GND          | GND           |
| TXD          | ESP32 RX (e.g. GPIO16) |
| RXD          | ESP32 TX (e.g. GPIO17) |

## Slider Control

Use the **NH3 (ppm)** slider (0-100 ppm) to set the simulated ammonia concentration.

## Protocol

UART: 9600 baud, 8N1

### Active Upload Mode (default)

Sensor transmits a 9-byte concentration frame every 1 second:

```
FF 86 [HIGH] [LOW] 00 00 00 00 [CHECKSUM]
```

Concentration (ppm) = HIGH * 256 + LOW

### Q&A Mode

Send read request:
```
FF 01 86 00 00 00 00 00 79
```

Response uses the same frame format as active upload.

### Mode Switching

| Command | Bytes |
|---------|-------|
| Switch to Q&A     | `FF 01 78 04 00 00 00 00 83` |
| Switch to Active   | `FF 01 78 03 00 00 00 00 84` |

### Checksum

Two's complement of the sum of bytes 1 through 7:

```
checksum = (~(byte1 + byte2 + ... + byte7) + 1) & 0xFF
```

## Datasheet

[Winsen ZE03 User's Manual (PDF)](https://www.winsen-sensor.com/d/files/ze03.pdf)

## Building

Requires [wasi-sdk](https://github.com/WebAssembly/wasi-sdk):

```bash
export WASI_SDK_PATH=/opt/wasi-sdk
make
```

## License

[MIT](LICENSE)

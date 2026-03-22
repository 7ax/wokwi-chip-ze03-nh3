# Wokwi ZE03-NH3 Custom Chip

[![Build WASM](https://github.com/7ax/wokwi-chip-ze03-nh3/actions/workflows/build.yaml/badge.svg)](https://github.com/7ax/wokwi-chip-ze03-nh3/actions/workflows/build.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Wokwi custom chip simulating the **Winsen ZE03-NH3** electrochemical ammonia gas sensor (UART module).

<p align="center">
  <img src="board/board.svg" alt="ZE03-NH3 board visual" width="270">
</p>

## Usage

Add to your project's `wokwi.toml`:

```toml
[chips.ze03-nh3]
url = "github:7ax/wokwi-chip-ze03-nh3@1.0.1"
```

Then reference it in `diagram.json`:

```json
{
  "type": "chip-ze03-nh3",
  "id": "nh3",
  "attrs": { "nh3_ppm": "5" }
}
```

## Pin Connections

| ZE03-NH3 Pin | Connect To |
|---|---|
| VCC | 5V |
| GND | GND |
| TXD | MCU RX (e.g. ESP32 GPIO16) |
| RXD | MCU TX (e.g. ESP32 GPIO17) |

## Slider Control

Use the **NH3 (ppm)** slider (0-100 ppm, integer) to set the simulated ammonia concentration in real time.

## Protocol

UART: 9600 baud, 8N1

### Active Upload Mode (default)

Sensor transmits a 9-byte concentration frame every 1 second:

```
FF 86 [HIGH] [LOW] 00 00 00 00 [CHECKSUM]
```

Concentration (ppm) = HIGH x 256 + LOW

### Q&A Mode

Host sends read request, sensor responds. Both long and short query formats are supported:

```
Long:  FF 01 86 00 00 00 00 00 79   (datasheet format)
Short: FF 86 00 00 00 00 00 00 7A   (Arduino library format)

Response: FF 86 [HIGH] [LOW] 00 00 00 00 [CHECKSUM]
```

### Mode Switching

| Command | Bytes |
|---|---|
| Switch to Q&A | `FF 01 78 04 00 00 00 00 83` |
| Switch to Active | `FF 01 78 03 00 00 00 00 84` |
| Response (OK) | `FF 78 01 00 00 00 00 00 87` |

### Calibration Commands

Zero and span calibration commands are accepted and acknowledged (stub — no internal state change):

| Command | Bytes |
|---|---|
| Zero calibration | `FF 01 87 00 00 00 00 00 78` |
| Span calibration | `FF 01 88 [HIGH] [LOW] 00 00 00 [CS]` |
| Zero cal response | `FF 87 01 00 00 00 00 00 78` |
| Span cal response | `FF 88 01 00 00 00 00 00 77` |

### Checksum

Two's complement of the sum of bytes 1 through 7:

```
checksum = (~(byte1 + byte2 + ... + byte7) + 1) & 0xFF
```

## Advanced Attributes

These attributes are set in `diagram.json` and are disabled by default:

| Attribute | Default | Description |
|---|---|---|
| `warmup_ms` | `0` | Warmup period in milliseconds. During warmup, concentration reads return 0 ppm. Set to `0` to disable (instant start). |
| `fault` | `0` | Fault injection. Set to `1` to simulate a dead sensor: all TX is suppressed and all commands are ignored. Toggle back to `0` to resume. Also available as a slider control. |

Example `diagram.json`:

```json
{
  "type": "chip-ze03-nh3",
  "id": "nh3",
  "attrs": { "nh3_ppm": "5", "warmup_ms": "30000" }
}
```

## Datasheet

[Winsen ZE03 User's Manual (PDF)](https://www.winsen-sensor.com/d/files/ze03.pdf)

## Building Locally

Requires [wasi-sdk](https://github.com/WebAssembly/wasi-sdk):

```bash
export WASI_SDK_PATH=/opt/wasi-sdk
make
```

Output: `dist/chip.wasm`

## License

[MIT](LICENSE)

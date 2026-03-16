# Credits

## Wokwi Chips API Header

- **Source:** https://github.com/wokwi/inverter-chip/blob/main/src/wokwi-api.h
- **License:** MIT
- **What was used:** `wokwi-api.h` header providing the Wokwi custom chips C API
- **Modifications:** None (used as-is)

## ZE03 UART Protocol Specification

- **Source:** Winsen ZE03 Electrochemical Gas Detection Module User's Manual V2.4
- **URL:** https://www.winsen-sensor.com/d/files/ze03.pdf
- **What was used:** UART frame format, checksum algorithm, command bytes, and timing specifications
- **License:** Datasheet used for reference only; no code borrowed

## Protocol Verification

- **Source:** https://github.com/fega/winsen-ze03-arduino-library
- **License:** MIT
- **What was used:** Cross-referenced command byte arrays and response parsing to verify datasheet interpretation
- **Modifications:** N/A (reference only, no code borrowed)

## UART Chip Pattern Reference

- **Source:** Wokwi GPS Neo6M custom chip example
- **URL:** https://github.com/upc-pre-202402-si572-sw74/iot-custom-chip-gps-neo6m-on-esp32-embedded-app-sw74
- **What was used:** Referenced UART initialization and timer-based periodic transmission pattern
- **Modifications:** N/A (pattern reference only, no code borrowed)

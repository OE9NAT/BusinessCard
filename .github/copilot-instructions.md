# AI Coding Agent Instructions

## Project Overview

This is a **business card PCB project** combining hardware design (KiCad) with embedded firmware (Arduino). The card features a USB-C powered Arduino Nano variant with health sensors (pulse/oxygen detection) and persistence-of-vision LED display for showing digits.

## Architecture

### Hardware (KiCad)
- **Main design**: `business_card_philipp/business_card_philipp.kicad_pro`
- **Schematic**: `business_card_philipp.kicad_sch` (9800+ lines, defines circuits)
- **PCB layout**: `business_card_philipp.kicad_pcb` (board traces and component placement)
- **Key subsystem**: `USB_connection.kicad_sch` (USB-C power delivery separate from main schematic)
- **Production files**: `business_card_philipp/production/` contains BOM, designators, and netlist for fabrication

### Firmware
- **Arduino sketch**: `Software/Blink_number/Blink_number.ino` 
- Target MCU: ATtiny85 (programmed via ICSP after Nano bootloader burn)
- **Functionality**: Persistence-of-vision digit display using 3 GPIO pins controlling 6 LEDs

## Key Development Patterns

### LED Matrix Control (ATtiny85)
The firmware uses a **3-pin, 6-LED multiplexed design** via persistence-of-vision:
```cpp
const int ledMap[6][2] = {         // [LED_index][{Anode, Cathode}]
  {0, 1}, {1, 0}, {1, 2}, ...
};
const int numPatterns[10][7] = {   // Digit patterns mapped to LED indices
  {2, 3, 4, -1, ...},              // Digit 0 uses LEDs 2,3,4
  ...
};
```
- Pins are switched between INPUT, OUTPUT-HIGH, OUTPUT-LOW to control current direction
- `povDelay = 40ms` creates display refresh; `displayDuration = 2000ms` per digit
- Pattern format: array of LED indices (-1 terminates list)

**When modifying patterns**: Update both `ledMap` spatial mapping AND `numPatterns` array to match desired digit shapes.

### Bootloader Workflow
1. Use Arduino Nano to **burn ATtiny85 bootloader** via ICSP
2. Flash desired firmware to the bootloader-enabled ATtiny85
3. ICSP connection documented in `business_card_philipp/ICSP_attiny85.png`

## Git Conventions

**Ignore these generated/backup files** (already in `.gitignore`):
- `business_card_philipp/business_card_philipp-backups/` — KiCad autosaves (large .zip files)
- `business_card_philipp/fp-info-cache` — Auto-generated footprint cache (50+ MB)

## Critical Files Reference

| File | Purpose |
|------|---------|
| `business_card_philipp/business_card_philipp.kicad_pro` | KiCad project root; open this in KiCad GUI |
| `business_card_philipp/business_card_philipp.kicad_sch` | Main schematic; defines power, USB, MCU circuits |
| `business_card_philipp/USB_connection.kicad_sch` | USB-C power delivery (separate schematic file) |
| `business_card_philipp/production/bom.csv` | Bill of materials for fabrication |
| `Software/Blink_number/Blink_number.ino` | Firmware; ATtiny85 digit display code |
| `docu/idear/` | Footprint definitions (`.kicad_mod` files) |

## Common Tasks

**Adding a new digit pattern**: Modify `numPatterns[10][7]` in `Blink_number.ino`, reference `ledMap[6][2]` for LED positions (pin-pair indices 0–5).

**Exporting for production**: KiCad generates production files to `business_card_philipp/production/` (BOM, positions, netlist). Verify fabrication house accepts IPC netlist format.

**Flashing firmware**: Use Arduino IDE with ATtiny85 board package; ensure bootloader exists on chip first (see Bootloader Workflow).

## Known Constraints

- **ATtiny85 MCU**: Limited GPIO (6 usable pins); multiplexing required for LED control
- **USB-C power only**: No battery; requires external USB source (5V)
- **ICSP programming**: Nano bootloader must be burned before flashing ATtiny85 firmware

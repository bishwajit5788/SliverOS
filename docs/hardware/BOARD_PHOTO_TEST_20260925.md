# SliverOS Hardware Photo & Physical Setup Experiment — 2026-09-25

## 1. Experiment Information

| Field | Value |
|---|---|
| **Experiment Date** | 2026-09-25 |
| **Project** | SliverOS |
| **Target Hardware** | 7Semi ESP32-S3 Development Board (`7Semi ESP32-S3-Dev-BoardC-1U-N8R8`) |
| **MCU Module Visible** | ESP32-S3-WROOM-1 (`MCN8R8` marking on RF shield) |
| **Host Computer** | Mac laptop (macOS development environment) |
| **Physical USB Link** | USB-C data cable connected to onboard USB port |
| **Physical Display Hardware** | None (Mac browser serves as graphical display via Web Serial) |
| **Evidence Type** | Physical Hardware Photographic Evidence |

---

## 2. Objective & Purpose

The purpose of this experiment is:

> "To record photographic evidence of the physical SliverOS ESP32-S3 development target and its USB-connected development setup before/alongside software and hardware validation."

### What These Photographs Establish:
1. The physical development board exists and is physically present in the development environment.
2. The specific development board used for SliverOS is the **7Semi ESP32-S3 Development Board**.
3. The **ESP32-S3-WROOM-1** module (specifically marked `MCN8R8`: 8 MB Flash, 8 MB Octal PSRAM) is installed on the board.
4. The board can be physically connected via USB-C to the Mac development workstation shown.

### What These Photographs DO NOT Independently Prove:
The photographic record documents physical hardware existence and setup only. The photographs **do not** prove:
- Firmware flashing success
- SliverOS boot and execution
- Web Serial communication or data throughput
- SLVR/1 protocol handshake or telemetry exchange
- PSRAM initialization or memory capacity
- Flash size verification
- Cooperative scheduler arbitration or budget overrun detection
- Watchdog (TWDT) timer servicing or recovery
- BLE-HID keyboard report generation
- Wi-Fi passive diagnostics functionality
- Network diagnostics ICMP/TCP operation
- Power-loss safe VFS block operations
- Core application execution

These functional items require physical execution against the validation test suite defined in [docs/HARDWARE_VALIDATION.md](../HARDWARE_VALIDATION.md).

---

## 3. Hardware Observed

The photographic evidence records the following physical hardware characteristics:

1. **Development Board Architecture**:
   - 7Semi branded ESP32-S3 DevKit board with matte black solder mask.
   - Dual USB-C connectors at the bottom edge.
   - Dual tactile momentary pushbuttons labeled `BOOT` and `RST`.
   - Onboard Silicon Labs CP2102N USB-UART bridge IC.
   - User status LED and power distribution circuitry.
   - Dual 22-pin breadboard-compatible GPIO headers along both board edges.
2. **RF Module Shielding**:
   - Espressif ESP32-S3-WROOM-1 module with integrated PCB trace inverted-F antenna.
   - Metal RF shield marked with:
     - `ESPRESSIF` logo
     - `ESP32-S3-WROOM-1`
     - Regulatory compliance logos: CE, FCC ID: `2AC7Z-ESPS3WROOM1`, IC: `21098-ESPS3WROOM1`, CMIIT ID: `2022DP2892`
     - `乐鑫信息科技 (上海) 股份有限公司`
     - Matrix 2D barcode
     - Variant designation: **`MCN8R8`** (8 MB Flash, 8 MB Octal PSRAM)
3. **Rear Silkscreen & Pinout**:
   - Silkscreen on rear: `7SEMI ESP32 S3 DEVKIT`.
   - Pinout labeling confirming GND, TX, RX, GPIO 1..48, 3V3, RST, and 5V rails.
   - Visible differential trace routing between USB-C connectors and the ESP32-S3 module.

---

## 4. Photographic Evidence

### 4.1 Photograph 01 — Board Front

![7Semi ESP32-S3 board front](../../assets/hardware/20260925/01_board_front.jpeg)

**Image Source:** `assets/hardware/20260925/01_board_front.jpeg`  
**Image Format:** JPEG (1024 × 1024, 277 KB)  
**Observations:**
- Top-down view of the 7Semi ESP32-S3 development board resting on a neutral workbench surface.
- The ESP32-S3-WROOM-1 module with PCB antenna is positioned at the top of the PCB.
- Below the module: passives, voltage regulation, status LED, and the Silicon Labs CP2102N bridge IC.
- Towards the bottom: `BOOT` pushbutton (left) and `RST` pushbutton (right).
- At the bottom edge: dual USB-C ports with a black USB-C cable inserted into the left port.
- Both lateral pin headers are clearly visible with silkscreen pin numbering (GPIO 0 through GPIO 48, power rails).

---

### 4.2 Photograph 02 — Board Connected to Mac

![7Semi ESP32-S3 board connected to Mac](../../assets/hardware/20260925/02_board_usb_connected.jpeg)

**Image Source:** `assets/hardware/20260925/02_board_usb_connected.jpeg`  
**Image Format:** JPEG (1024 × 1024, 300 KB)  
**Observations:**
- The 7Semi ESP32-S3 board is held in hand in front of an open Mac laptop keyboard.
- A black USB-C cable is plugged into the board's USB port, delivering power.
- The onboard blue LED is illuminated, confirming power delivery across the board's voltage rail.
- The Mac laptop screen in the background displays development code and text in a dark-mode terminal/editor.
- Note: This photograph confirms physical connectivity and power delivery; it does not independently establish active serial data communication.

---

### 4.3 Photograph 03 — ESP32-S3 Module Detail

![ESP32-S3-WROOM-1 module detail](../../assets/hardware/20260925/03_board_detail.jpeg)

**Image Source:** `assets/hardware/20260925/03_board_detail.jpeg`  
**Image Format:** JPEG (1024 × 1024, 352 KB)  
**Observations:**
- High-resolution close-up macro photograph centered on the metal RF shield of the ESP32-S3-WROOM-1 module.
- Silkscreen laser-etching on the can clearly reads:
  - `ESPRESSIF`
  - `ESP32-S3-WROOM-1`
  - `FCC ID: 2AC7Z-ESPS3WROOM1`
  - `IC: 21098-ESPS3WROOM1`
  - `CMIIT ID: 2022DP2892`
  - `MCN8R8`
- The `MCN8R8` code confirms 8 MB Flash and 8 MB Octal PSRAM hardware configuration.
- The integrated PCB meandering inverted-F antenna (MIFA) is visible at the top.
- Surface-mount components, decoupling capacitors, and the multi-color status LED are visible below the can.

---

### 4.4 Photograph 04 — Connected Board / Development Setup (Rear View)

![7Semi ESP32-S3 rear silkscreen and pinout](../../assets/hardware/20260925/04_board_connected.jpeg)

**Image Source:** `assets/hardware/20260925/04_board_connected.jpeg`  
**Image Format:** JPEG (1024 × 1024, 236 KB)  
**Observations:**
- The rear (bottom side) of the 7Semi board is held in hand in front of the Mac laptop keyboard.
- The rear silkscreen prominently displays the manufacturer brand and model: `7SEMI ESP32 S3 DEVKIT`.
- Complete pinout labeling is visible for both header columns, including `GND`, `TX`, `RX`, `01`–`48`, `3V3`, `RST`, and `5V`.
- USB-C connector through-hole solder anchor pads and differential trace pairs are visible leading from the USB ports.
- The black USB-C cable remains connected to the board.

---

## 5. Detailed Observations Summary

1. **Board Confirmation**: The board shown is without doubt a **7Semi ESP32-S3 DevKit** (`7Semi ESP32-S3-Dev-BoardC-1U-N8R8`). It is not an ESP32-C3, ESP32 Classic, or generic DevKitC-1.
2. **Silicon & Memory Confirmation**: The shield markings explicitly confirm **ESP32-S3-WROOM-1** with suffix **`MCN8R8`**, validating the 8 MB Flash and 8 MB Octal PSRAM configuration declared in the project's [sdkconfig.defaults](../../sdkconfig.defaults).
3. **Dual USB Ports**: The board contains two physical USB-C ports: one routing to the CP2102N bridge and one routing to the ESP32-S3 Native USB Serial/JTAG pins (GPIO 19/20).
4. **Physical Display**: Zero display hardware, ribbons, or TFT/OLED modules are attached to the board, consistent with the architecture where the Mac browser serves as the sole graphical interface.

---

## 6. Experiment Result

### Status:
**PASS — Physical hardware photographic evidence captured.**

### Scope of PASS:
> **This PASS applies strictly to the photographic documentation and physical presence of the development hardware setup.**
> It confirms that the physical 7Semi ESP32-S3 board with the ESP32-S3-WROOM-1 MCN8R8 module is present, connected to the Mac development workstation, and properly documented.
>
> **This result DOES NOT imply that the 25-point SliverOS hardware bench functional validation matrix has passed.** All physical hardware operational tests remain classified according to their actual bench execution state.

---

## 7. Limitations

- **Visual Evidence Only**: Photographs capture static hardware state and power indicator illumination; they do not verify runtime software execution, memory allocation, or serial protocols.
- **Physical Tests Pending**: Tests for boot sequence, SLVR/1 handshake, Web Serial communication, scheduler timing, application execution, and fault recovery must be run and documented through actual serial telemetry logs.

---

## 8. Related Documentation & Validation Cross-References

- [docs/HARDWARE_VALIDATION.md](../HARDWARE_VALIDATION.md): Complete 25-point hardware bench validation matrix.
- [docs/HARDWARE.md](../HARDWARE.md): Full hardware specifications and pinout reference.
- [docs/ARCHITECTURE.md](../ARCHITECTURE.md): System architecture and Mac Web Host interface model.
- [README.md](../../README.md): Project overview and directory structure.

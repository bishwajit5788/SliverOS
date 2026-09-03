# Hardware Specifications & Pinout Reference

## Target Platform Classification

| Classification | Target Device | Architecture | Memory Configuration | Primary Interfaces |
|---|---|---|---|---|
| **PRIMARY RELEASE TARGET** | **ESP32-S3-DevKitC-1** | Xtensa 32-bit LX7 Dual-Core @ 240 MHz | **8 MB Octal Flash, 8 MB PSRAM** | Native USB-Serial/JTAG, BLE 5.0, Wi-Fi 4 |
| **FUTURE PORTING TARGET** | ESP32-DevKitC | Xtensa 32-bit LX6 Dual-Core @ 240 MHz | 4 MB Quad Flash, 520 KB SRAM | CP2102 USB-UART, BLE 4.2, Wi-Fi 4 |
| **FUTURE PORTING TARGET** | ESP32-C3-DevKitM-1 | RISC-V 32-bit RV32IMC @ 160 MHz | 4 MB Quad Flash, 400 KB SRAM | USB-UART, BLE 5.0, Wi-Fi 4 |

---

## ESP32-S3-DevKitC-1 Default Pinout Mapping

### 1. Game Controller & Graphical Navigation
Active-low pushbuttons with internal software debouncing:

| Function | ESP32-S3 GPIO | Mode | Active Level |
|---|---|---|---|
| **D-Pad UP** | GPIO 12 | Input (Pull-Up) | LOW |
| **D-Pad DOWN** | GPIO 13 | Input (Pull-Up) | LOW |
| **D-Pad LEFT** | GPIO 14 | Input (Pull-Up) | LOW |
| **D-Pad RIGHT** | GPIO 21 | Input (Pull-Up) | LOW |
| **Action Button A** | GPIO 47 | Input (Pull-Up) | LOW |
| **Action Button B (Diag)** | GPIO 48 | Input (Pull-Up) | LOW |
| **Status Indicator LED** | GPIO 2 | Output | HIGH |

---

### 2. High-Speed SPI Display Interface (ST7789 / SSD1306)
Hardware SPI2/FSPI bus running at 20 MHz with DMA:

| Signal | ESP32-S3 GPIO | Description |
|---|---|---|
| **MOSI** | GPIO 11 | Master Out Slave In (Serial data line) |
| **SCLK** | GPIO 10 | Serial Clock (20 MHz) |
| **CS** | GPIO 9 | Chip Select (Active LOW) |
| **D/C** | GPIO 8 | Data / Command selector (LOW = Cmd, HIGH = Data) |
| **RST** | GPIO 3 | Hardware Display Reset line |

---

### 3. Native USB Interface (ESP32-S3-DevKitC-1)

| Signal | ESP32-S3 GPIO | Description |
|---|---|---|
| **USB D- (DN)** | GPIO 19 | Native USB Differential Data Negative |
| **USB D+ (DP)** | GPIO 20 | Native USB Differential Data Positive |

---

## Flash Memory Layout (8MB Flash with A/B OTA Support)

Configured in `partitions.csv`:

```text
Offset       Size       Partition Name  Type   SubType  Description
────────────────────────────────────────────────────────────────────────────────
0x0000_0000  28 KB      bootloader      boot   -        2nd-Stage ESP-IDF Bootloader
0x0000_8000  4 KB       partitions      part   -        Partition Table (partitions.csv)
0x0000_9000  24 KB      nvs             data   nvs      Non-Volatile Storage (WiFi/Keys)
0x0000_F000  8 KB       otadata         data   ota      OTA Selection / Rollback State
0x0001_1000  4 KB       phy_init        data   phy      PHY Radio Calibration Data
0x0002_0000  2.5 MB     ota_0           app    ota_0    Application Slot A (Primary)
0x002A_0000  2.5 MB     ota_1           app    ota_1    Application Slot B (Future OTA)
0x0052_0000  2.6 MB     osfs            data   spiffs   Custom Power-Loss Safe VFS
────────────────────────────────────────────────────────────────────────────────
Total Allocated: ~7.8 MB (Fits completely within 8MB SPI NOR Flash)
```

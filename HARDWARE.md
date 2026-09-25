# Hardware Specifications & Pinout Reference

## 1. Target Hardware Classification

| Parameter | Specification |
|---|---|
| **Target Board** | **7Semi ESP32-S3-Dev-BoardC-1U-N8R8** |
| **SoC / Silicon** | Espressif ESP32-S3 (Xtensa 32-bit LX7 Dual-Core @ 240 MHz) |
| **Module** | ESP32-S3-WROOM-1 MCN8R8 |
| **Internal SRAM** | 512 KiB (128 KiB allocated to SliverOS static arena) |
| **External PSRAM** | 8 MB Octal SPI External RAM (`CONFIG_SPIRAM_MODE_OCT=y`) |
| **Flash Memory** | 8 MB Quad/Octal SPI Flash (DIO Mode @ 80 MHz) |
| **Native USB** | Built-in USB Serial/JTAG Controller connected directly to USB-C port |
| **Serial Bridges** | Secondary Silicon Labs CP2102N USB-UART bridge |
| **Wireless** | 2.4 GHz Wi-Fi 4 (802.11 b/g/n) & Bluetooth 5.0 (LE) |
| **Physical Display** | **NONE**. The Mac browser serves as the graphical display via Web Serial. |

---

## 2. Pinout & Peripheral Assignment

### 1. Native USB Interface (Primary SliverOS Host Link)
| Signal | ESP32-S3 GPIO | Description |
|---|---|---|
| **USB D- (DN)** | GPIO 19 | Native USB Serial/JTAG differential data negative |
| **USB D+ (DP)** | GPIO 20 | Native USB Serial/JTAG differential data positive |

Connected directly to the onboard USB-C port labeled **USB**. Enumerates as `/dev/cu.usbmodem101` on macOS.

### 2. Secondary UART Interface (CP2102N Bridge)
| Signal | ESP32-S3 GPIO | Description |
|---|---|---|
| **UART0 TX** | GPIO 43 | Asynchronous Serial Transmit |
| **UART0 RX** | GPIO 44 | Asynchronous Serial Receive |

Connected to the secondary onboard USB-C port labeled **UART**.

### 3. Controls & Status Indicators
| Function | ESP32-S3 GPIO | Type | Active Level |
|---|---|---|---|
| **Status Indicator LED** | GPIO 2 | Digital Output | HIGH (Active) |
| **BOOT Button** | GPIO 0 | Digital Input (Pull-Up) | LOW (Press) |
| **RST Button** | CHIP_PU / EN | Hardware Reset | LOW (Hardware pulse) |

### 4. Optional Hardware Input Pushbuttons (Debounced)
| Function | ESP32-S3 GPIO | Mode | Active Level |
|---|---|---|---|
| **D-Pad UP** | GPIO 12 | Input (Pull-Up) | LOW |
| **D-Pad DOWN** | GPIO 13 | Input (Pull-Up) | LOW |
| **D-Pad LEFT** | GPIO 14 | Input (Pull-Up) | LOW |
| **D-Pad RIGHT** | GPIO 21 | Input (Pull-Up) | LOW |
| **Action Button A** | GPIO 47 | Input (Pull-Up) | LOW |
| **Action Button B** | GPIO 48 | Input (Pull-Up) | LOW |

---

## 3. Physical Display Architecture Notice

> [!IMPORTANT]
> **NO PHYSICAL SPI TFT / OLED DISPLAY HARDWARE**
> 
> SliverOS has eliminated all dependencies on local SPI displays, display DMA buffers, and local monochrome framebuffers. 
> 
> The previous implementation assigned SPI DC to GPIO 19, which clashed directly with Native USB D-. Removing the physical display permanently resolved this hardware conflict, dedicating GPIO 19 and GPIO 20 exclusively to Native USB Serial/JTAG.
> 
> The Mac browser window (via Web Serial API) serves as the primary graphical display and desktop host for SliverOS.

---

## 4. Flash Partition Layout (8 MB Flash)

Configured in `partitions.csv`:

```text
Offset       Size       Partition Name  Type   SubType  Description
────────────────────────────────────────────────────────────────────────────────
0x0000_0000  28 KB      bootloader      boot   -        1st-Stage ROM / 2nd-Stage Bootloader
0x0000_8000  4 KB       partitions      part   -        Partition Table (partitions.csv)
0x0000_9000  24 KB      nvs             data   nvs      Non-Volatile Storage (WiFi, Calibration)
0x0000_F000  8 KB       otadata         data   ota      OTA Selection Record
0x0001_1000  4 KB       phy_init        data   phy      RF Calibration Data
0x0002_0000  2.5 MB     ota_0           app    ota_0    SliverOS Application Slot 0 (Active)
0x002A_0000  2.5 MB     ota_1           app    ota_1    SliverOS Application Slot 1 (OTA Backup)
0x0052_0000  2.625 MB   osfs            data   spiffs   SliverOS Power-Safe Virtual File System
```

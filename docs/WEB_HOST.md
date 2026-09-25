# SliverOS Web Host Specification & Architecture

## 1. Overview

**SliverOS Web Host** is a modern, responsive web application hosted on Vercel that serves as the desktop operating environment, graphical display, and debug monitor for the ESP32-S3 hardware running the SliverOS executive.

- **URL Deployment**: Vercel HTTPS Deployment
- **Core Technology**: Native HTML5, ES Modules JavaScript, Vanilla CSS with Glassmorphism
- **Host Connection**: Web Serial API (`navigator.serial`)
- **Primary Client**: Chromium / Google Chrome on macOS
- **Hardware Interface**: Direct USB-C connection to 7Semi ESP32-S3 Native USB Serial/JTAG (`/dev/cu.usbmodem101`)

---

## 2. Web Serial Permission & Connection Workflow

In accordance with web security best practices, the Web Host **never** attempts silent port access. All communication requires explicit user consent:

1. **User Navigation**: User opens the Vercel-hosted SliverOS web application over HTTPS.
2. **Device Detection Prompt**: The top bar displays connection status `DISCONNECTED` with a green `CONNECT ESP32-S3` button.
3. **Explicit Consent**: User clicks `CONNECT ESP32-S3`. The browser triggers the native Web Serial port picker dialog filtered by Espressif USB vendor IDs (`0x303A`, `0x10C4`, etc.).
4. **Port Selection**: User explicitly selects the `ESP32-S3` or `USB JTAG/serial debug unit`.
5. **Port Configuration**: The browser opens the port at 115200 baud with 8-N-1 framing.
6. **SLVR/1 Handshake**:
   - Web Host sends `SLVR_CMD_CONNECT`.
   - Firmware responds with `SLVR_MSG_HELLO` (verifying protocol version `0x01`).
   - Web Host requests `SLVR_CMD_GET_INFO` and receives `SLVR_MSG_DEVICE_INFO`.
   - Web Host validates hardware identity (`7Semi ESP32-S3-N8R8`).
7. **Desktop Active**: The status bar transitions to `CONNECTED` (green pulse), displays silicon revision, clock frequency, and memory stats. Telemetry streams update in real-time.
8. **Clean Disconnection**:
   - If the user clicks `DISCONNECT` or physically unplugs the USB cable, the `disconnect` event fires.
   - The UI resets immediately to the `DISCONNECTED` state, releasing all reader/writer locks without browser freezing.

---

## 3. Web Desktop Subsystems & Navigation

The interface provides an intuitive desktop experience with a sidebar and floating windows:

### 1. Top Bar
- **System Logo**: Glowing `SliverOS` branding with active version badge.
- **Connection Badge**: Color-coded pulse indicator (`CONNECTED` / `CONNECTING` / `DISCONNECTED` / `SIMULATED`).
- **Telemetry Indicators**: Real-time display of CPU Usage %, Uptime, Internal SRAM arena usage, and PSRAM allocation.
- **Action Buttons**: `CONNECT ESP32-S3`, `SIMULATE DEVICE` (Mock mode), `DISCONNECT`.

### 2. Main Desktop (Dashboard)
- **App Launcher**: Instant launch cards for the 4 core applications.
- **System Overview**: Live gauges for CPU load, scheduler tick rate, active task, and memory breakdown.
- **Hardware Card**: Detected chip, flash mode, PSRAM capacity, and partition table layout.

### 3. Application Windows (The Four Apps)
1. **BLE-HID Macro**:
   - Displays macro files stored on the ESP32 VFS (`login.slm`, `payload.slm`, etc.).
   - Execution controls: `EXECUTE MACRO`, `STOP`.
   - Live keystroke execution status and report log.
2. **Wi-Fi Diagnostics**:
   - Strictly passive 802.11 beacon, probe, and channel telemetry received from the ESP32.
   - Channel spectrum distribution chart.
   - Zero credential or PMKID harvesting.
3. **Network Diagnostics**:
   - Authorized ICMP reachability ping and TCP port probes (Port 22 SSH, Port 80 HTTP, Port 443 HTTPS).
   - Real measured RTT values reported by the firmware (no fabricated metrics).
4. **Retro Games (Space Micro-Lander)**:
   - Real-time embedded game logic executes on the ESP32-S3 at 60 Hz.
   - Firmware streams compact vector telemetry (`SLVR_MSG_GAME_STATE`).
   - Browser renders smooth 60 FPS vector graphics on HTML5 Canvas (lander craft, lunar terrain, thruster particles, fuel gauge, velocity vector).
   - On-screen touch/keyboard controls (Arrow Keys, Thrust button) transmit `SLVR_CMD_APP_INPUT` back to the ESP32.

### 4. Interactive Developer Terminal
- Full command console (`root@sliver:~#`).
- Supported commands:
  - `help`: Command listing and usage.
  - `apps`: List installed applications and active app status.
  - `status`: Executive uptime, scheduler tick, and fault counters.
  - `mem`: Detailed SRAM arena, pool, and PSRAM memory breakdown.
  - `tasks`: Scheduler task control blocks, priorities, and periods.
  - `vfs`: VFS block count, allocated records, and wear metrics.
  - `reboot`: Soft reset of the SliverOS executive.
  - `clear`: Clear terminal buffer.

### 5. Real-Time Device Monitor
- Graphs and live telemetry tables updating every 500 ms via rate-limited telemetry frames.

### 6. Official ESP32-S3 Flasher
- Preserved Web Serial flasher using Espressif ROM protocol.
- Strict hardware check: rejects non-ESP32-S3 devices.
- SHA-256 pre-verification of bootloader, partition table, and application binaries.

---

## 4. In-Browser Hardware Simulator Mode

For automated UI testing and local development without physical hardware, the Web Host includes a built-in mock simulator (`simulator.js`):

- **Activation**: Clicking the `SIMULATE DEVICE` button in the top bar.
- **Visual Distinction**: A prominent amber badge displays `SIMULATED DEVICE` on the navigation bar, app windows, and terminal.
- **Protocol Fidelity**: The simulator encodes and decodes genuine `SLVR/1` binary frames, responding to commands (`CONNECT`, `GET_INFO`, `LAUNCH_APP`, `APP_INPUT`, etc.) and generating realistic lunar lander physics and terminal responses.
- **Zero Confusion**: Device telemetry is clearly tagged as simulated (`SLVR_FLAG_SIMULATED`). Real hardware telemetry is never mixed with simulated data.

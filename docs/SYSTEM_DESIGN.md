# SliverOS System Design

This document describes the SliverOS system architecture and host-interface model.

## High-Level Architecture

```mermaid
flowchart TB
    subgraph HOST[Mac Browser Web Host]
        WEB[SliverOS Web Host<br/>Chrome / Edge via Web Serial]
        DESK[Desktop Windows & Controls]
        TERM[Interactive Terminal]
        MON[Real-Time Device Monitor]
        CANV[60 FPS Retro Vector Canvas]
    end

    subgraph ESP[ESP32-S3 Hardware: 7Semi Dev-BoardC-1U-N8R8]
        USB[Native USB Serial/JTAG + SLVR/1 Host Protocol]
        K[SliverOS Cooperative Executive]
        S[Scheduler]
        M[Internal SRAM Static Arena]
        P[8MB Octal PSRAM]
        E[Event Bus Ring Buffer]
        V[VFS / OSFS Block Storage]
        H[HAL]

        subgraph FOUR[Exactly Four Core Applications]
            B[BLE-HID Macro]
            W[Wi-Fi Diagnostics]
            N[Network Diagnostics + Network Lab Terminal]
            R[Retro Games Engine]
        end
    end

    WEB <-->|SLVR/1 Binary Protocol via Web Serial| USB

    USB <--> K
    K --> S
    K --> M
    K --> P
    K --> E
    K --> V
    K --> H
    S --> FOUR
    FOUR --> H
    H --> ESPHW[ESP32-S3 Wi-Fi / BLE / GPIO / USB]
```

## Runtime Ownership

The ESP32-S3 owns embedded system execution. The Mac browser is the graphical display and host control console.

```mermaid
sequenceDiagram
    participant B as Mac Browser Web Host
    participant U as USB Protocol Service
    participant K as SliverOS Executive
    participant A as Active App

    B->>U: SLVR_CMD_CONNECT
    U-->>B: SLVR_MSG_HELLO (v0x01)
    B->>U: SLVR_CMD_GET_INFO
    U-->>B: SLVR_MSG_DEVICE_INFO
    loop Periodic Telemetry
        K->>U: Collect status & memory
        U-->>B: SLVR_MSG_DEVICE_STATUS / MEMORY_STATUS
    end
    B->>U: SLVR_CMD_LAUNCH_APP (e.g. Retro Games)
    U->>K: Launch app
    K->>A: Cooperative dispatch
    loop 60 Hz Physics Tick
        A->>U: Packed game state (14 bytes)
        U-->>B: SLVR_MSG_GAME_STATE
        B-->>B: Render 60 FPS HTML5 Canvas
        B->>U: SLVR_CMD_APP_INPUT (Thrust / Tilt)
        U->>A: Apply input to physics
    end
```

## Web Flasher & Web Host Integration

Both firmware installation and runtime interaction are unified within the SliverOS Web Host:

```text
NEW OR UNFLASHED HARDWARE
             │
             ▼
    Web Flasher Mode ──► Detect Chip ──► Verify Target ──► Flash Binaries ──► Hard Reset
                                                                                   │
                                                                                   ▼
                                                                       SliverOS Executive Boot
                                                                                   │
                                                                                   ▼
                                                                        SLVR/1 Protocol Handshake
                                                                                   │
                                                                                   ▼
                                                                         SliverOS Web Desktop
                                                              ┌────────────────────┼────────────────────┐
                                                              ▼                    ▼                    ▼
                                                        4 Core Apps       Developer Terminal     Device Monitor
```

## Network Lab Boundary

The Network Lab belongs inside the Network Diagnostics application so the project maintains exactly four application modules.

```text
Network Diagnostics
├── Target configuration
├── Bounded ICMP reachability checks
├── Bounded TCP service checks (22, 80, 443)
├── Wi-Fi passive metadata scan
├── Local authorized host discovery
└── Safe Terminal command parser (network_terminal.c)
    ├── help
    ├── status
    ├── wifi scan
    ├── network scan <authorized target>
    ├── port check <authorized host> <port>
    └── clear
```

The terminal is strictly bounded to authorized diagnostics. Credential harvesting, PMKID capture, deauthentication, password cracking, and attack automation are blocked by safety policy.

## Hardware & Validation Status

- **Target Hardware**: 7Semi ESP32-S3-Dev-BoardC-1U-N8R8 (8 MB Flash, 8 MB Octal PSRAM, Native USB Serial/JTAG).
- **Physical Display**: None required; zero SPI DMA framebuffer overhead.
- **Host Unit Tests**: 12/12 passing (100%).
- **Web Host Tests**: 15/15 passing (100%).
- **Physical Bench Matrix**: 25 points defined in `docs/HARDWARE_VALIDATION.md`, prepared for bench execution.

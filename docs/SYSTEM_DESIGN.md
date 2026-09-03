# SliverOS System Design

This document describes the current system boundary and the planned host-interface model.

## High-Level Architecture

```mermaid
flowchart TB
    subgraph HOST[Optional Host Computers]
        MAC[macOS Host Monitor]
        WIN[Windows Host Monitor]
        LIN[Linux Host Monitor]
        WEB[Web Flasher<br/>Chrome / Edge]
    end

    subgraph ESP[ESP32-S3 Device]
        USB[USB Transport + SliverOS Host Protocol]
        UI[Graphical UI Runtime]
        K[SliverOS Cooperative Executive]
        S[Scheduler]
        M[Internal SRAM Memory Manager]
        E[Event Bus]
        V[VFS / OSFS]
        H[HAL]
        D[Physical SPI Display]

        subgraph FOUR[Exactly Four Apps]
            B[BLE-HID Macro]
            W[Wi-Fi Diagnostics]
            N[Network Diagnostics + Network Lab Terminal]
            R[Retro Games]
        end
    end

    MAC <-->|USB protocol| USB
    WIN <-->|USB protocol| USB
    LIN <-->|USB protocol| USB
    WEB -->|Web Serial| USB

    USB <--> UI
    UI <--> K
    K --> S
    K --> M
    K --> E
    K --> V
    K --> H
    S --> FOUR
    FOUR --> H
    UI --> D
    H --> ESPHW[ESP32-S3 Wi-Fi / BLE / GPIO / SPI / USB]
```

## Runtime Ownership

The ESP32-S3 owns execution. The desktop host is optional.

```mermaid
sequenceDiagram
    participant H as Host Monitor
    participant U as USB Protocol
    participant K as SliverOS Executive
    participant A as Active App

    H->>U: HELLO / DEVICE_INFO
    U->>K: Request
    K-->>U: Response / telemetry
    U-->>H: Device state
    H->>U: INPUT_EVENT
    U->>K: Input event
    K->>A: Cooperative dispatch
    A-->>K: Result / state
    K-->>U: UI / status event
    U-->>H: Render/update
```

## Web Flasher vs Host Monitor

These are separate products sharing the USB connection at different stages.

```text
NEW DEVICE
    │
    ▼
Web Flasher ──► Detect ──► Verify ──► Flash ──► Reboot
                                                   │
                                                   ▼
                                              SliverOS OS
                                                   │
                                  ┌────────────────┴────────────────┐
                                  ▼                                 ▼
                           Local SPI Display             Optional Host Monitor
                                                            │
                                              ┌─────────────┼─────────────┐
                                              ▼             ▼             ▼
                                           macOS         Windows        Linux
```

## Network Lab Boundary

The Network Lab belongs inside the existing Network Diagnostics application so the project keeps exactly four application modules.

Supported direction:

```text
Network Diagnostics
├── Target configuration
├── ICMP reachability
├── Bounded TCP service checks
├── Wi-Fi metadata scan
├── Local authorized host discovery
└── Terminal command interface
    ├── help
    ├── status
    ├── wifi scan
    ├── network scan <authorized target>
    ├── port check <authorized host> <port>
    └── clear
```

The terminal is intentionally bounded to authorized diagnostics and defensive lab use. Credential harvesting, PMKID capture, deauthentication, password cracking, exploitation automation, and unrestricted attack commands are outside the SliverOS feature boundary.

## Implementation Status

- ESP32-S3 cooperative executive: implementation in progress / hardening.
- Graphical launcher: implemented.
- Four application model: defined.
- Network terminal parser: foundation implemented.
- Web Flasher: online on Vercel; physical installation validation remains required.
- macOS/Windows/Linux Host Monitor: architecture defined; desktop implementation remains planned.
- Physical hardware validation: required before release readiness.

# Security Architecture & Threat Model

## 1. Passive Wi-Fi Diagnostics Safety Model

The Wi-Fi Diagnostics module operates under strict ethical and safety rules:
- **Strictly Passive Monitoring**: Uses promiscuous receive mode without injecting deauthentication packets, association requests, or probe responses.
- **Safe Metadata Only**: Records timestamp, RSSI, channel, frame length, frame type (Mgmt/Ctrl/Data), and frame subtype (Beacon/Probe).
- **Prohibited Operations**:
  - NO PMKID or EAPOL handshake extraction.
  - NO credential harvesting or dictionary attacks.
  - NO storage of network authentication secrets or plain-text payload frames.
  - Frame buffers in the promiscuous callback are immediately dropped after extracting the 10-byte metadata struct into the bounded ring buffer.

---

## 2. Safe Network Diagnostics Scope

The Network Diagnostics module:
- Only operates against explicitly configured, authorized local IP addresses (e.g. `127.0.0.1` or the local default gateway).
- Performs only standard ICMP ping and non-blocking TCP connect checks on ports 22 (SSH), 80 (HTTP), and 443 (HTTPS).
- Does NOT perform port sweeps, subnet scans, OS fingerprinting, stealth TCP flags (SYN/FIN/XMAS), or exploit payloads.

---

## 3. Firmware Integrity vs Authenticity

The Web Flasher and build automation implement strict checks, but it is critical to distinguish **Integrity** from **Authenticity**:

### Integrity (Implemented):
- The Web Flasher uses `crypto.subtle.digest('SHA-256')` to verify that downloaded binary artifacts match the manifest byte-for-byte before writing to flash.
- After burning flash sectors, the flasher queries the ESP32 ROM bootloader via `ESP_CMD_SPI_FLASH_MD5` to ensure the flash bits match the intended binary.
- This prevents corrupted flashes, partial downloads, and bit rot.

### Authenticity (Production Requirement):
- A matching SHA-256 only proves that the binary matches what was published in `releases.json`. It does NOT guarantee that the binary was authored by a trusted authority.
- In production, hardware-enforced **ESP32 Secure Boot v2** must be enabled:
  1. An RSA-3072 or ECDSA private signing key generates cryptographic signatures appended to the firmware image header.
  2. The corresponding public key hash is burned into ESP32 Efuses (which are write-protected and read-protected).
  3. The ESP32 ROM bootloader verifies the digital signature before booting any firmware from flash.

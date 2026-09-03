#!/usr/bin/env python3
"""
package_firmware.py
Automates packaging of ESP-IDF build binaries into release manifests
and copies distribution assets to the Web Flasher firmware directory.
ESP32-S3 is configured as the PRIMARY target.
"""

import os
import sys
import json
import hashlib
import shutil
import argparse

def compute_sha256(filepath):
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()

def create_sample_binaries(output_dir, target):
    """Generates development test fixtures labeled explicitly to prevent accidental hardware flashing."""
    os.makedirs(output_dir, exist_ok=True)
    images = [
        (f"bootloader_{target}.bin", 28672, 0x1000 if target == "esp32" else 0x0),
        ("partition-table.bin", 3072, 0x8000),
        (f"microkernel-{target}.bin", 419430, 0x20000 if target == "esp32s3" else 0x10000)
    ]
    manifest_records = []
    for fname, size, offset in images:
        fpath = os.path.join(output_dir, fname)
        # Generate test fixture with explicit guard banner
        banner = b"TEST FIXTURE - NOT FOR HARDWARE FLASHING - ESP32 MOCK BINARY\x00"
        data = bytearray(b"\xE9\x03\x02\x20") # ESP header magic
        data.extend(banner)
        data.extend(os.urandom(size - len(data)))
        with open(fpath, "wb") as f:
            f.write(data)

        sha = compute_sha256(fpath)
        actual_size = os.path.getsize(fpath)
        manifest_records.append({
            "name": fname.split(".")[0].replace("_", " ").title(),
            "offset": f"0x{offset:X}",
            "filename": fname,
            "size": actual_size,
            "sha256": sha,
            "is_fixture": True
        })
    return manifest_records

def main():
    parser = argparse.ArgumentParser(description="MicroKernel OS Firmware Packaging Utility")
    parser.add_argument("--build-dir", default="build", help="ESP-IDF build output directory")
    parser.add_argument("--target", default="esp32s3", choices=["esp32s3", "esp32", "esp32c3", "all"],
                        help="Target chip architecture (default: esp32s3 primary)")
    parser.add_argument("--version", default="1.0.0", help="Firmware release version")
    parser.add_argument("--flasher-dir", default="flasher/public/firmware", help="Destination flasher directory")
    parser.add_argument("--generate-sample", action="store_true",
                        help="Generate mock test fixtures for frontend development")
    args = parser.parse_args()

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    flasher_dest = os.path.join(repo_root, args.flasher_dir)
    os.makedirs(flasher_dest, exist_ok=True)
    manifest_path = os.path.join(repo_root, "firmware", "manifests", "releases.json")

    print(f"[*] Packaging MicroKernel OS firmware for target: {args.target} (v{args.version})")

    # Load or initialize manifest data
    manifest_data = None
    if os.path.exists(manifest_path):
        try:
            with open(manifest_path, "r") as f:
                manifest_data = json.load(f)
        except Exception:
            manifest_data = None

    if not manifest_data or "releases" not in manifest_data:
        manifest_data = {
            "product": "MicroKernel OS",
            "latest": args.version,
            "primaryTarget": "esp32s3",
            "releases": [
                {
                    "version": args.version,
                    "releaseDate": "2026-09-04",
                    "targets": {}
                }
            ]
        }

    release_entry = manifest_data["releases"][0]
    if "targets" not in release_entry:
        release_entry["targets"] = {}

    targets_to_build = ["esp32s3", "esp32", "esp32c3"] if args.target == "all" else [args.target]

    if args.generate_sample:
        print("[*] Generating development test fixture binaries (labeled with TEST FIXTURE guard)...")
        for t in targets_to_build:
            t_images = create_sample_binaries(flasher_dest, t)
            release_entry["targets"][t] = {
                "chip": t.upper(),
                "flashSize": "8MB" if t == "esp32s3" else "4MB",
                "is_primary": (t == "esp32s3"),
                "images": t_images
            }
    else:
        # Strict production packaging from ESP-IDF build directory
        for t in targets_to_build:
            app_bin = os.path.join(args.build_dir, "microkernel-esp32.bin")
            boot_bin = os.path.join(args.build_dir, "bootloader", "bootloader.bin")
            part_bin = os.path.join(args.build_dir, "partition_table", "partition-table.bin")

            missing = [f for f in [app_bin, boot_bin, part_bin] if not os.path.exists(f)]
            if missing:
                print(f"[FATAL ERROR] Expected build artifacts are missing in '{args.build_dir}':")
                for m in missing:
                    print(f"  - {m}")
                print("\nPlease run 'idf.py build' first or pass '--generate-sample' for development testing.")
                sys.exit(1)

            dest_boot = os.path.join(flasher_dest, f"bootloader_{t}.bin")
            dest_part = os.path.join(flasher_dest, "partition-table.bin")
            dest_app = os.path.join(flasher_dest, f"microkernel-{t}.bin")

            shutil.copy2(boot_bin, dest_boot)
            shutil.copy2(part_bin, dest_part)
            shutil.copy2(app_bin, dest_app)

            offset_boot = 0x1000 if t == "esp32" else 0x0
            offset_part = 0x8000
            offset_app = 0x20000 if t == "esp32s3" else 0x10000

            release_entry["targets"][t] = {
                "chip": t.upper(),
                "flashSize": "8MB" if t == "esp32s3" else "4MB",
                "is_primary": (t == "esp32s3"),
                "images": [
                    {
                        "name": "Bootloader",
                        "offset": f"0x{offset_boot:X}",
                        "filename": f"bootloader_{t}.bin",
                        "size": os.path.getsize(dest_boot),
                        "sha256": compute_sha256(dest_boot)
                    },
                    {
                        "name": "Partition Table",
                        "offset": f"0x{offset_part:X}",
                        "filename": "partition-table.bin",
                        "size": os.path.getsize(dest_part),
                        "sha256": compute_sha256(dest_part)
                    },
                    {
                        "name": "MicroKernel OS",
                        "offset": f"0x{offset_app:X}",
                        "filename": f"microkernel-{t}.bin",
                        "size": os.path.getsize(dest_app),
                        "sha256": compute_sha256(dest_app)
                    }
                ]
            }

    os.makedirs(os.path.dirname(manifest_path), exist_ok=True)
    with open(manifest_path, "w") as f:
        json.dump(manifest_data, f, indent=2)

    shutil.copy2(manifest_path, os.path.join(flasher_dest, "releases.json"))

    print(f"[✓] Successfully packaged {len(targets_to_build)} target(s) into {flasher_dest}")
    print(f"[✓] Release manifest updated at {manifest_path}")

if __name__ == "__main__":
    main()

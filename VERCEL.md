# SliverOS Web Flasher — Vercel Deployment

## Live Installer

**Production URL:** https://sliver-o232adi1r-bishwajit5788.vercel.app/

The SliverOS Web Flasher is hosted on Vercel and provides the browser-based installation interface for ESP32-S3 devices.

## Deployment Configuration

- Repository: `bishwajit5788/SliverOS`
- Website root: `flasher/`
- Framework: Vite
- Build command: `npm run build`
- Output directory: `dist`
- Deployment platform: Vercel

## Beginner Installation

1. Open the live installer in Chrome or Microsoft Edge.
2. Connect an ESP32-S3 using a USB data cable.
3. Click **Connect Device**.
4. Select the ESP32-S3 serial device in the browser dialog.
5. Confirm the detected device and firmware target.
6. Start the SliverOS installation.
7. Keep the USB cable connected until flashing and verification complete.

> The live URL hosts the web interface. Successful real-device flashing still requires the browser Web Serial path, firmware manifest/images, and ESP32-S3 bootloader/flashing implementation to be operational.

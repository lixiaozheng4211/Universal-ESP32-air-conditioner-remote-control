# Universal AC Android APK

This directory is for Android release artifacts.

## Install

1. Build or copy `app-debug.apk` into this directory.
2. Transfer the APK to the Android phone.
3. Enable installing apps from the selected file manager or browser.
4. Install the APK.

## Hardware

- Android phone with USB OTG support.
- USB-C OTG cable.
- ESP32-S3 board connected through CH340K/CH34x USB-UART.

The phone does not need a system CH340K driver. The app requests Android USB Host
permission and talks to the adapter through an app-level USB serial driver.

## First Run

1. Connect the phone to the ESP32-S3 through the OTG cable.
2. Open the app.
3. Accept the USB permission dialog.
4. The app sends `PING` and `CATALOG`.
5. Tap `添加空调`, choose a brand, and confirm the tested remote candidate.
6. Double tap a saved air conditioner, or tap `详情`, to open the single-device
   control panel.

## Main UI

- `可选空调` shows the CATALOG result grouped by brand.
- `串口日志` opens TX/RX/USB/ERR logs with copy and clear actions.
- Saved air conditioners support search, sort, multi-select, rename, copy, and
  delete.
- Batch power uses selected devices when any are checked; otherwise it applies
  to all saved devices.
- Batch settings require checked devices and send commands with a 1500 ms
  interval. `停止任务` cancels the remaining batch commands.

## Troubleshooting

- If no USB permission dialog appears, confirm the phone supports OTG and the
  cable is an OTG cable.
- If direct USB-C-to-USB-C shows no USB device, but using an OTG adapter or hub
  shows `VID:PID 1A86:7523`, the app is not the failing layer. Keep the OTG
  adapter/hub in the connection, or use a powered OTG hub if the ESP32 board
  draws more current than the phone will supply.
- If `PING` does not return `OK PONG`, reconnect USB and confirm the ESP32
  firmware is running the text serial protocol at 115200 8N1.
- If commands return `ERR`, check the selected `remoteId` and temperature range.

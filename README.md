# AtomDeck-S3

AtomDeck-S3 is a clean-room, MIT-licensed firmware for the M5Stack AtomS3U. It
turns an explicitly authorized AtomS3U into a physically armed USB HID macro
controller that is managed from a browser on a trusted home Wi-Fi network.

This repository does **not** copy source code or binaries from
`kolec94/tdongle-s3-hid`, whose public repository currently has no license. That
project was used only to validate the AtomS3U hardware path before this new
implementation was started.

## First release scope

- ESP32-S3 native USB keyboard and mouse HID on M5Stack AtomS3U (8 MB flash)
- Home Wi-Fi STA mode with a temporary setup AP fallback
- Responsive on-device macro management page
- LittleFS-backed macro storage with atomic updates
- REST API for status, macro CRUD, typing, and macro execution
- Physical arming: HID-producing and mutating requests work only for 60 seconds
  after the AtomS3U button is pressed
- No embedded home Wi-Fi credentials, cloud keys, or tokens

Token authentication and AI API integration are intentionally deferred.

## Safety boundary

AtomDeck-S3 is for standard, authorized HID automation on devices you own or
administer. It does not implement payload scripting, privilege escalation,
credential collection, keylogging, data extraction, or automatic execution of
AI-generated actions.

Without token authentication, this version must be used only on a trusted home
LAN. A physical button press is required before any API can type, run, create,
change, or delete a macro. Do not store passwords or other secrets in macros.

## Build

Requirements: PlatformIO Core 6.x.

```text
pio run
```

The environment is pinned to PlatformIO's Espressif32 6.12.0 platform and an
8 MB partition layout. USB uses ESP32-S3 TinyUSB/USB-OTG mode.

## Flashing

1. Connect the AtomS3U to the development computer.
2. Hold its reset control for about two seconds until the internal green LED is
   shown, then release it to enter download mode.
3. Run `pio run -t upload` with the correct serial port selected.
4. The web UI is compiled into the firmware; no separate filesystem upload is
   needed.

## First-time Wi-Fi setup

When no saved home Wi-Fi exists, or connection fails, the device creates a
temporary open network named `AtomDeck-Setup-XXXX`. It is available for ten
minutes and only exposes Wi-Fi provisioning plus the status page.

Connect to that network, open `http://192.168.4.1`, enter the home SSID and
password, and save. The password is written to NVS, never returned by the API,
never logged, and never belongs in this repository. The device then restarts in
STA-only mode.

## Daily use

Open `http://atomdeck-xxxx.local` or the IP shown by your router. Press the
AtomS3U button once to arm it for 60 seconds. The web page then allows macro
editing and manual execution. Every execution remains explicit; saving a macro
never runs it.

Holding the programmable button for eight seconds clears saved home Wi-Fi and
restarts setup mode.

## API

See [docs/api.md](docs/api.md). Security assumptions and limitations are in
[docs/security.md](docs/security.md). The feature-parity plan is in
[docs/roadmap.md](docs/roadmap.md).

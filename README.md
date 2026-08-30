# AtomDeck-S3

AtomDeck-S3 is a clean-room, MIT-licensed firmware for the M5Stack AtomS3U. It
turns an explicitly authorized AtomS3U into a physically armed USB HID macro
controller that is managed from a browser on a trusted home Wi-Fi network.

This repository does **not** copy source code or binaries from
`kolec94/tdongle-s3-hid`, whose public repository currently has no license. That
project was used only to validate the AtomS3U hardware path before this new
implementation was started.

## Current release scope

- ESP32-S3 native USB keyboard and mouse HID on M5Stack AtomS3U (8 MB flash)
- Home Wi-Fi STA mode with a temporary setup AP fallback
- Responsive four-page control deck for mobile and desktop browsers
- Customizable shortcut cards with visual action editing and optional JSON mode
- LittleFS-backed macro storage with atomic updates
- REST API for status, macro CRUD, typing, and macro execution
- Administrator password login with an HttpOnly, SameSite browser session
- Hashed REST bearer tokens that are revealed only once when created
- Login throttling and content-free in-memory audit counters
- Volatile physical unlock: one button press authorizes operations until a
  second press, explicit lock, logout, password change, or restart
- No embedded home Wi-Fi credentials, cloud keys, or tokens

AI API integration is intentionally deferred. The local HTTP transport remains
documented as a trusted-LAN limitation; HTTPS is planned for v0.3.

## Hardware status

Version 0.1.0 was verified on a physical M5Stack AtomS3U on 2026-08-28:

- 8 MB firmware boots and enumerates as a composite USB keyboard and mouse
- first-time setup AP provisions home Wi-Fi without exposing the password
- station mode, mDNS, Chinese Web GUI, and physical arming work on an iPhone
- direct HID input and mouse controls operate through the authorized target PC

## Safety boundary

AtomDeck-S3 is for standard, authorized HID automation on devices you own or
administer. It does not implement payload scripting, privilege escalation,
credential collection, keylogging, data extraction, or automatic execution of
AI-generated actions.

Authentication does not make unencrypted HTTP safe on a hostile network, so use
this version only on a trusted home LAN. A physical button press is required
before any API can type, run, create, change, or delete a macro or manage a
token. The unlocked state remains active until explicitly ended, so lock the
device whenever the target PC is unattended. Do not store secrets in macros.

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
password, and create an administrator password of 12–72 bytes. The credentials
are written to NVS, never returned by the API, never logged, and never belong in
this repository. The administrator verifier uses PBKDF2-HMAC-SHA256 with a
random salt; the plaintext password is not stored. The device then restarts in
STA-only mode.

Upgrading from v0.1.0 starts setup mode once so an administrator password can be
created. Re-enter the existing home Wi-Fi details; saved macros are preserved.

## Daily use

Open `http://atomdeck-xxxx.local` or the IP shown by your router and log in.
Press the AtomS3U button once to unlock it. The state remains active in RAM until
you press the button again, click **Lock**, log out, change the administrator
password, or restart the device. Every execution remains explicit; saving a
card never runs it.

The dashboard supports launch, hotkey, text and multi-step macro cards. To open
an application, file, folder or website, assign a keyboard shortcut to a
Windows shortcut and configure the same chord on its AtomDeck launch card. The
firmware deliberately stores no PC path and executes no shell command. See
[docs/windows-launchers.md](docs/windows-launchers.md).

Holding the programmable button for eight seconds clears saved home Wi-Fi,
administrator authentication and all REST tokens, then restarts setup mode.
Stored macros are deliberately preserved.

## API

See [docs/api.md](docs/api.md). Security assumptions and limitations are in
[docs/security.md](docs/security.md). The feature-parity plan is in
[docs/roadmap.md](docs/roadmap.md).

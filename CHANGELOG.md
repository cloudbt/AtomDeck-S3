# Changelog

## 0.2.0 — 2026-08-28

- Added administrator password bootstrap and login throttling.
- Added one-hour in-memory browser sessions using HttpOnly, SameSite cookies.
- Added named REST bearer tokens; only SHA-256 token verifiers are persisted.
- Added create-once token display, listing and physical-presence revocation.
- Protected macro reads, writes and HID endpoints with authentication while
  retaining the 60-second physical arming requirement for sensitive actions.
- Added password rotation, logout and content-free in-memory audit counters.
- Extended the eight-second recovery gesture to clear Wi-Fi, authentication and
  tokens while preserving macros.

## 0.1.0 — 2026-08-28

- Initial clean-room M5Stack AtomS3U firmware.
- Added home Wi-Fi station mode and a temporary setup AP.
- Added composite USB keyboard and mouse HID.
- Added a Chinese Web GUI, REST API, mDNS, and LittleFS macro storage.
- Added a restricted macro schema and a 60-second physical arming window.
- Hardware-verified the complete phone-to-Wi-Fi-to-AtomS3U-to-USB-HID path.

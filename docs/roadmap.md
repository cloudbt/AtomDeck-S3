# Roadmap

The target architecture is:

```text
                         Home router
                   /         |          \
              iPhone     personal PC   AtomS3U
                 \          Web/API       |
                  +---------------------->|
                                          | USB keyboard + mouse HID
                                          v
                                      authorized PC
```

## v0.1 — implemented in this repository

- Home Wi-Fi station mode
- Temporary physical-presence setup AP
- Chinese responsive Web GUI
- USB keyboard and mouse
- REST API
- Atomic LittleFS macro storage
- Restricted macro data model
- mDNS
- 60-second physical arming window

## v0.2 — authentication

- One-time bootstrap credential shown only during physical setup
- Password login with an HttpOnly session cookie for browsers
- Hashed bearer tokens for REST clients
- Token/session rotation and revocation through physical presence
- Rate limiting and audit counters without storing typed content

## v0.3 — transport and lifecycle

- Optional device-side self-signed HTTPS
- Recommended reverse-proxy recipe for trusted home HTTPS
- WebSocket status updates
- Signed OTA releases with rollback
- Firmware and filesystem release manifests with published SHA-256 hashes

## v0.4 — optional AI gateway

- Home-server gateway using server-side environment variables for cloud keys
- AI produces a macro proposal only
- Firmware validates the restricted schema
- A person must review, save, physically arm, and run the proposal
- No cloud key in firmware, NVS, browser storage, or Git

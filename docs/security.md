# Security model

## Current controls

- Home Wi-Fi credentials are written to ESP32 NVS and never returned or logged.
- The fallback setup AP is temporary, starts only when provisioning is needed,
  and stops after ten minutes.
- HID and macro mutations require a physical button unlock. Unlock is volatile
  and ends on another button press, Web lock, logout, password change or reboot.
- Macro actions are an allow-listed data model, not a general scripting language.
- Macro files are size-limited, validated before saving and again before running.
- Browser write requests are protected against cross-origin submission.
- Administrator passwords use PBKDF2-HMAC-SHA256 with a per-device random salt;
  browser sessions exist only in RAM and use HttpOnly, SameSite cookies.
- REST tokens contain 256 bits of random data, are shown once, and are persisted
  only as SHA-256 hashes. Token creation and revocation require physical arming.
- Login failures are throttled. Audit counters record only event counts and never
  typed text, macro content, passwords or tokens.
- No secrets are compiled into firmware or committed to Git.

## Known limitations

- The local web server is HTTP, so trusted-LAN traffic is not encrypted.
- Session cookies cannot use the `Secure` attribute until HTTPS is implemented.
  An attacker able to observe trusted-LAN HTTP traffic may capture a session or
  bearer token; authentication is not a substitute for encrypted transport.
- ESP32 NVS is not encrypted unless the board is provisioned with ESP-IDF flash
  encryption. Physical extraction remains possible.
- The setup AP is open for usability on a screenless device. Provision promptly
  and do not expose setup mode in an untrusted location.
- The persistent-in-RAM unlock trades convenience for a longer authorization
  window. Lock the device whenever the target PC is unattended; bearer tokens
  can produce HID while the device remains unlocked.

## Future hardening

The next security phase should add HTTPS, signed OTA releases and rollback.
AI integration, if added later, should use a home gateway that keeps cloud API
keys in server-side environment variables. Cloud keys must not be stored in
firmware, NVS, browser storage or Git.

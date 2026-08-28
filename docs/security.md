# Security model

## Current controls

- Home Wi-Fi credentials are written to ESP32 NVS and never returned or logged.
- The fallback setup AP is temporary, starts only when provisioning is needed,
  and stops after ten minutes.
- HID and macro mutations require a recent physical button press.
- Macro actions are an allow-listed data model, not a general scripting language.
- Macro files are size-limited, validated before saving and again before running.
- Browser write requests are protected against cross-origin submission.
- No secrets are compiled into firmware or committed to Git.

## Known limitations

- The local web server is HTTP, so trusted-LAN traffic is not encrypted.
- Token authentication is deferred; a local attacker can read macro names and
  content, although physical presence is still required to change or run them.
- ESP32 NVS is not encrypted unless the board is provisioned with ESP-IDF flash
  encryption. Physical extraction remains possible.
- The setup AP is open for usability on a screenless device. Provision promptly
  and do not expose setup mode in an untrusted location.

## Future hardening

The next security phase should add a one-time bootstrap token, store only its
hash, protect every non-status endpoint, provide token rotation through physical
presence, and add per-client rate limits. AI integration, if added later, should
use a home gateway that keeps cloud API keys in server-side environment
variables. Official OpenAI guidance says API keys must not be exposed in
client-side apps or browsers.

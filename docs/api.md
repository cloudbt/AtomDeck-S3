# REST API v1

The API is served over HTTP from the AtomDeck-S3 device. Use it only on a
trusted home LAN. JSON request bodies must use `Content-Type: application/json`.
The firmware does not emit permissive CORS headers and rejects browser write
requests whose `Origin` does not match the device host.

## Authentication

The status endpoint and active setup flow are public. Every macro, HID and token
endpoint requires authentication. Browsers log in with the administrator
password and receive a one-hour `HttpOnly; SameSite=Strict` session cookie. REST
clients send a named bearer token:

```text
Authorization: Bearer adk_REPLACE_WITH_THE_ONCE_DISPLAYED_TOKEN
```

The plaintext administrator password is never stored. Its verifier is derived
with PBKDF2-HMAC-SHA256, 60,000 iterations and a random 16-byte salt. Each
bearer token contains 256 bits of random data; only its SHA-256 hash is stored.
The Web GUI reveals a new token once and cannot recover it later.

After five failed logins from one client, login is blocked for 60 seconds.
HTTP `401` means authentication is missing or invalid, `423` means the physical
arming window is closed, and `429` means login is temporarily throttled.

## Endpoints

| Method | Path | Authentication | Physical arm | Description |
|---|---|---|---:|---|
| `GET` | `/api/v1/status` | No | No | Device, Wi-Fi, storage and arm status |
| `POST` | `/api/v1/auth/login` | Password | No | Start a browser session |
| `POST` | `/api/v1/auth/logout` | No | No | End the current browser session |
| `GET` | `/api/v1/auth/me` | Session or token | No | Authentication kind and audit counters |
| `POST` | `/api/v1/auth/password` | Session | Yes | Change administrator password |
| `GET` | `/api/v1/tokens` | Session | No | List token IDs and names, never secrets |
| `POST` | `/api/v1/tokens` | Session | Yes | Create and reveal one token once |
| `DELETE` | `/api/v1/tokens/{id}` | Session | Yes | Revoke a token |
| `GET` | `/api/v1/macros` | Session or token | No | List stored macros |
| `POST` | `/api/v1/macros` | Session or token | Yes | Create a macro |
| `PUT` | `/api/v1/macros/{id}` | Session or token | Yes | Replace a macro |
| `DELETE` | `/api/v1/macros/{id}` | Session or token | Yes | Delete a macro |
| `POST` | `/api/v1/macros/{id}/run` | Session or token | Yes | Validate and run a macro |
| `POST` | `/api/v1/type` | Session or token | Yes | Type a short literal string |
| `POST` | `/api/v1/mouse` | Session or token | Yes | Move, click, or scroll the mouse |
| `POST` | `/api/v1/wifi` | Setup password | No | Save home Wi-Fi and restart |

During first setup, `/api/v1/wifi` accepts a new `admin_password`. During later
recovery or reprovisioning it requires the existing administrator password.
Never place real Wi-Fi passwords, administrator passwords or bearer tokens in
scripts, shell history, screenshots, issue reports or commits.

## Macro document

```json
{
  "name": "Insert signature",
  "actions": [
    { "type": "text", "value": "Kind regards," },
    { "type": "key", "value": "ENTER" },
    { "type": "delay", "ms": 250 },
    { "type": "chord", "value": "CTRL+S" }
  ]
}
```

Allowed action types are `text`, `key`, `chord`, `delay`, `mouse_move`,
`mouse_click`, and `scroll`. Firmware limits:

- 20 stored macros
- 32 actions per macro
- 48 UTF-8 bytes per name
- 256 UTF-8 bytes per text action
- delays from 0 to 3000 ms, with no more than 15 seconds total per macro
- chords containing at most three modifiers plus one key

There is deliberately no shell, command, URL, download, or script action.

Direct mouse requests use one of these JSON bodies:

```json
{"op":"move","x":20,"y":-10}
{"op":"click","button":"LEFT"}
{"op":"scroll","amount":-3}
```

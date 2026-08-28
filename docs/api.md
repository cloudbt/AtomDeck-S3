# REST API v1

The API is served over HTTP from the AtomDeck-S3 device. JSON request bodies
must use `Content-Type: application/json`. The firmware does not emit permissive
CORS headers and rejects browser write requests whose `Origin` does not match
the device host.

Token authentication is not implemented in this release. All mutating or
HID-producing endpoints require the 60-second physical arming window.

## Endpoints

| Method | Path | Physical arm | Description |
|---|---|---:|---|
| `GET` | `/api/v1/status` | No | Device, Wi-Fi, storage and arm status |
| `GET` | `/api/v1/macros` | No | List stored macros |
| `POST` | `/api/v1/macros` | Yes | Create a macro |
| `PUT` | `/api/v1/macros/{id}` | Yes | Replace a macro |
| `DELETE` | `/api/v1/macros/{id}` | Yes | Delete a macro |
| `POST` | `/api/v1/macros/{id}/run` | Yes | Validate and run a macro |
| `POST` | `/api/v1/type` | Yes | Type a short literal string |
| `POST` | `/api/v1/mouse` | Yes | Move, click, or scroll the mouse |
| `POST` | `/api/v1/wifi` | Setup mode only | Save home Wi-Fi and restart |

Locked operations return HTTP `423`. Invalid macro definitions return `422`.

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

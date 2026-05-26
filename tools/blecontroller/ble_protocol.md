
# BLE Hand Control Protocol

## Command: `sethand`

Set the relative position of all five fingers.

```text
sethand <thumb> <index> <middle> <ring> <pinky>
```

---

### Finger Order

| Index | Name   | Italian |
|-------|--------|---------|
| 0     | thumb  | pollice |
| 1     | index  | indice  |
| 2     | middle | medio   |
| 3     | ring   | anulare |
| 4     | pinky  | mignolo |

---

### Value Range

Each value is an integer from 0 to 100:

- `0` = fully open
- `100` = fully closed

Values outside the range must be clamped by firmware.

---

### Example

```text
sethand 0 25 80 100 40
```

---

### Transport

UTF-8 text written to the BLE writable characteristic.

Current default characteristic UUID:

`39dea685-a63e-44b2-8819-9a202581f8fe`

The command should also remain usable from serial/console when possible.
# MotorApp

Interactive console application for testing and operating motor actuators on the
enableit platform (ESP32-S3, M5Stack AtomS3).

---

## Philosophy

### Unified Motor Abstraction

All actuator types — linear actuators, smart bus servos, traditional PWM servos,
DC motors — are exposed through a single abstract interface: `enableit::Motor`
(defined in `lib/enableit/include/Motor.h`).

The design has three goals:

1. **Hardware independence** — application code calls `forward()`, `setPosition()`,
   `getTelemetry()`, etc. without knowing which driver is underneath.
2. **Value-mode transparency** — each driver supports two address spaces that can
   be switched at runtime with `setValueMode()`:
   - `EFFECTIVE` — native hardware units (PWM ticks, ADC counts, encoder ticks).
   - `NORMALIZED` — logical range common to all actuators: position `[0, 1000]`,
     speed `[-1000, 1000]`, acceleration `[0, 1000]`. Suitable for UI, BLE, and
     cross-device sequences.
3. **Capability introspection** — `getCapabilities()` returns the exact features
   each driver supports (`POSITION_READ`, `CURRENT_SENSE`, `ACCELERATION_SET`,
   etc.) so generic code can adapt to what is available.

### Motor Registry

At `enter()` time all instantiated motors are registered in a flat pointer array
(`_motors[]`, max 4 slots). Every generic command dispatches to
`_motors[selectedMotor]`, so `selectmotor <n>` switches the active target without
any driver-specific branching.

Bus-specific or configuration-only commands (servo EEPROM, baudrate scan, sync
moves) still call `ST3215Motor` directly because they use the SMS_STS protocol
layer that is not part of the `Motor` interface.

---

## Motor Drivers

| Class | Type | Interface | Sensing |
|---|---|---|---|
| `PQ12Actuator` | Actuonix PQ12 linear actuator via TI DRV8411 H-bridge | PWM | Wiper (ADC position) + IPROPI (current) |
| `STMotor` | Waveshare ST3215 (SMS_STS bus servo) | UART half-duplex @ 1 Mbps | Position, speed, current, voltage, temperature |
| `PwmServoMotor` | Traditional RC/PWM servo | 50 Hz PWM (ServoESP32) | None |

### PQ12Actuator

- Uses DRV8411 in slow-decay or fast-decay mode (compile-time `MOTOR_FASTDECAY_MODE`).
- Position is read from an analog wiper potentiometer; soft limits (`PQ12_POS_SOFT_MIN`,
  `PQ12_POS_SOFT_MAX`) stop the motor before the mechanical endstop.
- Current is measured via the DRV8411 IPROPI pin and converted to milliamps using
  the formula:
  `I_mA = (ADC_raw × VREF_mV × DRV8411_ISENSE_RATIO) / (ADC_MAX × RSENSE_Ω)`
- Over-current protection (`MOTOR_CURRENT_PROTECTION`, threshold `PQ12_CURRENT_MAX_MA`)
  stops the motor automatically when the limit is exceeded.

### STMotor (ST3215 smart servo)

- Communicates over a shared half-duplex UART bus (`STMotor::static_init()`).
- All motion commands (`forward`, `reverse`, `moveTo`, `setPosition`, `move`)
  accept both effective (encoder ticks `[0, 4095]`) and normalized (`[0, 1000]`)
  units depending on the active value mode.
- `getTelemetry()` performs a `FeedBack()` burst read for position, velocity,
  current, temperature and voltage in a single transaction.
- EEPROM operations (ID change, baudrate change) are protected by unlock/lock
  sequences and accessed through dedicated commands (`setid`, `setbaud`).

### PwmServoMotor

- Wraps the `ServoESP32` library (`ServoTemplate<int>`) as an `enableit::Motor`.
- Position range is configurable at `init()` time (default 0–180°, 544–2400 µs).
- `disable()` detaches the pin, leaving the servo mechanically free; `enable()`
  reattaches and restores the last commanded position.
- `setSpeed()` returns `false` (movement rate is governed by the servo's internal
  mechanics, not the PWM signal).

---

## Board Configuration

Pin assignments and motor presence are controlled by `#define` blocks in `MotorApp.h`:

```
USE_LEGACY_MOTOR_BOARD   — legacy board with DRV8411 H-bridge on G38/G39
USE_TWO_MOTORS           — enables second H-bridge on G6/G7
(default / new board)    — STMotor bus on G38(TX)/G39(RX), PwmServo on G7
```

Current default (`new board`):

| Signal | GPIO | Function |
|---|---|---|
| BUS_SERIAL_TX | G38 | STMotor UART TX |
| BUS_SERIAL_RX | G39 | STMotor UART RX |
| PWM_SERVO_PIN | G7  | PwmServoMotor signal |

Motor registry slot order:
- `[0]` — STMotor (ST3215)
- `[1]` — PwmServoMotor (G7)

---

## Console Commands

### Generic (dispatched to selected motor)

| Command | Description |
|---|---|
| `forward` | Move forward at current speed |
| `reverse` | Move reverse at current speed |
| `stop` | Coast to stop |
| `setspeed <n>` | Set speed (effective or normalized) |
| `getposition` | Read current position |
| `current` | Read instantaneous current [mA] |
| `sleep` | Disable motor (remove power/torque) |
| `selectmotor <n>` | Switch active motor (0 = STMotor, 1 = PwmServo) |
| `feedback` | Full telemetry dump from the active motor |

### ST3215 / SMS_STS specific

| Command | Description |
|---|---|
| `getservoinfo` | Read and print all servo registers |
| `pingservos` | Ping servo at current `servoId` |
| `scan` | Scan all baudrates for a responding servo |
| `setid <id>` | Change servo ID (unlocks EEPROM, writes, locks) |
| `setbaud <idx>` | Change servo baudrate (0 = 1 Mbps … 7 = 38400) |
| `useid <id>` | Set default servo ID for subsequent commands |
| `userate <idx>` | Set UART baudrate and reconnect |
| `testmove` | Run 3× forward/reverse cycles |
| `testsync` | Synchronously move two servos (IDs 1 and 2) |

### System

| Command | Description |
|---|---|
| `setpin <pin> <high\|low>` | Force a GPIO state (debug) |
| `ota` | Switch to OTA update app |
| `boot` | Switch to bootloader |
| `reboot` | Software reset |
| `help` | List all commands |

---

## Adding a New Motor Type

1. Create `lib/enableit/include/MyMotor.h` inheriting `enableit::Motor`.
2. Implement all pure-virtual methods; override `poll()` if the driver needs
   periodic work (protection checks, encoder refresh, etc.).
3. Implement value-mode conversion using inline `_remap()` helpers (see
   `PQ12Actuator.h` or `STMotor.h` for the pattern).
4. Add `#include <MyMotor.h>` and an instance to `MotorApp.h`.
5. Register the instance in `MotorApp::enter()`:
   ```cpp
   _motors[_motorCount++] = &myMotorInstance;
   ```
   All generic commands will work immediately.

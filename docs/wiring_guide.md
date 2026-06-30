# 🔌 Wiring Guide

> **Smart Home Controlled by Smart Gloves**
> Step-by-step wiring instructions for both subsystems.

---

## ⚠️ Safety First

- **Never** work on the 220V AC side while the power supply is plugged in.
- Build and test the low-voltage side first — add the mains wiring last.
- Use a **non-conductive enclosure** for the Smart Home unit.
- All 220V terminals must be **fully insulated** with heat-shrink tubing or electrical tape.
- The 2-channel relay module must be **optically isolated** (all common modules include this).
- If unsure about 220V wiring, have a qualified electrician complete that section.

---

## 📐 Tools & Materials Needed

| Item | Purpose |
|---|---|
| Multimeter | Voltage checks and continuity testing |
| Soldering iron + solder | Attaching wires to flex sensor pads |
| Wire strippers | Preparing jumper wires |
| Heat-shrink tubing + heat gun | Insulating solder joints |
| Jumper wires | Connections between modules |
| Velcro straps / hot glue | Mounting components on the glove |
| Screwdriver | Terminal block connections |

---

## Part 1 — Smart Glove Unit

### Circuit Overview

```
3.3V ---[ FLEX SENSOR ]---+---[10kΩ]--- GND
                          |
                      ESP32 ADC PIN

Repeat × 3 (Thumb, Index, Pinky)
```

**Classification logic:**
- Finger **BENT**     → Flex resistance ↑ → voltage at ADC pin **HIGH** → bit = **0**
- Finger **EXTENDED** → Flex resistance ↓ → voltage at ADC pin **LOW**  → bit = **1**

---

### Step 1 — Power Supply

```
[18650 +] ──── [Switch IN] ──── [Switch OUT] ──── [Boost Converter IN+]
[18650 −] ─────────────────────────────────────── [Boost Converter IN−]

[Boost Converter OUT+] ──── [ESP32 VIN]
[Boost Converter OUT−] ──── [ESP32 GND]
```

1. Solder wires to the 18650 battery holder (red = +, black = −).
2. Connect the positive wire through the on/off switch.
3. Connect switch output → boost converter **IN+**, battery − → boost converter **IN−**.
4. Power on and **measure** the boost converter output — adjust its trimmer until you read **5.0V**.
5. Connect boost converter **OUT+** → ESP32 **VIN**, **OUT−** → ESP32 **GND**.

> ✅ **Check:** Power on — the ESP32 should boot and the onboard LED should blink.

---

### Step 2 — Flex Sensor Voltage Dividers

Wire all 3 sensors using the same circuit. Assign each to its GPIO pin:

| Sensor | Finger | ESP32 GPIO | ADC Channel |
|---|---|---|---|
| Flex Sensor 1 | **Thumb** | **GPIO 39** | ADC1_CH3 (input-only pin) |
| Flex Sensor 2 | **Index** | **GPIO 34** | ADC1_CH6 (input-only pin) |
| Flex Sensor 3 | **Pinky** | **GPIO 32** | ADC1_CH4 |

```
For each sensor:

ESP32 3.3V ────────────────────────────┐
                                       │
                               [ FLEX SENSOR ]
                                       │
                          ┌────────────┤ ← connect this junction to ADC pin
                          │            │
                       [10kΩ]    (GPIO 39 / 34 / 32)
                          │
                         GND
```

Steps:
1. Solder a wire to each of the 2 pads at the base of the flex sensor.
2. Connect one pad → ESP32 **3.3V**.
3. Connect the other pad → one leg of a **10 kΩ** resistor.
4. Connect the other leg of the resistor → **GND**.
5. Connect the **junction** between sensor pad and resistor → the GPIO ADC pin.

> ⚠️ GPIO 39 and GPIO 34 are **input-only** — never connect them to a voltage source or output.

> ✅ **Check:** Open Serial Monitor at **115200 baud**. Fully extend each finger → low ADC value.
> Fully bend each finger → high ADC value. Typical range: ~1800 (extended) to ~3000 (bent).

---

### Step 3 — Calibration

With sensors wired and Serial Monitor open:

1. Hold all fingers **fully extended** — note the ADC value for each (e.g. ~1800–2000).
2. Fully **bend** each finger — note the ADC value (e.g. ~2800–3200).
3. Set the threshold to the **midpoint**, e.g. `2200`.

In `smart_glove.ino`:
```cpp
const int THRESHOLD[3] = {2200, 2200, 2200};
//                         Thumb  Index  Pinky
```

Adjust each value independently if your sensors differ.

---

### Step 4 — Mount on the Glove

1. Lay the glove flat. Position each flex sensor flat along the **top** of its finger (thumb, index, pinky).
2. Secure with thin fabric tape or sewn channels — the sensor must bend naturally with the finger.
3. Route wires along the back of the hand toward the wrist.
4. Mount the ESP32, boost converter, and battery in a small enclosure on the wrist/forearm.
5. Secure all wiring with cable ties to prevent strain on solder joints.

> 💡 **Tip:** Test all 8 gesture codes via Serial Monitor before closing the enclosure.

---

## Part 2 — Smart Home Unit

### Power Supply

```
[AC Mains Live]    ──── [PSU L]
[AC Mains Neutral] ──── [PSU N]
[AC Mains Earth]   ──── [PSU ⏚]

[PSU 5V  OUT+] ──── 5V Rail  → ESP32 VIN, Relay VCC, Servo VCC
[PSU 5V  OUT−] ──── GND (common ground for all low-voltage)
[PSU 12V OUT+] ──── 12V Rail → Motor Driver VCC
[PSU 12V OUT−] ──── GND (same common ground)
```

> ✅ **Check:** Measure 5V and 12V rails with a multimeter before connecting anything else.

---

### Step 1 — ESP32 Power

```
[5V Rail] ──── [ESP32 VIN]
[GND]     ──── [ESP32 GND]
```

> ✅ **Check:** The `SmartHome_AP` Wi-Fi network should appear within a few seconds of boot.

---

### Step 2 — 2-Channel Relay Module (Fan + Lamp)

The 2-channel relay module controls both the 220V fan and 220V lamp.

**Control side (low voltage):**
```
[5V Rail]      ──── [Relay VCC]
[GND]          ──── [Relay GND]
[ESP32 GPIO 33] ─── [Relay IN1]   ← Fan
[ESP32 GPIO 25] ─── [Relay IN2]   ← Lamp
```

**Switching side (220V AC — wire with mains UNPLUGGED):**
```
Channel 1 — Fan:
  AC Live ──── Relay CH1 COM
               Relay CH1 NO ──── Fan Live terminal
  AC Neutral ──────────────────── Fan Neutral terminal  (direct)

Channel 2 — Lamp:
  AC Live ──── Relay CH2 COM
               Relay CH2 NO ──── Lamp Live terminal
  AC Neutral ──────────────────── Lamp Neutral terminal (direct)
```

**Relay logic (active-LOW — most common):**

| GPIO signal | Relay state | Device |
|:---:|:---:|:---:|
| LOW | Energised | **ON** |
| HIGH | Released | **OFF** |

> This is already handled in firmware with `#define RELAY_ON LOW` and `#define RELAY_OFF HIGH`.
> If your relay module is active-HIGH, change both defines in `smart_home.ino`.

> ✅ **Check:** Send code `0` (gesture 000) → Fan ON. Send code `1` (gesture 001) → Fan OFF.
> Send code `2` (gesture 010) → Lamp ON. Send code `3` (gesture 011) → Lamp OFF.

---

### Step 3 — SG90 Servo Motor (Door)

SG90 has 3 wires: **Brown = GND**, **Red = 5V**, **Orange/Yellow = Signal (PWM)**

```
[SG90 Brown]         ──── GND
[SG90 Red]           ──── 5V Rail
[SG90 Orange/Yellow] ──── ESP32 GPIO 26
```

Mount the servo on the door mechanism so that:
- **90°** → Door **OPEN**
- **0°**  → Door **CLOSED**

Adjust `SERVO_DOOR_OPEN` and `SERVO_DOOR_CLOSE` in `smart_home.ino` to match your mechanism.

> ✅ **Check:** Send code `6` (gesture 110) → Door opens. Send code `7` (gesture 111) → Door closes.

---

### Step 4 — DC Motor (Curtain) via L298N

**Power connections:**
```
[12V Rail] ──── [L298N 12V IN]
[GND]      ──── [L298N GND]
```

> Do **not** use the L298N's onboard 5V output to power the ESP32 — use the PSU 5V rail.

**Control connections:**
```
[ESP32 GPIO 19] ──── [L298N IN1]   ← Curtain OPEN direction
[ESP32 GPIO 18] ──── [L298N IN2]   ← Curtain CLOSE direction
```

> No separate Enable pin needed if the L298N jumper is set to always-enabled.

**Motor connections:**
```
[L298N OUT1] ──── [DC Motor terminal A]
[L298N OUT2] ──── [DC Motor terminal B]
```

**Direction logic:**

| IN1 | IN2 | Motor |
|:---:|:---:|---|
| HIGH | LOW | Rotates → **Opens** curtain |
| LOW | HIGH | Rotates ← **Closes** curtain |
| LOW | LOW | **Stops** |

Adjust `CURTAIN_TRAVEL_MS` in `smart_home.ino` to the time your curtain needs to fully traverse its rail.

> ✅ **Check:** Send code `4` (gesture 100) → Curtain opens. Send code `5` (gesture 101) → Curtain closes.

---

## Part 3 — Complete Pin Reference

### Smart Glove ESP32

| GPIO | Connected to | Direction |
|---|---|---|
| 39 | Flex Sensor — Thumb (voltage divider mid-point) | Input only |
| 34 | Flex Sensor — Index (voltage divider mid-point) | Input only |
| 32 | Flex Sensor — Pinky (voltage divider mid-point) | Input |
| 2  | Onboard status LED | Output |
| VIN | Boost converter OUT+ (5V) | Power |
| GND | Common ground | Ground |

### Smart Home ESP32

| GPIO | Connected to | Direction |
|---|---|---|
| 33 | Relay IN1 — Fan (220V) | Output |
| 25 | Relay IN2 — Lamp (220V) | Output |
| 19 | Motor IN1 — Curtain OPEN | Output |
| 18 | Motor IN2 — Curtain CLOSE | Output |
| 26 | SG90 Servo Signal — Door | PWM Output |
| 2  | Onboard heartbeat LED | Output |
| VIN | PSU 5V rail | Power |
| GND | Common ground | Ground |

---

## Part 4 — Final Integration Test

### Pre-Power Checklist
- [ ] All GND terminals share a common ground
- [ ] ESP32 VIN = 5.0V (measured)
- [ ] No bare 220V wires exposed
- [ ] 2-Channel relay VCC, GND, IN1, IN2 connected
- [ ] Servo and motor mechanically free to move

### Power-On Sequence
1. Power on the **Smart Home Unit** first — wait 5 seconds for AP to broadcast.
2. Power on the **Smart Glove Unit** via the on/off switch.
3. Within ~5 seconds the glove connects to `SmartHome_AP` (confirm via Serial Monitor).
4. Perform each gesture and observe the actuator response.

### Full Gesture Test

| Code | Gesture (Thumb–Index–Pinky) | Expected Result |
|:---:|---|---|
| `000` | All bent | 🌀 Fan ON |
| `001` | Thumb bent, Index bent, Pinky extended | 🌀 Fan OFF |
| `010` | Thumb bent, Index extended, Pinky bent | 💡 Lamp ON |
| `011` | Thumb bent, Index extended, Pinky extended | 💡 Lamp OFF |
| `100` | Thumb extended, Index bent, Pinky bent | 🪟 Curtain OPEN |
| `101` | Thumb extended, Index bent, Pinky extended | 🪟 Curtain CLOSE |
| `110` | Thumb extended, Index extended, Pinky bent | 🚪 Door OPEN |
| `111` | All extended | 🚪 Door CLOSE |

> ✅ All 8 commands working correctly — build complete! 🎉

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| Glove won't connect to AP | Wrong SSID/password | Match `WIFI_SSID` and `WIFI_PASSWORD` exactly in both firmware files |
| ADC values don't change | Sensor wired to wrong junction | Connect the **mid-point** (between sensor and 10kΩ) to the GPIO pin, not the 3.3V or GND end |
| ADC reads ~0 always | Sensor and resistor swapped | The flex sensor connects to 3.3V; the 10kΩ connects to GND |
| ADC reads ~4095 always | Missing 10kΩ resistor | Check the pull-down resistor is connected to GND |
| Relay doesn't click | Wrong GPIO or relay VCC issue | Confirm GPIO 33 / 25 → IN1 / IN2, relay VCC = 5V |
| Both relay channels inverted | Module is active-HIGH | Change `#define RELAY_ON HIGH` and `#define RELAY_OFF LOW` in `smart_home.ino` |
| Fan ON but lamp also turns on | IN1/IN2 swapped | Swap GPIO 33 and 25 connections on the relay module |
| Servo doesn't move | Wrong GPIO | Confirm the signal wire (orange) is on GPIO 26, not VCC or GND |
| Curtain runs but won't stop | `CURTAIN_TRAVEL_MS` too high | Reduce the value to match your curtain's actual travel time |
| Serial Monitor shows garbled text | Wrong baud rate | Set to **115200** baud |
| ESP32 keeps rebooting | Insufficient current | Ensure boost converter is rated ≥1A output |

---

*Last updated: June 2026*

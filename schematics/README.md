# Schematics

This folder contains the circuit schematics for both subsystems.

## Files

| File | Description |
|---|---|
| `smart_glove_schematic.png` | Smart Glove Unit — flex sensor voltage dividers, ESP32, boost converter, battery |
| `smart_home_schematic.png` | Smart Home Unit — relay, servo, motor driver, fan, power supply |

> 📌 Add your schematic images (exported from KiCad, EasyEDA, Fritzing, or hand-drawn scans) to this folder and update the filenames above.

## Smart Glove — Circuit Description

```
3.3V ──┬──[Flex 1]──┬── GPIO 32
       │          [10kΩ]
       │            │
       │           GND
       │
       ├──[Flex 2]──┬── GPIO 33
       │          [10kΩ]
       │            │
       │           GND
       │
       └──[Flex 3]──┬── GPIO 34
                  [10kΩ]
                    │
                   GND

[18650+] → [Switch] → [Boost Conv IN+]     [Boost Conv OUT+] → ESP32 VIN
[18650−] ──────────── [Boost Conv IN−]     [Boost Conv OUT−] → ESP32 GND
```

## Smart Home — Circuit Description

```
AC Mains → [12V/5V PSU]
                │
          ┌─────┴──────┐
         5V            12V
          │              │
    ┌─────┼────┐      [Motor Driver VCC]
  ESP32  Relay  Servo
  VIN    VCC    VCC

GPIO 26 → Relay IN → [COM]─[NO] → 220V Lamp (series with Live wire)
GPIO 18 → SG90 Signal
GPIO 19 → Motor IN1
GPIO 21 → Motor IN2
GPIO 22 → Motor ENA (PWM)
GPIO 23 → [1kΩ] → NPN Base → Fan (collector), Emitter → GND
GPIO 25 → [330Ω] → LED → GND
```

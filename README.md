# LunaSense
**Smart home automation that actually understands if someone is in the room**

We built LunaSense for HACKHAZARDS '26. It's a system that controls lights and AC based on whether a room is actually occupied — not just whether someone moved recently.

---

## The Problem

Every smart light system we've seen has the same flaw. They use a basic motion sensor that only detects movement. The moment you sit still — reading, sleeping, working at your desk — the sensor thinks you left and turns everything off.

We got the idea from our college classroom. The AC is set to 16°C and whenever we want to change the temperature we have to find someone from outside who has the master control. We figured — why not let the AC adjust itself based on who's actually in the room? And while we were at it, the lights in our college labs keep staying on long after everyone leaves because the motion sensor stops detecting people who are sitting quietly.

So we built something that solves both.

---

## How It Works

LunaSense uses two sensors together instead of one:

- A **motion sensor** (PIR) that reacts instantly when someone enters a room
- A **presence sensor** (mmWave radar) that detects a human even when completely still — it can pick up the rise and fall of your chest while you breathe

The idea is simple. The motion sensor is fast but dumb — it misses still people and triggers on curtains and pets. The radar is slower but smart — it knows the difference between a human and a moving object. Together they cover every real scenario.

The **ESP32-CAM microcontroller** sits in the middle, reading both sensors and making decisions. It runs a small web server over WiFi so our React Native app can send commands and get live data back.

For lights we use a **WS2812B LED strip** which gives full RGB color control from the app. For AC we use an **IR LED** that blasts the same infrared signals your AC remote sends — no wiring into the unit, no voiding warranty.

We also added a **DHT22 temperature sensor** so the system knows the actual room temperature, not just what the AC dial is set to. This lets us do proper closed-loop control — if the room is still warm, keep cooling. If it overshot, ease up.

---

## Features

**Lights**
- Turns on when someone enters, stays on even when they're sitting still
- Full color control from the app — 16 million colors via a color wheel
- Brightness adjusts automatically based on how far you are from the sensor
- Turns off after a set period of no movement — configurable from the app
- Anomaly alerts — unexpected presence at unusual hours sends a push notification

**AC**
- Turn on/off, set temperature, change mode and fan speed from the app
- Reads actual room temperature every 5 seconds via DHT22
- Automatically adjusts the AC to reach your target temperature
- Detects when activity in the room increases and drops temperature by 1°C to compensate for extra body heat
- Turns off automatically after the room has been empty for a set duration
- Does not cut power directly — sends proper IR signals so the AC shuts down gracefully

**App**
- Built in React Native with Expo, works on Android and iOS
- Live room temperature, humidity and presence shown at the top
- Color wheel picker for the LED strip
- Separate configurable timers for lights and AC
- Works entirely on local WiFi — no internet required, no cloud, no data leaving your home

---

## Tech Stack

| Layer | Technology |
|---|---|
| Mobile App | React Native (Expo) |
| Firmware | Arduino C++ on ESP32-CAM |
| Communication | HTTP over local WiFi |
| LED Control | FastLED library |
| AC Control | IR NEC protocol |
| Temp/Humidity | DHT22 sensor |
| Presence | mmWave radar + PIR fusion |

---

## How the AC Smart Cooling Works

This is the part we're most proud of from a software perspective.

Most smart AC systems just turn the unit on or off. We built an actual control loop:

1. User sets a **target room temperature** in the app (say 24°C)
2. DHT22 reads the **actual room temperature** every 5 seconds
3. Every 60 seconds the ESP32 compares them:
   - Room more than 1.5°C above target → send IR temperature down signal
   - Room more than 0.5°C below target → send IR temperature up signal
   - Room within range → hold, do nothing
4. Every 30 seconds the radar energy level is checked — if activity spikes significantly the target automatically drops 1°C to account for extra body heat
5. When the radar confirms the room is empty, a countdown starts. If nobody returns within the set time, the AC turns off via IR

The system sends IR pulses one step at a time with a gap between each — the same way you'd press the temperature button on your remote repeatedly. This is important because IR has no feedback — you can't ask the AC what temperature it's at, you can only command it.

---

## Repository Structure

```
LunaSense/
├── firmware/
│   ├── lunasense_light_serial/     ← light module testing via serial
│   ├── lunasense_ac_serial/        ← AC module testing via serial
│   └── lunasense_combined/         ← full production firmware
├── src/
│   ├── app/
│   │   ├── _layout.tsx
│   │   └── index.tsx
│   ├── components/
│   │   ├── ClimateCard.tsx
│   │   ├── RoomCard.tsx
│   │   ├── SavingsCard.tsx
│   │   ├── GeneralTab.tsx
│   │   ├── StateHistory.tsx
│   │   ├── ColorPicker.tsx
│   │   ├── StatsChart.tsx
│   │   ├── LunaContext.tsx
│   │   └── PresenceIndicator.js
│   └── api.ts
├── docs/
│   ├── circuit_diagram.png
│   ├── system_flowchart.png
│   └── demo.html
└── README.md
```

---

## Setting Up

### What You Need
- Arduino IDE (for ESP32 firmware)
- Node.js installed on your computer
- Expo Go app on your phone (download from Play Store or App Store)
- ESP32-CAM with MB board
- All components wired as per the wiring guide below

---

### Part 1 — Flash the ESP32

**Step 1 — Install Arduino IDE**

Download from `arduino.cc/en/software` and install it.

**Step 2 — Add ESP32 board support**

Open Arduino IDE → `File → Preferences` → paste this in "Additional Board Manager URLs":
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
Then go to `Tools → Board → Board Manager` → search **esp32** → install **esp32 by Espressif Systems**

**Step 3 — Install libraries**

Go to `Sketch → Include Library → Manage Libraries` and install each one:
- `FastLED` by Daniel Garcia
- `IRremoteESP8266` by crankyoldgit
- `ArduinoJson` by Benoit Blanchon — install version 6
- `DHT sensor library` by Adafruit
- `Adafruit Unified Sensor` by Adafruit

**Step 4 — Open the firmware**

- Download this repository using the green **Code** button → **Download ZIP** → extract it
- Open the `firmware` folder
- For light testing: open `lunasense_light_serial` folder → open `lunasense_light_serial.ino`
- For AC testing: open `lunasense_ac_serial` folder → open `lunasense_ac_serial.ino`
- For full system: open `lunasense_combined` folder → open `lunasense_esp32_final.ino`

**Step 5 — Edit your credentials**

At the top of the `.ino` file change these two lines:
```cpp
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```
Also change this to match your LED strip length:
```cpp
#define NUM_LEDS 30
```

**Step 6 — Select board settings**

In Arduino IDE:
- `Tools → Board → ESP32 Arduino → AI Thinker ESP32-CAM`
- `Tools → Partition Scheme → Huge APP (3MB No OTA)`
- `Tools → Upload Speed → 115200`
- `Tools → Port → select your COM port`

**Step 7 — Flash**

Click the Upload button. If it fails, hold the RESET button on the MB board as soon as you see "Connecting..." in the console.

**Step 8 — Find your IP address**

After flashing open `Tools → Serial Monitor` and set baud rate to `115200`. You will see:
```
✅ IP: 192.168.1.XX
🚀 LunaSense server started!
```
Write this IP down — you need it for the app.

---

### Part 2 — Run the Mobile App

**Step 1 — Download the repo**

Click the green **Code** button on this page → **Download ZIP** → extract it.

**Step 2 — Open terminal in the project folder**

- Open the extracted folder
- Click the address bar in File Explorer → type `cmd` → press Enter

**Step 3 — Install dependencies**

Run these commands one by one:
```bash
npm install
npx expo install react-native-svg
npx expo install @react-native-async-storage/async-storage
npx expo install react-native-safe-area-context
```

**Step 4 — Start the app**

```bash
npx expo start
```
A QR code will appear in the terminal.

**Step 5 — Open on your phone**

- Install **Expo Go** from Play Store or App Store
- Make sure your phone is on the **same WiFi network** as your ESP32
- Android: open Expo Go → scan the QR code
- iOS: scan with your camera app

**Step 6 — Connect to ESP32**

- Open the app → go to **General tab**
- Type your ESP32 IP address from Step 8 above
- Tap **Save Settings**
- Go to AC tab → add a room → tap ON

---

## Wiring (Quick Reference)

```
LED Strip data  → GPIO 12  (+ 100Ω resistor)
Motion sensor   → GPIO 13
Radar TX        → GPIO 15
Radar RX        → GPIO 14
IR LED          → GPIO 2   (+ 100Ω resistor, point at AC unit)
Temp sensor     → GPIO 3   (+ 1kΩ pullup to 3.3V)
```

---

## API

The ESP32 runs a simple HTTP server. The app talks to it over local WiFi.

```
GET  /status                    → room temp, humidity, presence, AC state
POST /light/on                  → lights on
POST /light/off                 → lights off
POST /light/brightness?value=80 → brightness 0-100%
POST /light/color?hex=A0C4FF    → RGB color
POST /ac/on                     → AC on via IR
POST /ac/off                    → AC off via IR
POST /ac/temp?value=22          → set AC dial temperature
POST /ac/target?value=24        → set target room temperature
POST /ac/mode?value=Cool        → Cool / Heat / Fan
POST /ac/speed?value=Med        → Low / Med / High
POST /settings                  → update timers (JSON body)
```

---

## Team

**Mohd. Kasim** — Team lead. Handled overall project direction and built the main app UI — the dashboard, tab navigation and card layouts. Also wrote the blog article we published on Hashnode.

**Abhinav Chauhan** — Built the app features — the color wheel picker, AC card controls, live temperature display, timer settings, and the API layer that connects the app to the ESP32.

**Meghna Garai** — Wrote the ESP32 firmware. Implemented the sensor fusion logic, the smart cooling algorithm, the IR control system and the WiFi server that the app talks to.

**Visha Gangwar** — Handled all the hardware. Wired up every component, figured out the power setup for the LED strip, got the IR LED positioned correctly for the AC, and built the physical prototype.

---

## What We'd Do Next

The current prototype controls one room with one ESP32. The natural next step is one ESP32 per room — the app already supports multiple IP addresses conceptually. We'd also want to use the ESP32-CAM's onboard camera for actual person counting rather than just presence detection, and add real electricity usage tracking with cost calculations in rupees.

---

*HACKHAZARDS '26 — Team LunaSense*

# LunaSense
**Smart home automation that actually understands if someone is in the room**

We built LunaSense for HACKHAZARDS '26. It's a system that controls lights and AC based on whether a room is actually occupied — not just whether someone moved recently.

---

## The Problem

Every smart light system we've seen has the same flaw. They use a basic motion sensor that only detects movement. The moment you sit still — reading, sleeping, working at your desk — the sensor thinks you left and turns everything off.

We got the idea from our college classroom. The AC is set to 16°C and whenever we want to change the temperature we have to find someone from outside who has the master control. We figured — why not let the AC adjust itself based on who's actually in the room? And while we were at it, the lights in our college labs keep staying on long after everyone leaves because the motion sensor stops detecting people who are sitting quietly.

So we built something that solves both.

---

## Project Links

**Demo Video** — [Watch on YouTube](YOUR_YOUTUBE_LINK_HERE)

**Blog Article** — [Building LunaSense on Hashnode](https://lunasense.hashnode.dev/building-lunasense?utm_source=hashnode&utm_medium=feed)

**Live Demo** — [LunaSense Interactive Demo](https://abhinav-231.github.io/LunaSense/lunasense.html)

**GitHub** — [github.com/Abhinav-231/LunaSense](https://github.com/Abhinav-231/LunaSense)

---

## How It Works

LunaSense uses two sensors together instead of one:

- A **motion sensor** (PIR) that reacts instantly when someone enters a room
- A **presence sensor** (mmWave radar) that detects a human even when completely still — it can pick up the rise and fall of your chest while you breathe

The idea is simple. The motion sensor is fast but dumb — it misses still people and triggers on curtains and pets. The radar is slower but smart — it knows the difference between a human and a moving object. Together they cover every real scenario.

The **ESP32-CAM microcontroller** sits in the middle, reading both sensors and making decisions. It runs a small web server over WiFi so our React Native app can send commands and get live data back.

For lights we use a **WS2812B LED strip** which gives full RGB color control from the app. For AC we use an **IR LED module** that blasts the same infrared signals your AC remote sends — no wiring into the unit, no voiding warranty.

We also added a **DHT22 temperature sensor** so the system knows the actual room temperature, not just what the AC dial is set to. This lets us do proper closed-loop control — if the room is still warm, keep cooling. If it overshot, ease up.

---

## Features

**Lights**
- Turns on automatically when PIR detects someone entering
- Stays on even when person is sitting still — mmWave radar detects breathing
- Brightness adjusts automatically based on distance from sensor
- Full RGB color control from the app via a circular color wheel — 16 million colors
- Turns off after configurable period of no movement
- Anomaly detection — unexpected energy spike triggers visual alert and app notification

**AC**
- Turn on/off, set temperature, change mode and fan speed from the app
- Reads actual room temperature every 5 seconds via DHT22
- Automatically steps AC temperature up or down to reach your target room temp
- Detects when activity increases and drops temperature by 1°C for extra body heat
- Turns off automatically after room confirmed empty by radar
- Graceful shutdown via IR — no hard power cuts that damage the compressor

**App**
- Built with React Native and Expo — works on Android and iOS via Expo Go
- Live room temperature, humidity and presence shown at the top of the dashboard
- Circular HSV color wheel picker for the LED strip
- Configurable auto-off timers for both lights and AC
- Savings tab with live activity log showing the last 3 system events
- Full log modal showing complete system history
- Works entirely on local WiFi — no internet, no cloud, no data leaving your home

---

## Tech Stack

| Layer | Technology |
|---|---|
| Mobile App | React Native with Expo (expo-router) |
| Firmware | Arduino C++ on ESP32-CAM MB Board |
| Communication | HTTP over local WiFi (WebServer.h) |
| LED Control | FastLED library |
| AC Control | IRremoteESP8266 — NEC protocol |
| Temp + Humidity | DHT22 sensor |
| Presence | HLK-LD2410 mmWave radar + PIR fusion |
| Data Storage | AsyncStorage for room and timer config |

---

## How the Smart Cooling Works

Most smart AC systems just turn the unit on or off. We built a proper control loop:

1. User sets a **target room temperature** in the app (e.g. 24°C)
2. DHT22 reads **actual room temperature** every 5 seconds
3. Every 60 seconds the ESP32 compares them:
   - Room more than 1.5°C above target → IR signal TEMP DOWN
   - Room more than 0.5°C below target → IR signal TEMP UP
   - Room within range → hold, no signal sent
4. Every 30 seconds the radar energy level is checked — if activity spikes significantly the target automatically drops 1°C to account for extra body heat
5. When radar confirms the room is empty a countdown starts — AC turns off via IR after the set time

---

## Repository Structure

```
LunaSense/
├── src/
│   ├── app/
│   │   ├── _layout.tsx          ← Expo Router root layout
│   │   └── index.tsx            ← Main dashboard screen
│   └── components/
│       ├── ClimateCard.tsx      ← AC card with ON/OFF, temp, mode, speed
│       ├── RoomCard.tsx         ← Light card with HSV color wheel
│       ├── SavingsCard.tsx      ← Energy savings + live activity log
│       ├── GeneralTab.tsx       ← Timer settings
│       ├── ColorPicker.tsx      ← Color picker component
│       ├── LunaContext.tsx      ← Global ESP32 IP state
|       ├──PresenceIndicator.tsx ← Detects and indicates presence
├── firmware/
│   ├── 1_light_only/
│   │   └── 1_light_only_app.ino   ← Light control only
│   ├── 2_ac_only/
│   │   └── 2_ac_only_app.ino      ← AC control only
│   └── 3_combined/
│       └── 3_combined_app.ino     ← Full system (use this for demo)
├── docs/
│lunasense.html           ← Interactive system demo
│circuit_diagram.jpeg      ← Hardware wiring diagram
│system_flowchart.jpeg     ← Sensor fusion flowchart
├── assets/
├── api.ts                       ← All ESP32 HTTP calls
├── app.json                     ← Expo configuration
├── package.json
└── README.md
```

---

## Setting Up

### Flash the ESP32

**1. Install Arduino IDE** from `arduino.cc/en/software`

**2. Add ESP32 board support** — go to `File → Preferences` and paste in Additional Board Manager URLs:
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
Then `Tools → Board Manager` → search esp32 → install **esp32 by Espressif Systems**

**3. Install libraries** via `Sketch → Include Library → Manage Libraries`:
- FastLED by Daniel Garcia
- IRremoteESP8266 by crankyoldgit
- ArduinoJson by Benoit Blanchon (version 6)
- DHT sensor library by Adafruit
- Adafruit Unified Sensor by Adafruit

**4. Open firmware** — go to `firmware/3_combined/` and open `3_combined_app.ino`

**5. Edit credentials** at the top of the file:
```cpp
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
#define NUM_LEDS 30   // change to your strip length
```

**6. Board settings:**
- Board: `AI Thinker ESP32-CAM`
- Partition Scheme: `Huge APP (3MB No OTA)`
- Upload Speed: `115200`
- Port: select your COM port

**7. Upload** — click Upload, when you see `Connecting......` hold the BOOT button on the MB board for 2 seconds then release

**8. Find IP** — open Serial Monitor at 115200 baud:
```
✅ IP: 192.168.1.XX
🚀 LunaSense FULL SYSTEM ready!
```

### Run the App

**1. Clone the repo**
```bash
git clone https://github.com/Abhinav-231/LunaSense.git
cd LunaSense
```

**2. Install dependencies**
```bash
npm install
npx expo install react-native-svg
npx expo install @react-native-async-storage/async-storage
npx expo install react-native-safe-area-context
```

**3. Start**
```bash
npx expo start
```

**4. Open on phone** — install Expo Go from Play Store or App Store, make sure phone is on same WiFi as ESP32, scan the QR code

**5. Connect to ESP32** — go to General tab in the app, enter the IP address from Serial Monitor, tap Save Settings

---

## Wiring

```
WS2812B  DIN  → GPIO 12  (+ 100Ω resistor)
WS2812B  5V   → Separate 5V power supply
WS2812B  GND  → Common GND

PIR      OUT  → GPIO 13
PIR      VCC  → 5V
PIR      GND  → GND

LD2410   TX   → GPIO 15  (ESP32 RX2)
LD2410   RX   → GPIO 14  (ESP32 TX2)
LD2410   VCC  → 3.3V
LD2410   GND  → GND

IR Module  S  → GPIO 2
IR Module  +  → 3.3V
IR Module  -  → GND

DHT22    OUT  → GPIO 4   (+ 1kΩ resistor to 3.3V)
DHT22    VCC  → 3.3V
DHT22    GND  → GND
```

---

## API Endpoints

The ESP32 runs a local HTTP server on your WiFi network.

```
GET  /status                     → room temp, humidity, presence, AC and light state
POST /light/on                   → lights on
POST /light/off                  → lights off
POST /light/brightness?value=80  → brightness 0–100%
POST /light/color?hex=A0C4FF     → RGB color from hex
POST /light/anomaly?enabled=1    → enable anomaly detection
POST /ac/on                      → AC on via IR
POST /ac/off                     → AC off via IR
POST /ac/temp?value=22           → set AC dial temperature 16–32°C
POST /ac/target?value=24         → set target room temperature
POST /ac/mode?value=Cool         → Cool / Heat / Fan
POST /ac/speed?value=Med         → Low / Med / High
POST /settings                   → update timers (JSON body)
```

---

## How We Used Expo

We used Expo with the managed workflow throughout the project:

- **expo-router** for file-based routing — `_layout.tsx` defines the Stack navigator and `index.tsx` is the main screen, navigation happens automatically based on file structure
- **react-native-svg** installed via `npx expo install` for the custom circular HSV color wheel in RoomCard — built using SVG Circle, Path and RadialGradient primitives
- **AsyncStorage** via `@react-native-async-storage/async-storage` to persist the ESP32 IP address, room names and timer settings locally on the phone
- **fetch API** in Expo's JS runtime to send HTTP POST requests to the ESP32 and poll `/status` every 5 seconds for live sensor data
- **Animated API** from React Native for the fade-in animations in the live activity log in SavingsCard
- **Expo Go** on Android for instant hot reload testing throughout development — never needed to touch Android Studio
- **app.json** for managed config — adaptive icons, splash screen via expo-splash-screen plugin, and scheme setup

---

## Team

**Mohd. Kazim** — Team lead. Handled overall project direction and built the main app UI — the dashboard, tab navigation and card layouts. Also wrote the blog article published on Hashnode.
[LinkedIn](https://www.linkedin.com/in/mohd-kazim-202bb5337)

**Abhinav Chauhan** — Built the app features — the color wheel picker, AC card controls, live temperature display, timer settings, savings tab with live activity log, and the API layer connecting the app to the ESP32.
[LinkedIn](https://www.linkedin.com/in/abhinav-chauhan-8800743b1)

**Meghna Garai** — Wrote the ESP32 firmware. Implemented the dual-sensor fusion logic, smart cooling algorithm, IR control system and the WiFi HTTP server.
[LinkedIn](https://www.linkedin.com/in/meghna-garai-44b077380)

**Visha Gangwar** — Handled all the hardware. Wired every component, set up the separate power supply for the LED strip, positioned the IR module for the AC, and built the physical prototype.
[LinkedIn](https://www.linkedin.com/in/visha-gangwar-026721421)
---

## What We'd Do Next

The current prototype controls one room with one ESP32. The next step is one ESP32 per room with the app managing multiple IPs. We'd also use the ESP32-CAM's onboard camera for actual person counting rather than just presence detection, add voice control using Sarvam AI for Hindi and English commands, and build real electricity cost tracking in rupees.

---

*HACKHAZARDS '26 — Team LunaSense*

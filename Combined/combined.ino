/*
  ╔══════════════════════════════════════════════════════════╗
  ║         LUNASENSE — FULL SYSTEM FINAL                   ║
  ║                                                         ║
  ║  WIRING:                                                ║
  ║  WS2812B  DIN  → GPIO 12  (+ 100Ω resistor)            ║
  ║  WS2812B  5V   → Separate 5V power supply              ║
  ║  WS2812B  GND  → Common GND                            ║
  ║  PIR      OUT  → GPIO 13                               ║
  ║  PIR      VCC  → 5V                                    ║
  ║  PIR      GND  → GND                                   ║
  ║  LD2410   TX   → GPIO 15  (ESP32 RX2)                  ║
  ║  LD2410   RX   → GPIO 14  (ESP32 TX2)                  ║
  ║  LD2410   VCC  → 3.3V                                  ║
  ║  LD2410   GND  → GND                                   ║
  ║  IR Module S   → GPIO 2                                ║
  ║  IR Module +   → 3.3V                                  ║
  ║  IR Module -   → GND                                   ║
  ║  DHT22    OUT  → GPIO 4   (+ 1kΩ pullup to 3.3V)      ║
  ║  DHT22    VCC  → 3.3V                                  ║
  ║  DHT22    GND  → GND                                   ║
  ╚══════════════════════════════════════════════════════════╝

  LIBRARIES — Arduino IDE Library Manager:
  1. FastLED              by Daniel Garcia
  2. IRremoteESP8266      by crankyoldgit
  3. ArduinoJson          by Benoit Blanchon (v6)
  4. DHT sensor library   by Adafruit
  5. Adafruit Unified Sensor by Adafruit

  WebServer is BUILT IN — no extra install needed
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <DHT.h>

// ─── CHANGE THESE ──────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
// ───────────────────────────────────────────────────────────

// ─── PINS ──────────────────────────────────────────────────
#define LED_PIN     12
#define PIR_PIN     13
#define MM_RX       15
#define MM_TX       14
#define IR_PIN      2
#define DHT_PIN     4
#define DHT_TYPE    DHT22
// ───────────────────────────────────────────────────────────

// ─── WS2812B ───────────────────────────────────────────────
#define NUM_LEDS    30        // ← change to your strip length
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];
// ───────────────────────────────────────────────────────────

// ─── AC IR CODES ───────────────────────────────────────────
#define AC_ON_CODE    0x10AF8877UL
#define AC_OFF_CODE   0x10AF08F7UL
#define AC_TEMP_UP    0x10AF48B7UL
#define AC_TEMP_DOWN  0x10AFC837UL
#define AC_MODE_COOL  0x10AF28D7UL
#define AC_MODE_HEAT  0x10AFA857UL
#define AC_MODE_FAN   0x10AF6897UL
#define AC_FAN_LOW    0x10AF18E7UL
#define AC_FAN_MED    0x10AF9867UL
#define AC_FAN_HIGH   0x10AFE817UL
// ───────────────────────────────────────────────────────────

WebServer server(80);
HardwareSerial mmWaveSerial(2);
IRsend irsend(IR_PIN);
DHT dht(DHT_PIN, DHT_TYPE);

// ─── STATE ─────────────────────────────────────────────────
// Light
bool    lightOn          = false;
uint8_t lightBrightness  = 200;
CRGB    lightColor       = CRGB::White;
int     lightTimerMins   = 15;
bool    anomalyEnabled   = false;   // set per room from app
int     anomalyThreshold = 4;       // people count threshold
unsigned long lastActivityTime = 0;

// AC
bool    acOn             = false;
int     acTemp           = 24;
int     targetTemp       = 22;
String  acMode           = "Cool";
String  acSpeed          = "Med";
int     acTimerMins      = 10;

// Sensors — shared between light and AC logic
bool    roomOccupied     = false;
bool    motionDetected   = false;
float   roomTemp         = 0;
float   roomHumidity     = 0;
uint8_t motionEnergy     = 0;
uint8_t staticEnergy     = 0;
uint16_t targetDistance  = 0;
bool    prevOccupied     = false;  // track changes

// Anomaly
bool    anomalyTriggered = false;
unsigned long anomalyTime = 0;

// Timers
unsigned long roomEmptyTime      = 0;
unsigned long lastDHTRead        = 0;
unsigned long lastSmartCool      = 0;
unsigned long lastActivityCheck  = 0;
bool          roomWasEmpty       = false;
int           lastEnergyLevel    = 0;

// ─── HELPERS ───────────────────────────────────────────────
void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendIR(uint32_t code) {
  irsend.sendNEC(code, 32);
  delay(100);
}

void applyLEDs() {
  if (!lightOn) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  } else {
    fill_solid(leds, NUM_LEDS, lightColor);
    FastLED.setBrightness(lightBrightness);
  }
  FastLED.show();
}

CRGB hexToCRGB(String hex) {
  hex.replace("#", "");
  long val = strtol(hex.c_str(), NULL, 16);
  return CRGB((val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF);
}

// ─── mmWave READER ─────────────────────────────────────────
// Reads full LD2410 frame:
// [AA][FF][03][00][presence][motionEnergy][staticEnergy][distL][distH][55][CC]
void readMmWave() {
  if (mmWaveSerial.available() >= 4) {
    uint8_t buf[32];
    int len = mmWaveSerial.readBytes(
      buf, min((int)mmWaveSerial.available(), 32)
    );
    for (int i = 0; i < len - 9; i++) {
      if (buf[i] == 0xAA && buf[i+1] == 0xFF) {
        if (i + 9 < len) {
          roomOccupied   = (buf[i+4] != 0x00);
          motionEnergy   = buf[i+5];
          staticEnergy   = buf[i+6];
          targetDistance = buf[i+7] | (buf[i+8] << 8);
        }
        break;
      }
    }
  }
}

// ─── ANOMALY DETECTION ─────────────────────────────────────
// Fires when motion is detected at unusual hours (11pm - 6am)
// OR when mmWave energy spikes above threshold unexpectedly
// Used for LIGHT module — sends alert via /status endpoint
void checkAnomaly() {
  if (!anomalyEnabled) return;

  int hour = 0; // In production use NTP time
  // For now use energy spike as anomaly trigger
  int totalEnergy = motionEnergy + staticEnergy;

  // Anomaly: very high energy (multiple people) beyond threshold
  if (totalEnergy > (anomalyThreshold * 20) && !anomalyTriggered) {
    anomalyTriggered = true;
    anomalyTime      = millis();
    Serial.println("⚠️ ANOMALY DETECTED — energy spike: " + String(totalEnergy));
    // Flash light red as visual alert
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(500);
    applyLEDs(); // restore normal color
  }

  // Reset anomaly after 30 seconds
  if (anomalyTriggered && (millis() - anomalyTime) > 30000) {
    anomalyTriggered = false;
  }
}

// ─── SMART COOLING ─────────────────────────────────────────
// Runs every 60s — compares DHT22 room temp to target
// and steps AC temperature up or down via IR
void smartCoolRoom() {
  if (!acOn || roomTemp <= 0) return;
  float diff = roomTemp - targetTemp;
  if (diff > 1.5) {
    Serial.println("🌡 Room " + String(roomTemp,1) + "°C > target " +
      String(targetTemp) + "°C → TEMP DOWN");
    sendIR(AC_TEMP_DOWN);
    if (acTemp > 16) acTemp--;
  } else if (diff < -0.5) {
    Serial.println("🌡 Room " + String(roomTemp,1) + "°C < target " +
      String(targetTemp) + "°C → TEMP UP");
    sendIR(AC_TEMP_UP);
    if (acTemp < 32) acTemp++;
  } else {
    Serial.println("✅ Room at " + String(roomTemp,1) + "°C — holding");
  }
}

// ─── ACTIVITY BASED COOLING ────────────────────────────────
// Runs every 30s — if mmWave energy spikes significantly
// (more people / more movement) drop AC 1°C automatically
void checkActivityLevel() {
  if (!acOn) return;
  int currentEnergy = motionEnergy + staticEnergy;
  int energyDiff    = currentEnergy - lastEnergyLevel;
  if (energyDiff > 40 && acTemp > 16) {
    Serial.println("👥 Activity spike → dropping AC 1°C");
    sendIR(AC_TEMP_DOWN);
    acTemp--;
  }
  lastEnergyLevel = currentEnergy;
}

// ─── ROUTE HANDLERS ────────────────────────────────────────
void handleOptions() {
  addCORS();
  server.send(200, "text/plain", "");
}

void handleStatus() {
  StaticJsonDocument<300> doc;
  // Light
  doc["lightOn"]        = lightOn;
  doc["brightness"]     = lightBrightness;
  doc["anomaly"]        = anomalyTriggered;
  doc["anomalyEnabled"] = anomalyEnabled;
  doc["lightTimer"]     = lightTimerMins;
  // AC
  doc["acOn"]           = acOn;
  doc["acTemp"]         = acTemp;
  doc["targetTemp"]     = targetTemp;
  doc["acMode"]         = acMode;
  doc["acSpeed"]        = acSpeed;
  doc["acTimer"]        = acTimerMins;
  // Sensors — shared
  doc["roomTemp"]       = roomTemp;
  doc["roomHumidity"]   = roomHumidity;
  doc["roomOccupied"]   = roomOccupied;
  doc["motion"]         = motionDetected;
  doc["motionEnergy"]   = motionEnergy;
  doc["staticEnergy"]   = staticEnergy;
  doc["distance"]       = targetDistance;
  String out;
  serializeJson(doc, out);
  addCORS();
  server.send(200, "application/json", out);
}

// ── LIGHT ──
void handleLightOn() {
  lightOn = true;
  lastActivityTime = millis();
  applyLEDs();
  Serial.println("💡 Light ON");
  addCORS();
  server.send(200, "application/json", "{\"status\":\"light_on\"}");
}

void handleLightOff() {
  lightOn = false;
  applyLEDs();
  Serial.println("💡 Light OFF");
  addCORS();
  server.send(200, "application/json", "{\"status\":\"light_off\"}");
}

void handleBrightness() {
  if (server.hasArg("value")) {
    int pct = constrain(server.arg("value").toInt(), 0, 100);
    lightBrightness = map(pct, 0, 100, 0, 255);
    FastLED.setBrightness(lightBrightness);
    FastLED.show();
  }
  addCORS();
  server.send(200, "application/json", "{\"status\":\"brightness_set\"}");
}

void handleColor() {
  if (server.hasArg("hex")) {
    lightColor = hexToCRGB(server.arg("hex"));
    if (lightOn) applyLEDs();
    Serial.println("🎨 Color: #" + server.arg("hex"));
  }
  addCORS();
  server.send(200, "application/json", "{\"status\":\"color_set\"}");
}

// Enable/disable anomaly detection from app
void handleAnomaly() {
  if (server.hasArg("enabled")) {
    anomalyEnabled = server.arg("enabled") == "1";
    Serial.println("🛡 Anomaly: " + String(anomalyEnabled ? "ON" : "OFF"));
  }
  if (server.hasArg("threshold")) {
    anomalyThreshold = server.arg("threshold").toInt();
  }
  addCORS();
  server.send(200, "application/json", "{\"status\":\"anomaly_set\"}");
}

// ── AC ──
void handleAcOn() {
  acOn = true;
  sendIR(AC_ON_CODE);
  Serial.println("❄️ AC ON");
  addCORS();
  server.send(200, "application/json", "{\"status\":\"ac_on\"}");
}

void handleAcOff() {
  acOn = false;
  sendIR(AC_OFF_CODE);
  Serial.println("❄️ AC OFF");
  addCORS();
  server.send(200, "application/json", "{\"status\":\"ac_off\"}");
}

void handleTemp() {
  if (server.hasArg("value")) {
    int newTemp = constrain(server.arg("value").toInt(), 16, 32);
    if (newTemp > acTemp)
      for (int i = acTemp; i < newTemp; i++) { sendIR(AC_TEMP_UP);   delay(200); }
    else
      for (int i = acTemp; i > newTemp; i--) { sendIR(AC_TEMP_DOWN); delay(200); }
    acTemp = newTemp;
    Serial.println("🌡 AC temp → " + String(acTemp) + "°C");
  }
  addCORS();
  server.send(200, "application/json", "{\"status\":\"temp_set\"}");
}

void handleTargetTemp() {
  if (server.hasArg("value")) {
    targetTemp = constrain(server.arg("value").toInt(), 16, 30);
    Serial.println("🎯 Target → " + String(targetTemp) + "°C");
  }
  addCORS();
  server.send(200, "application/json", "{\"status\":\"target_set\"}");
}

void handleMode() {
  if (server.hasArg("value")) {
    acMode = server.arg("value");
    if      (acMode == "Cool") sendIR(AC_MODE_COOL);
    else if (acMode == "Heat") sendIR(AC_MODE_HEAT);
    else if (acMode == "Fan")  sendIR(AC_MODE_FAN);
    Serial.println("💨 Mode → " + acMode);
  }
  addCORS();
  server.send(200, "application/json", "{\"status\":\"mode_set\"}");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    acSpeed = server.arg("value");
    if      (acSpeed == "Low")  sendIR(AC_FAN_LOW);
    else if (acSpeed == "Med")  sendIR(AC_FAN_MED);
    else if (acSpeed == "High") sendIR(AC_FAN_HIGH);
    Serial.println("🌀 Fan → " + acSpeed);
  }
  addCORS();
  server.send(200, "application/json", "{\"status\":\"speed_set\"}");
}

void handleSettings() {
  String body = server.arg("plain");
  if (body.length() > 0) {
    StaticJsonDocument<128> doc;
    if (!deserializeJson(doc, body)) {
      if (doc.containsKey("lightTimer")) lightTimerMins = doc["lightTimer"].as<int>();
      if (doc.containsKey("acTimer"))    acTimerMins    = doc["acTimer"].as<int>();
      Serial.println("⚙️ Timers — Light: " + String(lightTimerMins) +
        "m  AC: " + String(acTimerMins) + "m");
    }
  }
  addCORS();
  server.send(200, "application/json", "{\"status\":\"settings_saved\"}");
}

// ─── SETUP ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  mmWaveSerial.begin(256000, SERIAL_8N1, MM_RX, MM_TX);
  pinMode(PIR_PIN, INPUT);

  // FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(200);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  irsend.begin();
  dht.begin();

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ IP: " + WiFi.localIP().toString());

  // ── Routes ──
  server.on("/status",           HTTP_GET,     handleStatus);
  server.on("/light/on",         HTTP_POST,    handleLightOn);
  server.on("/light/off",        HTTP_POST,    handleLightOff);
  server.on("/light/brightness", HTTP_POST,    handleBrightness);
  server.on("/light/color",      HTTP_POST,    handleColor);
  server.on("/light/anomaly",    HTTP_POST,    handleAnomaly);
  server.on("/ac/on",            HTTP_POST,    handleAcOn);
  server.on("/ac/off",           HTTP_POST,    handleAcOff);
  server.on("/ac/temp",          HTTP_POST,    handleTemp);
  server.on("/ac/target",        HTTP_POST,    handleTargetTemp);
  server.on("/ac/mode",          HTTP_POST,    handleMode);
  server.on("/ac/speed",         HTTP_POST,    handleSpeed);
  server.on("/settings",         HTTP_POST,    handleSettings);

  // CORS preflight
  server.on("/status",           HTTP_OPTIONS, handleOptions);
  server.on("/light/on",         HTTP_OPTIONS, handleOptions);
  server.on("/light/off",        HTTP_OPTIONS, handleOptions);
  server.on("/light/brightness", HTTP_OPTIONS, handleOptions);
  server.on("/light/color",      HTTP_OPTIONS, handleOptions);
  server.on("/light/anomaly",    HTTP_OPTIONS, handleOptions);
  server.on("/ac/on",            HTTP_OPTIONS, handleOptions);
  server.on("/ac/off",           HTTP_OPTIONS, handleOptions);
  server.on("/ac/temp",          HTTP_OPTIONS, handleOptions);
  server.on("/ac/target",        HTTP_OPTIONS, handleOptions);
  server.on("/ac/mode",          HTTP_OPTIONS, handleOptions);
  server.on("/ac/speed",         HTTP_OPTIONS, handleOptions);
  server.on("/settings",         HTTP_OPTIONS, handleOptions);

  server.begin();
  Serial.println("🚀 LunaSense FULL SYSTEM ready!");
  Serial.println("   WS2812B : GPIO 12 (" + String(NUM_LEDS) + " LEDs)");
  Serial.println("   PIR     : GPIO 13");
  Serial.println("   mmWave  : GPIO 15/14");
  Serial.println("   IR      : GPIO 2");
  Serial.println("   DHT22   : GPIO 4");
}

// ─── LOOP ──────────────────────────────────────────────────
void loop() {
  server.handleClient();
  readMmWave();

  // PIR — shared between light and AC
  bool pirNow = digitalRead(PIR_PIN);
  if (pirNow && !motionDetected) {
    Serial.println("⚡ PIR — motion detected!");
  }
  motionDetected = pirNow;
  if (motionDetected) lastActivityTime = millis();

  // Log mmWave presence changes
  if (roomOccupied != prevOccupied) {
    Serial.println(roomOccupied ?
      "👤 mmWave — person detected (energy: " + String(motionEnergy + staticEnergy) + ")" :
      "🚶 mmWave — room empty"
    );
    prevOccupied = roomOccupied;
  }

  unsigned long now = millis();

  // ── DHT22 every 5 seconds ──
  if (now - lastDHTRead >= 5000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      roomTemp     = t;
      roomHumidity = h;
      Serial.println("🌡 " + String(roomTemp,1) + "°C  💧 " + String(roomHumidity,0) + "%");
    }
    lastDHTRead = now;
  }

  // ══════════════════════════════════════════════════
  // LIGHT LOGIC — uses BOTH PIR and mmWave
  // ══════════════════════════════════════════════════

  // Auto ON: PIR detects motion → turn light on
  if (motionDetected && !lightOn) {
    lightOn = true;
    applyLEDs();
    Serial.println("💡 Auto ON — PIR motion");
  }

  // Distance-based brightness — mmWave distance grading
  // Closer person = brighter light
  if (lightOn && targetDistance > 0) {
    uint8_t autoBright;
    if      (targetDistance < 100) autoBright = 255; // very close
    else if (targetDistance < 200) autoBright = 200; // normal
    else if (targetDistance < 350) autoBright = 140; // far
    else                           autoBright = 80;  // edge of range
    if (abs((int)autoBright - (int)lightBrightness) > 20) {
      lightBrightness = autoBright;
      FastLED.setBrightness(lightBrightness);
      FastLED.show();
    }
  }

  // Inactivity timer — PIR no motion → auto off
  // mmWave can still detect person but light dims anyway
  // (intentional — saves power when person is completely still)
  if (lightOn) {
    unsigned long lightMs = (unsigned long)lightTimerMins * 60UL * 1000UL;
    if ((now - lastActivityTime) >= lightMs) {
      lightOn = false;
      applyLEDs();
      Serial.println("💤 Light auto-off: no PIR activity for " +
        String(lightTimerMins) + " mins");
    }
  }

  // Anomaly check — uses mmWave energy spike
  checkAnomaly();

  // ══════════════════════════════════════════════════
  // AC LOGIC — uses BOTH PIR and mmWave
  // ══════════════════════════════════════════════════

  // Smart cooling — DHT22 closed loop
  if (acOn && now - lastSmartCool >= 60000) {
    smartCoolRoom();
    lastSmartCool = now;
  }

  // Activity spike — mmWave energy
  if (acOn && now - lastActivityCheck >= 30000) {
    checkActivityLevel();
    lastActivityCheck = now;
  }

  // Empty room timer — mmWave confirms truly empty
  // PIR alone not enough — person sitting still
  // would give false empty reading
  if (acOn) {
    if (!roomOccupied) {
      if (!roomWasEmpty) {
        roomWasEmpty  = true;
        roomEmptyTime = now;
        Serial.println("🚶 Room empty — AC timer started");
      } else {
        unsigned long acMs = (unsigned long)acTimerMins * 60UL * 1000UL;
        if ((now - roomEmptyTime) >= acMs) {
          acOn = false;
          sendIR(AC_OFF_CODE);
          roomWasEmpty = false;
          Serial.println("❄️ AC auto-off: empty for " +
            String(acTimerMins) + " mins");
        }
      }
    } else {
      if (roomWasEmpty)
        Serial.println("👤 Person back — AC timer cancelled");
      roomWasEmpty = false;
    }
  }

  delay(100);
}

/*
  ╔══════════════════════════════════════════╗
  ║   LUNASENSE — LIGHT CONTROL VIA APP     ║
  ║                                         ║
  ║  WS2812B DIN → GPIO 12 (+ 100Ω)        ║
  ║  WS2812B 5V  → Separate power           ║
  ║  WS2812B GND → GND                     ║
  ║  PIR     OUT → GPIO 13                 ║
  ║  PIR     VCC → 5V                      ║
  ║  LD2410  TX  → GPIO 15                 ║
  ║  LD2410  RX  → GPIO 14                 ║
  ║  LD2410  VCC → 3.3V                    ║
  ╚══════════════════════════════════════════╝

  LIBRARIES: FastLED, ArduinoJson
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <FastLED.h>

const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

#define LED_PIN     12
#define PIR_PIN     13
#define MM_RX       15
#define MM_TX       14
#define NUM_LEDS    30
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
WebServer server(80);
HardwareSerial mmWaveSerial(2);

bool    lightOn         = false;
uint8_t lightBrightness = 200;
CRGB    lightColor      = CRGB::White;
int     lightTimerMins  = 15;
bool    anomalyEnabled  = false;
bool    anomalyTriggered = false;
unsigned long anomalyTime = 0;

bool    roomOccupied    = false;
bool    motionDetected  = false;
uint8_t motionEnergy    = 0;
uint8_t staticEnergy    = 0;
uint16_t targetDistance = 0;
bool    prevOccupied    = false;
unsigned long lastActivityTime = 0;

void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void applyLEDs() {
  if (!lightOn) fill_solid(leds, NUM_LEDS, CRGB::Black);
  else {
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

void readMmWave() {
  if (mmWaveSerial.available() >= 4) {
    uint8_t buf[32];
    int len = mmWaveSerial.readBytes(buf, min((int)mmWaveSerial.available(), 32));
    for (int i = 0; i < len - 9; i++) {
      if (buf[i] == 0xAA && buf[i+1] == 0xFF && i + 9 < len) {
        roomOccupied   = (buf[i+4] != 0x00);
        motionEnergy   = buf[i+5];
        staticEnergy   = buf[i+6];
        targetDistance = buf[i+7] | (buf[i+8] << 8);
        break;
      }
    }
  }
}

void checkAnomaly() {
  if (!anomalyEnabled) return;
  int totalEnergy = motionEnergy + staticEnergy;
  if (totalEnergy > 80 && !anomalyTriggered) {
    anomalyTriggered = true;
    anomalyTime = millis();
    Serial.println("⚠️ ANOMALY — energy: " + String(totalEnergy));
    // Flash red
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show(); delay(500);
    applyLEDs();
  }
  if (anomalyTriggered && (millis() - anomalyTime) > 30000)
    anomalyTriggered = false;
}

void handleOptions() { addCORS(); server.send(200, "text/plain", ""); }

void handleStatus() {
  StaticJsonDocument<200> doc;
  doc["lightOn"]        = lightOn;
  doc["brightness"]     = lightBrightness;
  doc["anomaly"]        = anomalyTriggered;
  doc["anomalyEnabled"] = anomalyEnabled;
  doc["roomOccupied"]   = roomOccupied;
  doc["motion"]         = motionDetected;
  doc["motionEnergy"]   = motionEnergy;
  doc["staticEnergy"]   = staticEnergy;
  doc["distance"]       = targetDistance;
  doc["lightTimer"]     = lightTimerMins;
  String out; serializeJson(doc, out);
  addCORS(); server.send(200, "application/json", out);
}

void handleLightOn() {
  lightOn = true; lastActivityTime = millis(); applyLEDs();
  Serial.println("💡 ON");
  addCORS(); server.send(200, "application/json", "{\"status\":\"light_on\"}");
}

void handleLightOff() {
  lightOn = false; applyLEDs();
  Serial.println("💡 OFF");
  addCORS(); server.send(200, "application/json", "{\"status\":\"light_off\"}");
}

void handleBrightness() {
  if (server.hasArg("value")) {
    int pct = constrain(server.arg("value").toInt(), 0, 100);
    lightBrightness = map(pct, 0, 100, 0, 255);
    FastLED.setBrightness(lightBrightness);
    FastLED.show();
  }
  addCORS(); server.send(200, "application/json", "{\"status\":\"brightness_set\"}");
}

void handleColor() {
  if (server.hasArg("hex")) {
    lightColor = hexToCRGB(server.arg("hex"));
    if (lightOn) applyLEDs();
  }
  addCORS(); server.send(200, "application/json", "{\"status\":\"color_set\"}");
}

void handleAnomaly() {
  if (server.hasArg("enabled"))
    anomalyEnabled = server.arg("enabled") == "1";
  Serial.println("🛡 Anomaly: " + String(anomalyEnabled ? "ON" : "OFF"));
  addCORS(); server.send(200, "application/json", "{\"status\":\"anomaly_set\"}");
}

void handleSettings() {
  String body = server.arg("plain");
  if (body.length() > 0) {
    StaticJsonDocument<64> doc;
    if (!deserializeJson(doc, body))
      if (doc.containsKey("lightTimer")) lightTimerMins = doc["lightTimer"].as<int>();
  }
  addCORS(); server.send(200, "application/json", "{\"status\":\"settings_saved\"}");
}

void setup() {
  Serial.begin(115200);
  mmWaveSerial.begin(256000, SERIAL_8N1, MM_RX, MM_TX);
  pinMode(PIR_PIN, INPUT);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(200);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ IP: " + WiFi.localIP().toString());

  server.on("/status",           HTTP_GET,     handleStatus);
  server.on("/light/on",         HTTP_POST,    handleLightOn);
  server.on("/light/off",        HTTP_POST,    handleLightOff);
  server.on("/light/brightness", HTTP_POST,    handleBrightness);
  server.on("/light/color",      HTTP_POST,    handleColor);
  server.on("/light/anomaly",    HTTP_POST,    handleAnomaly);
  server.on("/settings",         HTTP_POST,    handleSettings);
  server.on("/status",           HTTP_OPTIONS, handleOptions);
  server.on("/light/on",         HTTP_OPTIONS, handleOptions);
  server.on("/light/off",        HTTP_OPTIONS, handleOptions);
  server.on("/light/brightness", HTTP_OPTIONS, handleOptions);
  server.on("/light/color",      HTTP_OPTIONS, handleOptions);
  server.on("/light/anomaly",    HTTP_OPTIONS, handleOptions);
  server.on("/settings",         HTTP_OPTIONS, handleOptions);

  server.begin();
  Serial.println("🚀 LIGHT server ready!");
}

void loop() {
  server.handleClient();
  readMmWave();

  bool pirNow = digitalRead(PIR_PIN);
  if (pirNow && !motionDetected) Serial.println("⚡ PIR motion!");
  motionDetected = pirNow;
  if (motionDetected) lastActivityTime = millis();

  if (roomOccupied != prevOccupied) {
    Serial.println(roomOccupied ? "👤 Person detected" : "🚶 Room empty");
    prevOccupied = roomOccupied;
  }

  // Auto ON via PIR
  if (motionDetected && !lightOn) {
    lightOn = true; applyLEDs();
    Serial.println("💡 Auto ON");
  }

  // Distance-based brightness via mmWave
  if (lightOn && targetDistance > 0) {
    uint8_t autoBright;
    if      (targetDistance < 100) autoBright = 255;
    else if (targetDistance < 200) autoBright = 200;
    else if (targetDistance < 350) autoBright = 140;
    else                           autoBright = 80;
    if (abs((int)autoBright - (int)lightBrightness) > 20) {
      lightBrightness = autoBright;
      FastLED.setBrightness(lightBrightness);
      FastLED.show();
    }
  }

  // Inactivity timer
  if (lightOn) {
    unsigned long ms = (unsigned long)lightTimerMins * 60UL * 1000UL;
    if ((millis() - lastActivityTime) >= ms) {
      lightOn = false; applyLEDs();
      Serial.println("💤 Auto OFF — inactivity");
    }
  }

  checkAnomaly();
  delay(100);
}

/*
  ╔══════════════════════════════════════════╗
  ║   LUNASENSE — AC CONTROL VIA APP        ║
  ║                                         ║
  ║  IR Module S  → GPIO 2                  ║
  ║  IR Module +  → 3.3V                   ║
  ║  IR Module -  → GND                    ║
  ║  PIR     OUT  → GPIO 13                ║
  ║  PIR     VCC  → 5V                     ║
  ║  LD2410  TX   → GPIO 15                ║
  ║  LD2410  RX   → GPIO 14                ║
  ║  LD2410  VCC  → 3.3V                   ║
  ║  DHT22   OUT  → GPIO 4 (+ 1kΩ pullup) ║
  ║  DHT22   VCC  → 3.3V                   ║
  ╚══════════════════════════════════════════╝

  LIBRARIES: IRremoteESP8266, ArduinoJson,
             DHT sensor library, Adafruit Unified Sensor
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <DHT.h>

const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

#define PIR_PIN   13
#define MM_RX     15
#define MM_TX     14
#define IR_PIN    2
#define DHT_PIN   4
#define DHT_TYPE  DHT22

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

IRsend irsend(IR_PIN);
WebServer server(80);
HardwareSerial mmWaveSerial(2);
DHT dht(DHT_PIN, DHT_TYPE);

bool    acOn            = false;
int     acTemp          = 24;
int     targetTemp      = 22;
String  acMode          = "Cool";
String  acSpeed         = "Med";
int     acTimerMins     = 10;

bool    roomOccupied    = false;
bool    motionDetected  = false;
float   roomTemp        = 0;
float   roomHumidity    = 0;
uint8_t motionEnergy    = 0;
uint8_t staticEnergy    = 0;
bool    prevOccupied    = false;

unsigned long roomEmptyTime     = 0;
unsigned long lastDHTRead       = 0;
unsigned long lastSmartCool     = 0;
unsigned long lastActivityCheck = 0;
bool          roomWasEmpty      = false;
int           lastEnergyLevel   = 0;

void addCORS() {
  server.sendHeader("Access-Control-Allow-Origin",  "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendIR(uint32_t code) { irsend.sendNEC(code, 32); delay(100); }

void readMmWave() {
  if (mmWaveSerial.available() >= 4) {
    uint8_t buf[32];
    int len = mmWaveSerial.readBytes(buf, min((int)mmWaveSerial.available(), 32));
    for (int i = 0; i < len - 9; i++) {
      if (buf[i] == 0xAA && buf[i+1] == 0xFF && i + 9 < len) {
        roomOccupied = (buf[i+4] != 0x00);
        motionEnergy = buf[i+5];
        staticEnergy = buf[i+6];
        break;
      }
    }
  }
}

void smartCoolRoom() {
  if (!acOn || roomTemp <= 0) return;
  float diff = roomTemp - targetTemp;
  if (diff > 1.5) {
    sendIR(AC_TEMP_DOWN); if (acTemp > 16) acTemp--;
    Serial.println("❄️ TEMP DOWN → " + String(acTemp));
  } else if (diff < -0.5) {
    sendIR(AC_TEMP_UP); if (acTemp < 32) acTemp++;
    Serial.println("🔥 TEMP UP → " + String(acTemp));
  }
}

void checkActivityLevel() {
  if (!acOn) return;
  int cur  = motionEnergy + staticEnergy;
  int diff = cur - lastEnergyLevel;
  if (diff > 40 && acTemp > 16) {
    sendIR(AC_TEMP_DOWN); acTemp--;
    Serial.println("👥 Activity spike → AC -1°C");
  }
  lastEnergyLevel = cur;
}

void handleOptions() { addCORS(); server.send(200, "text/plain", ""); }

void handleStatus() {
  StaticJsonDocument<250> doc;
  doc["acOn"]         = acOn;
  doc["acTemp"]       = acTemp;
  doc["targetTemp"]   = targetTemp;
  doc["acMode"]       = acMode;
  doc["acSpeed"]      = acSpeed;
  doc["roomTemp"]     = roomTemp;
  doc["roomHumidity"] = roomHumidity;
  doc["roomOccupied"] = roomOccupied;
  doc["motion"]       = motionDetected;
  doc["motionEnergy"] = motionEnergy;
  doc["staticEnergy"] = staticEnergy;
  doc["acTimer"]      = acTimerMins;
  String out; serializeJson(doc, out);
  addCORS(); server.send(200, "application/json", out);
}

void handleAcOn() {
  acOn = true; sendIR(AC_ON_CODE);
  Serial.println("❄️ AC ON");
  addCORS(); server.send(200, "application/json", "{\"status\":\"ac_on\"}");
}

void handleAcOff() {
  acOn = false; sendIR(AC_OFF_CODE);
  Serial.println("❄️ AC OFF");
  addCORS(); server.send(200, "application/json", "{\"status\":\"ac_off\"}");
}

void handleTemp() {
  if (server.hasArg("value")) {
    int newTemp = constrain(server.arg("value").toInt(), 16, 32);
    if (newTemp > acTemp) for (int i = acTemp; i < newTemp; i++) { sendIR(AC_TEMP_UP);   delay(200); }
    else                  for (int i = acTemp; i > newTemp; i--) { sendIR(AC_TEMP_DOWN); delay(200); }
    acTemp = newTemp;
  }
  addCORS(); server.send(200, "application/json", "{\"status\":\"temp_set\"}");
}

void handleTargetTemp() {
  if (server.hasArg("value")) targetTemp = constrain(server.arg("value").toInt(), 16, 30);
  Serial.println("🎯 Target → " + String(targetTemp));
  addCORS(); server.send(200, "application/json", "{\"status\":\"target_set\"}");
}

void handleMode() {
  if (server.hasArg("value")) {
    acMode = server.arg("value");
    if (acMode == "Cool") sendIR(AC_MODE_COOL);
    else if (acMode == "Heat") sendIR(AC_MODE_HEAT);
    else if (acMode == "Fan")  sendIR(AC_MODE_FAN);
  }
  addCORS(); server.send(200, "application/json", "{\"status\":\"mode_set\"}");
}

void handleSpeed() {
  if (server.hasArg("value")) {
    acSpeed = server.arg("value");
    if (acSpeed == "Low")  sendIR(AC_FAN_LOW);
    else if (acSpeed == "Med")  sendIR(AC_FAN_MED);
    else if (acSpeed == "High") sendIR(AC_FAN_HIGH);
  }
  addCORS(); server.send(200, "application/json", "{\"status\":\"speed_set\"}");
}

void handleSettings() {
  String body = server.arg("plain");
  if (body.length() > 0) {
    StaticJsonDocument<64> doc;
    if (!deserializeJson(doc, body))
      if (doc.containsKey("acTimer")) acTimerMins = doc["acTimer"].as<int>();
  }
  addCORS(); server.send(200, "application/json", "{\"status\":\"settings_saved\"}");
}

void setup() {
  Serial.begin(115200);
  mmWaveSerial.begin(256000, SERIAL_8N1, MM_RX, MM_TX);
  pinMode(PIR_PIN, INPUT);
  irsend.begin();
  dht.begin();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ IP: " + WiFi.localIP().toString());

  server.on("/status",     HTTP_GET,     handleStatus);
  server.on("/ac/on",      HTTP_POST,    handleAcOn);
  server.on("/ac/off",     HTTP_POST,    handleAcOff);
  server.on("/ac/temp",    HTTP_POST,    handleTemp);
  server.on("/ac/target",  HTTP_POST,    handleTargetTemp);
  server.on("/ac/mode",    HTTP_POST,    handleMode);
  server.on("/ac/speed",   HTTP_POST,    handleSpeed);
  server.on("/settings",   HTTP_POST,    handleSettings);
  server.on("/status",     HTTP_OPTIONS, handleOptions);
  server.on("/ac/on",      HTTP_OPTIONS, handleOptions);
  server.on("/ac/off",     HTTP_OPTIONS, handleOptions);
  server.on("/ac/temp",    HTTP_OPTIONS, handleOptions);
  server.on("/ac/target",  HTTP_OPTIONS, handleOptions);
  server.on("/ac/mode",    HTTP_OPTIONS, handleOptions);
  server.on("/ac/speed",   HTTP_OPTIONS, handleOptions);
  server.on("/settings",   HTTP_OPTIONS, handleOptions);

  server.begin();
  Serial.println("🚀 AC server ready!");
}

void loop() {
  server.handleClient();
  readMmWave();

  bool pirNow = digitalRead(PIR_PIN);
  if (pirNow && !motionDetected) Serial.println("⚡ PIR motion!");
  motionDetected = pirNow;

  if (roomOccupied != prevOccupied) {
    Serial.println(roomOccupied ? "👤 Person detected" : "🚶 Room empty");
    prevOccupied = roomOccupied;
  }

  unsigned long now = millis();

  // DHT22 every 5 seconds
  if (now - lastDHTRead >= 5000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      roomTemp = t; roomHumidity = h;
      Serial.println("🌡 " + String(roomTemp,1) + "°C 💧" + String(roomHumidity,0) + "%");
    }
    lastDHTRead = now;
  }

  // Smart cooling every 60s
  if (acOn && now - lastSmartCool >= 60000) {
    smartCoolRoom(); lastSmartCool = now;
  }

  // Activity check every 30s
  if (acOn && now - lastActivityCheck >= 30000) {
    checkActivityLevel(); lastActivityCheck = now;
  }

  // Empty room timer — mmWave only
  if (acOn) {
    if (!roomOccupied) {
      if (!roomWasEmpty) { roomWasEmpty = true; roomEmptyTime = now; Serial.println("🚶 Timer started"); }
      else {
        unsigned long ms = (unsigned long)acTimerMins * 60UL * 1000UL;
        if ((now - roomEmptyTime) >= ms) {
          acOn = false; sendIR(AC_OFF_CODE); roomWasEmpty = false;
          Serial.println("❄️ AC auto-off");
        }
      }
    } else {
      if (roomWasEmpty) Serial.println("👤 Timer cancelled");
      roomWasEmpty = false;
    }
  }

  delay(100);
}

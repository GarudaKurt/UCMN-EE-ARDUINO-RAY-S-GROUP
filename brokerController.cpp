#include "brokerController.h"
#include <ctype.h>
#include <math.h>

namespace {
constexpr uint16_t MQTT_BUFFER_SIZE = 1536;
constexpr size_t JSON_CAPACITY = 1536;

const int8_t SSR_PINS[3] = {SSR_PIN_MASTER, SSR_PIN_SALA, SSR_PIN_LIVING};
const char* const SSR_NAMES[3] = {"master", "sala", "living"};

void addDecimal(JsonObject obj, const char* key, float value) {
  if (!isfinite(value)) {
    obj[key] = nullptr;
    return;
  }
  // Keep six-decimal precision as a JSON number. Do not hand ArduinoJson a
  // pointer to a temporary character buffer that expires before serialization.
  obj[key] = round(static_cast<double>(value) * 1000000.0) / 1000000.0;
}

bool validPin(int pin) { return pin >= 0 && pin <= 39; }
}

Controller* Controller::activeController = nullptr;

Controller::Controller(const char* ssid, const char* password)
    : ssid(ssid), password(password), client(espClient) {
  activeController = this;
}

void Controller::begin() {
  for (uint8_t i = 0; i < 3; ++i) {
    if (!validPin(SSR_PINS[i])) continue;
    const uint8_t onLevel = SSR_ACTIVE_HIGH ? HIGH : LOW;
    digitalWrite(SSR_PINS[i], onLevel);  // Set latch before enabling output.
    pinMode(SSR_PINS[i], OUTPUT);
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " connected" : " FAILED (will keep retrying)");

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setBufferSize(MQTT_BUFFER_SIZE);
  client.setCallback(mqttCallback);
  logStatus();
}

bool Controller::ensureMqtt() {
  if (client.connected()) return true;
  if (WiFi.status() != WL_CONNECTED) return false;

  const unsigned long now = millis();
  if (now - lastReconnectAttempt < 5000) return false;
  lastReconnectAttempt = now;

  String clientId = "ESP32Client-" + String(static_cast<uint32_t>(esp_random()), HEX);
  Serial.print("Attempting MQTT connection...");
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    // The retained Node-RED command is delivered right after this subscription.
    if (!client.subscribe(SSR_CONTROL_TOPIC, 1)) {
      Serial.println("SSR subscribe failed, retrying");
      client.disconnect();
      return false;
    }
    return true;
  }
  Serial.print("failed, rc=");
  Serial.println(client.state());
  return false;
}

void Controller::loop() {
  ensureMqtt();
  client.loop();
  if (millis() - lastStatusLog >= 5000) {
    lastStatusLog = millis();
    logStatus();
  }
}

void Controller::mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (activeController) activeController->handleMqttMessage(topic, payload, length);
}

void Controller::handleMqttMessage(const char* topic, const byte* payload, unsigned int length) {
  if (!topic || !payload || length == 0) return;
  if (strcmp(topic, SSR_CONTROL_TOPIC) == 0) handleSsrCommand(payload, length);
}

// Expects {"sala":true,"livingroom":false,"master":true}; missing keys are left unchanged.
void Controller::handleSsrCommand(const byte* payload, unsigned int length) {
  StaticJsonDocument<192> doc;
  const DeserializationError err =
      deserializeJson(doc, reinterpret_cast<const char*>(payload), length);
  if (err) {
    Serial.printf("[SSR] invalid JSON: %s\n", err.c_str());
    return;
  }

  bool changed = false;
  auto apply = [&](JsonVariantConst v, SsrRoom room) {
    if (!v.is<bool>()) return;
    const bool want = v.as<bool>();
    if (want != ssrOn(room)) {
      setSsr(room, want);
      changed = true;
    }
  };

  apply(doc["master"], SsrRoom::master);
  apply(doc["sala"], SsrRoom::sala);
  JsonVariantConst living = doc["living"];
  if (living.isNull()) living = doc["livingroom"];   // accept either key name
  apply(living, SsrRoom::living);

  if (changed) logStatus();
}

void Controller::setSsr(SsrRoom room, bool enabled) {
  const uint8_t index = static_cast<uint8_t>(room);
  if (index >= 3) return;
  ssrState[index] = enabled;
  if (validPin(SSR_PINS[index])) {
    const bool pinHigh = SSR_ACTIVE_HIGH ? enabled : !enabled;
    digitalWrite(SSR_PINS[index], pinHigh ? HIGH : LOW);
  }
}

void Controller::logStatus() {
  Serial.printf("[STATUS] SSR master=%s(GPIO%d=%s) sala=%s(GPIO%d=%s) living=%s(GPIO%d=%s)\n",
      ssrState[0] ? "ON" : "OFF", SSR_PINS[0],
      validPin(SSR_PINS[0]) ? (digitalRead(SSR_PINS[0]) == (SSR_ACTIVE_HIGH ? HIGH : LOW) ? "ON" : "OFF") : "N/A",
      ssrState[1] ? "ON" : "OFF", SSR_PINS[1],
      validPin(SSR_PINS[1]) ? (digitalRead(SSR_PINS[1]) == (SSR_ACTIVE_HIGH ? HIGH : LOW) ? "ON" : "OFF") : "N/A",
      ssrState[2] ? "ON" : "OFF", SSR_PINS[2],
      validPin(SSR_PINS[2]) ? (digitalRead(SSR_PINS[2]) == (SSR_ACTIVE_HIGH ? HIGH : LOW) ? "ON" : "OFF") : "N/A");
}

void Controller::addReadings(JsonObject obj, const EnergyData& readings, SsrRoom room) {
  const bool ok = readings.error == ErrorType::none;
  obj["ok"] = ok;
  if (ok) {
    addDecimal(obj, "voltage", readings.voltage);
    addDecimal(obj, "current", readings.current);
    addDecimal(obj, "power", readings.power);
    addDecimal(obj, "energy", readings.energy);
    addDecimal(obj, "frequency", readings.frequency);
    addDecimal(obj, "power_factor", readings.powerFactor);
  }
  obj["ssr_on"] = ssrOn(room);
}

bool Controller::publishEnergy(const EnergyData& sala, const EnergyData& living,
                               const EnergyData& master) {
  if (!client.connected()) return false;

  StaticJsonDocument<JSON_CAPACITY> doc;
  doc["device_id"] = DEVICE_ID;
  doc["uptime_s"] = static_cast<unsigned long>(millis() / 1000);
  addReadings(doc["master"].to<JsonObject>(), master, SsrRoom::master);
  addReadings(doc["sala"].to<JsonObject>(), sala, SsrRoom::sala);
  addReadings(doc["living"].to<JsonObject>(), living, SsrRoom::living);

  if (doc.overflowed()) {
    Serial.println("MQTT JSON document overflow");
    return false;
  }

  char payload[MQTT_BUFFER_SIZE];
  const size_t n = serializeJson(doc, payload, sizeof(payload));
  if (n == 0 || n >= sizeof(payload)) {
    Serial.println("MQTT payload too large / serialize failed");
    return false;
  }
  Serial.print("Publishing: ");
  Serial.println(payload);
  return client.publish(MQTT_TOPIC, payload);
}

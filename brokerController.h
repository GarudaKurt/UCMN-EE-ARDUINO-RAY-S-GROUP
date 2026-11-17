#ifndef BROKER_CONTROLLER_H
#define BROKER_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "monitoring.h"

// Replace these placeholders with the actual GPIO numbers before wiring.
#ifndef SSR_PIN_MASTER
#define SSR_PIN_MASTER 27 // ORANGE WIRE NEXT PIN 14
#endif
#ifndef SSR_PIN_SALA
#define SSR_PIN_SALA 32 // BLUE WIRE BEFORE PIN 35
#endif
#ifndef SSR_PIN_LIVING
#define SSR_PIN_LIVING 33 //GREEN WIRE AFTER BLUE WIRE FROM PIN 32
#endif

// Set to 0 for an active-LOW SSR module.
#ifndef SSR_ACTIVE_HIGH
#define SSR_ACTIVE_HIGH 1
#endif

enum class SsrRoom : uint8_t { master = 0, sala = 1, living = 2 };

class Controller {
 public:
  Controller(const char* ssid, const char* password);
  void begin();
  void loop();
  bool connected() { return client.connected(); }
  bool wifiConnected() const { return WiFi.status() == WL_CONNECTED; }
  bool ssrOn(SsrRoom room) const { return ssrState[static_cast<uint8_t>(room)]; }
  bool publishEnergy(const EnergyData& sala, const EnergyData& living,
                     const EnergyData& master);

 private:
  static void mqttCallback(char* topic, byte* payload, unsigned int length);
  void handleMqttMessage(const char* topic, const byte* payload, unsigned int length);
  void handleSsrCommand(const byte* payload, unsigned int length);   // NEW
  void setSsr(SsrRoom room, bool enabled);
  bool ensureMqtt();
  void addReadings(JsonObject obj, const EnergyData& readings, SsrRoom room);
  void logStatus();

  const char* ssid;
  const char* password;
  const char* MQTT_SERVER = "broker.hivemq.com";
  const uint16_t MQTT_PORT = 1883;
  const char* MQTT_TOPIC = "iotmonitoring/energy";
  const char* DEVICE_ID = "esp32-monitoring";
  const char* SSR_CONTROL_TOPIC = "iotmonitoring/ssr/set";           // NEW (replaces CONTROL_TOPICS)

  unsigned long lastReconnectAttempt = 0;
  unsigned long lastStatusLog = 0;
  bool ssrState[3] = {true, true, true};
  static Controller* activeController;

  // The transport must be constructed before PubSubClient uses it.
  WiFiClient espClient;
  PubSubClient client;
};

#endif

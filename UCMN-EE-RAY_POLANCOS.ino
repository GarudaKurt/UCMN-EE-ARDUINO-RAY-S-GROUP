#include <Arduino.h>
#include "config.h"
#include "monitoring.h"
#include "brokerController.h"
#include "display.h"

Monitoring monitoring;
Controller controller(WIFI_SSID, WIFI_PASS);
Display display;
unsigned long lastPublish = 0;
unsigned long lastDisplay = 0;

void setup() {
  Serial.begin(115200);
  display.begin();
  controller.begin();
}

void loop() {
  controller.loop();
  const unsigned long now = millis();
  monitoring.update(now);

  if (now - lastDisplay >= DISPLAY_INTERVAL_MS) {
    lastDisplay = now;
    display.render(monitoring.data(), WiFi.status() == WL_CONNECTED,
                   controller.connected(), controller.ssrOn(SsrRoom::sala),
                   controller.ssrOn(SsrRoom::living), controller.ssrOn(SsrRoom::master));
  }

  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;
    controller.publishEnergy(monitoring.salaArea(), monitoring.livingRoom(),
                             monitoring.masterRoom());
  }
}

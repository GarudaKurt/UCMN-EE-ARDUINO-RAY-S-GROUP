#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include "config.h"
#include "monitoring.h"

class Display {
 public:
  Display();
  void begin();
  void render(const EnergyData data[SENSOR_COUNT], bool wifiOk, bool mqttOk,
              bool salaSsrOn, bool livingSsrOn, bool masterSsrOn);

 private:
  void drawValue(float value, uint8_t decimals, int x, int y, const char* suffix);
  void drawEnergy(float kilowattHours, int x, int y);
#if OLED_IS_SSD1309
  U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI _oled;
#else
  U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI _oled;
#endif
};

#endif

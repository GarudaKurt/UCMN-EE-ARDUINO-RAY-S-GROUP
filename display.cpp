#include "display.h"
#include "config.h"
#include <SPI.h>

Display::Display() : _oled(U8G2_R0, OLED_CS, OLED_DC, OLED_RES) {}

void Display::begin() {
  SPI.begin(OLED_CLK, -1, OLED_MOSI, OLED_CS);
  _oled.begin();
  _oled.setContrast(180);
}

void Display::drawValue(float value, uint8_t decimals, int x, int y, const char* suffix) {
  char text[16];
  if (isnan(value)) snprintf(text, sizeof(text), "--");
  else snprintf(text, sizeof(text), "%.*f%s", decimals, value, suffix);
  _oled.drawStr(x, y, text);
}

void Display::drawEnergy(float kilowattHours, int x, int y) {
  char text[20];
  if (!isfinite(kilowattHours)) {
    snprintf(text, sizeof(text), "--");
  } else if (kilowattHours == 0.0f) {
    snprintf(text, sizeof(text), "0Wh");
  } else if (kilowattHours < 0.00001f) {
    snprintf(text, sizeof(text), "%.1fmWh", kilowattHours * 1000000.0f);
  } else if (kilowattHours < 1.0f) {
    const float wattHours = kilowattHours * 1000.0f;
    if (wattHours < 1.0f) snprintf(text, sizeof(text), "%.2fWh", wattHours);
    else if (wattHours < 10.0f) snprintf(text, sizeof(text), "%.1fWh", wattHours);
    else snprintf(text, sizeof(text), "%.0fWh", wattHours);
  } else if (kilowattHours < 100.0f) {
    snprintf(text, sizeof(text), "%.3fkWh", kilowattHours);
  } else if (kilowattHours < 10000.0f) {
    snprintf(text, sizeof(text), "%.1fkWh", kilowattHours);
  } else {
    snprintf(text, sizeof(text), "%.0fkWh", kilowattHours);
  }
  _oled.drawStr(x, y, text);
}

void Display::render(const EnergyData data[SENSOR_COUNT], bool wifiOk, bool mqttOk,
                     bool salaSsrOn, bool livingSsrOn, bool masterSsrOn) {
  (void)wifiOk;
  (void)mqttOk;
  static const char* names[SENSOR_COUNT] = {"Sala", "Living", "Master"};
  const bool relayOn[SENSOR_COUNT] = {salaSsrOn, livingSsrOn, masterSsrOn};
  static unsigned long lastPageMs = 0;
  static uint8_t page = 0;
  const unsigned long now = millis();
  if (now - lastPageMs >= 4000) {
    lastPageMs = now;
    page = (page + 1) % 2;
  }

  _oled.clearBuffer();
  _oled.setFont(u8g2_font_5x8_tf);
  _oled.drawStr(0, 7, "ROOM");
  if (page == 0) {
    _oled.drawStr(38, 7, "V");
    _oled.drawStr(61, 7, "A");
    _oled.drawStr(82, 7, "W");
    _oled.drawStr(105, 7, "Hz");
  } else {
    _oled.drawStr(40, 7, "Energy");
    _oled.drawStr(100, 7, "PF");
  }
  _oled.drawHLine(0, 9, 128);

  for (size_t i = 0; i < SENSOR_COUNT; i++) {
    const int y = 20 + static_cast<int>(i) * 14;
    _oled.setFont(u8g2_font_4x6_tf);
    _oled.drawStr(0, y, names[i]);
    _oled.drawStr(27, y, relayOn[i] ? "+" : "-");
    _oled.setFont(u8g2_font_5x8_tf);
    if (data[i].error != ErrorType::none) {
      _oled.drawStr(40, y, "UNAVAILABLE");
    } else if (page == 1) {
      drawEnergy(data[i].energy, 40, y);
      drawValue(data[i].powerFactor, 2, 100, y, "");
    } else {
      drawValue(data[i].voltage, 0, 40, y, "");
      drawValue(data[i].current, 2, 60, y, "");
      drawValue(data[i].power, 0, 83, y, "");
      drawValue(data[i].frequency, 1, 106, y, "");
    }
    if (i < SENSOR_COUNT - 1) _oled.drawHLine(0, y + 4, 128);
  }
  _oled.drawStr(111, 63, page == 0 ? "1/2" : "2/2");
  _oled.sendBuffer();
}

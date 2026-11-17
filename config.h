#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define WIFI_SSID "KurtTyler"
#define WIFI_PASS "Pandemaca0"

// Each meter has its own RX/TX pair: ESP32 RX <- PZEM TX, ESP32 TX -> PZEM RX.
// PZEM 1 (Master) uses UART2. PZEM 2 (Living) and 3 (Sala) use
// separate wires selected on UART1 before each reading.
constexpr int PZEM_RX_PIN_MASTER = 17;
constexpr int PZEM_TX_PIN_MASTER = 16;
constexpr int PZEM_RX_PIN_LIVING = 13;
constexpr int PZEM_TX_PIN_LIVING = 14;
constexpr int PZEM_RX_PIN_SALA = 4;
constexpr int PZEM_TX_PIN_SALA = 15;

constexpr unsigned long READ_INTERVAL_MS = 2000;
constexpr unsigned long PUBLISH_INTERVAL_MS = 10000;
constexpr unsigned long DISPLAY_INTERVAL_MS = 250;

// 2.42-inch 128x64 OLED, 4-wire SPI on ESP32 VSPI.
// Set to 0 only if the module's controller is SSD1306 instead of SSD1309.
#define OLED_IS_SSD1309 1
#define OLED_CLK  18
#define OLED_MOSI 23
#define OLED_CS   5
#define OLED_DC   26
#define OLED_RES  25

#endif

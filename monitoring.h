#ifndef MONITORING_H
#define MONITORING_H

#include <Arduino.h>
#include <PZEM004Tv30.h>

enum class ErrorType { none, unavailable };
enum class SensorArea { sala, livingroom, masterbedroom };

struct EnergyData {
  float voltage = NAN;
  float current = NAN;
  float power = NAN;
  float energy = NAN;
  float frequency = NAN;
  float powerFactor = NAN;
  ErrorType error = ErrorType::unavailable;
  SensorArea area = SensorArea::sala;
};

constexpr size_t SENSOR_COUNT = 3;

class Monitoring {
 public:
  Monitoring();
  void update(unsigned long now);
  const EnergyData& salaArea() const { return _data[0]; }
  const EnergyData& livingRoom() const { return _data[1]; }
  const EnergyData& masterRoom() const { return _data[2]; }
  const EnergyData* data() const { return _data; }

 private:
  void readSensor(uint8_t index);
  PZEM004Tv30 _sensorSala;
  PZEM004Tv30 _sensorLiving;
  PZEM004Tv30 _sensorMaster;
  EnergyData _data[SENSOR_COUNT];
  uint8_t _nextSensor = 0;
  uint8_t _activeUart1Sensor = 1; // Living constructor configures UART1 last.
  bool _uart1Settling = false;
  unsigned long _uart1SwitchAt = 0;
  unsigned long _lastRead = 0;
};

#endif

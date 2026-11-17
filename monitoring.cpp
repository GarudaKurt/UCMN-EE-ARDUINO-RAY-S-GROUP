#include "monitoring.h"
#include "config.h"

Monitoring::Monitoring()
    : _sensorSala(Serial1, PZEM_RX_PIN_SALA, PZEM_TX_PIN_SALA),
      _sensorLiving(Serial1, PZEM_RX_PIN_LIVING, PZEM_TX_PIN_LIVING),
      _sensorMaster(Serial2, PZEM_RX_PIN_MASTER, PZEM_TX_PIN_MASTER) {
  _data[0].area = SensorArea::sala;
  _data[1].area = SensorArea::livingroom;
  _data[2].area = SensorArea::masterbedroom;
}

void Monitoring::update(unsigned long now) {
  if (now - _lastRead < READ_INTERVAL_MS / SENSOR_COUNT) return;

  // PZEM 2 and 3 have distinct wires but share the UART1 peripheral.
  // Let the selected pins settle without holding up MQTT or OLED updates.
  if (_nextSensor != 2 && _nextSensor != _activeUart1Sensor) {
    Serial1.flush();
    Serial1.end();
    Serial1.begin(9600, SERIAL_8N1,
                  _nextSensor == 0 ? PZEM_RX_PIN_SALA : PZEM_RX_PIN_LIVING,
                  _nextSensor == 0 ? PZEM_TX_PIN_SALA : PZEM_TX_PIN_LIVING);
    _activeUart1Sensor = _nextSensor;
    _uart1SwitchAt = now;
    _uart1Settling = true;
    return;
  }
  if (_uart1Settling && now - _uart1SwitchAt < 50) return;
  _uart1Settling = false;

  _lastRead = now;
  readSensor(_nextSensor);
  _nextSensor = (_nextSensor + 1) % SENSOR_COUNT;
}

void Monitoring::readSensor(uint8_t index) {
  PZEM004Tv30* sensor = nullptr;
  switch (index) {
    case 0: sensor = &_sensorSala; break;
    case 1: sensor = &_sensorLiving; break;
    case 2: sensor = &_sensorMaster; break;
    default: return;
  }

  EnergyData next = _data[index];
  next.voltage = sensor->voltage();
  next.current = sensor->current();
  next.power = sensor->power();
  next.energy = sensor->energy();
  next.frequency = sensor->frequency();
  next.powerFactor = sensor->pf();
  next.error = (isnan(next.voltage) || isnan(next.current) || isnan(next.power) ||
                isnan(next.energy) || isnan(next.frequency) || isnan(next.powerFactor))
                   ? ErrorType::unavailable : ErrorType::none;
  if (next.error != ErrorType::none) {
    next.voltage = next.current = next.power = next.energy = NAN;
    next.frequency = next.powerFactor = NAN;
  }
  _data[index] = next;
}

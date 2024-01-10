#include <Wire.h>

#include "Adafruit_DPS310.h"

Adafruit_DPS310 dps;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  dps.begin_I2C();

  dps.configurePressure(DPS310_64HZ, DPS310_64SAMPLES);
  dps.configureTemperature(DPS310_64HZ, DPS310_64SAMPLES);
}

void loop() {
  sensors_event_t pressure_event;
  sensors_event_t temperature_event;
  dps.getEvents(&temperature_event, &pressure_event);  

  Serial.print(F("DPS310 sensor data - Pressure: '"));
  Serial.print(pressure_event.pressure);
  Serial.print(F("' Temperature: '"));
  Serial.print(temperature_event.temperature);
  Serial.println(F(""));
}

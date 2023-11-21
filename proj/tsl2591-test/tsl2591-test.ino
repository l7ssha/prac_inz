#include <Wire.h>

#include "Adafruit_TSL2591.h"

Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);

void setup() {
  Wire.begin();
  Serial.begin(115200);

  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
}

void loop() {
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir = lum >> 16;
  uint16_t full = lum & 0xFFFF;

  Serial.print(F("TSL2591 sensor data - Calculated Lux: '"));
  Serial.print(tsl.calculateLux(full, ir));
  Serial.print(F("' Full spectrum light: '"));
  Serial.print(full);
  Serial.print(F("' Infrared light: '"));
  Serial.print(ir);
  Serial.print(F("' Visible light: '"));
  Serial.print(full - ir);
  Serial.println(F("'"));
}

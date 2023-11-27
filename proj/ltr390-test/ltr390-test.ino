#include <Wire.h>

#include "Adafruit_LTR390.h"

Adafruit_LTR390 ltr = Adafruit_LTR390();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  ltr.begin();

  ltr.setMode(LTR390_MODE_UVS);
  ltr.setGain(LTR390_GAIN_3);
  ltr.setResolution(LTR390_RESOLUTION_16BIT);
}

void loop() {
  Serial.print(F("LTR390 sensor data - UVS: '"));
  Serial.print(ltr.readUVS());
  Serial.println(F("'"));
}

#include "DHT.h"

#define DHTPIN 23
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  dht.begin();
}

void loop() {
  auto humidity = dht.readHumidity();
  auto temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  

  Serial.print(F("DHT22 sensor data - Temp: '"));
  Serial.print(temperature);
  Serial.print(F("' Humidity: '"));
  Serial.print(humidity);
  Serial.print(F("' Heat index: '"));
  Serial.print(dht.computeHeatIndex(temperature, humidity, false));
  Serial.println(F("'"));
}

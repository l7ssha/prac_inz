#include <DHT.h>

#define DHTPIN 23
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  
  dht.begin();
  Serial.print(F("Works"));
}

void loop() {
  auto humidity = dht.readHumidity();
  auto temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  auto heat_index = dht.computeHeatIndex(temperature, humidity, false);
  
  Serial.print(F("DHT22 sensor data - Temp: '"));
  Serial.print(temperature);
  Serial.print(F("' Humidity: '"));
  Serial.print(humidity);
  Serial.print(F("' Heat index: '"));
  Serial.print(heat_index);
  Serial.println(F("'"));
}

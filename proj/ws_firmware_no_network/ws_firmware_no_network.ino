#include <Wire.h>

#include "DHT.h"
#include "Adafruit_TSL2591.h"
#include "Adafruit_DPS310.h"

#define DHTPIN 23
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
Adafruit_DPS310 dps;

struct CurrentReadingData {
  float temperature;
  float humidity;
  float heat_index;
  uint16_t visible_light;
  uint16_t full_spectrum_light;
  uint16_t infrared_light;
  float calculated_lux;
  float pressure;
} current_reading_data;

void readTSL() {
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir = lum >> 16;
  uint16_t full = lum & 0xFFFF;

  current_reading_data.visible_light = full - ir;
  current_reading_data.full_spectrum_light = full;
  current_reading_data.infrared_light = ir;
  current_reading_data.calculated_lux = tsl.calculateLux(full, ir);
}

void readDHT() {
  auto humidity = dht.readHumidity();
  auto temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  current_reading_data.humidity = humidity;
  current_reading_data.temperature = temperature;
  current_reading_data.heat_index = dht.computeHeatIndex(temperature, humidity, false);
}

void readDPS()
{  
  while (!dps.temperatureAvailable() || !dps.pressureAvailable()) {
    return;
  }

  sensors_event_t pressure_event;
  dps.getEvents(NULL, &pressure_event);

  current_reading_data.pressure = pressure_event.pressure;
}

void send_debug_info_to_serial()
{
  Serial.print(F("DHT22 sensor data - Temp: '"));
  Serial.print(current_reading_data.temperature);
  Serial.print(F("' Humidity: '"));
  Serial.print(current_reading_data.humidity);
  Serial.print(F("' Heat index: '"));
  Serial.print(current_reading_data.heat_index);
  Serial.println(F("'"));

  Serial.print(F("TSL2591 sensor data - Calculated Lux: '"));
  Serial.print(current_reading_data.calculated_lux);
  Serial.print(F("' Full spectrum light: '"));
  Serial.print(current_reading_data.full_spectrum_light);
  Serial.print(F("' Infrared light: '"));
  Serial.print(current_reading_data.infrared_light);
  Serial.print(F("' Visible light: '"));
  Serial.print(current_reading_data.visible_light);
  Serial.println(F("'"));

  Serial.print(F("DPS310 sensor data - Pressure: '"));
  Serial.print(current_reading_data.pressure);
  Serial.println(F("hPa'"));

  Serial.println(F(""));
}

void setup()
{
  Wire.begin();
  Serial.begin(115200);

  dht.begin();
  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  dps.begin_I2C();
}

void loop()
{
  readDHT();
  readTSL();
  readDPS();

  send_debug_info_to_serial();
}

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <Wire.h>

#include "DHT.h"
#include "Adafruit_TSL2591.h"

#define DHTPIN 23
#define DHTTYPE DHT22

const char* ssid = "2137";
const char* password = "TwojaMama123";

WiFiServer server(80);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_TSL2591 tsl(2591);

struct CurrentReadingData {
  float temperature;
  float humidity;
  float heat_index;
  uint16_t visible_light;
  uint16_t full_spectrum_light;
  uint16_t infrared_light;
  float calculated_lux;
} current_reading_data;

void readTSL(void) {
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir = lum >> 16;
  uint16_t full = lum & 0xFFFF;

  current_reading_data.visible_light = full - ir;
  current_reading_data.full_spectrum_light = full;
  current_reading_data.infrared_light = ir;
  current_reading_data.calculated_lux = tsl.calculateLux(full, ir);

  Serial.print(F("Lux: "));
  Serial.println(current_reading_data.calculated_lux);
}

void readDHT(void) {
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

void setup(void) {
  Wire.begin();
  Serial.begin(115200);

  dht.begin();
  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);

  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // esp32-weather-station.local
  if (!MDNS.begin("esp32-weather-station")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  server.begin();
  Serial.println("TCP server started");

  MDNS.addService("http", "tcp", 80);
}

void loop(void) {
  readDHT();
  readTSL();

  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  Serial.println("");
  Serial.println("New client");

  while (client.connected() && !client.available()) {
    delay(1);
  }

  String req = client.readStringUntil('\r');

  // "GET /path HTTP/1.1"
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return;
  }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);

  String s;
  if (req == "/") {
    IPAddress ip = WiFi.localIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP32 at ";
    s += ipStr;
    s += "</html>\r\n\r\n";
    Serial.println("Sending 200");
  } else {
    s = "HTTP/1.1 404 Not Found\r\n\r\n";
    Serial.println("Sending 404");
  }
  client.print(s);

  client.stop();
  Serial.println("Done with client");
}

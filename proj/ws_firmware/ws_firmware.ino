#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#include <HTTPServer.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

#include <DHT.h>
#include <Adafruit_TSL2591.h>
#include <Adafruit_DPS310.h>
#include <Adafruit_LTR390.h>

#define DHTPIN 23
#define DHTTYPE DHT22

const int read_delay = 1000;

const char* ssid = "2137";
const char* password = "TwojaMama123";
const char* mdns_hostname = "esp32-weather-station";

const char html_template[] = R"###(
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>%s</title>
    <style>
        table, th, td {
            border: 1px solid black;
            border-collapse: collapse;
            padding: 3px;
        }
    </style>
</head>
<body>
<h1>%s</h1>

<table>
    <tr>
        <td>Temperatura powietrza</td>
        <td>%.2f °C</td>
        </tr>
        <tr>
        <td>Relatywna wilgotność powietrza</td>
        <td>%.2f &#37;</td>
        </tr>
        <tr>
        <td>Temepratura powietrza odczuwalna</td>
        <td>%.2f °C</td>
        </tr>
        <tr>
        <td>Ilość światła widzialnego</td>
        <td>%d</td>
        </tr>
        <tr>
        <td>Ilość światła w zakresie pełnego spektrum</td>
        <td>%d</td>
        </tr>
        <tr>
        <td>Ilość światła podczerwonego</td>
        <td>%d</td>
        </tr>
        <tr>
        <td>Obliczone oświetlenie otoczenia</td>
        <td>%.3f Lux</td>
        </tr>
        <tr>
        <td>Ciśnienie atmosferyczne</td>
        <td>%.2f hPa</td>
        </tr>
        <tr>
        <td>Ilość promieniowania ultrafioletowego</td>
        <td>%d</td>
    </tr>
</table>
<p>Ostatnia aktualizacja: %d ms temu (Uptime: %d s)</p>
</body>
</html>
)###";

httpsserver::HTTPServer http_server = httpsserver::HTTPServer();

DHT dht(DHTPIN, DHTTYPE);
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
Adafruit_DPS310 dps;
Adafruit_LTR390 ltr = Adafruit_LTR390();

int sensor_read_delay = 0;

struct CurrentReadingData {
  float temperature;
  float humidity;
  float heat_index;
  uint16_t visible_light;
  uint16_t full_spectrum_light;
  uint16_t infrared_light;
  float calculated_lux;
  float pressure;
  uint32_t uvs;
} current_reading_data;

void read_tsl() {
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir = lum >> 16;
  uint16_t full = lum & 0xFFFF;

  current_reading_data.visible_light = full - ir;
  current_reading_data.full_spectrum_light = full;
  current_reading_data.infrared_light = ir;
  current_reading_data.calculated_lux = tsl.calculateLux(full, ir);
}

void read_dht() {
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

void read_dps()
{  
  while (!dps.temperatureAvailable() || !dps.pressureAvailable()) {
    return;
  }

  sensors_event_t pressure_event;
  dps.getEvents(NULL, &pressure_event);

  current_reading_data.pressure = pressure_event.pressure;
}

void read_ltr()
{
  if (!ltr.newDataAvailable()) {
    return;
  }

  current_reading_data.uvs = ltr.readUVS();
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

  Serial.print(F("LTR390 sensor data - UVS: '"));
  Serial.print(current_reading_data.uvs);
  Serial.println(F("'"));

  Serial.println(F(""));
}

int calculate_uptime(void)
{
  return (int) (millis() / 1000);
}

void handle_api_request(httpsserver::HTTPRequest *req, httpsserver::HTTPResponse *res)
{
  StaticJsonBuffer<JSON_OBJECT_SIZE(10)> jsonBuffer;
  JsonObject& obj = jsonBuffer.createObject();

  obj["temperature"] = current_reading_data.temperature;
  obj["humidity"] = current_reading_data.humidity;
  obj["heat_index"] = current_reading_data.heat_index;
  obj["visible_light"] = current_reading_data.visible_light;
  obj["full_spectrum_light"] = current_reading_data.full_spectrum_light;
  obj["infrared_light"] = current_reading_data.infrared_light;
  obj["calculated_lux"] = current_reading_data.calculated_lux;
  obj["pressure"] = current_reading_data.pressure;
  obj["uvs"] = current_reading_data.uvs;
  obj["last_update_attempt"] = sensor_read_delay;
  obj["uptime"] = calculate_uptime();

  res->setHeader("Content-Type", "application/json");
  obj.printTo(*res);
}

void handle_root_request(httpsserver::HTTPRequest *req, httpsserver::HTTPResponse *res)
{
  res->setHeader("Content-Type", "text/html");
  
  char buffer[2000];
  sprintf(
    buffer, 
    html_template,
    mdns_hostname,
    mdns_hostname,
    current_reading_data.temperature,
    current_reading_data.humidity,
    current_reading_data.heat_index,
    current_reading_data.visible_light,
    current_reading_data.full_spectrum_light,
    current_reading_data.infrared_light,
    current_reading_data.calculated_lux,
    current_reading_data.pressure,
    current_reading_data.uvs,
    sensor_read_delay,
    calculate_uptime()
  );

  res->print(buffer);
}

void setup_network(void)
{
  Serial.println("");

  WiFi.begin(ssid, password);

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
  if (!MDNS.begin(mdns_hostname)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  httpsserver::ResourceNode *nodeRoot = new httpsserver::ResourceNode("/", "GET", &handle_root_request);
  httpsserver::ResourceNode *nodeApi = new httpsserver::ResourceNode("/api", "GET", &handle_api_request);
  http_server.registerNode(nodeRoot);
  http_server.registerNode(nodeApi);

  Serial.println("Starting server...");
  http_server.start();
  if (http_server.isRunning()) {
    Serial.println("Server ready.");
  }

  MDNS.addService("http", "tcp", 80);
}

void read_sensors(void)
{
  read_dht();
  read_tsl();
  read_dps();
  read_ltr();
}

void setup()
{
  Wire.begin();
  Serial.begin(115200);

  dht.begin();
  
  tsl.setGain(TSL2591_GAIN_MED);
  tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  
  dps.begin_I2C();
  
  ltr.begin();
  ltr.setMode(LTR390_MODE_UVS);
  ltr.setGain(LTR390_GAIN_3);
  ltr.setResolution(LTR390_RESOLUTION_16BIT);
  ltr.setThresholds(100, 1000);

  setup_network();

  // Read sensors initially
  read_sensors();
}

void loop(void)
{
  sensor_read_delay += 1;

  http_server.loop();

  if (sensor_read_delay >= read_delay) {
    read_sensors();

    send_debug_info_to_serial();

    sensor_read_delay = 0;
    return;
  }

  delay(1);
}

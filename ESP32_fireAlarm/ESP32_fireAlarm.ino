#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <PMserial.h>   // Library for PM sensors with serial interface
#include "DHT.h"        // Library for DHT sensor

// Network Configuration
const char* ssid = "BK Star";
const char* password = "bkstar2021";

// MQTT Configuration
#define MQTT_PORT 1884
#define MQTT_USER "test"
#define MQTT_PASSWORD "testadmin"
#define MQTT_SERVER_IP_ADDRESS "103.1.238.175"
#define MQTT_SERVER_PORT 1885U

// Sensor Configuration
#define DHTPIN 4       // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22  // DHT 22 (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);
SerialPM pmsSensor(PMSx003, Serial2); // PMSx003 sensor on UART
#define GAS_SENSOR_PIN 5

// Time Client Configuration
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "de.pool.ntp.org", 3600 * 7, 60000);

// Global Variables
WiFiClient espClient;
PubSubClient mqttClient(espClient);
char espID[32];
char topic[32];

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600); // For PM sensor
  
  // Initialize Sensors
  pmsSensor.init();
  dht.begin();
  // Initialize the gas sensor pin as an input
  pinMode(GAS_SENSOR_PIN, INPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  for (int retry = 0; WiFi.status() != WL_CONNECTED && retry < 50; ++retry) {
    delay(1000);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
  } else {
    Serial.println("WiFi initialization failed!");
  }

  // Prepare MQTT
  uint8_t espMacAddress[6];				// mang luu dia chi MAC
	WiFi.macAddress(espMacAddress);			// lay dia chi MAC cua WIFI
	sprintf(topic, "fireAlarm/");
	sprintf(espID, "%x%x%x%x%x%x", espMacAddress[0],  espMacAddress[1],  espMacAddress[2],  espMacAddress[3],  espMacAddress[4],  espMacAddress[5]);
  mqttClient.setServer(MQTT_SERVER_IP_ADDRESS, MQTT_SERVER_PORT);
  mqttClient.setKeepAlive(45);

  // Connect to MQTT
  if (mqttClient.connect(espID, MQTT_USER, MQTT_PASSWORD)) {
    mqttClient.subscribe(topic);
    Serial.println("MQTT initialized successfully!");
  } else {
    Serial.println("MQTT initialization failed!");
  }
}

void loop() {
  delay(2000); // Measurement interval

  // Read from sensors
  pmsSensor.read();
  float humidity = dht.readHumidity();
  float tempC = dht.readTemperature();
  float tempF = dht.readTemperature(true);
  int gasValue = digitalRead(GAS_SENSOR_PIN);
  if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
    Serial.println("Failed to read from DHT sensor!");
  }

  // Time update and JSON creation
  timeClient.update();
  StaticJsonDocument<512> doc;
  doc["Id"] = espID;
  doc["Time"] = timeClient.getEpochTime();
  doc["Temp"] = tempC;
  doc["Humi"] = humidity;
  doc["Gas"] = gasValue;
  doc["PM1"] = pmsSensor.pm01;
  doc["PM2p5"] = pmsSensor.pm25;
  doc["PM10"] = pmsSensor.pm10;

  // Serialize JSON and send via MQTT
  char output[512];
  serializeJson(doc, output);
  Serial.println(output);

  if (mqttClient.connected()) {
    if (mqttClient.publish(topic, output, true)) {
      mqttClient.loop();
      Serial.println("MQTT data sent successfully!");
    } else {
      Serial.println("MQTT data send failed!");
    }
  } else {
    Serial.println("MQTT not connected!");
    mqttClient.connect(espID, MQTT_USER, MQTT_PASSWORD);
  }
}

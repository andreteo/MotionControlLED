#include "FastLED.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <Ticker.h>

#define NUM_LEDS 60
#define DATA_PIN 26
#define PIR_MOTION_SENSOR 35

// SSID/Password combination
const char* ssid = "Teo";
const char* password = "S1414466H";
// const char* mqttUser = "pi";
// const char* mqttPassword = "12J71f94";

// MQTT Broker IP address
const char* mqtt_server = "192.168.86.24";
const int OnboardLED = 2; // LED pin

// Get time from server
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;
const int   daylightOffset_sec = 0;

// Define the array of leds
CRGB leds[NUM_LEDS];

// DHT humidity sensor + MQTT
DHTesp dht;
WiFiClient espClient;
PubSubClient client(espClient);

bool getTemperature();
bool gotNewTemperature = false;
bool lightFlag = false;
bool tasksEnabled = false;
char msg[50];
const long interval = 15000;
float moisture = 0;
float temperature = 0;
int value = 0;
int count = 0;
int colourValue = 0;
int dhtPin = 32;
long lastMsg = 0;
unsigned long previousMillis = 0;
void triggerGetTemp();
void tempTask(void *pvParameters);

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Ticker for temperature reading */
Ticker tempTicker;
/** Comfort profile */
ComfortState cf;

TempAndHumidity TempSensor;

void setup() {
  // Serial plotting
  Serial.begin(115200);
  delay(10);
  // Connect to Wifi
  SetupWifi();
  // Init temp sensor
  initTemp();
  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(OnboardLED, OUTPUT);
  tasksEnabled = true;
  // Init time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // Init LED strip
  FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  // Init PIR sensor
  pinMode(PIR_MOTION_SENSOR, INPUT);
}

void loop() {
  // PrintLocalTime();
  int currentHour = getHour();

  if (!client.connected()) {
    setAll(0x00, 0x00, 0x00);
    reconnect();
  }
  client.loop();

  if (gotNewTemperature) {
    // Serial.println("Sensor 1 data:");
    Serial.println("Temp: " + String(TempSensor.temperature, 2) + "'C Humidity: " + String(TempSensor.humidity, 1) + "%");
    char tempString[8];
    dtostrf(TempSensor.temperature, 5, 2, tempString);
    client.publish("esp32/temperature", tempString);

    char humString[8];
    dtostrf(TempSensor.humidity, 6, 2, humString);
    client.publish("esp32/humidity", humString);

    gotNewTemperature = false;
  }

  if ((currentHour <= 6) || (currentHour >= 21)){
    if(IsPeopleDetected()){
      char statusString[] = "true";
      // Serial.printf("Motion Detect: %s\n", statusString);
      client.publish("esp32/motion", statusString);
      lightFlag = true;
    }
    else{
      char statusString[] = "false";
      // Serial.printf("Motion Detect: %s\n", statusString);
      client.publish("esp32/motion", statusString);
      setAll(0x00, 0x00, 0x00);
    }
   }

    while (lightFlag == true && value != 3000) {
      if (colourValue == 0){
        setAll(0xff, 0xff, 0xff);
      }
      else if (colourValue == 1){
        Fire(55, 120, 20);
      }
      else if (colourValue == 2){
        setAll(0xff, 0x00, 0x00);
      }
      else if (colourValue == 3){
        setAll(0x00, 0xff, 0x00);
      }
      else if (colourValue == 4){
        setAll(0x00, 0x00, 0xff);
      }
      else if (colourValue == 5){
        setAll(0xff, 0x69, 0xb4);
      }
      else if (colourValue == 6){
        setAll(0x80, 0x00, 0x80);
      }
      else if (colourValue == 7){
        setAll(0x00, 0x80, 0x80);
      }
      value++;
      delay(10);
    }
  lightFlag = false;
  value = 0;
  delay(500);
}

boolean IsPeopleDetected() {
  int sensorValue = digitalRead(PIR_MOTION_SENSOR);
  if (sensorValue == HIGH) {
    return true;
  }
  else {
    return false;
  }
}

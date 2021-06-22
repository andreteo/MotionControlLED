#include "FastLED.h"
#include <WiFi.h>

#define NUM_LEDS 60
#define DATA_PIN 26
#define PIR_MOTION_SENSOR 35

// SSID/Password combination
const char* ssid = "Teo";
const char* password = "S1414466H";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 28800;
const int   daylightOffset_sec = 0;

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() {
      // Serial plotting
      Serial.begin(115200);
      delay(10);
      // Connect to Wifi
      SetupWifi(); 
      // Init time
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      // Init LED strip
  	  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
      // Init PIR sensor
      pinMode(PIR_MOTION_SENSOR, INPUT);
}

int value = 0;

void loop() {
  int timenow = PrintLocalTime();
  Serial.printf("%d\n", timenow);

  if ((timenow <= 5) || (timenow >= 23)){
    Serial.println("Time is between 11 pm - 5 am, Activating motion sensor");
    if(IsPeopleDetected()){
      TurnOnLED();
      delay(5000);
    }
    else{
      TurnOffLED();
    }
  }
  delay(1000);
}

void TurnOnLED(){
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Azure;
  }
  FastLED.show();
}

void TurnOffLED() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

boolean IsPeopleDetected() {
  int sensorValue = digitalRead(PIR_MOTION_SENSOR);
  if(sensorValue == HIGH){
    return true;
  }
  else{
    return false;
  }
}

void SetupWifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

int PrintLocalTime(){
  struct tm timeinfo;
  int timeHour[3];
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return 1;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  //timeHour = timeinfo.tm_hour;
  return timeinfo.tm_hour;
}

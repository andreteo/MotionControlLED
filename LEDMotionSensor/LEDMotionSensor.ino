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

void tempTask(void *pvParameters);
bool getTemperature();
void triggerGetTemp();
long lastMsg = 0;
char msg[50];
int value = 0;
int count = 0;
bool lightFlag = false;
int colourValue = 2; 
bool gotNewTemperature = false;
// bool currentMotionSensorStatus = false;
float moisture = 0;
float temperature = 0;

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Ticker for temperature reading */
Ticker tempTicker;
/** Comfort profile */
ComfortState cf;
/** Flag if task should run */
bool tasksEnabled = false;
/** Pin number for DHT22 data pin */
int dhtPin = 32;
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
      TurnOnLED();
}

void loop() {
  int timenow = PrintLocalTime();
 
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (gotNewTemperature) {
      Serial.println("Sensor 1 data:");
      Serial.println("Temp: " + String(TempSensor.temperature,2) + "'C Humidity: " + String(TempSensor.humidity,1) + "%");
      char tempString[8];
      dtostrf(TempSensor.temperature, 5, 2, tempString);
      client.publish("esp32/temperature", tempString);

      char humString[8];
      dtostrf(TempSensor.humidity, 6, 2, humString);
      client.publish("esp32/humidity", humString);

      gotNewTemperature = false;
    }
    
  if ((timenow <= 5) || (timenow >= 18)){
    if(IsPeopleDetected()){
      char statusString[] = "true";
      Serial.printf("Motion Detect: %s\n", statusString);
      client.publish("esp32/motion", statusString);
      lightFlag = true;
      // TurnOnLED();
      // delay(5000);
    }
    else{
      char statusString[] = "false";
      Serial.printf("Motion Detect: %s\n", statusString);
      client.publish("esp32/motion", statusString);
      TurnOffLED();
    }
  }

  while (lightFlag == true && value != 500) {
    Fire(55, 120, 20);  
    // meteorRain(0x42,0x16,0x8f, 6, 64, true, 30);  
    value++;
    delay(10);
  }
  lightFlag = false;
  value = 0;
  delay(100);
}

void showStrip(){
  #ifndef ADAFRUIT_NEOPIXEL_H
  // FastLED
  FastLED.show();
  #endif
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
 #ifndef ADAFRUIT_NEOPIXEL_H
   // FastLED
   leds[Pixel].r = red;
   leds[Pixel].g = green;
   leds[Pixel].b = blue;
 #endif
}

void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue);
  }
  showStrip();
}

void TurnOnLED(){
  // colourValue = 0: Default, 1: blue, 2: white
  if (colourValue == 0){
    for (int i = 0; i < NUM_LEDS; i++){
        leds[i] = CRGB::Azure;
        showStrip();
      }
  }
  else if (colourValue == 1){
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Blue;
        showStrip(); 
      }
  }
  else if (colourValue == 2){
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Red;
        showStrip(); 
      }
  }  
}

void TurnOffLED() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  showStrip();
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
  WiFi.mode(WIFI_STA);
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
  return timeinfo.tm_hour;
}

/**
 * initTemp
 * Setup DHT library
 * Setup task and timer for repeated measurement
 * @return bool
 *    true if task and timer are started
 *    false if task or timer couldn't be started
 */
bool initTemp() {
  byte resultValue = 0;
  // Initialize temperature sensor
  dht.setup(dhtPin, DHTesp::DHT22);
  Serial.println("DHT initiated");

  // Start task to get temperature
  xTaskCreatePinnedToCore(
      tempTask,                       /* Function to implement the task */
      "tempTask ",                    /* Name of the task */
      4000,                           /* Stack size in words */
      NULL,                           /* Task input parameter */
      5,                              /* Priority of the task */
      &tempTaskHandle,                /* Task handle. */
      1);                             /* Core where the task should run */

  if (tempTaskHandle == NULL) {
    Serial.println("Failed to start task for temperature update");
    return false;
  } else {
    // Start update of environment data every 20 seconds
    tempTicker.attach(10, triggerGetTemp);
  }

  // Signal end of setup() to tasks
  tasksEnabled = true;
}

/**
 * triggerGetTemp
 * Sets flag dhtUpdated to true for handling in loop()
 * called by Ticker getTempTimer
 */
void triggerGetTemp() {
  if (tempTaskHandle != NULL) {
     xTaskResumeFromISR(tempTaskHandle);
  }
}

/**
 * Task to reads temperature from DHT22 sensor
 * @param pvParameters
 *    pointer to task parameters
 */
void tempTask(void *pvParameters) {
  Serial.println("tempTask loop started");
  while (1) // tempTask loop
  {
    if (tasksEnabled && !gotNewTemperature) {
      // Get temperature values
      getTemperature();
      gotNewTemperature = true;
    }
    // Got sleep again
    vTaskSuspend(NULL);
  }
}

/**
 * getTemperature
 * Reads temperature from DHT11 sensor
 * @return bool
 *    true if temperature could be aquired
 *    false if aquisition failed
*/
bool getTemperature() {
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempSensor = dht.getTempAndHumidity();
  // Check if any reads failed and exit early (to try again).
  if (dht.getStatus() != 0) {
    Serial.println("DHT22 error status: " + String(dht.getStatusString()));
    return false;
  }

  float heatIndex = dht.computeHeatIndex(TempSensor.temperature, TempSensor.humidity);
  float dewPoint = dht.computeDewPoint(TempSensor.temperature, TempSensor.humidity);
  float cr = dht.getComfortRatio(cf, TempSensor.temperature, TempSensor.humidity);

  String comfortStatus;
  switch(cf) {
    case Comfort_OK:
      comfortStatus = "Comfort_OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "Comfort_TooHot";
      break;
    case Comfort_TooCold:
      comfortStatus = "Comfort_TooCold";
      break;
    case Comfort_TooDry:
      comfortStatus = "Comfort_TooDry";
      break;
    case Comfort_TooHumid:
      comfortStatus = "Comfort_TooHumid";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Comfort_HotAndHumid";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Comfort_HotAndDry";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Comfort_ColdAndHumid";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Comfort_ColdAndDry";
      break;
    default:
      comfortStatus = "Unknown:";
      break;
  };

  Serial.println(" T:" + String(TempSensor.temperature) + " H:" + String(TempSensor.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  
  
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(OnboardLED, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(OnboardLED, LOW);
    }
  }
  else if (String(topic) == "esp32/ledstrip") {
    if (messageTemp == "blue"){
      Serial.printf("Changing Colour of LED strip to %s\n", messageTemp);
      colourValue = 1;
    }
    else if (messageTemp == "pink"){
      Serial.printf("Changing Colour of LED strip to %s\n", messageTemp);
      colourValue = 2;
    }
    else {
      Serial.println("Switching to Default Operation");
      colourValue = 0;
    }
  }
    
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
      client.subscribe("esp32/ledstrip");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void Fire(int Cooling, int Sparking, int SpeedDelay) {
  static byte heat[NUM_LEDS];
  int cooldown;
 
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS; i++) {
    cooldown = random(0, ((Cooling * 10) / NUM_LEDS) + 2);
   
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
 
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
   
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < NUM_LEDS; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  showStrip();
  delay(SpeedDelay);
}

void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature/255.0)*191);
 
  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // figure out which third of the spectrum we're in:
  if( t192 > 0x80) {                     // hottest
    setPixel(Pixel, 255, 255, heatramp);
  } else if( t192 > 0x40 ) {             // middle
    setPixel(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    setPixel(Pixel, heatramp, 0, 0);
  }
}

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {  
  setAll(0,0,0);
 
  for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {
   
   
    // fade brightness all LEDs one step
    for(int j=0; j<NUM_LEDS; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(j, meteorTrailDecay );        
      }
    }
   
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
        setPixel(i-j, red, green, blue);
      }
    }
   
    showStrip();
    delay(SpeedDelay);
  }
}

void fadeToBlack(int ledNo, byte fadeValue) {
 #ifdef ADAFRUIT_NEOPIXEL_H
    // NeoPixel
    uint32_t oldColor;
    uint8_t r, g, b;
    int value;
   
    oldColor = strip.getPixelColor(ledNo);
    r = (oldColor & 0x00ff0000UL) >> 16;
    g = (oldColor & 0x0000ff00UL) >> 8;
    b = (oldColor & 0x000000ffUL);

    r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
    g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
    b=(b<=10)? 0 : (int) b-(b*fadeValue/256);
   
    strip.setPixelColor(ledNo, r,g,b);
 #endif
 #ifndef ADAFRUIT_NEOPIXEL_H
   // FastLED
   leds[ledNo].fadeToBlackBy( fadeValue );
 #endif  
}

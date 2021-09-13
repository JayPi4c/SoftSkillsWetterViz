// einbinden der geheimen Daten
#include "secrets.h"
#include "Definitions.h"

// einbinden weiterer Bibliotheken
#include <ArduinoJson.h>
#define FASTLED_INTERNAL  // define to stop FastLED Pragma, comment out to get \
                          // more information
#include <FastLED.h>

#include <ArduinoMqttClient.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>

#include <WiFiUdp.h>
#include <TimeLib.h>

#define DEBUG true
#define DEBUG_SERIAL \
  if (DEBUG) Serial

#define DEBUG_MQTT true
#define DEBUG_MQTT_SERIAL \
  if (DEBUG_MQTT) Serial
#define DEBUG true
#define DEBUG_SERIAL \
  if (DEBUG) Serial

#define DEBUG_MQTT true
#define DEBUG_MQTT_SERIAL \
  if (DEBUG_MQTT) Serial

// LED Stripe
CRGB leds[NUM_LEDS];

// WiFi setup
// https://forum.arduino.cc/t/simultaneous-mqtt-and-http-post/430361/8
WiFiClient wifiClient;
WiFiClient mqttWifiClient;
WiFiUDP Udp;
MqttClient mqttClient(mqttWifiClient);

bool lightsOn = true;
bool isActive = true;

bool topLightOn = false;

uint8_t brightness = 255;

bool dimBySun = false;
uint8_t dimTime = 15;  // time in which the light is dimmed (minutes)
uint8_t dimMin = 30;
uint8_t dimMax = 255;
unsigned long sunrise;
unsigned long sunset;

uint32_t DIMMING_INTERVAL = 30000;  // every 30 seconds
unsigned long lastcheckDimming = 0;

int prev_weatherID = 0;
int weatherID = 0;
int prev_temperature_Celsius = 15;
int temperature_Celsius = 15;

uint32_t INTERVAL = 900000;
unsigned long lastcheck = 0;

String CITY = "Oldenburg";
String countryCode = "DE";

uint8_t animationMode = 0;
CHSV animColor = CHSV(0, 255, 255);

int8_t paneIndex = -1;

// define functions to allow default parameter
void applyConditions(bool forceUpdate = false);

void updateBrightness(int b, bool applyOnActive = true);
void publishMQTT(String topic, String message, bool retain = true, uint8_t qos = 0, bool dup = false);


void setup() {
#if DEBUG == true || DEBUG_MQTT == true
  Serial.begin(115200);
#endif

  // init top LED and turn on.
  pinMode(TOP_LED, OUTPUT);
  digitalWrite(TOP_LED, HIGH);

  // init LED stripe
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS)
    .setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400);
  FastLED.clear(true);
  FastLED.show();
  // FastLED.setBrightness(255);

  // set all panes to random color
  for (uint8_t i = 0; i < NUM_PANES; i++) {
    showPane(i, CRGB(random(0, 255), random(0, 255), random(0, 255)));
  }

  // starting WiFi setup
  // 192.168.4.1 configuration IP

  WiFiManager wifiManager;
  wifiManager.setWiFiAutoReconnect(true);
  DEBUG_SERIAL.println("starting connection");
  wifiManager.autoConnect("Wetter-L");
  DEBUG_SERIAL.println("connected...yeey :)");

  // MQTT setup
  mqttClient.setId("Wetter-L");
  // mqttClient.setUsernamePassword(username, password);

  // String willPayload = "oh no!";
  // bool willRetain = true;
  // int willQos = 1;

  // mqttClient.beginWill(willTopic, willPayload.length(), willRetain, willQos);
  // mqttClient.print(willPayload);
  // mqttClient.endWill();


  if (!mqttClient.connect(broker, port)) {
    DEBUG_MQTT_SERIAL.print("MQTT connection failed! Error code = ");
    DEBUG_MQTT_SERIAL.println(mqttClient.connectError());
    while (1)
      ;
  }
  DEBUG_MQTT_SERIAL.println("Connected to MQTT Broker!");
  mqttClient.onMessage(onMqttMessage);


  mqttClient.subscribe(GADGET_ACTIVE);
  mqttClient.subscribe(GADGET_UPDATE);
  mqttClient.subscribe(GADGET_FAKEWEATHER);
  mqttClient.subscribe(GADGET_PANESELECT);
  mqttClient.subscribe(GADGET_PANECOLOR);
  mqttClient.subscribe(GADGET_ANIMATIONMODE);
  mqttClient.subscribe(GADGET_LIGHTS);
  mqttClient.subscribe(GADGET_BRIGHTNESS);


  // turn off panes when connected
  FastLED.clear(true);
  FastLED.show();

  // get initial weather conditions and apply them to the panes
  getCurrentWeatherConditions();
  applyConditions();

  // turn off LED on top, when connected and inform Blynk-App that the panes are
  // active
  digitalWrite(TOP_LED, LOW);
  // Blynk.virtualWrite(V12, HIGH);

  // ermittele Zeit mittels UDP
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

void loop() {
  mqttClient.poll();

  // if lights are off don't apply conditions or do an animation
  if (!lightsOn)
    return;

  if (dimBySun) {
    time_t t = now();
    int bright = brightness;
    if (t > sunrise - dimTime * 60 && t < sunrise) {
      bright = map(t, sunrise - dimTime * 60, sunrise, dimMin, dimMax);
    } else if (t > sunset && t < sunset + dimTime * 60) {
      bright = map(t, sunset, sunset + dimTime * 60, dimMax, dimMin);
    }
    if (bright != brightness) {
      if (!isActive) {
        updateBrightness(bright);
      } else {
        if (millis() - lastcheckDimming >= DIMMING_INTERVAL) {
          updateBrightness(bright);
          lastcheckDimming = millis();
        } else {
          updateBrightness(bright, false);
        }
      }
    }
  }

  if (isActive) {
    // get regularly new weather data
    if (millis() - lastcheck >= INTERVAL) {
      getCurrentWeatherConditions();
      lastcheck = millis();
      applyConditions();
    } else if ((weatherID / 100) == 2) {
      // thunderstorm has extra animation
      if (random(1000) < 8) {
        showPane(CLOUD, CRGB(255, 255, 255));
        showPane(RAIN, CRGB(255, 255, 0));
      } else {
        showPane(CLOUD, CRGB(60, 60, 60));
        showPane(RAIN, CRGB(0, 0, 255));
      }
    }

  } else {
    doAnimation();
  }
}


uint8_t animCounter = 0;
int8_t animInc = 1;
void doAnimation() {
  CRGB rgb;
  uint8_t offset = 255 / NUM_PANES;
  switch (animationMode) {
    case 0:
      // do nothing;
      break;
    case 1:
      FastLED.clear(true);
      break;
    case 2:
      // it iterates really fast over all the colors. Maybe a small timeout would
      // be apropiate.

      hsv2rgb_rainbow(animColor, rgb);
      for (uint8_t i = 0; i < NUM_PANES; i++) {
        showPane(i, rgb);
      }
      animColor.h++;
      animColor.h = animColor.h % 255;
      break;
    case 3:
      hsv2rgb_rainbow(animColor, rgb);
      for (uint8_t i = 0; i < NUM_PANES; i++) {
        hsv2rgb_rainbow(
          CHSV(animColor.h + (i * offset), animColor.s, animColor.v), rgb);
        showPane(i, rgb);
      }
      animColor.h++;
      animColor.h = animColor.h % 255;
      break;
    case 4:
      // this mode is still experimental.
      hsv2rgb_rainbow(animColor, rgb);
      showPane(animCounter, rgb);
      for (uint8_t i = 0; i < NUM_PANES; i++) {
        if (i == animCounter)
          continue;
        showPane(i, CRGB(255, 255, 255));
      }
      animColor.h++;
      if (animColor.h >= 255) {
        animColor.h = 0;
        ++animCounter %= NUM_PANES;
      }
      delay(10);
      break;
    case 5:
      hsv2rgb_rainbow(animColor, rgb);
      for (uint8_t i = 0; i < NUM_PANES; i++) {
        hsv2rgb_rainbow(
          CHSV(animColor.h + (i * offset), animColor.s, animColor.v), rgb);
        showPane(i, rgb);
      }
      animColor.h += animInc;
      animColor.h = animColor.h % 255;
      animCounter++;
      if (animCounter == 0) {
        animInc *= -1;
        animColor.h = 255;
      }
      break;
    case 255:
      if ((weatherID / 100) == 2) {
        if (random(1000) < 8) {
          showPane(CLOUD, CRGB(255, 255, 255));
          showPane(RAIN, CRGB(255, 255, 0));
        } else {
          showPane(CLOUD, CRGB(60, 60, 60));
          showPane(RAIN, CRGB(0, 0, 255));
        }
      }
      break;
  }
  FastLED.show();
}

// The IDs definitions can be found online:
// https://openweathermap.org/weather-conditions
void applyConditions(bool forceUpdate) {

  // we only need to make changes to the leds if the conditions have changed
  if (!forceUpdate && prev_weatherID == weatherID && prev_temperature_Celsius == temperature_Celsius) {
    // Serial.println("id has not changed... skipping!");
    // Serial.print(prev_weatherID); Serial.print("==");
    // Serial.println(weatherID);
    return;
  }

  FastLED.clear(true);

  // turn off the front pane
  showPane(FRONT, CRGB(0, 0, 0));

  // color the front pane according to the current temperature
  if (temperature_Celsius < 10) {
    showPane(BACK, CRGB(0, 0, 255));
  } else if (temperature_Celsius > 20) {
    showPane(BACK, CRGB(255, 0, 0));
  } else {
    showPane(BACK, CRGB(255, 94, 29));
  }

  if (weatherID == 800) {  // clear sky
    showPane(SUN, CRGB(255, 190, 90));
    FastLED.show();
    // Serial.println("Clear sky");
    return;
  }

  int8_t id = weatherID / 100;  // reduce the id to the main definition
  // Serial.println();
  // Serial.print("weatherID is "); Serial.println(weatherID);
  // Serial.print("ID is "); Serial.println(id);

  switch (id) {
    case 2:  // thunderstorm
      // Serial.println("thunderstorm");
      showPane(CLOUD, CRGB(60, 60, 60));
      showPane(RAIN, CRGB(0, 0, 255));
      break;
    case 3:  // drizzle
      // Serial.println("drizzle");
      showPane(CLOUD, CRGB(180, 180, 180));
      showPane(RAIN, CRGB(0, 0, 200));
      break;
    case 5:  // rain
      // Serial.println("rain");
      showPane(CLOUD, CRGB(255, 255, 255));
      showPane(RAIN, CRGB(0, 0, 255));
      break;
    case 6:  // snow
      // Serial.println("snow");
      showPane(CLOUD, CRGB(255, 255, 255));
      showPane(RAIN, CRGB(255, 255, 255));
      break;
    case 7:  // atmosphere
      // Serial.println("atmosphere");
      showPane(CLOUD, CRGB(180, 180, 180));
      showPane(RAIN, CRGB(180, 180, 180));
      break;
    case 8:  // clouds
      // Serial.println("clouds");
      showPane(SUN, CRGB(255, 190, 90));
      showPane(CLOUD, CRGB(180, 180, 180));
      break;
    default:
      // Serial.println("error");
      CRGB col = CRGB(255, 0, 0);
      for (uint8_t i = 0; i < NUM_PANES; i++) {
        showPane(i, col);
      }
  }
  FastLED.show();
  // Serial.println("All done!");
}

void getCurrentWeatherConditions() {
  Serial.println("connecting to api.openweathermap.org");
  // get data from api
  if (wifiClient.connect("api.openweathermap.org", 80)) {
    wifiClient.println("GET /data/2.5/weather?q=" + CITY + "," + countryCode + "&units=metric&lang=de&APPID=" + OWM_API_KEY);
    wifiClient.println("Host: api.openweathermap.org");
    wifiClient.println("Connection: close");
    wifiClient.println();
  } else {
    Serial.println("connection failed");
  }
  const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(14) + 360;
  DynamicJsonDocument doc(capacity);
  // create json from api's data
  deserializeJson(doc, wifiClient);
  wifiClient.stop();

  // set variables according to api answer
  prev_weatherID = weatherID;
  weatherID = doc["weather"][0]["id"];
  prev_temperature_Celsius = temperature_Celsius;
  temperature_Celsius = doc["main"]["temp"];
  long tz = doc["timezone"];
  sunrise = doc["sys"]["sunrise"];
  sunrise += tz;
  sunset = doc["sys"]["sunset"];
  sunset += tz;

  // printSunposition();
  // serializeJson(doc, Serial);Serial.println();
}

void showPane(uint8_t pane, CRGB color) {
  for (uint8_t i = 0; i < 2; i++) {
    leds[PANES_LOOKUP[pane][i]] = color;
  }
  FastLED.show();
}

void setLights(bool on) {
  lightsOn = on;

  if (!lightsOn) {
    // Blynk.virtualWrite(V7, LOW);
    // Blynk.virtualWrite(V13, LOW);
    FastLED.clear(true);
    FastLED.show();
    digitalWrite(TOP_LED, LOW);
  } else {
    // Blynk.virtualWrite(V7, HIGH);
    if (isActive) {
      applyConditions(true);
    } else if (topLightOn)
      digitalWrite(TOP_LED, HIGH);
    // Blynk.virtualWrite(V13, topLightOn);
    if (dimBySun) {
      time_t t = now();
      int bright = brightness;
      if (t < sunrise - dimTime * 60) {
        bright = dimMin;
      } else if (t > sunrise && t < sunset) {
        bright = dimMax;
      } else if (t > sunset + dimTime * 60) {
        bright = dimMin;
      }
      updateBrightness(bright);
    }
  }
}

void setActive() {
  isActive = true;
  topLightOn = false;
  // Blynk.virtualWrite(V13, topLightOn);
  getCurrentWeatherConditions();
  applyConditions(true);
  digitalWrite(TOP_LED, LOW);
  // Blynk.virtualWrite(V1, HIGH);
  // Blynk.virtualWrite(V7, HIGH);
}

void setInactive() {
  isActive = false;
  animationMode = 0;
  topLightOn = true;
  // Blynk.virtualWrite(V13, HIGH);
  digitalWrite(TOP_LED, HIGH);
  // Blynk.virtualWrite(V1, LOW);
  publishMQTT(CONTROLLER_ACTIVE, "0");
}

void updateBrightness(int b, bool applyOnActive) {
  brightness = b;
  FastLED.setBrightness(brightness);
  if (isActive && applyOnActive) {
    applyConditions(true);
  }
  // Blynk.virtualWrite(V12, brightness);
  publishMQTT(CONTROLLER_BRIGHTNESS, String(brightness));
}


//===================== HELPER ============================//
void publishMQTT(String topic, String message, bool retain, uint8_t qos, bool dup) {
  mqttClient.beginMessage(topic, retain, qos, dup);
  mqttClient.print(message);
  mqttClient.endMessage();
}

void onMqttMessage(int messageSize) {
  String topic = mqttClient.messageTopic();
  DEBUG_MQTT_SERIAL.println("Received a message with topic '");
  DEBUG_MQTT_SERIAL.print(topic);
  DEBUG_MQTT_SERIAL.print("', length ");
  DEBUG_MQTT_SERIAL.print(messageSize);
  DEBUG_MQTT_SERIAL.println(" bytes:");

  // use the Stream interface to print the contents
  String value = "";
  while (mqttClient.available()) {
    value += (char)mqttClient.read();
  }
  DEBUG_MQTT_SERIAL.println(value);
  DEBUG_MQTT_SERIAL.println();
  if (topic.equals(GADGET_ACTIVE)) {
    setLights(true);
    if (value.toInt()) {
      setActive();
    } else {
      setInactive();
      FastLED.clear(true);
      FastLED.show();
    }
  } else if (topic.equals(GADGET_UPDATE)) {
    setLights(true);
    if (value.toInt()) {
      setActive();
      lastcheck = millis();
    }
    // Blynk.virtualWrite(V1, HIGH);
    publishMQTT(CONTROLLER_ACTIVE, "1");
  } else if (topic.equals(GADGET_FAKEWEATHER)) {
    setLights(true);
    setInactive();
    animationMode = 255;

    switch (value.toInt()) {
      case 1:  // clear
        weatherID = 800;
        break;
      case 2:  // cloudy
        weatherID = 801;
        break;
      case 3:  // rainy
        weatherID = 500;
        break;
      case 4:  // snowy
        weatherID = 600;
        break;
      case 5:  // thunderstorm
        weatherID = 200;
        break;
      case 6:  // drizzle
        weatherID = 300;
        break;
      case 7:  // atmosphere
        weatherID = 700;
        break;
      default:  // error
        weatherID = -1;
    }
    applyConditions(true);
  } else if (topic.equals(GADGET_PANESELECT)) {
    paneIndex = value.toInt();
    if (paneIndex == 5)
      paneIndex = -1;
  } else if (topic.equals(GADGET_PANECOLOR)) {
    setLights(true);
    setInactive();
    int strlen = value.length() + 1;
    char buffer[strlen];
    value.toCharArray(buffer, strlen);
    byte red, green, blue;
    getRGB(buffer, red, green, blue);

    if (paneIndex == -1) {
      for (uint8_t i = 0; i < NUM_PANES; i++) {
        showPane(i, CRGB(red, green, blue));
      }
    } else {
      showPane(paneIndex, CRGB(red, green, blue));
    }
  } else if (topic.equals(GADGET_ANIMATIONMODE)) {
    setLights(true);
    setInactive();
    animationMode = value.toInt();
  } else if (topic.equals(GADGET_LIGHTS)) {
    setLights(value.toInt());
  } else if (topic.equals(GADGET_BRIGHTNESS)) {
    dimBySun = false;
    // Blynk.virtualWrite(V15, dimBySun);
    publishMQTT(CONTROLLER_DIMBYSUN, "false");
    updateBrightness(value.toInt());
  }
}

//https://forum.arduino.cc/t/hex-in-rgb-umrechnen/152842/4
void getRGB(char *text, byte &r, byte &g, byte &b) {
  long l = strtol(text + 1, NULL, 16);
  r = l >> 16;
  g = l >> 8;
  b = l;
}

//================================ NTP ==========================================//

time_t getNtpTime() {
  IPAddress ntpServerIP;  // NTP server's ip address

  while (Udp.parsePacket() > 0)
    ;  // discard any previously received packets
  DEBUG_SERIAL.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  DEBUG_SERIAL.print(ntpServerName);
  DEBUG_SERIAL.print(": ");
  DEBUG_SERIAL.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      DEBUG_SERIAL.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  DEBUG_SERIAL.println("No NTP Response :-(");
  return 0;  // return 0 if unable to get the time
}


// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123);  //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

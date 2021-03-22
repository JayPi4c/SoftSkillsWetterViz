// einbinden der geheimen Daten
#include "secrets.h"

// einbinden weiterer Bibliotheken
#include <FastLED.h>
#include <ArduinoJson.h>
#include <math.h>
#include <WiFiManager.h>

// Weitere Definitionen
#define DATA_PIN D3
#define NUM_LEDS 8

// LED Stripe
CRGB leds[NUM_LEDS];

// WiFi setup
WiFiManager wifiManager;
WiFiClient client;

uint8_t weatherID = 0;

uint8_t counter;
const uint8_t NUM_PANES = 5;
const uint32_t INTERVAL = 1800000;
unsigned long lastcheck = 0;

const String CITY = "Oldenburg";


void setup() {
  Serial.begin(115200);

  // init LED stripe
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400);
  FastLED.clear();
  FastLED.show();
  FastLED.setBrightness(128);

  // set all panes to random color
  for (int i = 0; i < 5; i++) {
    showPane(i, CRGB( random(0, 255), random(0, 255), random(0, 255)));
  }

  // init WiFi
  wifiManager.autoConnect("Wetter-Gadget");

  // turn off panes when connected
  FastLED.clear();
  FastLED.show();


  getCurrentWeatherConditions();
  counter = 0;
}


void loop() {

  // get regularly new weather data 
  if (millis() - lastcheck >= INTERVAL) {
    getCurrentWeatherConditions();
    lastcheck = millis();
  }

  
  FastLED.clear();
  showPane(counter, CRGB( random(0, 255), random(0, 255), random(0, 255)));
  counter++;
  counter %= NUM_PANES;

  delay(1000);
}

void getCurrentWeatherConditions() {
  int WeaterData;
  Serial.println("connecting to api.openweathermap.org");
  // get data from api
  if (client.connect("api.openweathermap.org", 80)) {
    client.println("GET /data/2.5/weather?q=" + CITY + ",DE&units=metric&lang=de&APPID=" + API_KEY);
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed");
  }
   const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(14) + 360;
  DynamicJsonDocument doc(capacity);
  // create json from api's data
  deserializeJson(doc, client);
  client.stop();

  // set variables according to api answer
  weatherID = doc["weather"][0]["id"];
  int temperature_Celsius = doc["main"]["temp"];
}

// lookup table um die LEDs für die jeweiligen Platten zu finden.
const uint8_t PANES [NUM_PANES][2] = {{4, 5}, {3, 6}, {2, 7}, {1, 8}, {0, 9}};

void showPane(int pane, CRGB color) {
  for (int i = 0; i < 2; i++) {
    leds[PANES[pane][i]] = color;
  }
  FastLED.show();
}

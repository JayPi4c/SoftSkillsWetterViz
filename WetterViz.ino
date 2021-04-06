// einbinden der geheimen Daten
#include "secrets.h"

// einbinden weiterer Bibliotheken
#include <ArduinoJson.h>
#include <FastLED.h>
//#include "WiFiManager.h"

// BLYNK
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>

// Weitere Definitionen
#define DATA_PIN D3
#define NUM_LEDS 10

#define TOP_LED D2

// LED Stripe
CRGB leds[NUM_LEDS];

// WiFi setup
// WiFiManager wifiManager;
WiFiClient client;

boolean lightsOn = true;
boolean isActive = true;

int prev_weatherID = 0;
int weatherID = 0;

const uint8_t NUM_PANES = 5;
const uint32_t INTERVAL = 1800000;
unsigned long lastcheck = 0;

const String CITY = "Oldenburg";

uint8_t animationMode = 0;
CHSV animColor = CHSV(0, 255, 255);

// define function to allow default parameter
void applyConditions(boolean forceUpdate = false);

// allows to turn on and off the device via the App
BLYNK_WRITE(V1) {
  setLights(true);
  if (param.asInt()) {
    setActive();
  } else {
    setInactive();
    FastLED.clear();
    FastLED.show();
  }
}

// allow conditions update on user input
BLYNK_WRITE(V2) {
  setLights(true);
  if (param.asInt()) {
    setActive();
    lastcheck = millis();
  }
  Blynk.virtualWrite(V1, HIGH);
}

// light up panes for weather conditions indepent of the real weather
// turns off the update functionality
BLYNK_WRITE(V3) {
  setLights(true);
  setInactive();
  animationMode = 255;

  switch (param.asInt()) {
    case 1: // clear
      weatherID = 800;
      break;
    case 2: // cloudy
      weatherID = 801;
      break;
    case 3: // rainy
      weatherID = 500;
      break;
    case 4: // snowy
      weatherID = 600;
      break;
    case 5: // thunderstorm
      weatherID = 200;
      break;
    case 6: // drizzle
      weatherID = 300;
      break;
    case 7: // atmosphere
      weatherID = 700;
      break;
    default: // error
      weatherID = -1;
  }
  applyConditions(true);
}

// sets the pane index which should light up in custom color
int paneIndex = -1;
BLYNK_WRITE(V4) {
  paneIndex = param.asInt() - 1;
  if (paneIndex == 5)
    paneIndex = -1;
}

// function to light up panes with a color received from Blynk
// turns off the update functionality
BLYNK_WRITE(V5) {
  setLights(true);
  setInactive();

  int red = param[0].asInt();
  int green = param[1].asInt();
  int blue = param[2].asInt();

  if (paneIndex == -1) {
    for (int i = 0; i < NUM_PANES; i++) {
      showPane(i, CRGB(red, green, blue));
    }
  } else {
    showPane(paneIndex, CRGB(red, green, blue));
  }
}

BLYNK_WRITE(V6) {
  setLights(true);
  setInactive();
  animationMode = param.asInt();
}

// allow user to completely disable lights
BLYNK_WRITE(V7) {
  setLights(param.asInt());
}

// Timer to turn off lights between certain times
BLYNK_WRITE(V8) {
  setLights(param.asInt());
}

void setup() {
  Serial.begin(115200);

  // init top LED and turn on.
  pinMode(TOP_LED, OUTPUT);
  digitalWrite(TOP_LED, HIGH);

  // init LED stripe
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400);
  FastLED.clear();
  FastLED.show();
  FastLED.setBrightness(128);

  // set all panes to random color
  for (int i = 0; i < 5; i++) {
    showPane(i, CRGB(random(0, 255), random(0, 255), random(0, 255)));
  }

  // init WiFi
  // 192.168.4.1 configuration IP
  // wifiManager.autoConnect("Wetter-Gadget");

  // Serial.println(wifiManager.getWiFiPass());
  // Serial.println(wifiManager.getWiFiSSID());

  // connect to Blynk
  Blynk.begin(BLYNK_API_KEY, ssid, pass, "iot.informatik.uni-oldenburg.de", 8080);

  // turn off panes when connected
  FastLED.clear();
  FastLED.show();

  // get initial weather conditions and apply them to the panes
  // getCurrentWeatherConditions();
  // applyConditions();

  // turn off LED on top, when connected and inform Blynk-App that the panes are
  // active
  // digitalWrite(TOP_LED, LOW);
  // Blynk.virtualWrite(V12, HIGH);

  setActive();
}

void loop() {
  Blynk.run();

  // if lights are off don't apply conditions or do an animation
  if (!lightsOn)
    return;

  if (isActive) {
    // get regularly new weather data
    if (millis() - lastcheck >= INTERVAL) {
      getCurrentWeatherConditions();
      lastcheck = millis();
      applyConditions();
    } else if ((weatherID / 100) == 2) {
      // thunderstorm has extra animation
      if (random(1000) < 8) {
        showPane(2, CRGB(255, 255, 255));
        showPane(1, CRGB(255, 255, 0));
      } else {
        showPane(2, CRGB(60, 60, 60));
        showPane(1, CRGB(0, 0, 255));
      }
    }

  } else {
    doAnimation();
  }
}

uint8_t animCounter = 0;
void doAnimation() {
  CRGB rgb;
  uint8_t offset = 255 / NUM_PANES;
  switch (animationMode) {
    case 0:
      // do nothing;
      break;
    case 1:
      FastLED.clear();
      break;
    case 2:
      // it iterates really fast over all the colors. Maybe a small timeout would
      // be apropiate.

      hsv2rgb_rainbow(animColor, rgb);
      for (int i = 0; i < NUM_PANES; i++) {
        showPane(i, rgb);
      }
      animColor.h++;
      animColor.h = animColor.h % 255;
      break;
    case 3:
      hsv2rgb_rainbow(animColor, rgb);
      for (int i = 0; i < NUM_PANES; i++) {
        hsv2rgb_rainbow(CHSV(animColor.h + (i * offset), animColor.s, animColor.v), rgb);
        showPane(i, rgb);
      }
      animColor.h++;
      animColor.h = animColor.h % 255;
      break;
    case 4:
      // this mode is still experimental.
      hsv2rgb_rainbow(animColor, rgb);
      showPane(animCounter, rgb);
      for (int i = 0; i < NUM_PANES; i++) {
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
    case 255:
      if ((weatherID / 100) == 2) {
        if (random(1000) < 8) {
          showPane(2, CRGB(255, 255, 255));
          showPane(1, CRGB(255, 255, 0));
        } else {
          showPane(2, CRGB(60, 60, 60));
          showPane(1, CRGB(0, 0, 255));
        }
      }
      break;
  }
  FastLED.show();
}

// The IDs definitions can be found online:
// https://openweathermap.org/weather-conditions
void applyConditions(boolean forceUpdate) {

  // we only need to make changes to the leds if the conditions have changed
  if (!forceUpdate && prev_weatherID == weatherID) {
    // Serial.println("id has not changed... skipping!");
    // Serial.print(prev_weatherID); Serial.print("==");
    // Serial.println(weatherID);
    return;
  }

  FastLED.clear();

  // set the backpane to white
  showPane(4, CRGB(255, 255, 255));

  if (weatherID == 800) { // clear sky
    showPane(3, CRGB(255, 190, 90));
    FastLED.show();
    // Serial.println("Clear sky");
    return;
  }

  uint8_t id = weatherID / 100; // reduce the id to the main definition
  // Serial.println();
  // Serial.print("weatherID is "); Serial.println(weatherID);
  // Serial.print("ID is "); Serial.println(id);

  switch (id) {
    case 2: // thunderstorm
      // Serial.println("thunderstorm");
      showPane(2, CRGB(60, 60, 60));
      showPane(1, CRGB(0, 0, 255));
      break;
    case 3: // drizzle
      // Serial.println("drizzle");
      showPane(2, CRGB(180, 180, 180));
      showPane(1, CRGB(0, 0, 200));
      break;
    case 5: // rain
      // Serial.println("rain");
      showPane(2, CRGB(255, 255, 255));
      showPane(1, CRGB(0, 0, 255));
      break;
    case 6: // snow
      // Serial.println("snow");
      showPane(2, CRGB(255, 255, 255));
      showPane(1, CRGB(255, 255, 255));
      break;
    case 7: // atmosphere
      // Serial.println("atmosphere");
      showPane(2, CRGB(180, 180, 180));
      showPane(1, CRGB(180, 180, 180));
      break;
    case 8: // clouds
      // Serial.println("clouds");
      showPane(3, CRGB(255, 190, 90));
      showPane(2, CRGB(180, 180, 180));
      break;
    default:
      // Serial.println("error");
      CRGB col = CRGB(255, 0, 0);
      for (byte i = 0; i < NUM_PANES; i++) {
        showPane(i, col);
      }
  }
  FastLED.show();
  // Serial.println("All done!");
}

void getCurrentWeatherConditions() {
  int WeaterData;
  Serial.println("connecting to api.openweathermap.org");
  // get data from api
  if (client.connect("api.openweathermap.org", 80)) {
    client.println("GET /data/2.5/weather?q=" + CITY + ",DE&units=metric&lang=de&APPID=" + OWM_API_KEY);
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
  prev_weatherID = weatherID;
  weatherID = doc["weather"][0]["id"];
  int temperature_Celsius = doc["main"]["temp"];
  // serializeJson(doc, Serial);
}

// lookup table um die LEDs für die jeweiligen Platten zu finden.
const uint8_t PANES[NUM_PANES][2] = {{4, 5}, {3, 6}, {2, 7}, {1, 8}, {0, 9}};

void showPane(int pane, CRGB color) {
  for (int i = 0; i < 2; i++) {
    leds[PANES[pane][i]] = color;
  }
  FastLED.show();
}

void setLights(boolean on) {
  lightsOn = on;

  if (!lightsOn) {
    Blynk.virtualWrite(V7, LOW);
    FastLED.clear();
    FastLED.show();
    digitalWrite(TOP_LED, LOW);
  } else {
    Blynk.virtualWrite(V7, HIGH);
    if (isActive) {
      applyConditions(true);
    } else
      digitalWrite(TOP_LED, HIGH);
  }
}

void setActive() {
  isActive = true;
  getCurrentWeatherConditions();
  applyConditions(true);
  digitalWrite(TOP_LED, LOW);
  Blynk.virtualWrite(V1, HIGH);
}

void setInactive() {
  isActive = false;
  animationMode = 0;
  digitalWrite(TOP_LED, HIGH);
  Blynk.virtualWrite(V1, LOW);
}

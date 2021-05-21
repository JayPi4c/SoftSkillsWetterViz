// einbinden der geheimen Daten
#include "secrets.h"

// einbinden weiterer Bibliotheken
#include <ArduinoJson.h>
#define FASTLED_INTERNAL // define to stop FastLED Pragma, comment out to get
                         // more information
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

bool lightsOn = true;
bool isActive = true;

uint8_t brightness = 255;

int prev_weatherID = 0;
int weatherID = 0;
int prev_temperature_Celsius = 15;
int temperature_Celsius = 15;

const uint8_t NUM_PANES = 5;

// lookup table um die LEDs für die jeweiligen Platten zu finden.
const uint8_t PANES_LOOKUP[NUM_PANES][2] = {
    {4, 5}, {3, 6}, {2, 7}, {1, 8}, {0, 9}};

enum PANES { FRONT, RAIN, CLOUD, SUN, BACK };

uint32_t INTERVAL = 900000;
unsigned long lastcheck = 0;

String CITY = "Oldenburg";
String countryCode = "DE";

uint8_t animationMode = 0;
CHSV animColor = CHSV(0, 255, 255);

// define function to allow default parameter
void applyConditions(bool forceUpdate = false);

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
int8_t paneIndex = -1;
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
    for (uint8_t i = 0; i < NUM_PANES; i++) {
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
BLYNK_WRITE(V7) { setLights(param.asInt()); }

// Timer to turn off lights between certain times
BLYNK_WRITE(V8) { setLights(param.asInt()); }

// setting city to userinput
BLYNK_WRITE(V9) {
  CITY = param.asString();
  setActive();
  lastcheck = millis();
  Blynk.virtualWrite(V1, HIGH);
  // Serial.print("Changing city to ");Serial.println(CITY);
}

// setting country code to userinput
BLYNK_WRITE(V10) {
  countryCode = param.asString();
  // Serial.print("Changing country code to ");Serial.println(countryCode);
}

// setting update interval to user input
BLYNK_WRITE(V11) {
  INTERVAL = param.asInt() * 60000l;
  setActive();
  lastcheck = millis();
  Blynk.virtualWrite(V1, HIGH);
  // Serial.print("Changing Updateinterval to ");Serial.println(INTERVAL);
}

BLYNK_WRITE(V12) {
  brightness = param.asInt();
  FastLED.setBrightness(brightness);
  if (isActive) {
    applyConditions(true);
  }
}

void setup() {
  Serial.begin(115200);

  // init top LED and turn on.
  pinMode(TOP_LED, OUTPUT);
  digitalWrite(TOP_LED, HIGH);

  // init LED stripe
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400);
  FastLED.clear();
  FastLED.show();
  // FastLED.setBrightness(255);

  // set all panes to random color
  for (uint8_t i = 0; i < NUM_PANES; i++) {
    showPane(i, CRGB(random(0, 255), random(0, 255), random(0, 255)));
  }

  // init WiFi
  // 192.168.4.1 configuration IP
  // wifiManager.autoConnect("Wetter-Gadget");

  // Serial.println(wifiManager.getWiFiPass());
  // Serial.println(wifiManager.getWiFiSSID());

  // connect to Blynk
  Blynk.begin(BLYNK_API_KEY, ssid, pass, BLYNK_SERVER,
              BLYNK_PORT);

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

  // write the default settings to Blynk App
  Blynk.virtualWrite(V9, CITY);
  Blynk.virtualWrite(V10, countryCode);
  Blynk.virtualWrite(V11, INTERVAL / 60000l);
  Blynk.virtualWrite(V12, brightness);

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
    FastLED.clear();
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
  if (!forceUpdate && prev_weatherID == weatherID &&
      prev_temperature_Celsius == temperature_Celsius) {
    // Serial.println("id has not changed... skipping!");
    // Serial.print(prev_weatherID); Serial.print("==");
    // Serial.println(weatherID);
    return;
  }

  FastLED.clear();

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

  if (weatherID == 800) { // clear sky
    showPane(SUN, CRGB(255, 190, 90));
    FastLED.show();
    // Serial.println("Clear sky");
    return;
  }

  int8_t id = weatherID / 100; // reduce the id to the main definition
  // Serial.println();
  // Serial.print("weatherID is "); Serial.println(weatherID);
  // Serial.print("ID is "); Serial.println(id);

  switch (id) {
  case 2: // thunderstorm
    // Serial.println("thunderstorm");
    showPane(CLOUD, CRGB(60, 60, 60));
    showPane(RAIN, CRGB(0, 0, 255));
    break;
  case 3: // drizzle
    // Serial.println("drizzle");
    showPane(CLOUD, CRGB(180, 180, 180));
    showPane(RAIN, CRGB(0, 0, 200));
    break;
  case 5: // rain
    // Serial.println("rain");
    showPane(CLOUD, CRGB(255, 255, 255));
    showPane(RAIN, CRGB(0, 0, 255));
    break;
  case 6: // snow
    // Serial.println("snow");
    showPane(CLOUD, CRGB(255, 255, 255));
    showPane(RAIN, CRGB(255, 255, 255));
    break;
  case 7: // atmosphere
    // Serial.println("atmosphere");
    showPane(CLOUD, CRGB(180, 180, 180));
    showPane(RAIN, CRGB(180, 180, 180));
    break;
  case 8: // clouds
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
  if (client.connect("api.openweathermap.org", 80)) {
    client.println("GET /data/2.5/weather?q=" + CITY + "," + countryCode +
                   "&units=metric&lang=de&APPID=" + OWM_API_KEY);
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed");
  }
  const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(1) +
                          2 * JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(4) +
                          JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) +
                          JSON_OBJECT_SIZE(14) + 360;
  DynamicJsonDocument doc(capacity);
  // create json from api's data
  deserializeJson(doc, client);
  client.stop();

  // set variables according to api answer
  prev_weatherID = weatherID;
  weatherID = doc["weather"][0]["id"];
  prev_temperature_Celsius = temperature_Celsius;
  temperature_Celsius = doc["main"]["temp"];
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
  Blynk.virtualWrite(V7, HIGH);
}

void setInactive() {
  isActive = false;
  animationMode = 0;
  digitalWrite(TOP_LED, HIGH);
  Blynk.virtualWrite(V1, LOW);
}

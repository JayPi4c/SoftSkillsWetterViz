// einbinden der geheimen Daten
#include "secrets.h"

// einbinden weiterer Bibliotheken
#include <FastLED.h>
#include <ArduinoJson.h>
#include "WiFiManager.h"

// Weitere Definitionen
#define DATA_PIN D3
#define NUM_LEDS 10

// LED Stripe
CRGB leds[NUM_LEDS];

// WiFi setup
WiFiManager wifiManager;
WiFiClient client;


int prev_weatherID = 0;
int weatherID = 0;

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
  // 192.168.4.1 configuration IP
  wifiManager.autoConnect("Wetter-Gadget");

  // turn off panes when connected
  FastLED.clear();
  FastLED.show();

  // get initial weather conditions and apply them to the panes
  getCurrentWeatherConditions();
  applyConditions();

}


void loop() {

  // get regularly new weather data
  if (millis() - lastcheck >= INTERVAL) {
    getCurrentWeatherConditions();
    lastcheck = millis();
    applyConditions();
  }
}


// The IDs definitions can be found online:
// https://openweathermap.org/weather-conditions
void applyConditions() {

  // we only need to make changes to the leds if the conditions have changed
  if (prev_weatherID == weatherID) {
    // Serial.println("id has not changed... skipping!");
    // Serial.print(prev_weatherID); Serial.print("=="); Serial.println(weatherID);
    return;
  }

  FastLED.clear();

  // set the backpane to white
  showPane(4, CRGB(255, 255, 255));

  if (weatherID == 800) { // clear sky
    showPane(3, CRGB( 255, 190, 90));
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
      showPane(2, CRGB( 60, 60, 60));
      showPane(1, CRGB( 0, 0, 255));
      break;
    case 3: // drizzle
      // Serial.println("drizzle");
      showPane(2, CRGB( 180, 180, 180));
      showPane(1, CRGB( 0, 0, 200));
      break;
    case 5: // rain
      // Serial.println("rain");
      showPane(2, CRGB( 255, 255, 255));
      showPane(1, CRGB( 0, 0, 255));
      break;
    case 6: // snow
      // Serial.println("snow");
      showPane(2, CRGB( 255, 255, 255));
      showPane(1, CRGB( 255, 255, 255));
      break;
    case 7: // atmosphere
      // Serial.println("atmosphere");
      showPane(2, CRGB( 180, 180, 180));
      showPane(1, CRGB( 180, 180, 180));
      break;
    case 8: // clouds
      // Serial.println("clouds");
      showPane(3, CRGB( 255, 190, 90));
      showPane(2, CRGB( 180, 180, 180));
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
  //serializeJson(doc, Serial);
}

// lookup table um die LEDs für die jeweiligen Platten zu finden.
const uint8_t PANES [NUM_PANES][2] = {{4, 5}, {3, 6}, {2, 7}, {1, 8}, {0, 9}};

void showPane(int pane, CRGB color) {
  for (int i = 0; i < 2; i++) {
    leds[PANES[pane][i]] = color;
  }
  FastLED.show();
}

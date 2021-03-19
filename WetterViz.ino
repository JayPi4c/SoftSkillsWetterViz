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


uint8_t counter;
const uint8_t NUM_PANES = 5;
void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400);
  FastLED.clear();
  FastLED.show();
  FastLED.setBrightness(128);

  counter = 0;
}


void loop() {
  FastLED.clear();
  showPane(counter, CRGB( random(0, 255), random(0, 255), random(0, 255)));
  counter++;
  counter %= NUM_PANES;

  delay(1000);
}

// lookup table um die LEDs für die jeweiligen Platten zu finden.
const uint8_t PANES [NUM_PANES][2] = {{4, 5}, {3, 6}, {2, 7}, {1, 8}, {0, 9}};

void showPane(int pane, CRGB color) {
  for (int i = 0; i < 2; i++) {
    leds[PANES[pane][i]] = color;
  }
  FastLED.show();
}

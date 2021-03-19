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


void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400);
  FastLED.clear();
  FastLED.show();
  FastLED.setBrightness(64);

  showPane(3, CRGB( 253, 184, 19));

}



void loop() {

}


const uint8_t PANES [5][2]={{4, 5}, {3, 6}, {2, 7},{1, 8}, {0, 9}};

void showPane(int pane, CRGB color){
  for(int i= 0; i < 2; i++){
    leds[PANES[pane][i]]= color;
    }
    FastLED.show();
  }

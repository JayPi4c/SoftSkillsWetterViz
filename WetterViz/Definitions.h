#ifndef DEFINITIONS
#define DEFINITIONS

#define DATA_PIN D3
#define NUM_LEDS 10

#define TOP_LED D2

const uint8_t NUM_PANES = 5;
// lookup table um die LEDs für die jeweiligen Platten zu finden.
const uint8_t PANES_LOOKUP[NUM_PANES][2] = {
  { 4, 5 }, { 3, 6 }, { 2, 7 }, { 1, 8 }, { 0, 9 }
};

enum PANES {
  FRONT,
  RAIN,
  CLOUD,
  SUN,
  BACK
};

// UDP TIME
const static char ntpServerName[] = "de.pool.ntp.org";
const static int timeZone = 2;
const static unsigned int localPort = 8888;

// NTP
const static int NTP_PACKET_SIZE = 48;
static byte packetBuffer[NTP_PACKET_SIZE];

const char GADGET_ACTIVE[] = "Wetter-L/gadget/active";
const char GADGET_UPDATE[] = "Wetter-L/gadget/update";
const char GADGET_FAKEWEATHER[] = "Wetter-L/gadget/fakeWeather";
const char GADGET_PANESELECT[] = "Wetter-L/gadget/paneSelect";
const char GADGET_PANECOLOR[] = "Wetter-L/gadget/paneColor";
const char GADGET_ANIMATIONMODE[] = "Wetter-L/gadget/animationMode";
const char GADGET_LIGHTS[] = "Wetter-L/gadget/lights";
const char GADGET_BRIGHTNESS[] = "Wetter-L/gadget/brightness";

const char CONTROLLER_ACTIVE[] = "Wetter-L/controller/active";
const char CONTROLLER_DIMBYSUN[] = "Wetter-L/controller/dimBySun";
const char CONTROLLER_LIGHTS[] = "Wetter-L/controller/lights";
const char CONTROLLER_BRIGHTNESS[] = "Wetter-L/controller/brightness";

#endif
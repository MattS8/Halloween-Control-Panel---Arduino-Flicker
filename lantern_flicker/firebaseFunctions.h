#ifndef LANTERN_FIREBASE_FUNCTIONS_H
#define LANTERN_FIREBASE_FUNCTIONS_H

#include "FirebaseESP8266.h"
#include "WiFiCreds.h"
#include <ESP8266WiFi.h>
#include <FS.h>

#ifndef LOCAL_FILE
#define LOCAL_FILE
const char* LanternFilePath = "Lantern.dat";
#endif

typedef struct LanternData {
    int pin;					// Pin number associated with lamp
	int maxBrightness;			// Highest value possibile
	int minBrightness;			// Lowest value possible
	int smoothing;			// Controls the speed at which the light ramps up and down
	int flickerRate;			// Controls the frequency of flickering effect. A higher rate will increase the perceived "flickering" of the light
	int dropDelay;				// The delay after the light level drops upon reaching the upLimit 
    int dropValue;              // Controls how much of a drop is observered after hitting upLimit
} LanternData;

static const String FIREBASE_HOST = "halloween-control-center.firebaseio.com";
static const String FIREBASE_AUTH = "dHKCDHppSOQcBKU5a49DgfOOzVoxXdJIB7PaJDa7";

FirebaseData firebaseDataRECV;
FirebaseData firebaseDataSEND;
String DevicePath = "";
bool newDataReceived = false;
bool WiFiSetup = false;
bool FirebaseSetup = false;

/*
    Setting default values will allow the latern to work even before it has read
    data from Firebase backend.
*/
LanternData Lantern = {
    2, //pin
    150, //maxBrightness
    0, //minBrightness
    1, //smoothing
    1000, //flickerRate
    65000, //dropDelay 
    1  //dropValue
};

bool lanternDataReceived();

#endif
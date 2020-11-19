// Version for ESP8266 module on single lantern
#include "lantern_flicker.h"

/* 
	NOTE: These initial values are irrelevent. They are automatically set upon connecting to Firebase! 
*/
int upLimit = -1;					// Highest value during this cycle (Randomied each dimming cycle)
int downLimit = -1;					// lowest value during this cycle (Randomied each dimming cycle)
int level = -1;						// Actual value being sent to the dimmer circuit
int direction = -1;					// +1 for ramping up, -1 for ramping down

// Nano PWM: 3, 5, 6, 9, 10, and 11

#define RLD_DEBUG
void readLocalData()
{
	if (!SPIFFS.begin()) {
#ifdef RLD_DEBUG
		Serial.println("SPIFFS Initialization FAILED.");
#endif
		return;
	}

	File f = SPIFFS.open(LanternFilePath, "r");

	if (!f) {
#ifdef RLD_DEBUG
		Serial.println("Failed to open file.");
#endif
		return;
	}

	uint8_t* pointer = (uint8_t*) &Lantern;

	while (f.available()) {
		*pointer = f.read();
	}

	if (Lantern.dropValue == 0)
		Lantern.dropValue = 1;

	f.close();
#ifdef RLD_DEBUG
	Serial.println("Read in LanternData from file:");
	Serial.print("Pin: "); Serial.println(Lantern.pin);
	Serial.print("maxBrightness: "); Serial.println(Lantern.maxBrightness);
	Serial.print("minBrightness: "); Serial.println(Lantern.minBrightness);
	Serial.print("smoothing: "); Serial.println(Lantern.smoothing);
	Serial.print("flickerRate: "); Serial.println(Lantern.flickerRate);
	Serial.print("dropDelay: "); Serial.println(Lantern.dropDelay);
	Serial.print("dropValue: "); Serial.println(Lantern.dropValue);
#endif
}

void setup() 
{
	Serial.begin(115200);
	// Loop through all lamps and set the pinMode
	
	randomSeed(analogRead(A0));

	readLocalData();

	setupFirebaseFunctions();
}

void lanternSetup()
{
	downLimit = Lantern.minBrightness;
	upLimit = Lantern.maxBrightness;
	level = downLimit + (upLimit - downLimit)/2;
	direction = Lantern.smoothing;
}

void loop() 
{
	if (!WiFiSetup)
		connectToWiFi();

	if (!FirebaseSetup && WiFi.status() == WL_CONNECTED)
		connectToFirebase();


	// Set instance variables to initial values if they were never set up before
	// or if new data was read from firebase
	if (newDataReceived) {
		lanternSetup();
		newDataReceived = false;
	}

	if (direction > 0) {
		if (level >= upLimit) {
			analogWrite(Lantern.pin, downLimit + (upLimit - downLimit)/Lantern.dropValue);
			upLimit = random(downLimit, Lantern.maxBrightness);
			direction *= -1;
			delayMicroseconds(Lantern.dropDelay);
		}
	} else {
		if (level <= downLimit) {
			downLimit = random(Lantern.minBrightness, upLimit);
			direction *= -1;
		}	
	}
	level += direction;
	delayMicroseconds(Lantern.flickerRate);

	analogWrite(Lantern.pin, level);
}
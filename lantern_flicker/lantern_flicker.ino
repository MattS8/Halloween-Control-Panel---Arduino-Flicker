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

void setup() 
{
	Serial.begin(115200);
	// Loop through all lamps and set the pinMode
	
	randomSeed(analogRead(A0));

	setupFirebaseFunctions();	
	connectToFirebase();
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
	// Don't do anything until the lantern is properly set up!
	if (!lanternDataReceived())
		return;

	// Set instance variables to initial values if they were never set up before
	// or if new data was read from firebase
	if (upLimit == -1 || newDataReceived) {
		lanternSetup();
		newDataReceived = false;
	}

	if (direction > 0) {
		if (level >= upLimit) {
			analogWrite(Lantern.pin, downLimit + (upLimit - downLimit)/Lantern.dropValue);
			upLimit = random(downLimit, Lantern.maxBrightness);
			direction *= -1;
			delay(Lantern.dropDelay);
		}
	} else {
		if (level <= downLimit) {
			downLimit = random(Lantern.minBrightness, upLimit);
			direction *= -1;
		}	
	}
	level += direction;
	delay(Lantern.flickerRate);

	analogWrite(Lantern.pin, level);
}
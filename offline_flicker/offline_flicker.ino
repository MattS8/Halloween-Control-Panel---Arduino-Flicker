// Version for OFFLINE modules
#include "offline_flicker.h"


/** --------------------- CHANGE THESE VALUES --------------------- **/

/* 
Set this to the number of flicker effects you are controlling with the ardunio device: */
	#define NUM_FLICKER_EFFECTS 2

/* 
Set this to the number of flicker effects you are controlling with the ardunio device: */
	#define NUM_FLICKER_EFFECTS 2

/* 
Uncomment the type of Arduino device you are using: */
	#define TYPE_NANO 1
	//#define TYPE_ESP8266 1

/* 
Uncomment to enable some debugging statements */
	//#define RLD_DEBUG

/** --------------------- -------------------- --------------------- **/



/* 
	Instance variables for all controlled flicker effects 
*/
int upLimit[NUM_FLICKER_EFFECTS];					// Highest value during this cycle (Randomized each dimming cycle)
int downLimit[NUM_FLICKER_EFFECTS];					// lowest value during this cycle (Randomized each dimming cycle)
int level[NUM_FLICKER_EFFECTS];						// Actual value being sent to the dimmer circuit
int direction[NUM_FLICKER_EFFECTS];					// +1 for ramping up, -1 for ramping down
unsigned long delayTimes[NUM_FLICKER_EFFECTS];			// The next delay for the flicker effect


#ifdef TYPE_NANO
FlickerData* Flicker = new NanoFlickerData();
#endif
#ifdef TYPE_ESP8266
FlickerData* Flicker = new ESP8266Data();
#endif

void setup() 
{
	Serial.begin(115200);
	
	randomSeed(analogRead(A0));

	// Initial Setup
	for (int i = 0; i < NUM_FLICKER_EFFECTS; ++i)
	{
		downLimit[i] = Flicker->minBrightness;
		upLimit[i] = Flicker->maxBrightness;
		level[i] = downLimit[i] + (upLimit[i] - downLimit[i])/2;
		direction[i] = Flicker->smoothing;
		delayTimes[i] = 0;
	}
}

void loop() 
{
	for (int i = 0; i < NUM_FLICKER_EFFECTS; ++i)
	{
		if (millis() < delayTimes[i])	// Flicker effect is delaying
			continue;

		// Otherwise, delay period has expired
		if (direction[i] > 0)	// flicker effect is ramping up 
		{
			if (level[i] >= upLimit[i])	// Max brightness has been reached 
			{
				// Instantly drop some amount
				unsigned int instantDropLevel = downLimit[i] + (upLimit[i] - downLimit[i])/Flicker->dropValue;
				analogWrite(Flicker->pin, instantDropLevel);

				// Slowly drop the rest of the way after this delay
				int actualDropDelay = Flicker->dropDelay * (upLimit[i] - instantDropLevel)/Flicker->dropValue;
				
				// Ensure the delay is valid
				if (actualDropDelay < 1)
					actualDropDelay = 1;
				if(actualDropDelay > MAX_DROP_DELAY)
					actualDropDelay = MAX_DROP_DELAY;

				// Update the current light level
				level[i] = instantDropLevel;

				// Find a new max brightness for the next time flicker effect ramps up
				upLimit[i] = random(downLimit[i], Flicker->maxBrightness);

				// Reverse the direction, ramping down
				direction[i] *= -1;

				// Set new delay time (NOTE: actualDropDelay is in Microseconds, must convert to Milliseconds)
				delayTimes[i] = millis() + (actualDropDelay * 1000);
			}
		} 
		else // Flicker effect is ramping down
		{
			if (level[i] <= downLimit[i])	//Min brightness has been reached 
			{
				// Find a new min brightness for the next time flicker effect ramps down
				downLimit[i] = random(Flicker->minBrightness, upLimit[i]);

				// Reverse the direction, ramping up
				direction[i] *= -1;

				// Set new delay time
				delayTimes[i] = millis() + random(Flicker->flickerDelayMin, Flicker->flickerDelayMax);
			}	
		}

		// Ramp light level by the current interval (Note: direction is a product of the smoothing value)
		level[i] += direction[i];

		// Write the new light level
		analogWrite(Flicker->pin, level[i]);
	}
}
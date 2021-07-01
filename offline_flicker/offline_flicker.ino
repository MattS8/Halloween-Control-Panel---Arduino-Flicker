// Version for OFFLINE modules
#include "offline_flicker.h"

/* 
	NOTE: These initial values are irrelevent. They are automatically set upon connecting to Firebase! 
*/
int upLimit = -1;					// Highest value during this cycle (Randomied each dimming cycle)
int downLimit = -1;					// lowest value during this cycle (Randomied each dimming cycle)
int level = -1;						// Actual value being sent to the dimmer circuit
int direction = -1;					// +1 for ramping up, -1 for ramping down

/*
    Default settings for NANO flicker effect.
*/
FlickerData NanoFlicker = {
    2, 		//pin 				-> Pin number associated with lamp
    255, 	//maxBrightness 	-> Highest value possibile, for NANO this is 255				
    0, 		//minBrightness 	-> Lowest value possible				
    1, 		//smoothing 		-> Controls the speed at which the light ramps up and down				
    1000, 	//rampDelay 		-> Controls the delay between each ramp increment. A higher rate will increase the perceived "flickering" of the light				
    1000, 	//dropDelay 		-> The delay after the light level drops upon reaching the upLimit 				
    1,  	//dropValue 		-> Controls how much of a drop is observered after hitting upLimit, a value of 1 negates the "instant drop" effect while higher values increase the effect				
    10, 	//flickerDelayMin 	-> Conrols the minimum time between flicker cycles				
    200 	//flickerDelayMax 	-> Controls the maximum time between flicker cycles				  
};

FlickerData* Flicker = &NanoFlicker;


/*
	Set Variables:
		minRampLength
		minBrightness

	Dynamic Variables:
		rampRate

*/

// Nano PWM: 3, 5, 6, 9, 10, and 11

#define RLD_DEBUG

void setup() 
{
	Serial.begin(115200);
	
	randomSeed(analogRead(A0));

	// Initial Setup
	downLimit = Flicker->minBrightness;
	upLimit = Flicker->maxBrightness;
	level = downLimit + (upLimit - downLimit)/2;
	direction = Flicker->smoothing;
}

void loop() 
{
	if (direction > 0) 
	{
		if (level >= upLimit) 
		{
			int instantDropLevel = downLimit + (upLimit - downLimit)/Flicker->dropValue;
			analogWrite(Flicker->pin, instantDropLevel);
			int actualDropDelay = Flicker->dropDelay * (upLimit - instantDropLevel)/Flicker->dropValue;
			
			if (actualDropDelay < 1)
				actualDropDelay = 1;
			if(actualDropDelay > MAX_DROP_DELAY)
				actualDropDelay = MAX_DROP_DELAY;

			level = instantDropLevel;

			upLimit = random(downLimit, Flicker->maxBrightness);
			direction *= -1;
			delayMicroseconds(actualDropDelay);
		}
	} else 
	{
		if (level <= downLimit) 
		{
			downLimit = random(Flicker->minBrightness, upLimit);
			direction *= -1;
			delay(random(Flicker->flickerDelayMin, Flicker->flickerDelayMax));
		}	
	}

	level += direction;
	
	delayMicroseconds(Flicker->rampDelay);

	analogWrite(Flicker->pin, level);
}
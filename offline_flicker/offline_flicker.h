#ifndef LANTERN_FLICKER_H
#define LANTERN_FLICKER_H



typedef struct FlickerData {
    int pin;					// Pin number associated with lamp
	int maxBrightness;			// Highest value possibile
	int minBrightness;			// Lowest value possible
	int smoothing;			    // Controls the speed at which the light ramps up and down
	int rampDelay;			    // Controls the delay between each ramp increment. A higher rate will increase the perceived "flickering" of the light
	int dropDelay;				// The delay after the light level drops upon reaching the upLimit 
    int dropValue;              // Controls how much of a drop is observered after hitting upLimit
    int flickerDelayMin;         // Conrols the minimum time between flicker cycles    0
    int flickerDelayMax;         // Controls the maximum time between flicker cycles   300
} FlickerData;

#define MAX_DROP_DELAY 15000

#endif
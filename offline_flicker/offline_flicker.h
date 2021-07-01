#ifndef OFFLINE_FLICKER_H
#define OFFLINE_FLICKER_H


class FlickerData 
{
public:
	int pin = 2; 						//pin 				-> Pin number associated with lamp
	int maxBrightness = 1200; 			//maxBrightness 	-> Highest value possibile				
	int minBrightness = 0; 				//minBrightness 	-> Lowest value possible				
	int smoothing = 1; 					//smoothing 		-> Controls the speed at which the light ramps up and down				
	unsigned int rampDelay = 1000; 		//rampDelay 		-> Controls the delay between each ramp increment. A higher rate will increase the perceived "flickering" of the light				
	unsigned int dropDelay = 1000; 		//dropDelay 		-> The delay after the light level drops upon reaching the upLimit 				
	int dropValue = 1;  				//dropValue 		-> Controls how much of a drop is observered after hitting upLimit, a value of 1 negates the "instant drop" effect while higher values increase the effect				
	unsigned int flickerDelayMin = 10; 	//flickerDelayMin 	-> Conrols the minimum time between flicker cycles				
	unsigned int flickerDelayMax = 200; //flickerDelayMax 	-> Controls the maximum time between flicker cycles	
};

class NanoFlickerData : public FlickerData 
{
public:
	NanoFlickerData() 
	{
		pin = 2; 				//pin 				-> Pin number associated with lamp
		maxBrightness = 255; 	//maxBrightness 	-> Highest value possibile, for NANO this is 255				
		minBrightness = 0; 		//minBrightness 	-> Lowest value possible				
		smoothing = 1; 			//smoothing 		-> Controls the speed at which the light ramps up and down				
		rampDelay = 1000; 		//rampDelay 		-> Controls the delay between each ramp increment. A higher rate will increase the perceived "flickering" of the light				
		dropDelay = 1000; 		//dropDelay 		-> The delay after the light level drops upon reaching the upLimit 				
		dropValue = 1;  		//dropValue 		-> Controls how much of a drop is observered after hitting upLimit, a value of 1 negates the "instant drop" effect while higher values increase the effect				
		flickerDelayMin = 10; 	//flickerDelayMin 	-> Conrols the minimum time between flicker cycles				
		flickerDelayMax = 200; 	//flickerDelayMax 	-> Controls the maximum time between flicker cycles	
	}
};

class ESP8266Data : public FlickerData 
{
public:
	ESP8266Data() 
	{
		pin = 2; 				//pin 				-> Pin number associated with lamp
		maxBrightness = 1200; 	//maxBrightness 	-> Highest value possibile, for NANO this is 255				
		minBrightness = 0; 		//minBrightness 	-> Lowest value possible				
		smoothing = 1; 			//smoothing 		-> Controls the speed at which the light ramps up and down				
		rampDelay = 1000; 		//rampDelay 		-> Controls the delay between each ramp increment. A higher rate will increase the perceived "flickering" of the light				
		dropDelay = 1000; 		//dropDelay 		-> The delay after the light level drops upon reaching the upLimit 				
		dropValue = 1;  		//dropValue 		-> Controls how much of a drop is observered after hitting upLimit, a value of 1 negates the "instant drop" effect while higher values increase the effect				
		flickerDelayMin = 10; 	//flickerDelayMin 	-> Conrols the minimum time between flicker cycles				
		flickerDelayMax = 200; 	//flickerDelayMax 	-> Controls the maximum time between flicker cycles	
	}
};

#define MAX_DROP_DELAY 15000

#endif
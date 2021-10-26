#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

#include "RTDBHelper.h"
#include "TokenHelper.h"
#include "Credentials.h"
#include "flicker.h"
#include <FS.h>

#define LOCAL_FILE_PATH "Lantern.dat"
#define MAX_DROP_DELAY 15000
#define FIREBASE_SEND_DELAY 3000

unsigned long FirebaseSetDelay = 0;     // timer used for non-blocking delay after initial send

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	NOTE: These initial values to these variables are mostly irrelevent. 
//        They are automatically set upon connecting to Firebase! 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
int upLimit = -1;					// Highest value during this cycle (Randomied each dimming cycle)
int downLimit = -1;					// lowest value during this cycle (Randomied each dimming cycle)
int level = -1;						// Actual value being sent to the dimmer circuit
int direction = -1;					// +1 for ramping up, -1 for ramping down

FirebaseData dataSEND;              // Firebase Data object used to send initial data on first-time bootup
FirebaseData firebaseData;          // Firebase Data object
FirebaseAuth auth;                  // FirebaseAuth data for authentication data
FirebaseConfig firebaseConfig;      // FirebaseConfig data for config data

std::string devicePath;             // Firebase path for this flicker device's data

// LaternData contains all the variables to control the flicker effect.
// The default values will allow the latern to work even before it has read the proper 
//   values from Firebase backend.
LanternData Lantern = {
    2, //pin
    150, //maxBrightness
    0, //minBrightness
    1, //smoothing
    1000, //rampDelay
    1000, //dropDelay 
    1,  //dropValue
    10, //flickerDelayMin
    200 //flickerDelayMax
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////	 
//////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//  readLocalData
//
//  Purpose: Attempts to read lantern data from local file. This allows the 
//      flicker modual to immediately begin working before firebase has
//      gotten around to connecting.
////////////////////////////////////////////////////////////////////////////////
void readLocalData()
{
//#define RLD_DEBUG

	if (!SPIFFS.begin()) {
        #ifdef RLD_DEBUG
		    Serial.println("SPIFFS Initialization FAILED.");
        #endif
		return;
	}

	File f = SPIFFS.open(LOCAL_FILE_PATH, "r");
	if (!f) {
        #ifdef RLD_DEBUG
		    Serial.println("Failed to open file.");
        #endif
		return;
	}

	uint8_t* pointer = (uint8_t*) &Lantern;
	while (f.available()) {
		*pointer = f.read();
		++pointer;
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
        Serial.print("rampDelay: "); Serial.println(Lantern.rampDelay);
        Serial.print("dropDelay: "); Serial.println(Lantern.dropDelay);
        Serial.print("dropValue: "); Serial.println(Lantern.dropValue);
        Serial.print("flickerDelayMin: "); Serial.println(Lantern.flickerDelayMin);
        Serial.print("flickerDelayMax: "); Serial.println(Lantern.flickerDelayMax);
    #endif
}

////////////////////////////////////////////////////////////////////////////////
//  setupFirebase
//
//  Purpose: Handles all logic with configuring the firebase library's
//      setup. This includes setting the db url, api key, authentication,
//      steam callback, etc.
////////////////////////////////////////////////////////////////////////////////
void setupFirebase()
{
//#define SF_DEBUG

    #ifdef SF_DEBUG
        Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
    #endif

    firebaseConfig.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

    firebaseConfig.database_url = FIREBASE_DB_URL;
    firebaseConfig.api_key = FIREBASE_API;

    auth.user.email = FIREBASE_USER_EMAIL;
    auth.user.password = FIREBASE_USER_PASS;

    Firebase.reconnectWiFi(true);
    Firebase.setDoubleDigits(5);

    Firebase.begin(&firebaseConfig, &auth);

    //Recommend for ESP8266 stream, adjust the buffer size to match stream data size
    #if defined(ESP8266)
        firebaseData.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);
    #endif

    // First, check if the device has been set up before
    getInitialDeviceData();

    // Once the device has been confirmed set up, start stream
    if (!Firebase.RTDB.beginStream(&firebaseData, devicePath))
       Serial.printf("stream begin error, %s\n\n", firebaseData.errorReason().c_str());

    Firebase.RTDB.setStreamCallback(&firebaseData, streamCallback, streamTimeoutCallback); 
}

////////////////////////////////////////////////////////////////////////////////
//  streamCallback
//
//  Purpose: Handles data received from Firebase. Is in charge of setting new
//      values for the LanternData. 
//
//  Parameters: 
//      StreamData data:
//          Should contain a json object with entries for each value within the
//          LanternData object. On successful read, the new data is written to
//          local data. When null is read, it is assumed this device has not 
//          connected to Firebase before. The default values are sent to be 
//          read back.
////////////////////////////////////////////////////////////////////////////////
void streamCallback(FirebaseStream data)
{
//#define SCB_DEBUG

    #ifdef SCB_DEBUG
        Serial.printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                    data.streamPath().c_str(),
                    data.dataPath().c_str(),
                    data.dataType().c_str(),
                    data.eventType().c_str());
        printResult(data); //see addons/RTDBHelper.h
        Serial.println();

        //This is the size of stream payload received (current and max value)
        //Max payload size is the payload size under the stream path since the stream connected
        //and read once and will not update until stream reconnection takes place.
        //This max value will be zero as no payload received in case of ESP8266 which
        //BearSSL reserved Rx buffer size is less than the actual stream payload.
        Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());
    #endif

    if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json)
    {
        parseJson(data.to<FirebaseJson*>());

        writeLocalData();
    }
    else if (strcmp(data.dataType().c_str(), "null") == 0)
    {
        #ifdef SCB_DEBUG
            Serial.println("No endpoint found on backend. Creating new device... ");
            Serial.println();
        #endif
        sendInitialDeviceData();
    }
    else
    {
        #ifdef SCB_DEBUG
            Serial.printf("Stream returned non-JSON response: %s\n", data.dataType().c_str());
            Serial.println();
        #endif 
    }
}

////////////////////////////////////////////////////////////////////////////////
//  writeLocalData
//
// Purpose: Writes the current values in LanternData to local storage
////////////////////////////////////////////////////////////////////////////////
void writeLocalData()
{
//#define WLD_DEBUG

    if (!SPIFFS.format()) 
    {
        #ifdef WLD_DEBUG
            Serial.println("File System Formatting Failed.");
        #endif
        return; 
    }

    File f = SPIFFS.open(LOCAL_FILE_PATH, "w");
    if (!f) 
    {
        #ifdef WLD_DEBUG
            Serial.println("Failed to open file.");
        #endif
        return;
    }

    f.write(((uint8_t *) &Lantern), sizeof(LanternData));
    f.flush();
    f.close();

    #ifdef WLD_DEBUG
    Serial.println("Finished writing Lantern to local storage.");
    #endif
}

////////////////////////////////////////////////////////////////////////////////
//  streamTimeoutCallback
//
// Purpose: Simple callback to track timeouts to Firebase stream
////////////////////////////////////////////////////////////////////////////////
void streamTimeoutCallback(bool timeout)
{
    if (timeout)
        Serial.println("stream timed out, resuming...\n");

    if (!firebaseData.httpConnected())
    {
        Serial.printf("error code: %d, reason: %s\n\n", firebaseData.httpCode(), firebaseData.errorReason().c_str());
        //ESP.reset();
    }
}

////////////////////////////////////////////////////////////////////////////////
//  sendInitialDeviceData
//
//  Purpose: Sends LanternData to the device's Firebase endpoint. 
//      This should only be done once!
////////////////////////////////////////////////////////////////////////////////
void sendInitialDeviceData()
{
//#define SIDD_DEBUG

    FirebaseJson json;
    json.add("dropDelay", Lantern.dropDelay);
    json.add("dropValue", Lantern.dropValue);
    json.add("rampDelay", Lantern.rampDelay);
    json.add("groupName", "Lanterns");
    json.add("name", String(ESP.getChipId()));
    json.add("maxBrightness", Lantern.maxBrightness);
    json.add("minBrightness", Lantern.minBrightness);
    json.add("smoothing", Lantern.smoothing);
    json.add("pin", Lantern.pin);
    json.add("flickerDelayMin", Lantern.flickerDelayMin);
    json.add("flickerDelayMax", Lantern.flickerDelayMax);

    while (!Firebase.ready()) ;
    
    #ifdef SIDD_DEBUG
        Serial.printf("Sending initial device data ... %s\n\n", Firebase.RTDB.setJSON(&dataSEND, devicePath.c_str(), &json) ? "ok" : dataSEND.errorReason().c_str());
    #endif

    FirebaseSetDelay = millis() + FIREBASE_SEND_DELAY;
}

////////////////////////////////////////////////////////////////////////////////
//  getInitialDeviceData
//
//  Purpose: Attempts to fetch LanternData from Firebase during setup. If a
//      blank Json object is returned, the flicker device has never been set
//      up before and should be initialized before streaming for data changes.
//      This function is only run one time per startup!
////////////////////////////////////////////////////////////////////////////////
void getInitialDeviceData()
{
//#define GTDD_DEBUG

    FirebaseJson initialData;
    if (Firebase.RTDB.getJSON(&firebaseData, devicePath, &initialData))
    {
        #ifdef GTDD_DEBUG
            Serial.println("Received initial device data from Firebase!");
            //Print all object data
            Serial.println((const char *)FPSTR("Pretty printed JSON data:"));
            initialData.toString(Serial, true);
            Serial.println();
            Serial.println((const char *)FPSTR("Iterate JSON data:"));
            Serial.println();
        #endif

        size_t len = initialData.iteratorBegin();

        if (len == 0)
        {
            #ifdef GTDD_DEBUG
                Serial.println("Empty object returned, setting up new device...");
            #endif 
            sendInitialDeviceData();
        }
        else 
        {
            parseJson(&initialData);
        }

        initialData.iteratorEnd();
        initialData.clear();
    }
    else
    {
        #ifdef GTDD_DEBUG
            Serial.println("Failed to get initial device data from Firebase!");
        #endif
    }
}

////////////////////////////////////////////////////////////////////////////////
//  parseJson
//
//  Purpose: Iterates through each value within the json object and calls the
//      helper function "parseValue".
////////////////////////////////////////////////////////////////////////////////
void parseJson(FirebaseJson* json)
{
//#define PJ_DEBUG

    size_t len = json->iteratorBegin();
    FirebaseJson::IteratorValue value;
    for (size_t i = 0; i < len; i++)
    {
        value = json->valueAt(i);
        parseValue(value);
        #ifdef PJ_DEBUG
            Serial.printf((const char *)FPSTR("%d, Type: %s, Name: %s, Value: %s\n"), 
                i, 
                value.type == FirebaseJson::JSON_OBJECT 
                    ? (const char *)FPSTR("object") 
                    : (const char *)FPSTR("array"), 
                value.key.c_str(), 
                value.value.c_str());
        #endif

    }
    json->iteratorEnd();
    json->clear();
}

////////////////////////////////////////////////////////////////////////////////
//  parseValue
//
//  Sets the proper LanterData member value to the passed value.
//      Parameters:
//          IteratorValue value: Contains the key and actual
//              data from Firebase. If the key does not match
//              a known LanternData variable name, it will
//              be skipped.
////////////////////////////////////////////////////////////////////////////////
void parseValue(FirebaseJson::IteratorValue value)
{
//#define PV_DEBUG

    int temp = -1;
    if (value.key == "pin")
        Lantern.pin = std::stoi(value.value.c_str());
    else if (value.key == "maxBrightness")
        Lantern.maxBrightness = std::stoi(value.value.c_str());
    else if (value.key == "minBrightness")
        Lantern.minBrightness = std::stoi(value.value.c_str());
    else if (value.key == "smoothing") {
        temp = std::stoi(value.value.c_str());
        if (temp > 0) Lantern.smoothing = temp;
    }
    else if (value.key == "rampDelay") {
        temp = std::stoi(value.value.c_str());
        if (temp > 0) Lantern.rampDelay = temp;
    }
    else if (value.key == "dropDelay") {
        temp = std::stoi(value.value.c_str());
        if (temp > 0) Lantern.dropDelay = temp;
    }
    else if (value.key == "dropValue") {
        temp = std::stoi(value.value.c_str());
        if (temp > 0) Lantern.dropValue = temp;
    }
    else if (value.key == "flickerDelayMin") {
        temp = std::stoi(value.value.c_str());
        if (temp > 0) Lantern.flickerDelayMin = temp;
    }
    else if (value.key == "flickerDelayMax") {
        temp = std::stoi(value.value.c_str());
        if (temp > 0) Lantern.flickerDelayMax = temp;
    }
    else 
    {
#ifdef PV_DEBUG
        Serial.print("Skipping value...  ");
        Serial.print("TYPE: ");
        Serial.print(value.type == FirebaseJson::JSON_OBJECT ? (const char *)FPSTR("object") : (const char *)FPSTR("array"));
        Serial.print(", KEY: ");
        Serial.println(value.key);
#endif                
    }
}

////////////////////////////////////////////////////////////////////////////////
//  connectToWiFi
//
// Connects to the AP using the given password found in Credentials.h
////////////////////////////////////////////////////////////////////////////////
void connectToWiFi()
{
#define CTW_DEBUG

    WiFi.begin(WIFI_AP_NAME, WIFI_AP_PASS);
    #ifdef CTW_DEBUG
        Serial.print("Connecting to Wi-Fi");
    #endif
    while (WiFi.status() != WL_CONNECTED)
    {
        #ifdef CTW_DEBUG
        Serial.print(".");
        #endif
        delay(300);
    }

    #ifdef CTW_DEBUG
        Serial.println();
        Serial.print("Connected with IP: ");
        Serial.println(WiFi.localIP());
        Serial.println();
    #endif
}

////////////////////////////////////////////////////////////////////////////////
//  setupDevicePath
//
// Gets the uid to find/set this device's firebase path
////////////////////////////////////////////////////////////////////////////////
void setupDevicePath() 
{
//#define SETUP_FF_DEBUG

    char* temp = (char* )malloc(50 * sizeof(char));
	sprintf(temp, "/devices/%lu", ESP.getChipId());
    devicePath = std::string(temp);
    delete[] temp;

    #ifdef SETUP_FF_DEBUG
        Serial.print("DevicePath: ");
        Serial.println(devicePath.c_str());
    #endif
}

// Arduino Setup Function
void setup() 
{
	Serial.begin(115200);
	// Loop through all lamps and set the pinMode
	
	randomSeed(analogRead(A0));

	readLocalData();

    connectToWiFi();

	setupDevicePath();

    setupFirebase();
}

// Arduino Loop Function
void loop()
{
	if (direction > 0) {
		if (level >= upLimit) {
			int instantDropLevel = downLimit + (upLimit - downLimit)/Lantern.dropValue;
			analogWrite(Lantern.pin, instantDropLevel);
			int actualDropDelay = Lantern.dropDelay * (upLimit - instantDropLevel)/Lantern.dropValue;
			
			if (actualDropDelay < 1)
				actualDropDelay = 1;
			if(actualDropDelay > MAX_DROP_DELAY)
				actualDropDelay = MAX_DROP_DELAY;

			level = instantDropLevel;

			upLimit = random(downLimit, Lantern.maxBrightness);
			direction *= -1;
			delayMicroseconds(actualDropDelay);
		}
	} else {
		if (level <= downLimit) {
			downLimit = random(Lantern.minBrightness, upLimit);
			direction *= -1;
			delay(random(Lantern.flickerDelayMin, Lantern.flickerDelayMax));
		}	
	}
	level += direction;
	delayMicroseconds(Lantern.rampDelay);

	analogWrite(Lantern.pin, level);
}
# Halloween Control Panel
The Halloween Control Panel is a passion project I started to help manage several animatronics and other Halloween props for my family's annual Halloween parties. Props are controlled via an Android application that communicates with a Firebase backend and WiFi-connected Arduino devices (primarily ESP8266's).

![Simple Flow Diagram of how the pieces fit together.](https://i.ibb.co/tm8zzZh/Halloween-Control-Panel-Diagram.png)   

## Arduino Light Flicker Code
This particular repo deals with the Arduino code to make lights flicker. Several variables, such as flicker rate, intensity, flicker variability, etc, are controllable from the Android application. 

On first startup, the Arduino device connects to the WiFi AP as hardcoded in the WiFiCreds.h file. It then attempts to reach its unique backend on firebase utilizing the Arduino's UID. Once that initial request returns a null object (signifying no such endpoint exists), it generates the information for a default arduino flicker data structure and sends that to its respective endpoint. Afterwards, it begins flickering and listening for any changes to the newly created endpoint. 

Any android applications will automatically update their UI with the new device added to Firebase.

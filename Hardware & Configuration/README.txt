
*IMPORTANT NOTES*
1) HARDWARE
Atmoscan is built with an integrated custom board but can also be built with standard components if you are willing to give up on battery power, size etc. Specifically the prototype was built with only the following components:
- NodeMCU
- Level shifter

2) FLASHING
As the serial port is occupied by a sensor, programming that way is impractical after the first time. Therefore the sketch supports SYSLOG debugging and OTA updates.
The ATMOSCAN binary is over 700Kb and ArduinoOTA requires the program space to be at least twice that size, which rules out the "4M (3M SPIFFS)" option.
However, the standard "4M (1M SPIFFS)" option is also unsuitable as the SPIFFS partition would be insufficient for the graphical resources related to weather station, plane spotter and for the onfing file.

THE SOLUTION: create a custom configuration "4M (2M SPIFFS)".

To do so:
1) Install ESP8266 Arduino support from Github (not from board manager)
2) Add the lines to boards.txt as specified in the accompanying file "mod to boards.txt"
3) Copy the "eagle.flash.4m2m.ld" file to the location ...Arduino/Hardware/esp8266com/esp8266/tools/sdk/ld/
4) Restart Arduino IDE and under NODEMCU 1.0 a new options will appear "4M (2M SPIFFS)"


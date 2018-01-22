
As the serial port is occupied by a sensor, programming that way is impractical after the first time. Therefore the sketch supports OTA updates.
The ATMOSCAN binary is over 700Kb and ArduinoOTA requires the program space to be  at least twice that size, which rules out the "4M (3M SPIFFS)" option.
However, the standard "4M (1M SPIFFS)" is unsuitable as the SPIFFS partition would be insufficient for the graphical resources related to weather station, plane spotter.

THE SOLUTION: create a custom configuration "4M (2M SPIFFS)".

To do so:
1) Install ESP8266 Arduino support from Github (not from board manager)
2) add the lines to boards.txt as specified in the accompanying file

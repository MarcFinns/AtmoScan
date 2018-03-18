/********************************************************/
/*          ATMOSCAN - Simple OTA flasher               */
/*  Creates an access point and waits for OTA flash     */
/*  To be used on first flash only                      */
/* Author: Marc Finns 2017                              */
/*                                                      */
/********************************************************/

#define FS_NO_GLOBALS
#include <FS.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

void setup()
{
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_AP);
  Serial.println("******** ATMOSCAN FLASHER ********");
  Serial.print("Setting Access point configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Starting Access point ... ");
  Serial.println(WiFi.softAP("BOARD-TO-FLASH") ? "Ready" : "Failed!");

  Serial.print("Board IP address = ");
  Serial.println(WiFi.softAPIP());

  Serial.println("WiFi access point name is: BOARD-TO-FLASH ");

  // Print chip config
  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();

  Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
  Serial.printf("Flash real size: %u\n\n", realSize);

  Serial.printf("Flash ide  size: %u\n", ideSize);
  Serial.printf("Flash ide speed: %u\n", ESP.getFlashChipSpeed());
  Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

  if (ideSize != realSize) {
    Serial.println("Flash Chip configuration wrong!\n");
  } else {
    Serial.println("Flash Chip configuration ok.\n");
  }

  // Format SPIFFS
  Serial.println("Formatting SPIFFS");
  SPIFFS.format();

  // OTA firmware update management

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("BOARD-TO-FLASH");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"123456");

  ArduinoOTA.onStart([]()
  {
    Serial.println("Firmware update");
  });

  ArduinoOTA.onEnd([]()
  {
    Serial.println("OTA complete, rebooting...");
    delay(2000);
    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    // Print OTA progress
    Serial.println("Progress: " + String(progress / (total / 100)) + "% of " + String(total) + " bytes");
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    Serial.println("OTA Update Error[%u]: " + error);
    if (error == OTA_AUTH_ERROR) Serial.println("OTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("OTA Update Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("OTA Update Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA Update Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("OTA Update End Failed");
  });

  ArduinoOTA.begin();

  Serial.println("******** Waiting to be flashed... (Password: 123456");

}

void loop()
{
  // Handle OTA
  ArduinoOTA.handle();
}

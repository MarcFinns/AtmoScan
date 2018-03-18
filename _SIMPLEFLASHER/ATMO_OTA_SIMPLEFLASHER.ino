
#define FS_NO_GLOBALS
#include <FS.h>
#include <ArduinoOTA.h>
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#define SYSLOG_PORT 514


const char* ssid = "GuestNet";
const char* password = "Francesco2011!";
const char* syslog_server = "10.0.1.230";


// UDP instance to let us send and receive packets over UDP
WiFiUDP udpClient;

// WiFiClient wifiClient;

// Create a new empty syslog instance
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

void setup()
{
  Serial.begin (115200);
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(".");

  Serial.println("CONNECTED");

  SPIFFS.format();

  // Prepare syslog configuration
  syslog.server(syslog_server, SYSLOG_PORT);
  syslog.deviceHostname("ESP-FLASHER");
  syslog.appName("ESP-FLASHER");
  syslog.defaultPriority(LOG_KERN);

  syslog.log(LOG_INFO, "******* Booting ESP_FLASHER ");


  // OTA firmware update management

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("BOARD-TO-FLASH");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"123456");

  ArduinoOTA.onStart([]()
  {
    syslog.log(LOG_INFO, "Firmware update");
  });

  ArduinoOTA.onEnd([]()
  {
    syslog.log(LOG_INFO, "OTA complete, rebooting...");
    delay(2000);
    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    // Print OTA progress
    syslog.log(LOG_INFO, "Progress: " + String(progress / (total / 100)) + "%");
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    syslog.log(LOG_INFO, "OTA Update Error[%u]: " + error);
    if (error == OTA_AUTH_ERROR) syslog.log(LOG_INFO, "OTA Auth Failed");
    else if (error == OTA_BEGIN_ERROR) syslog.log(LOG_INFO, "OTA Update Begin Failed");
    else if (error == OTA_CONNECT_ERROR) syslog.log(LOG_INFO, "OTA Update Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) syslog.log(LOG_INFO, "OTA Update Receive Failed");
    else if (error == OTA_END_ERROR) syslog.log(LOG_INFO, "OTA Update End Failed");
  });

  ArduinoOTA.begin();
  syslog.log(LOG_INFO, "Waiting to be flashed..");


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

}

void loop()
{
  // Handle OTA
  ArduinoOTA.handle();

  /////// TEST CODE HERE ///////////
}

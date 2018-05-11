/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson

#include "ScreenSetup.h"
#include "Free_Fonts.h"
#include "GlobalDefinitions.h"
#include "Bitmaps.h"
#include "ArialRoundedMTBold_36.h"
#include "artwork.h"

// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern GfxUi ui;
extern struct Configuration config;
extern String systemID;

bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void ScreenSetup::saveConfigCallback()
{
  shouldSaveConfig = true;
}

void ScreenSetup::activate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenSetup::activate()"));
#endif
  LCD.fillScreen(TFT_BLACK);

  // Print title
  LCD.setFreeFont(&ArialRoundedMTBold_36);
  LCD.setTextDatum(BC_DATUM);
  LCD.setTextColor(TFT_YELLOW, TFT_BLACK);
  LCD.drawString(F("SETUP"), 120, 50);

  ui.drawSeparator(55);

  // If normal mode, show options
  if (config.configValid)
  {
    // Print message
    LCD.setFreeFont(&Dialog_plain_15);

    LCD.setTextColor(TFT_YELLOW, TFT_BLACK);
    LCD.setTextDatum(CL_DATUM);

    // UP
    LCD.fillTriangle(30, 95, // top
                     15, 110, // left
                     45, 110, // right
                     TFT_RED);
    LCD.drawString(F(" RECONFIGURE"), 55, 100);

    // DOWN
    LCD.fillTriangle(30, 170, // top
                     15, 155, // left
                     45, 155, // right
                     TFT_RED);
    LCD.drawString(F(" RESTART "), 55, 160);

    // RIGHT
    LCD.fillTriangle(30, 205, // top
                     45, 220, // middle
                     30, 235, // bottom
                     TFT_RED);

    LCD.drawString(F(" RESUME"), 55, 220);
  }
}

void ScreenSetup::update()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenSetup::update()"));
#endif

  // if not configured, jump directly to hotspot
  if (!config.configValid)
    startHotspot();
}

void ScreenSetup::deactivate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenSetup::deactivate()"));
#endif
}


bool ScreenSetup::onUserEvent(int event)
{

  if (event == GES_CNTRCLOCKWISE || event == GES_LEFT)
  {
    // Cancel event
    return true;
  }
  else if (event == GES_DOWN)
  {
    LCD.setFreeFont(&Dialog_plain_15);
    LCD.setTextDatum(BC_DATUM);
    LCD.setTextColor(TFT_YELLOW, TFT_RED);
    LCD.drawString(F(" Restarting the system "), 120, 300);
    delay(3000);
    ESP.restart();
  }
  else if (event == GES_UP)
  {
    startHotspot();
  }

  // Bubble event
  return false;
}

long ScreenSetup::getRefreshPeriod()
{
  return 5000;
}

String ScreenSetup::getScreenName()
{
  return F("SETUP SCREEN");
}


bool ScreenSetup::isFullScreen()
{
  return true;
}

bool ScreenSetup::getRefreshWithScreenOff()
{
  return false;
}


void  ScreenSetup::startHotspot()

{
  syslog.log(LOG_INFO, F("Starting hotspot"));

  // Clear screen except title
  LCD.fillRect(0, 50, 240, 270, TFT_BLACK);

  // Print message
  LCD.setFreeFont(&Dialog_plain_15);
  LCD.setTextColor(TFT_RED, TFT_BLACK);
  LCD.setTextDatum(BC_DATUM);
  LCD.drawString(F("Connect to Wifi hotspot"), 120, 90);
  LCD.drawString(systemID, 120, 110);

  // Draw hotspot icon
  ui.drawBitmap(hotspot, (LCD.width() - hotspotWidth) / 2, 145, hotspotWidth, hotspotHeight);

  // Position cursor for further messages
  LCD.setTextColor(TFT_YELLOW, TFT_BLACK);
  LCD.setCursor(0, 300);

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "MQTT server", config.mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_topic1("topic1", "MQTT Topic1", config.mqtt_topic1, 64);
  WiFiManagerParameter custom_mqtt_topic2("topic2", "MQTT Topic2", config.mqtt_topic2, 64);
  WiFiManagerParameter custom_mqtt_topic3("topic3", "MQTT Topic3", config.mqtt_topic3, 64);
  WiFiManagerParameter custom_syslog_server("syslog", "Syslog server", config.syslog_server, 20);

  WiFiManagerParameter custom_google_key("Google Key", "Google Key", config.google_key, 64);
  WiFiManagerParameter custom_wunderground_key("WUnderground Key", "WUnderground Key", config.wunderground_key, 64);
  WiFiManagerParameter custom_geonames_user("Geonames user", "Geonames user", config.geonames_user, 32);
  WiFiManagerParameter custom_timezonedb_key("Timezonedb key", "Timezonedb key", config.timezonedb_key, 64);


  //Local intialization
  WiFiManager wifiManager;

  // Dont touch the serial!
  wifiManager.setDebugOutput(false);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(ScreenSetup::saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_topic1);
  wifiManager.addParameter(&custom_mqtt_topic2);
  wifiManager.addParameter(&custom_mqtt_topic3);
  wifiManager.addParameter(&custom_syslog_server);
  wifiManager.addParameter(&custom_google_key);
  wifiManager.addParameter(&custom_wunderground_key);
  wifiManager.addParameter(&custom_geonames_user);
  wifiManager.addParameter(&custom_timezonedb_key);


  // Goes into a blocking loop awaiting configuration
  wifiManager.setConfigPortalTimeout(300);
  wifiManager.startConfigPortal(systemID.c_str());

  //read updated parameters
  strcpy(config.mqtt_server, custom_mqtt_server.getValue());
  strcpy(config.mqtt_topic1, custom_mqtt_topic1.getValue());
  strcpy(config.mqtt_topic2, custom_mqtt_topic2.getValue());
  strcpy(config.mqtt_topic3, custom_mqtt_topic3.getValue());
  strcpy(config.syslog_server, custom_syslog_server.getValue());
  strcpy(config.google_key, custom_google_key.getValue());
  strcpy(config.wunderground_key, custom_wunderground_key.getValue());
  strcpy(config.geonames_user, custom_geonames_user.getValue());
  strcpy(config.timezonedb_key, custom_timezonedb_key.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig)
  {
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json[F("mqtt_server")] = config.mqtt_server;
    json[F("mqtt_topic1")] = config.mqtt_topic1;
    json[F("mqtt_topic2")] = config.mqtt_topic2;
    json[F("mqtt_topic3")] = config.mqtt_topic3;
    json[F("syslog_server")] = config.syslog_server;
    json[F("google_key")] = config.google_key;
    json[F("wunderground_key")] = config.wunderground_key;
    json[F("geonames_user")] = config.geonames_user;
    json[F("timezonedb_key")] = config.timezonedb_key;  

    fs::File configFile = SPIFFS.open(F("/config.json"), "w");
    if (configFile)
    {
      // Save configuration to file
      json.printTo(configFile);
      configFile.close();
      LCD.println(F("Configuration saved"));
    }
    else
      LCD.println(F("Could NOT save configuration"));

  }

  // Reboot
  LCD.setTextColor(TFT_YELLOW, TFT_RED);
  LCD.println(F(" Restarting the system "));
  delay(2000);
  ESP.restart();
}

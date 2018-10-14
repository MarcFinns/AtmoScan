/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI

#include "ScreenStatus.h"
#include "ESP8266WiFi.h"
#include "GlobalDefinitions.h"
#include "P_AirSensors.h"
#include "Free_Fonts.h"
#include "Fonts.h"

// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern struct ProcessContainer procPtr;
extern struct Configuration config;
extern String systemID;

void ScreenStatus::activate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenStatus::activate()"));
#endif

  LCD.fillScreen(TFT_BLACK);

  LCD.setTextColor(TFT_YELLOW, TFT_BLACK);
  LCD.setFreeFont(&Dialog_plain_13);

  LCD.setTextDatum(TC_DATUM);
  LCD.drawString(systemID, 120, 68, GFXFF);

  LCD.setTextDatum(TL_DATUM);

  int xpos = 0;
  int ypos = 83;
  int lineSpacing = LCD.fontHeight(GFXFF) - 2;

  LCD.drawString(F("Version"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;

  LCD.drawString(F("Built"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;

  LCD.drawString(F("UpTime"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Free"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Charge"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Volt"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Temp"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("WiFi"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Net"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("IP"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Syslog"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("MQTT"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Topic1"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Topic2"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Topic3"), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Updated"), xpos, ypos, GFXFF);
}

void ScreenStatus::update()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenStatus::update()"));
#endif

  LCD.setTextDatum(TL_DATUM);
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);
  LCD.setFreeFont(&Dialog_plain_13);

  int xpos = 75;
  int ypos = 83;
  int lineSpacing = LCD.fontHeight(GFXFF) - 2;

  LCD.drawString(F(ATMOSCAN_VERSION), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(__DATE__ " " __TIME__), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(procPtr.UIManager.upTime(), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(ESP.getFreeHeap()) + F(" Bytes    "), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(procPtr.UIManager.getSoC(), 0) + F("%    "), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(procPtr.UIManager.getVolt()) + F(" V    "), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(procPtr.ComboPressureHumiditySensor.getTemperature()) + F(" C   "), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(config.connected ? String(WiFi.RSSI()) + F(" dbm   ") : String(F("                ")), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(config.connected ? String(WiFi.SSID())  : String(F("                ")), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(config.connected ? String(WiFi.localIP().toString()) : String(F("                ")) , xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(config.syslog_server), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(config.mqtt_server), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(config.mqtt_topic1), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(config.mqtt_topic2), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(config.mqtt_topic3), xpos, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(String(procPtr.MQTTUpdate.getLastMqttUpdate()), xpos, ypos, GFXFF);
}

void ScreenStatus::deactivate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenStatus::deactivate()"));
#endif
}


bool ScreenStatus::onUserEvent(int event)
{
  return false;
}

long ScreenStatus::getRefreshPeriod()
{
  return 3000;
}

String ScreenStatus::getScreenName()
{
  return F("System status");
}

bool ScreenStatus::isFullScreen()
{
  return false;
}

bool ScreenStatus::getRefreshWithScreenOff()
{
  return false;
}

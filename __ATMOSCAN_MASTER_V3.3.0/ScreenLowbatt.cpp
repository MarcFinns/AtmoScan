/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


#include <NtpClientLib.h>         // https://github.com/gmag11/NtpClient
#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI

#include "ScreenLowbatt.h"
#include "Free_Fonts.h"
#include "Fonts.h"
#include "GfxUi.h"                // Additional UI functions
#include "GlobalDefinitions.h"


// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern GfxUi ui;

// Prototypes
#ifdef KILL_INSTALLED
void turnOff();
#endif

void ScreenLowbatt::activate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenLowbatt::activate()"));
#endif

  LCD.fillScreen(TFT_BLACK);
  LCD.setTextDatum(BC_DATUM);
  LCD.setFreeFont(&ArialRoundedMTBold_14);
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);

  // Print shutdown time
  char timeNow[10];
  sprintf(timeNow, "%d:%02d", hour(), minute());
  LCD.drawString(String(F("Shutdown at ")) + String(timeNow), 120, 280, GFXFF);

  // Show low battery graphics
  ui.drawBitmap(batteryLowIcon, (LCD.width() - batteryLowWidth) / 2, 60, batteryLowWidth, batteryLowHeight);

  syslog.log(LOG_CRIT, F("***** LOW BATTERY - SYSTEM HALTED"));

#ifndef KILL_INSTALLED
  // Turn off Wifi
  delay(1000);
  WiFi.mode(WIFI_OFF);
#endif

}

void ScreenLowbatt::update()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenLowbatt::update()"));
#endif

#ifdef KILL_INSTALLED
  syslog.log(LOG_CRIT, F("***** LOW BATTERY - SHUTTING DOWN"));
  LCD.drawString(F("TURNING OFF "), 120, 300, GFXFF);
  delay(3000);
  turnOff();
#endif
}

void ScreenLowbatt::deactivate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenLowbatt::deactivate()"));
#endif
}

bool ScreenLowbatt::onUserEvent(int event)
{
  // Cancel any user event - no further processing
  return true;
}

long ScreenLowbatt::getRefreshPeriod()
{
  return 5000;
}

String ScreenLowbatt::getScreenName()
{
  return F("Low Battery");
}

bool ScreenLowbatt::isFullScreen()
{
  return true;
}


bool ScreenLowbatt::getRefreshWithScreenOff()
{
  return false;
}

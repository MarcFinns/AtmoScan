/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


#include <RingBufCPP.h>           //https://github.com/wizard97/Embedded_RingBuf_CPP
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog

#include "ScreenErrLog.h"
#include "ESP8266WiFi.h"
#include "GlobalDefinitions.h"
#include "Free_Fonts.h"
#include "Fonts.h"

// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern struct ProcessContainer procPtr;
extern struct Configuration config;
extern String systemID;
extern RingBufCPP<String, 18> lastErrors;

void ScreenErrLog::activate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenErrLog::activate()"));
#endif

  LCD.fillScreen(TFT_BLACK);

  LCD.setTextColor(TFT_YELLOW, TFT_BLACK);
  LCD.setFreeFont(&Dialog_plain_13);

  LCD.setTextDatum(TC_DATUM);
  LCD.drawString(F("Last error events"), 120, 68, GFXFF);

}

void ScreenErrLog::update()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenErrLog::update()"));
#endif

  LCD.setTextDatum(TL_DATUM);
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);
  LCD.setFreeFont(&Dialog_plain_9);
  LCD.setTextWrap(true);
  LCD.setCursor(0, 100);

  // Clear print area
  LCD.fillRect(0, 80, 240, 240, TFT_BLACK);

  // Loop through all existing elements (if any)
  for ( int x = lastErrors.numElements(); x > 0; x--)
  {
    // Retrieve element
    String msg = *lastErrors.peek(x - 1);

    // Print it
    LCD.println(msg);
  }
}

void ScreenErrLog::deactivate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenErrLog::deactivate()"));
#endif
}


bool ScreenErrLog::onUserEvent(int event)
{
  return false;
}

long ScreenErrLog::getRefreshPeriod()
{
  return 3000;
}

String ScreenErrLog::getScreenName()
{
  return F("Error Log");
}

bool ScreenErrLog::isFullScreen()
{
  return false;
}

bool ScreenErrLog::getRefreshWithScreenOff()
{
  return false;
}

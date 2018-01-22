
#include "ScreenGeiger.h"
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI
#include "Free_Fonts.h"
#include "ArialRoundedMTBold_14.h"
#include "GlobalDefinitions.h"
#include "LogChart.h"
#include "AnalogMeter.h"


// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern struct ProcessContainer procPtr;


ScreenGeiger::ScreenGeiger()
  :  analogMeter(64, 4, 30, 1000, "Count/min", "CPM"),
     logChart(200, 118, 4) {};

void ScreenGeiger::activate()
{
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("ScreenGeiger::activate()"));
#endif

  // Clear screnn
  LCD.fillScreen(TFT_BLACK);

  // Draw gauge
  analogMeter.begin();

  // Draw chart
  logChart.begin();

}

void ScreenGeiger::update()
{
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("ScreenGeiger::update()"));
#endif

  logChart.drawPoint(procPtr.GeigerSensor.getCPM());
  analogMeter.drawNeedle(procPtr.GeigerSensor.getCPM());

}

void ScreenGeiger::deactivate()
{
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("ScreenGeiger::deactivate()"));
#endif

}


bool ScreenGeiger::onUserEvent(int event)
{
  return false;
}

long ScreenGeiger::getRefreshPeriod()
{
  return FAST_SAMPLE_PERIOD;
}

String ScreenGeiger::getScreenName()
{
  return F("GEIGER COUNTER");
}

bool ScreenGeiger::isFullScreen()
{
  return false;
}

bool ScreenGeiger::getRefreshWithScreenOff()
{
  return true;
}



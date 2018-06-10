/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI

#include "ScreenSensors.h"
#include "GlobalDefinitions.h"
#include "P_AirSensors.h"
#include "Free_Fonts.h"
#include "Artwork.h"


// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern GfxUi ui;
extern struct ProcessContainer procPtr;

void ScreenSensors::activate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, "ScreenSensors::activate()");
#endif

  LCD.fillScreen(TFT_BLACK);
  LCD.setTextDatum(TL_DATUM);
  LCD.setTextColor(TFT_YELLOW, TFT_BLACK);
  // LCD.setFreeFont(FM9);                 // Select the font
  //LCD.setFreeFont(FSSB9);
  LCD.setFreeFont(&Dialog_plain_15);
  int xpos = 5;
  int ypos = 75;

  LCD.drawString(F("Temp"), xpos, ypos);

  ypos +=  LCD.fontHeight(GFXFF);
  LCD.drawString(F("Humid"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  LCD.drawString(F("Press"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  ui.drawSeparator(ypos);
  ypos +=  LCD.fontHeight(GFXFF) / 2;

  LCD.drawString(F("CO2"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  LCD.drawString(F("CO"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  LCD.drawString(F("NO2"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  LCD.drawString(F("VOC"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  ui.drawSeparator(ypos);
  ypos +=  LCD.fontHeight(GFXFF) / 2;

  LCD.drawString(F("PM01"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  LCD.drawString(F("PM2.5"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  LCD.drawString(F("PM10"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  ui.drawSeparator(ypos);
  ypos +=  LCD.fontHeight(GFXFF) / 2;

  LCD.drawString(F("CPM"), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  LCD.drawString(F("Rad"), xpos, ypos, GFXFF);

}

void ScreenSensors::update()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenSensors::update()"));
#endif

  LCD.setTextDatum(TL_DATUM);
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);
  //  LCD.setFreeFont(FM9);                 // Select the font
  //LCD.setFreeFont(FSS9);
  LCD.setFreeFont(&Dialog_plain_15);

  float currValue;

  int xpos = 75;
  int ypos = 75;

  // TEMPERATURE
  if (procPtr.ComboTemperatureHumiditySensor.isEnabled())
    printWithTrend(lastTemperatureColor, lastTemperature, procPtr.ComboTemperatureHumiditySensor.getTemperature(), F(" C  "), 1 , xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

  // HUMIDITY
  ypos +=  LCD.fontHeight(GFXFF);
  if (procPtr.ComboTemperatureHumiditySensor.isEnabled())
    printWithTrend(lastHumidityColor, lastHumidity, procPtr.ComboTemperatureHumiditySensor.getHumidity() , F(" % "), 1, xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

  // PRESSURE
  ypos +=  LCD.fontHeight(GFXFF);
  if (procPtr.ComboPressureHumiditySensor.isEnabled())
    printWithTrend(lastPressureColor, lastPressure, procPtr.ComboPressureHumiditySensor.getPressure() , F(" hPa "), 1, xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  ui.drawSeparator(ypos);
  ypos +=  LCD.fontHeight(GFXFF) / 2;

  // CO2
  if (procPtr.CO2Sensor.isEnabled())
    printWithTrend(lastCO2Color, lastCO2, procPtr.CO2Sensor.getCO2() , F(" ppm     "), 0,  xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

  // CO
  ypos +=  LCD.fontHeight(GFXFF);
  if (procPtr.MultiGasSensor.isEnabled())
    printWithTrend(lastCOColor, lastCO , procPtr.MultiGasSensor.getCO() , F(" ppm   "), 2,  xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

  // NO2
  ypos +=  LCD.fontHeight(GFXFF);
  if (procPtr.MultiGasSensor.isEnabled())
    printWithTrend(lastNO2Color, lastNO2 , procPtr.MultiGasSensor.getNO2() , F(" ppm  "), 2,  xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

  // VOC
  ypos +=  LCD.fontHeight(GFXFF);
  if (procPtr.VOCSensor.isEnabled())
    printWithTrend(lastVOCColor, lastVOC , procPtr.VOCSensor.getVOC() , F("         "), 0,  xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);


  ypos +=  LCD.fontHeight(GFXFF);
  ui.drawSeparator(ypos);
  ypos +=  LCD.fontHeight(GFXFF) / 2;

  // PM01
  if (procPtr.ParticleSensor.isEnabled())
    printWithTrend(lastPM01Color, lastPM01 , procPtr.ParticleSensor.getPM01(), F(" ug/m3   "), 0,  xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

  // PM25
  ypos +=  LCD.fontHeight(GFXFF);
  if (procPtr.ParticleSensor.isEnabled())
    printWithTrend(lastPM2_5Color, lastPM2_5 , procPtr.ParticleSensor.getPM2_5() , F(" ug/m3   "), 0,  xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

  // PM10
  ypos +=  LCD.fontHeight(GFXFF);
  if (procPtr.ParticleSensor.isEnabled())
    printWithTrend(lastPM10Color, lastPM10 , procPtr.ParticleSensor.getPM10() , F(" ug/m3   "), 0,  xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

  ypos +=  LCD.fontHeight(GFXFF);
  ui.drawSeparator(ypos);
  ypos +=  LCD.fontHeight(GFXFF) / 2;

  // CPM
  if (procPtr.GeigerSensor.isEnabled())
    printWithTrend(lastCPMColor, lastCPM , procPtr.GeigerSensor.getCPM() , F(" Counts        "), 0,  xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);


  // RADIATION
  ypos +=  LCD.fontHeight(GFXFF);
  if (procPtr.GeigerSensor.isEnabled())
    printWithTrend(lastRadiationColor, lastRadiation , procPtr.GeigerSensor.getRadiation() , F(" uSv/h     "), 2,  xpos, ypos);
  else
    LCD.drawString(F("------                   "), xpos, ypos, GFXFF);

}



void ScreenSensors::printWithTrend(int &lastColor, float &lastValue, float newValue, String suffix, int decimals, int xpos, int ypos)
{
  // Decide new color
  if (lastValue != -1)
    if (newValue < lastValue)
      lastColor = TFT_GREEN;
    else if (newValue > lastValue)
      lastColor = TFT_RED;

  // NOTE: If value constant, dont change color (show past trent)

  // Set color
  LCD.setTextColor(lastColor, TFT_BLACK);

  // Print value
  LCD.drawString(" " + String(newValue, decimals) + suffix, xpos, ypos, GFXFF);

  // Remember last value
  lastValue = newValue;

  // Reset color to white
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);
}

void ScreenSensors::deactivate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenSensors::deactivate()"));
#endif
}


bool ScreenSensors::onUserEvent(int event)
{
  return false;
}

long ScreenSensors::getRefreshPeriod()
{
  return 5000;
}

String ScreenSensors::getScreenName()
{
  return F("Sensor Data");
}

bool ScreenSensors::isFullScreen()
{
  return false;
}

bool ScreenSensors::getRefreshWithScreenOff()
{
  return false;
}

#pragma once

#include "Screen.h"
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI

// Screen Handler definition
class ScreenSensors: public Screen
{
  public:
    // Call the Process constructor
    ScreenSensors() {}
    virtual ~ScreenSensors() {}
    virtual void activate();
    virtual void update();
    virtual void deactivate();
    virtual bool onUserEvent(int event);
    virtual long getRefreshPeriod();
    virtual String getScreenName();
    virtual bool isFullScreen();
    virtual bool getRefreshWithScreenOff();

  private:

    void printWithTrend(int &lastColor, float &lastValue, float newValue, String suffix, int decimals, int xpos, int ypos);
    float lastTemperature = -1;
    float lastHumidity = -1;
    float lastPressure = -1;
    float lastCO2 = -1;
    float lastCO = -1;
    float lastNO2 = -1;
    float lastVOC = -1;
    float lastPM01 = -1;
    float lastPM2_5 = -1;
    float lastPM10 = -1;
    float lastCPM = -1;
    float lastRadiation = -1;

    int  lastTemperatureColor = TFT_WHITE;
    int  lastHumidityColor = TFT_WHITE;
    int  lastPressureColor = TFT_WHITE;
    int  lastCO2Color = TFT_WHITE;
    int  lastCOColor = TFT_WHITE;
    int  lastNO2Color = TFT_WHITE;
    int  lastVOCColor = TFT_WHITE;
    int  lastPM01Color = TFT_WHITE;
    int  lastPM2_5Color = TFT_WHITE;
    int  lastPM10Color = TFT_WHITE;
    int  lastCPMColor = TFT_WHITE;
    int  lastRadiationColor = TFT_WHITE;

};




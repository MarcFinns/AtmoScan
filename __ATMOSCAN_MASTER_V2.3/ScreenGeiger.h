/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include "Screen.h"
#include "LogChart.h"
#include "AnalogMeter.h"

// Screen Handler definition
class ScreenGeiger: public Screen
{
  public:
    ScreenGeiger() ;
    virtual ~ScreenGeiger() {}
    virtual void activate();
    virtual void update();
    virtual void deactivate();
    virtual bool onUserEvent(int event);
    virtual long getRefreshPeriod();
    virtual String getScreenName();
    virtual bool isFullScreen();
    virtual bool getRefreshWithScreenOff();

  private:
    AnalogMeter analogMeter;
    LogChart logChart;
};



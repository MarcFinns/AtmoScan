/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

//#include <Arduino.h>
#include "Screen.h"


// Screen Handler definition
class ScreenErrLog: public Screen
{
  public:
    ScreenErrLog() {}
    virtual ~ScreenErrLog() {}
    virtual void activate();
    virtual void update();
    virtual void deactivate();
    virtual bool onUserEvent(int event);
    virtual long getRefreshPeriod();
    virtual String getScreenName();
    virtual bool isFullScreen();
    virtual bool getRefreshWithScreenOff();
};

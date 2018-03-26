/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include "Screen.h"

// Screen Handler definition
class ScreenLowbatt: public Screen
{
  public:
    // Call the Process constructor
    ScreenLowbatt() {}
    virtual ~ScreenLowbatt() {}
    virtual void activate();
    virtual void update();
    virtual void deactivate();
    virtual bool onUserEvent(int event);
    virtual long getRefreshPeriod();
    virtual String getScreenName();
    virtual bool isFullScreen();
    virtual bool getRefreshWithScreenOff();
};



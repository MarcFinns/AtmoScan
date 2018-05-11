/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once
#include "Arduino.h"

// Screen Handler definition
class Screen
{
  public:
    // Call the Process constructor
    Screen() {}
    virtual ~Screen() {}
    virtual void activate() = 0;
    virtual void update() = 0;
    virtual void deactivate() = 0;
    virtual bool onUserEvent(int event) = 0;  // returned bool is cancelEvent (if screen consumed the event)
    virtual long getRefreshPeriod() = 0;
    virtual bool getRefreshWithScreenOff() = 0;
    virtual String getScreenName() = 0;
    virtual bool isFullScreen() = 0;
    long lastUpdate = 0;
};






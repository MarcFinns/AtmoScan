#pragma once

#include "Screen.h"


// Screen Handler definition
class ScreenStatus: public Screen
{
  public:
    // Call the Process constructor
    ScreenStatus() {}
    virtual ~ScreenStatus() {}
    virtual void activate();
    virtual void update();
    virtual void deactivate();
    virtual bool onUserEvent(int event);
    virtual long getRefreshPeriod();
    virtual String getScreenName();
    virtual bool isFullScreen();
    virtual bool getRefreshWithScreenOff();
};





#pragma once

#include "Screen.h"

// Screen Handler definition
class ScreenSetup: public Screen
{
  public:
    // Call the Process constructor
    ScreenSetup() {}
    virtual ~ScreenSetup() {}
    virtual void activate();
    virtual void update();
    virtual void deactivate();
    virtual bool onUserEvent(int event);
    virtual long getRefreshPeriod();
    virtual String getScreenName();
    virtual bool isFullScreen();
    virtual bool getRefreshWithScreenOff();

  private:
    static void saveConfigCallback();
    void  startHotspot();
};



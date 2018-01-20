#pragma once

#include "Screen.h"

// Screen Handler definition
class ScreenLowbatt: public Screen
{
  public:
    // Call the Process constructor
    ScreenLowbatt() {
      //-#ifdef DEBUG_SERIAL Serial.println("ScreenLowbatt:Constructor");
    }
    virtual ~ScreenLowbatt() {
      //-#ifdef DEBUG_SERIAL Serial.println("ScreenLowbatt:Destructor");
    }
    virtual void activate();
    virtual void update();
    virtual void deactivate();
    virtual bool onUserEvent(int event);
    virtual long getRefreshPeriod();
    virtual String getScreenName();
    virtual bool isFullScreen();
    virtual bool getRefreshWithScreenOff();
};



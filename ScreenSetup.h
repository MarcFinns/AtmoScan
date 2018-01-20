
#pragma once

#include "Screen.h"

// Screen Handler definition
class ScreenSetup: public Screen
{
  public:
    // Call the Process constructor
    ScreenSetup() {
      //-#ifdef DEBUG_SERIAL Serial.println("ScreenSetup:Constructor");
    }
    virtual ~ScreenSetup() {
      //-#ifdef DEBUG_SERIAL Serial.println("ScreenSetup:Destructor");
    }
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



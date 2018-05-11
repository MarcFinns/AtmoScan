/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include <ProcessScheduler.h>
#include <libpaj7620.h>           // https://github.com/MarcFinns/Gesture_PAJ7620
#include <MAX17043.h>             // https://github.com/MarcFinns/ArduinoLib_MAX17043
#include <Average.h>              // https://github.com/MajenkoLibraries/Average

#include "ScreenFactory.h"
#include "GfxUi.h"      // Additional UI functions

struct TopBar
{
  String dateLine;
  String timeLine;
  String locationLine;
  int batLevel = 0;
  int dBm = 0;
};


// UI Process definition
class Proc_UIManager : public Process
{
  public:
    Proc_UIManager(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations);
    void onGesture();
    bool eventPending();
    static void onGestureISR();
    void displayOn();
    void displayOff();
    bool initDisplay();
    bool isDisplayOn = false;
    String getCurrentScreenName();
    String upTime();
    void communicationsFlag(bool commOngoing);

    // Battery gauge
    float getVolt();
    float getSoC();
    float getNativeSoC();
    String batteryStats();

  protected:
    virtual void setup();
    virtual void service();

  private:
    // properties
    volatile bool eventFlag;
    volatile unsigned long eventTime = 0;
    unsigned long lastEventProcessing = 0;
    int currentScreenID = 0;
    int currentScreenRotation = 2;
    Screen * currentScreen;
    PAJ7620U gestureSensor;
    MAX17043 batteryMonitor;
    Average<float> avgSOC;
    Average<float> avgVolt;

    bool displayInitialized;
    static Proc_UIManager * instance;
    TopBar topBar;
    bool initSuccess = false;

    // methods
    int getUserEvent();
    int handleSwipe(int evt, int curScrn);
    void initScreen();
    void drawBar(bool forceDraw = false);
    void drawSeparator(uint16_t y);
    void drawBatteryGauge(int topX, int topY, int level, int redLevel, bool forceDraw);
    void drawWifiGauge(int topX, int topY, int rssi, bool forceDraw);
    bool initGesture();
    void batterySetup();
    String printDigits(int digits);
};




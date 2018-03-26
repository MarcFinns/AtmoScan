/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include "Screen.h"
#include "WebResource.h"  // Download helper
#include "WundergroundClient.h"

// Screen Handler definition
class ScreenWeatherStation: public Screen
{
  public:
    // Call the Process constructor
    ScreenWeatherStation();
    virtual ~ScreenWeatherStation() {};
    virtual void activate();
    virtual void update();
    virtual void deactivate();
    virtual bool onUserEvent(int event);
    virtual long getRefreshPeriod();
    virtual String getScreenName();
    virtual bool isFullScreen();
    virtual bool getRefreshWithScreenOff();


  private:
    void downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal);
    ProgressCallback _downloadCallback;
    void downloadResources();
    void updateData();
    void drawProgress(uint8_t percentage, String text);
    void drawCurrentWeather();
    void drawForecast();
    void drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex);
    String getMeteoconIcon(String iconText);
    void drawAstronomy();


    // properties

    bool firstRun = true;
    // WebResource webResource;
    ///WundergroundClient wunderground;
    //long lastDownloadUpdate;
    // float lat, lon;
    long lastDrew = 0;
    bool isInitialised = false;

};



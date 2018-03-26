/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include "Screen.h"
#include "AdsbExchangeClient.h"
#include "GeoMap.h"
#include "PlaneSpotter.h"
#include "GlobalDefinitions.h"


// External variables
extern TFT_eSPI LCD;
extern struct Configuration config;

// Screen Handler definition
class ScreenPlaneSpotter: public Screen
{
  public:
    // Call the Process constructor
    ScreenPlaneSpotter(): geoMap(MapProvider::Google, config.google_key, MAP_WIDTH, MAP_HEIGHT),
      planeSpotter(&LCD, &geoMap) {};
    virtual ~ScreenPlaneSpotter() {};
    virtual void activate();
    virtual void update();
    virtual void deactivate();
    virtual bool onUserEvent(int event);
    virtual long getRefreshPeriod();
    virtual String getScreenName();
    virtual bool isFullScreen();
    virtual bool getRefreshWithScreenOff();
    bool isInitialised = false;

  private:
    //PlaneSpotter* planeSpotter;
    //AdsbExchangeClient* adsbClient;
    // GeoMap* geoMap;

    GeoMap geoMap;
    PlaneSpotter planeSpotter;
    Coordinates mapCenter;
    Coordinates northWestBound;
    Coordinates southEastBound;
};



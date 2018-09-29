/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


//#include <ESP8266WiFi.h>
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI

#include "ScreenPlaneSpotter.h"
#include "Free_Fonts.h"
#include "Fonts.h"

// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern struct Configuration config;
extern struct ProcessContainer procPtr;
extern GfxUi ui;

// Prototypes
void setTurbo(bool setTurbo);
bool isTurbo();
void errLog(String msg);

//const String QUERY_STRING PROGMEM = "fAltL=1500&trFmt=sa";

void ScreenPlaneSpotter::activate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenPlaneSpotter::activate()"));
#endif

  LCD.fillScreen(TFT_BLACK);
  LCD.setFreeFont(&ArialRoundedMTBold_14);
  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);

  // Only works connected!!
  if (!procPtr.GeoLocation.isValid() || !config.connected)
  {
    LCD.setTextDatum(TC_DATUM);
    LCD.drawString(F("Plane Spotter needs"), 120, 120, GFXFF);
    LCD.drawString(F("network & geolocation"), 120, 140, GFXFF);
    isInitialised = false;
    return;
  }

  // Set center of the map by using system coordinates
  mapCenter.lat = procPtr.GeoLocation.getLatitude();
  mapCenter.lon = procPtr.GeoLocation.getLongitude();

  // Draw Planespotter Splash Screen but only if map is not loaded yet
  if (!geoMap.setMap(mapCenter, MAP_ZOOM))
  {
    //  TURBO mode
    setTurbo(true);

    // Display splash screen
    JpegDec.decodeArray(plane_splash, plane_splash_len);
    ui.renderJPEG(0, 100);

    //  NORMAL mode
    setTurbo(false);

    LCD.setTextDatum(BC_DATUM);
    LCD.setTextColor(TFT_WHITE, TFT_BLACK);
    LCD.drawString(F("Adapted: MarcFinns"), 120, 240);


    LCD.setTextDatum(TC_DATUM);
    LCD.setTextColor(TFT_ORANGE, TFT_BLACK);
    LCD.drawString(F("Loading map..."), 120, 280, 1 );

    geoMap.downloadMap();
  }

  // NOTE: clipping on top by 15 pixels, not to dirty the upper bar...
  northWestBound = geoMap.convertToCoordinates({0, 15});
  southEastBound = geoMap.convertToCoordinates({MAP_WIDTH, MAP_HEIGHT - 15});

  // Draw map
  ui.drawJpeg(geoMap.getMapName(), 0, TOP_BAR_HEIGHT);

  LCD.fillRect(0, geoMap.getMapHeight() + TOP_BAR_HEIGHT, LCD.width(), LCD.height() - geoMap.getMapHeight() - TOP_BAR_HEIGHT, TFT_BLACK);

  isInitialised = true;

}

void ScreenPlaneSpotter::update()
{

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenPlaneSpotter::update()"));
  long startMillis = millis();
#endif

  // Only works connected!!
  if ( !config.connected || !isInitialised)
    return;

  //local variable for test
  AdsbExchangeClient * adsbClient;

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("1 - START UPDATING ADSB = ")) + String(ESP.getFreeHeap()) + F(" bytes"));
#endif

  adsbClient = new AdsbExchangeClient();

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("2 - AFTER INSTANCIATING ADSBCLIENT = ")) + String(ESP.getFreeHeap()) + F(" bytes"));
#endif

  String queryString = F("fAltL=1500&trFmt=sa");
  adsbClient->updateVisibleAircraft(queryString +
                                    F("&lat=") +
                                    String(mapCenter.lat, 6) +
                                    F("&lng=") + String(mapCenter.lon, 6) +
                                    F("&fNBnd=") + String(northWestBound.lat, 9) +
                                    F("&fWBnd=") + String(northWestBound.lon, 9) +
                                    F("&fSBnd=") + String(southEastBound.lat, 9) +
                                    F("&fEBnd=") + String(southEastBound.lon, 9));


#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("3 - AFTER CALL TO ADSBCLIENT = ")) + String(ESP.getFreeHeap()) + F(" bytes"));
#endif

  Aircraft closestAircraft = adsbClient->getClosestAircraft(mapCenter.lat, mapCenter.lon);

  // Before refreshing display, check if a userEvent is pending and skip in case
  if ( !procPtr.UIManager.eventPending())
  {

    // Draw map
    ui.drawJpeg(geoMap.getMapName(), 0, TOP_BAR_HEIGHT);

    // Get aircrafts data
    for (int i = 0; i < adsbClient->getNumberOfAircrafts(); i++)
    {
      Aircraft aircraft = adsbClient->getAircraft(i);
      AircraftHistory history = adsbClient->getAircraftHistory(i);
      planeSpotter.drawAircraftHistory(aircraft, history);
      planeSpotter.drawPlane(aircraft, aircraft.call == closestAircraft.call);
    }

    // Draf info of closest aircraft
    if (adsbClient->getNumberOfAircrafts())
    {
      // YES - print infobox of the closes
      planeSpotter.drawInfoBox(closestAircraft);
    }
    else
    {
      // NO - erase infobox
      LCD.fillRect(0, geoMap.getMapHeight() + TOP_BAR_HEIGHT, LCD.width(), LCD.height() - geoMap.getMapHeight() - TOP_BAR_HEIGHT, TFT_BLACK);
    }

    // Draw center of map
    CoordinatesPixel p = geoMap.convertToPixel(mapCenter);
    LCD.fillCircle(p.x, p.y, 2, TFT_BLUE);

  }
  else
  {
#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, String(F(" USER EVENT detected, aborting rendering after ")) + String(millis() - startMillis));
#endif
  }

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("4 - AFTER DRAWING = ")) + String(ESP.getFreeHeap()) + F(" bytes"));
#endif

  // Free up memory
  delete adsbClient;

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("5 - AFTER CLEANUP = ")) + String(ESP.getFreeHeap()) + F(" bytes"));

  syslog.log(LOG_DEBUG, String(F("Rendering took (mS) ")) + String(millis() - startMillis));
#endif

}

void ScreenPlaneSpotter::deactivate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenPlaneSpotter::deactivate()"));
#endif

  // delete geoMap;
  // delete planeSpotter;

}


bool ScreenPlaneSpotter::onUserEvent(int event)
{
  return false;
}

long ScreenPlaneSpotter::getRefreshPeriod()
{
  return 1500;
}

String ScreenPlaneSpotter::getScreenName()
{
  return F("Plane Spotter");
}

bool ScreenPlaneSpotter::isFullScreen()
{
  return false;
}

bool ScreenPlaneSpotter::getRefreshWithScreenOff()
{
  return false;
}

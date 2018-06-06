/**The MIT License (MIT)

  Copyright (c) 2015 by Daniel Eichhorn

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  See more at https://blog.squix.org


*/

#include <SPI.h>

// Go to settings to change important parameters
#include "ScreenPlaneSpotterSettings.h"
#include "PlaneSpotter.h"
#include "Artwork.h"


PlaneSpotter::PlaneSpotter(TFT_eSPI* tft, GeoMap* geoMap) {
  tft_ = tft;
  geoMap_ = geoMap;

}


void PlaneSpotter::drawAircraftHistory(Aircraft aircraft, AircraftHistory history)
{
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("PlaneSpotter::drawAircraftHistory"));
#endif

  Coordinates lastCoordinates;
  lastCoordinates.lat = aircraft.lat;
  lastCoordinates.lon = aircraft.lon;
  for (int j = 0; j < min(history.counter, MAX_HISTORY); j++)
  {

    AircraftPosition position = history.positions[j];
    Coordinates coordinates = position.coordinates;
    CoordinatesPixel p1 = geoMap_->convertToPixel(coordinates);

    CoordinatesPixel p2 = geoMap_->convertToPixel(lastCoordinates);
    uint16_t color = heightPalette_[min(position.altitude / 4000, 9)];
    tft_->drawLine(p1.x, p1.y, p2.x, p2.y, color);
    tft_->drawLine(p1.x + 1, p1.y + 1, p2.x + 1, p2.y + 1, color);

    lastCoordinates = coordinates;
  }
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("END PlaneSpotter::drawAircraftHistory"));
#endif

}

void PlaneSpotter::drawPlane(Aircraft aircraft, bool isSpecial)
{
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("PlaneSpotter::drawPlane"));
#endif

  Coordinates coordinates;
  coordinates.lon = aircraft.lon;
  coordinates.lat = aircraft.lat;
  CoordinatesPixel p = geoMap_->convertToPixel(coordinates);

  tft_->setFreeFont(&Dialog_plain_9);

  tft_->setTextPadding(0);
  tft_->setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tft_->setTextDatum(BC_DATUM);
  //tft_->drawString(aircraft.call, p.x + 8, p.y - 5, GFXFONT );
  tft_->drawString(aircraft.call, p.x + 8, p.y + 15, GFXFONT );

  int planeDotsX[planeDots_];
  int planeDotsY[planeDots_];
  //isSpecial = false;
  for (int i = 0; i < planeDots_; i++)
  {
    planeDotsX[i] = cos((-450 + (planeDeg_[i] + aircraft.heading)) * PI / 180) * planeRadius_[i] + p.x;
    planeDotsY[i] = sin((-450 + (planeDeg_[i] + aircraft.heading)) * PI / 180) * planeRadius_[i] + p.y;
  }
  if (isSpecial)
  {
    tft_->fillTriangle(planeDotsX[0], planeDotsY[0], planeDotsX[1], planeDotsY[1], planeDotsX[2], planeDotsY[2], TFT_RED);
    tft_->fillTriangle(planeDotsX[2], planeDotsY[2], planeDotsX[3], planeDotsY[3], planeDotsX[4], planeDotsY[4], TFT_RED);
  } else
  {
    for (int i = 1; i < planeDots_; i++)
    {
      tft_->drawLine(planeDotsX[i], planeDotsY[i], planeDotsX[i - 1], planeDotsY[i - 1], TFT_RED);
    }
  }
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("END PlaneSpotter::drawPlane"));
#endif
}

void PlaneSpotter::drawInfoBox(Aircraft closestAircraft)
{
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("PlaneSpotter::drawInfoBox"));
#endif

  int line1 = geoMap_->getMapHeight() + 13 + TOP_BAR_HEIGHT;
  int line2 = geoMap_->getMapHeight() + 23 + TOP_BAR_HEIGHT;
  int line3 = geoMap_->getMapHeight() + 33 + TOP_BAR_HEIGHT;
  int right = tft_->width();

  // Clean infobox's lower lines
  tft_->fillRect(0, geoMap_->getMapHeight() + TOP_BAR_HEIGHT, tft_->width(), tft_->height() - (geoMap_->getMapHeight() + TOP_BAR_HEIGHT), TFT_BLACK);

  if (closestAircraft.call != "")
  {
    tft_->setFreeFont(&Dialog_plain_9);

    // Line 1 - aircraftType
    int xwidth = tft_->textWidth(F("ABC1234XY"), GFXFONT ) + 12;
    tft_->setTextPadding(xwidth);
    tft_->setTextDatum(BL_DATUM);
    tft_->setTextColor(TFT_WHITE, TFT_BLACK);
    tft_->drawString(closestAircraft.call, 0, line1, GFXFONT );

    tft_->setTextPadding(240 - xwidth);
    tft_->setTextDatum(BR_DATUM);
    tft_->drawString(closestAircraft.aircraftType, right - 2, line1, GFXFONT );

    // Line 2 - altitude + speed + distance
    tft_->setTextColor(TFT_YELLOW, TFT_BLACK);
    tft_->setTextDatum(BL_DATUM);
    xwidth = tft_->textWidth(F("Alt: XXXXXm"), GFXFONT ) + 12;
    tft_->setTextPadding(xwidth);
    tft_->drawString("Alt: " + String(closestAircraft.altitude * 0.3048, 0) + "m", 0, line2, GFXFONT );

    int xpos = xwidth;
    tft_->setTextDatum(BL_DATUM);
    xwidth = tft_->textWidth(F("Spd : XXXXkmh"), GFXFONT ) + 10;
    tft_->setTextPadding(xwidth);
    tft_->drawString("Spd: " + String(closestAircraft.speed * 1.852, 0) + "kmh", xpos, line2, GFXFONT );

    xpos += xwidth;
    tft_->setTextDatum(BL_DATUM);
    xwidth = tft_->textWidth(F("Dst : XXXXkm"), GFXFONT ) + 10;
    tft_->setTextPadding(xwidth);
    tft_->drawString("Dst: " + String(closestAircraft.distance, 2) + "km", xpos, line2, GFXFONT );

    /*
       heading removed due to portrait mode
        tft_->setTextDatum(BL_DATUM);
        xwidth = tft_->textWidth(F("Hdg: 000"), GFXFONT );
        tft_->setTextPadding(xwidth);
        tft_->drawString("Hdg: " + String(closestAircraft.heading, 0), right - xwidth, line2, GFXFONT );
    */
    if (closestAircraft.fromShort != "" && closestAircraft.toShort != "")
    {
      // Use print stream so the line wraps (tft_->print does not work, kludge is to get the String returned so we can use the print class!)
      tft_->setFreeFont(&Dialog_plain_9);
      tft_->setCursor(0, line3);
      tft_->setTextColor(TFT_GREEN, TFT_BLACK);
      tft_->setTextDatum(BL_DATUM);
      tft_->setTextWrap(1);
      tft_->print("From: " + closestAircraft.fromShort + "=>" + closestAircraft.toShort);
    }
  }

#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("END PlaneSpotter::drawInfoBox"));
#endif
}


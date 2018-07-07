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

  See more at http://blog.squix.ch
*/
#include "GeoMap.h"
#include <Syslog.h>

// External variables
extern Syslog syslog;
extern WebResource webResource;

GeoMap::GeoMap(MapProvider mapProvider, String apiKey, int mapWidth, int mapHeight) {
  mapProvider_ = mapProvider;
  apiKey_ = apiKey;
  mapWidth_ = mapWidth;
  mapHeight_ = mapHeight;
}

GeoMap::~GeoMap() {}

int GeoMap::getMapWidth() {
  return mapWidth_;
}
int GeoMap::getMapHeight() {
  return mapHeight_;
}


void GeoMap::downloadMap() {
  downloadMap(nullptr);
}


bool GeoMap::setMap(Coordinates mapCenter, int zoom)
{
  mapCenter_ = mapCenter;
  zoom_ = zoom;

  return SPIFFS.exists(getMapName());
}


void GeoMap::downloadMap(ProgressCallback progressCallback)
{
  switch (mapProvider_) {
    case MapProvider::MapQuest:
      /*     downloadFile("http://open.mapquestapi.com/staticmap/v4/getmap?key=" + apiKey_ + "&type=map&scalebar=false&size="
                        + String(mapWidth_) + "," + String(mapHeight_) + "&zoom=" + String(zoom_) + "&center=" + String(mapCenter_.lat) + "," + String(mapCenter_.lon), getMapName(), progressCallback);
      */
      webResource.downloadFile("http://open.mapquestapi.com/staticmap/v4/getmap?key="
                               + apiKey_
                               + F("&type=map&scalebar=false&size=")
                               + String(mapWidth_)
                               + F(",")
                               + String(mapHeight_)
                               + F("&zoom=")
                               + String(zoom_)
                               + F("&center=")
                               + String(mapCenter_.lat)
                               + F(",")
                               + String(mapCenter_.lon), getMapName(), progressCallback);
      break;
    case MapProvider::Google:
      webResource.downloadFile("http://maps.googleapis.com/maps/api/staticmap?key="
                               + apiKey_
                               + F("&center=")
                               + String(mapCenter_.lat)
                               + F(",")
                               + String(mapCenter_.lon)
                               + F("&zoom=")
                               + String(zoom_)
                               + F("&size=")
                               + String(mapWidth_)
                               + F("x")
                               + String(mapHeight_)
                               + F("&format=jpg-baseline&maptype=roadmap"), getMapName(), progressCallback);
      break;
  }
}

String GeoMap::getMapName() {
  return "/map" + String(mapCenter_.lat) + "_" + String(mapCenter_.lon) + "_" + String(zoom_) + ".jpg";
}

CoordinatesPixel GeoMap::convertToPixel(Coordinates coordinates)
{
  CoordinatesTiles centerTile = convertToTiles(mapCenter_);
  CoordinatesTiles poiTile = convertToTiles(coordinates);
  CoordinatesPixel poiPixel;
  poiPixel.x = (poiTile.x - centerTile.x) * MAPQUEST_TILE_LENGTH + mapWidth_ / 2;
  poiPixel.y = (poiTile.y - centerTile.y) * MAPQUEST_TILE_LENGTH + mapHeight_ / 2 + TOP_BAR_HEIGHT;
  return poiPixel;
}

CoordinatesTiles GeoMap::convertToTiles(Coordinates coordinates) {

  double lon_rad = coordinates.lon * PI / 180;
  double lat_rad = coordinates.lat * PI / 180;;
  double n = pow(2.0, zoom_);

  CoordinatesTiles tile;
  tile.x = ((coordinates.lon + 180) / 360) * n;
  tile.y = (1 - (log(tan(lat_rad) + 1.0 / cos(lat_rad)) / PI)) * n / 2.0;

  return tile;
}

Coordinates GeoMap::convertToCoordinatesFromTiles(CoordinatesTiles tiles) {
  Coordinates result;

  double n = pow(2.0, zoom_);


  result.lon = (360 * tiles.x / n)  - 180;
  result.lat = 180 / PI * (atan(sinh(PI * (1 - 2 * tiles.y / n))));

  return result;
}

Coordinates GeoMap::convertToCoordinates(CoordinatesPixel poiPixel) {
  CoordinatesTiles centerTile = convertToTiles(mapCenter_);
  CoordinatesTiles poiTile;
  //-#ifdef DEBUG_SERIAL Serial.println(String(centerTile.x, 9) + ", " + String(centerTile.y, 9));
  poiTile.x = ((poiPixel.x - (mapWidth_ / 2.0)) / MAPQUEST_TILE_LENGTH) + centerTile.x;
  poiTile.y = ((poiPixel.y - (mapHeight_ / 2.0)) / MAPQUEST_TILE_LENGTH) + centerTile.y;
  //-#ifdef DEBUG_SERIAL Serial.println(String(poiTile.x, 9) + ", " + String(poiTile.y, 9));
  Coordinates poiCoordinates = convertToCoordinatesFromTiles(poiTile);
  return poiCoordinates;
}






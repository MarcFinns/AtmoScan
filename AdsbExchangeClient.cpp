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

#include "AdsbExchangeClient.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// Extern variables
extern WiFiClient wifiClient;

AdsbExchangeClient::AdsbExchangeClient() {}

void AdsbExchangeClient::updateVisibleAircraft(String searchQuery)
{

  JsonStreamingParser parser;
  parser.setListener(this);

  //WiFiClient wifiClient;

  // http://public-api.adsbexchange.com/VirtualRadar/AircraftList.json?lat=47.437691&lng=8.568854&fDstL=0&fDstU=20&fAltL=0&fAltU=5000

  const char host[] = "global.adsbexchange.com";
  /*
    String url = "/VirtualRadar/AircraftList.json?" + searchQuery;
  */

  const int httpPort = 80;
  if (!wifiClient.connect(host, httpPort))
  {
    //-#ifdef DEBUG_SERIAL Serial.println(F("connection failed"));
    return;
  }

  /*
    // This will send the request to the server
    wifiClient.print("GET " + url + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");
  */
  wifiClient.print(F("GET "));
  wifiClient.print(F("/VirtualRadar/AircraftList.json?"));
  wifiClient.print(searchQuery);
  wifiClient.print(F(" HTTP/1.1\r\nHost: "));
  wifiClient.print(host);
  wifiClient.print(F("\r\nConnection: close\r\n\r\n"));

  int retryCounter = 0;
  while (!wifiClient.available())
  {
    delay(1000);
    retryCounter++;
    if (retryCounter > 10)
    {
      return;
    }
  }

  int pos = 0;
  bool isBody = false;
  char c;

  int size = 0;
  wifiClient.setNoDelay(false);
  while (wifiClient.connected())
  {
    while ((size = wifiClient.available()) > 0)
    {
      c = wifiClient.read();
      if (c == '{' || c == '[')
      {
        isBody = true;
      }
      if (isBody) {
        parser.parse(c);
      }
    }
  }
  endDocument();
}


void AdsbExchangeClient::whitespace(char c)
{

}

void AdsbExchangeClient::startDocument() {
  counter = 0;
  index = -1;
}

void AdsbExchangeClient::key(String key) {
  currentKey = key;
}

void AdsbExchangeClient::value(String value) {
  /*String from = "";
    String to = "";
    String altitude = "";
    String aircraftType = "";
    String currentKey = "";
    String operator = "";


    "Type": "A319",
    "Mdl": "Airbus A319 112",

    "From": "LSZH Z\u00c3\u00bcrich, Zurich, Switzerland",
    "To": "LEMD Madrid Barajas, Spain",
    "Op": "Swiss International Air Lines",
    "OpIcao": "SWR",
    "Dst": 6.23,
    "Year": "1996"
  */
  if (counter >= MAX_AIRCRAFTS - 1) {
    //-#ifdef DEBUG_SERIAL Serial.println(F("Max Aircrafts reached...."));
    return;
  }
  if (currentKey == F("Id")) {

    counter++;
    index = counter - 1;
    aircrafts[index] = {};
    histories[index] = {};

    trailIndex = 0;
    for (int i = 0; i < MAX_HISTORY_TEMP; i++)
    {
      positionTemp[i] = {};
    }

  } else if (currentKey == F("From")) {
    // aircrafts[index].from = value;
    // aircrafts[index].fromCode = value.substring(0, 4);
    int indexOfFirstComma = value.indexOf(F(","));
    aircrafts[index].fromShort = value.substring(4, indexOfFirstComma);
  } else if (currentKey == F("To")) {
    // aircrafts[index].to = value;
    // aircrafts[index].toCode = value.substring(0, 4);
    int indexOfFirstComma = value.indexOf(F(","));
    aircrafts[index].toShort = value.substring(4, indexOfFirstComma);
  } else if (currentKey == F("OpIcao")) {
    // aircrafts[index].operatorCode = value;
  } else if (currentKey == F("Dst")) {
    aircrafts[index].distance = value.toFloat();
  } else if (currentKey == F("Mdl")) {
    aircrafts[index].aircraftType = value;
  } else if (currentKey == F("Trak")) {
    aircrafts[index].heading = value.toFloat();
  } else if (currentKey == F("Alt")) {
    aircrafts[index].altitude = value.toInt();;
  } else if (currentKey == F("Lat")) {
    aircrafts[index].lat = value.toFloat();
  } else if (currentKey == F("Long")) {
    aircrafts[index].lon = value.toFloat();
  } else if (currentKey == F("Spd")) {
    aircrafts[index].speed = value.toFloat();
  } else if (currentKey == F("Icao")) {
    // aircrafts[index].icao = value;
  } else if (currentKey == F("Call")) {
    //-#ifdef DEBUG_SERIAL Serial.println("Saw " + value);
    aircrafts[index].call = value;
  } else if (currentKey == F("PosStale")) {
    aircrafts[index].posStall = (value == F("true"));
  } else if (currentKey == F("Cos")) {
    int tempIndex = trailIndex / 4;
    if (tempIndex < MAX_HISTORY_TEMP) {
      AircraftPosition position = positionTemp[tempIndex];
      Coordinates coordinates = position.coordinates;
      if (trailIndex % 4 == 0) {
        coordinates.lat = value.toFloat();
      } else if (trailIndex % 4 == 1) {
        coordinates.lon = value.toFloat();
      } else if (trailIndex % 4 == 3) {
        position.altitude = value.toInt();
      }
      position.coordinates = coordinates;
      positionTemp[tempIndex] = position;
      trailIndex++;
    }

  } else if (currentKey == F("Trt")) {
    if (aircrafts[index].posStall) {
      //-#ifdef DEBUG_SERIAL Serial.println(F("This aircraft is stalled. Ignoring it"));
      counter--;
      index = counter - 1;
    }
  }

}

Aircraft AdsbExchangeClient::getAircraft(int i) {
  if (i < MAX_AIRCRAFTS) {
    return aircrafts[i];
  }
  //return nullptr;
}

AircraftHistory AdsbExchangeClient::getAircraftHistory(int i) {
  return histories[i];
}

int AdsbExchangeClient::getNumberOfAircrafts() {
  return counter;
}

Aircraft AdsbExchangeClient::getClosestAircraft(double lat, double lon) {
  double minDistance = 999999.0;
  Aircraft closestAircraft = aircrafts[0];
  for (int i = 0; i < getNumberOfAircrafts(); i++) {
    Aircraft currentAircraft = aircrafts[i];

    if (currentAircraft.distance < minDistance) {
      minDistance = currentAircraft.distance;
      closestAircraft = currentAircraft;
    }
  }
  return closestAircraft;
}

void AdsbExchangeClient::endArray() {
  if (counter >= MAX_AIRCRAFTS - 1) {
    //-#ifdef DEBUG_SERIAL Serial.println(F("Max Aircrafts reached:end array"));
    return;
  }
  if (currentKey == F("Cos") && trailIndex > 0) {
    AircraftHistory history = {};
    uint16_t items = (trailIndex / 4);
    //-#ifdef DEBUG_SERIAL Serial.println("Finished history array: " + String(items) + " elements");
    int historyCounter = 0;
    for (int i = 0; i < min(items, MAX_HISTORY); i++) {
      AircraftPosition position = {};
      Coordinates coordinates = position.coordinates;
      coordinates.lat = positionTemp[items - i - 1].coordinates.lat;
      coordinates.lon = positionTemp[items - i - 1].coordinates.lon;
      position.coordinates = coordinates;
      position.altitude = positionTemp[items - i - 1].altitude;
      history.positions[i] = position;
      historyCounter++;
    }
    history.call = aircrafts[index].call;
    history.counter = historyCounter;
    histories[index] = history;
    currentKey = F("");
  }
}

void AdsbExchangeClient::endObject() {

}

void AdsbExchangeClient::endDocument() {
  /*//-#ifdef DEBUG_SERIAL Serial.println("End of document:");
    for (int i = 0; i < getNumberOfAircrafts(); i++) {
    AircraftHistory history = histories[i];
    for (int j = 0; j < history.counter; j++) {
      AircraftPosition position = history.positions[j];
      //-#ifdef DEBUG_SERIAL Serial.println(history.call + ": " + String(j) + ". " + String(position.coordinates.lat, 9) + ", " + String(position.coordinates.lon, 9) + ", " + String(position.altitude));
    }
    }*/
}

void AdsbExchangeClient::startArray() {

}

void AdsbExchangeClient::startObject() {

}

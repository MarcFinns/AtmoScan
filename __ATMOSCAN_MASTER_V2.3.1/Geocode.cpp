/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#include <ArduinoJson.h>
#include "ESP8266WiFi.h"
#include <NtpClientLib.h>         // http://github.com/gmag11/NtpClient
#include <JsonStreamingParser.h>
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include "WsClient.h"
#include "TimeSpace.h"
#include "GlobalDefinitions.h"

extern struct Configuration config;
extern Syslog syslog;

// **********************************************************
//   Step 3 - Geocoding
//   Get Location name from latitude and longitude
// **********************************************************

bool Geocode::acquire(double latitude, double longitude)
{

#ifdef DEBUG_SYSLOG
  String myBuffer;
#endif

  //Connect to the client and make the api call
  if (httpConnect())
  {
    // Concatenate url and key
    String url = FPSTR(geoCodeURL);
    url += F("&lat=");
    url += String(latitude, 8);
    url += F("&lng=");
    url += String(longitude, 8);
    url += F("&username=" );
    url += config.geonames_user;

    // Serial.println("URL = " + url);

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, "URL = " + url);
#endif

    if (httpGet(url) && skipResponseHeaders())
    {
      // Invalidate current values
      locality = FPSTR(empty);
      country = FPSTR(empty);
      countryCode = FPSTR(empty);

      // Allow for slow server...
      int retryCounter = 0;
      while (!client.available())
      {
        delay(1000);
        retryCounter++;
        if (retryCounter > 10)
        {
          return false;
        }
      }

      while (client.available())
      {
        JsonStreamingParser parser;
        parser.setListener(this);
        char c;

        while (client.available())
        {
          c = client.read();

#ifdef DEBUG_SERIAL
          Serial.print(c);
#endif

#ifdef DEBUG_SYSLOG
          myBuffer = myBuffer + String(c);
#endif

          parser.parse(c);

          // Improves reliability from ESP version 2.4.0
          yield();
        }
      }
    }
    else
      // Get failed
      return false;
  }
  else
    // Could not connect
    return false;

  disconnect();

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "RESPONSE = " + myBuffer);
#endif

  if (country == "" || countryCode == "")
    return false;
  else
    return true;
}

void Geocode::whitespace(char c) {
#ifdef DEBUG_LOG
  Serial.println("whitespace");
#endif
}

void Geocode::startDocument() {
#ifdef DEBUG_LOG
  Serial.println("start document");
#endif
}

void Geocode::key(String key)
{
#ifdef DEBUG_LOG
  Serial.println("key: " + key);
#endif
  currentKey = key;
}

void Geocode::value(String value)
{
#ifdef DEBUG_LOG
  Serial.println("value: " + value);
#endif
  if (currentKey == F("name"))
  {
    locality = value;
  }
  else if (currentKey == F("countryName"))
  {
    country = value;
  }
  else if (currentKey == F("countryCode"))
  {
    countryCode = value;
  }
}


void Geocode::endArray() {
#ifdef DEBUG_LOG
  Serial.println("end array. ");
#endif
}

void Geocode::endObject() {
#ifdef DEBUG_LOG
  Serial.println("end object. ");
#endif
}

void Geocode::endDocument() {
#ifdef DEBUG_LOG
  Serial.println("end document. ");
#endif
}

void Geocode::startArray() {
#ifdef DEBUG_LOG
  Serial.println("start array. ");
#endif
}

void Geocode::startObject() {
#ifdef DEBUG_LOG
  Serial.println("start object. ");
#endif
}

String Geocode::getLocality()
{
  return locality;
}

String Geocode::getCountry()
{
  return country;
}

String Geocode::getCountryCode()
{
  return countryCode;
}


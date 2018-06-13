/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#include "ESP8266WiFi.h"
#include <JsonStreamingParser.h> // https://github.com/squix78/json-streaming-parser
#include "WsClient.h"
#include "TimeSpace.h"
#include <Syslog.h>
#include "GlobalDefinitions.h"

extern struct Configuration config;
extern Syslog syslog;


bool Timezone::acquire(double latitude, double longitude)
{
  /**********************************************************
     Step 2 - Timezone
     Get Timezone from latitude and longitude
   **********************************************************/

#ifdef DEBUG_SYSLOG
  String myBuffer;
#endif

  //Connect to the client and make the api call
  if (httpConnect())
  {
    // Concatenate url and key
    String url = FPSTR(timeZoneURL);
    url += F("lat=");
    url += String(latitude, 6);
    url += F("&lng=") ;
    url += String(longitude, 6) ;
    url += F("&key=") ;
    url += config.timezonedb_key;

#ifdef DEBUG_SERIAL
    Serial.println("URL = " + url);
#endif

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, "URL = " + url);
#endif

    if (httpGet(url) && skipResponseHeaders())
    {
      JsonStreamingParser parser;
      parser.setListener(this);
      char c;

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
    else
    {
      // Get failed
#ifdef DEBUG_SERIAL
      Serial.println("get failed");
#endif
      return false;
    }
  }
  else
  {
    // Could not connect
#ifdef DEBUG_SERIAL
    Serial.println("Could not connect");
#endif
    return false;
  }

  disconnect();

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "RESPONSE = " + myBuffer);
#endif

  return true;
}

void Timezone::whitespace(char c) {
#ifdef DEBUG_SERIAL
  Serial.println("whitespace");
#endif
}

void Timezone::startDocument() {
#ifdef DEBUG_SERIAL
  Serial.println("start document");
#endif
}


void Timezone::key(String key) {
#ifdef DEBUG_SERIAL
  Serial.println("key: " + key);
#endif
  currentKey = String(key);
}

void Timezone::value(String value)
{
#ifdef DEBUG_SERIAL
  Serial.println("value: " + value);
#endif
  if (currentKey == F("dst"))
  {
    dst = value.toInt();
  }
  else if (currentKey == F("gmtOffset"))
  {
    utcOffset = value.toInt();
  }
  else if (currentKey == F("abbreviation"))
  {
    timeZoneId = value;
  }
  else if (currentKey == F("zoneName"))
  {
    timeZoneName = value;
  }
}

void Timezone::endArray() {
#ifdef DEBUG_SERIAL
  Serial.println("end array. ");
#endif
}

void Timezone::endObject() {
#ifdef DEBUG_SERIAL
  Serial.println("end object. ");
#endif
}

void Timezone::endDocument() {
#ifdef DEBUG_SERIAL
  Serial.println("end document. ");
#endif
}

void Timezone::startArray() {
#ifdef DEBUG_SERIAL
  Serial.println("start array. ");
#endif
}

void Timezone::startObject() {
#ifdef DEBUG_SERIAL
  Serial.println("start object. ");
#endif
}

String Timezone::getTimeZoneId()
{
  return timeZoneId;
}

String Timezone::getTimeZoneName()
{
  return timeZoneName;
}

bool Timezone::isDst()
{
  return dst;
}

int Timezone::getUtcOffset()
{
  return utcOffset;
}


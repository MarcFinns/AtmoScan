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

  Base64 encoding code by Rene Nyfenegger:
  http://www.adp-gmbh.ch/cpp/common/base64.html


  Modified by MarcFinns for ATMOSCAN

*/

//#define DEBUG_SYSLOG

#include "ESP8266WiFi.h"
#include <JsonStreamingParser.h>
#include "WsClient.h"
#include "TimeSpace.h"
//#include "UserConfig.h"

#ifdef DEBUG_SYSLOG
#include <Syslog.h>
extern Syslog syslog;
#endif

/**********************************************************
  Step 1 - Geolocation
  Get latitude and longitude via Wifi triangulation
**********************************************************/

bool Geolocate::acquire()
{
  int n = min(WiFi.scanNetworks(false, true), MAX_SSIDS);
  String multiAPString = "";

  for (int i = 0; i < n; i++)
  {
    if (i > 0)
    {
      multiAPString += ";";
    }
    multiAPString += WiFi.BSSIDstr(i) + "," + WiFi.RSSI(i);
  }

#ifdef DEBUG_SERIAL
  Serial.println(multiAPString);
#endif

  // Base64-encode the AP string..
  char multiAPs[multiAPString.length() + 1];
  multiAPString.toCharArray(multiAPs, multiAPString.length());
  multiAPString = encodeBase64(multiAPs, multiAPString.length());

  String url = FPSTR(geoLocateURL);
  url += multiAPString;

#ifdef DEBUG_SERIAL
  Serial.println("URL = " + url);
#endif

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "URL = " + url);
  String myBuffer;
#endif

  //Connect to the client and make the api call
  if (httpConnect())
  {
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
        parser.parse(c);

#ifdef DEBUG_SYSLOG
        myBuffer = myBuffer + String(c);
#endif

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

  if (latitude == 0 || longitude == 0)
    return false;
  else
    return true;
}

void Geolocate::key(String key) {
  currentKey = key;
}

void Geolocate::value(String value)
{
  if (currentKey == "lat")
  {
    latitude = value.toFloat();
  }
  if (currentKey == "lon")
  {
    longitude = value.toFloat();
  }
#ifdef DEBUG_SERIAL
  Serial.println("Key " + currentKey + ", value: " + value);
#endif
}

double Geolocate::getLongitude() {
  return longitude;
}

double Geolocate::getLatitude() {
  return latitude;
}


void Geolocate::whitespace(char c) {

}

void Geolocate::startDocument() {

}

void Geolocate::endArray() {

}

void Geolocate::endObject() {

}

void Geolocate::endDocument() {

}

void Geolocate::startArray() {

}

void Geolocate::startObject() {

}

/*
   Base64 code by Rene Nyfenegger:
   http://www.adp-gmbh.ch/cpp/common/base64.html
*/
String Geolocate::encodeBase64(char* bytes_to_encode, unsigned int in_len)
{
  String ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for (j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while ((i++ < 3))
      ret += '=';

  }

  return ret;

}


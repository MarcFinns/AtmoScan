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

#include <FS.h>
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog

#include "WebResource.h"

// External variables
extern Syslog syslog;

WebResource::WebResource() {

}

void WebResource::downloadFile(String url, String filename)
{
  downloadFile(url, filename, nullptr);
}

void WebResource::downloadFile(String url, String filename, ProgressCallback progressCallback)
{

  // Download only if file is not there yet
  if (SPIFFS.exists(filename))
  {
#ifdef DEBUG_SYSLOG
    syslog.logf(LOG_DEBUG, "File already exists in SPIFFS. Skipping download of %s\n", filename.c_str());
#endif
    return;
  }

  syslog.logf(LOG_INFO, "File does not exist in SPIFFS. Downloading %s and saving as %s\n", url.c_str(), filename.c_str());

  //---------------------
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("[HTTP] begin...\n"));
#endif

  HTTPClient http;

  // configure server and url
  http.begin(url);

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("[HTTP] GET..."));
#endif

  // start connection and send HTTP header
  int httpCode = http.GET();
  if (httpCode > 0) {
    SPIFFS.remove(filename);
    fs::File f = SPIFFS.open(filename, "w+");
    if (!f)
    {
#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, F("file open failed"));
#endif
      return;
    }
    // HTTP header has been sent and Server response header has been handled

#ifdef DEBUG_SYSLOG
    syslog.logf(LOG_DEBUG, "[HTTP] GET... code: %d\n", httpCode);
#endif

    // file found at server
    if (httpCode == HTTP_CODE_OK) {

      // get lenght of document (is -1 when Server sends no Content-Length header)
      int total = http.getSize();
      int len = total;
      if (progressCallback != nullptr)
        progressCallback(filename, 0, total);

      // create buffer for read
      uint8_t buff[128] = { 0 };

      // get tcp stream
      WiFiClient * stream = http.getStreamPtr();

      // read all data from server
      while (http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();

        if (size) {
          // read up to 128 byte
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

          // write it to
          f.write(buff, c);

          if (len > 0) {
            len -= c;
          }
          if (progressCallback != nullptr)
            progressCallback(filename, total - len, total);
        }
        delay(1);
      }

#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, "[HTTP] connection closed or file end.");
#endif
    }
    f.close();
  } else
  {
#ifdef DEBUG_SYSLOG
    syslog.logf(LOG_DEBUG, "[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
#endif
  }

  http.end();
}

/*
  HTTPClient http;

  #ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("[HTTP] begin...\n"));
  #endif

  // configure server and url
  http.begin(url);

  #ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("[HTTP] GET..."));
  #endif
  // start connection and send HTTP header
  int httpCode = http.GET();

  syslog.log(LOG_DEBUG, F("STEP 1"));

  if (httpCode > 0)
  {
    syslog.log(LOG_DEBUG, F("STEP 2"));
    // SPIFFS.remove(filename);
    syslog.log(LOG_DEBUG, F("STEP 3"));
    //-----------------------------------------------------------------
    fs::File f = SPIFFS.open(filename, "w+");
    syslog.log(LOG_DEBUG, F("STEP 4"));
    if (!f)
    {
      syslog.log(LOG_DEBUG, F("STEP 5"));
  #ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, F("file open failed"));
  #endif
      return;
    }
    // HTTP header has been send and Server response header has been handled

  #ifdef DEBUG_SYSLOG
    syslog.logf(LOG_DEBUG, "[HTTP] GET... code: %d\n", httpCode);
  #endif

    // file found at server
    if (httpCode == HTTP_CODE_OK) {

      // get length of document (is -1 when Server sends no Content-Length header)
      int total = http.getSize();
      int len = total;
      progressCallback(filename, 0, total);
      // create buffer for read
      uint8_t buff[128] = { 0 };

      // get tcp stream
      WiFiClient * stream = http.getStreamPtr();

      // read all data from server
      while (http.connected() && (len > 0 || len == -1)) {
        // get available data size
        size_t size = stream->available();

        if (size) {
          // read up to 128 byte
          int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

          // write it to
          f.write(buff, c);

          if (len > 0) {
            len -= c;
          }
          progressCallback(filename, total - len, total);
        }
        delay(1);
      }

  #ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, "[HTTP] connection closed or file end.");
  #endif

    }
    f.close();
  } else {

  #ifdef DEBUG_SYSLOG
    syslog.logf(LOG_DEBUG, "[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  #endif
  }

  http.end();
  }
*/

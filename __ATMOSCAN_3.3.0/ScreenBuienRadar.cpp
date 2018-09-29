/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include "FS.h"
#include <Chronos.h>              // https://github.com/MarcFinns/Chronos
#include <NtpClientLib.h>         // https://github.com/gmag11/NtpClient                  NOTE: Requires https://github.com/PaulStoffregen/Time

#include "ScreenBuienRadar.h"
#include "ESP8266WiFi.h"
#include "GlobalDefinitions.h"
#include "Free_Fonts.h"
#include "Fonts.h"
#include "StringTokenizer.h"

#define MAX_IMAGES 23

// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern struct ProcessContainer procPtr;
extern struct Configuration config;
extern GfxUi ui;

// Prototypes
void errLog(String msg);

// Static variables
long ScreenBuienRadar::lastImageRefreshTime = 0;
int ScreenBuienRadar::lastImageDownloaded = 0;

void ScreenBuienRadar::activate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenBuienRadar::activate()"));
#endif
  syslog.log(LOG_DEBUG, String(F("lastImageRefreshTime=")) + String(lastImageRefreshTime));

  LCD.fillScreen(TFT_BLACK);

  if (!config.connected || !(NTP.getLastNTPSync() > 0))
  {
    LCD.setTextDatum(TC_DATUM);
    LCD.setFreeFont(&ArialRoundedMTBold_14);
    LCD.setTextColor(TFT_ORANGE, TFT_BLACK);

    LCD.drawString(F("BuienRadar needs"), 120, 120, GFXFF);
    LCD.drawString(F("connection and time synchronisation"), 120, 140, GFXFF);
    isInitialised = false;
    return;
  }

  // Detect if downloads were previously interrupted (i.e. swiping out)
  if (lastImageDownloaded > 0 && lastImageDownloaded < 23)
  {
    // For the time being, display the last downloaded image
    if (SPIFFS.exists(String(F("/forecast")) + String(lastImageDownloaded) + F(".jpg")) == true)
    {
      ui.drawJpeg(String(F("/forecast")) + String(lastImageDownloaded) + F(".jpg"), 0,  1 + TOP_BAR_HEIGHT);
    }

    // Restart from next one
    currentImage = lastImageDownloaded + 1;
    syslog.log(LOG_DEBUG, "------ Downnload was interrupted, resuming from " + String(currentImage));

  }
  else
  {
    // Start from 0
    currentImage = 0;

    // Show logo but on first start
    if (lastImageRefreshTime == 0 )
    {
      // Display logo
      LCD.fillRect(0, 1 + TOP_BAR_HEIGHT, 240, 255, TFT_WHITE);
      ui.drawBitmap(BuienLogo, 0, 160, 240, 44);
    }
  }

  isInitialised = true;
}

void ScreenBuienRadar::update()
{
  if (isInitialised)
  {
    if (config.connected && (NTP.getLastNTPSync() > 0))
    {
      // Refresh images from web site every 30 minutes
      if ( millis() > lastImageRefreshTime + 30 * 60 * 1000 || lastImageRefreshTime == 0 )
      {
        // Download images
        syslog.log(LOG_DEBUG, "------ LOADING IMG" + String(currentImage));

        // Compute filename timestamp, rounded to the hour
        char timeStamp[13];
        Chronos::DateTime forecastTime = Chronos::DateTime::now();
        forecastTime = forecastTime + Chronos::Span::Hours(currentImage);
        sprintf(timeStamp, "%4d%02d%02d%02d00" , forecastTime.year(), forecastTime.month(), forecastTime.day(), forecastTime.hour()) ;

        // Set visual communications flag on screen
        procPtr.UIManager.communicationsFlag(true);

        // Download file
        int len = getForecastImage(F("api.buienradar.nl"),
                                   String(F("/image/1.0/24hourforecastmapnl/jpg/?t=")) + String(timeStamp) + F("&w=240&h=192&type=rain"),
                                   String(F("/forecast")) + String(currentImage) + F(".jpg"));

        // Reset visual communications flag
        procPtr.UIManager.communicationsFlag(false);

        // If file was not downloaded, retry at next cycle
        if (len <= 0)
        {
          syslog.log(LOG_DEBUG, F("No file downloaded, retrying"));
          return;
        }

        // Remember last downkload, in case we get interrupted
        lastImageDownloaded = currentImage;

        syslog.log(LOG_DEBUG, "DOWNLOAD SIZE = " + String(len));

        // Downloaded finished
        if (currentImage == MAX_IMAGES)
        {
          lastImageRefreshTime = millis();
        }

      }

      // Remove logo on first instance
      if (currentImage == 0 && lastImageRefreshTime == 0 )
      {
        LCD.fillRect(0, 1 + TOP_BAR_HEIGHT, 240, 255, TFT_BLACK);
      }

      // Display current image from SPIFFS
      if (SPIFFS.exists(String(F("/forecast")) + String(currentImage) + F(".jpg")) == true)
      {
        ui.drawJpeg(String(F("/forecast")) + String(currentImage) + F(".jpg"), 0,  1 + TOP_BAR_HEIGHT);
      }

      // Advance to next image
      currentImage++;
      if (currentImage > MAX_IMAGES)
        currentImage = 0;


      // Refresh histogram every 5 minutes
      if ( millis() > lastChartRefreshTime + 5 * 60 * 1000 || lastChartRefreshTime == 0 )
      {
        syslog.log(LOG_DEBUG, "------ Refreshing histogram" );

        // Get forecasts for next hours
        String hours[24];
        int forecasts[24];

        // Set visual communications flag
        procPtr.UIManager.communicationsFlag(true);

        // Get data
        int dataPoints = getLocalForecast( procPtr.GeoLocation.getLatitude(), procPtr.GeoLocation.getLongitude(), hours, forecasts);

        // Reset visual communications flag
        procPtr.UIManager.communicationsFlag(false);

        // Clear histogram area
        LCD.fillRect(0, 257, 240, 64, TFT_BLACK);

        // Histogram forecast points, if received
        if (dataPoints > 0)
        {
          for (int i = 0; i < dataPoints; i++)
          {
            // Print hours
            //   LCD.println(buienRadar->hours[i]);

            int h = max(forecasts[i] / 4, 2);

            // Draw bar
            LCD.fillRect(i * 10, 319 - h, 7, h, TFT_BLUE);

            // Remember refresh time
            lastChartRefreshTime = millis();

            // Refresh title
            LCD.setTextColor(TFT_YELLOW, TFT_BLACK);
            LCD.setTextDatum(TC_DATUM);

            //LCD.setFreeFont(&Dialog_plain_15);
            LCD.setFreeFont(&ArialRoundedMTBold_14);
            LCD.drawString("Next 2 hours", 120, 280, GFXFF);
          }
        }
      }
    }
  }
  //syslog.log(LOG_DEBUG, String(F("UPDATE END - Heap=")) + String(ESP.getFreeHeap()) + F(" bytes"));
}

void ScreenBuienRadar::deactivate()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("ScreenBuienRadar::deactivate()"));
#endif
}


bool ScreenBuienRadar::onUserEvent(int event)
{
  return false;
}

long ScreenBuienRadar::getRefreshPeriod()
{
  return 1000;
}

String ScreenBuienRadar::getScreenName()
{
  return F("BuienRadar");
}

bool ScreenBuienRadar::isFullScreen()
{
  return false;
}

bool ScreenBuienRadar::getRefreshWithScreenOff()
{
  return true;
}

int ScreenBuienRadar::getForecastImage(String host, String resource, String filename)
{

  int contentLength = -1;
  int httpCode;

  // HTTPS but dont verify certificates
  BearSSL::WiFiClientSecure client;
  client.setBufferSizes(1024, 256);
  client.setInsecure();

  // syslog.log(LOG_DEBUG, String(F("AFTER CLIENTSECURE - Heap=")) + String(ESP.getFreeHeap()) + F(" bytes"));

  // Connect
  client.connect(host, 443);

  // If not connected, return
  if (!client.connected())
  {
    client.stop();
    errLog("HTTPS: Can't connect");
    return -1;
  }

  // HTTP GET
  client.print(F("GET "));
  client.print(resource);
  client.print(F(" HTTP/1.1\r\nHost: "));
  client.print(host);
  client.print(F("\r\nUser-Agent: ESP8266\r\n"));
  client.print(F("\r\n"));

  // Handle headers
  while (client.connected())
  {
    String header = client.readStringUntil('\n');
    if (header.startsWith(F("HTTP/1.")))
    {
      httpCode = header.substring(9, 12).toInt();

      if (httpCode != 200)
      {
        errLog(String(F("HTTP GET code=")) + String(httpCode));
        client.stop();
        return -1;
      }
    }

    if (header.startsWith(F("Content-Length: ")))
    {
      contentLength = header.substring(15).toInt();
    }
    if (header == F("\r"))
    {
      break;
    }
  }

  if (!(contentLength > 0))
  {
    errLog(F("HTTP content length=0"));
    client.stop();
    return -1;
  }

  // remove old file and open for write
  SPIFFS.remove(filename);
  fs::File f = SPIFFS.open(filename, "w+");
  if (!f)
  {
    errLog( F("file open failed"));
    client.stop();
    return -1;
  }

  // Download file
  int remaining = contentLength;
  int received;
  uint8_t buff[512] = { 0 };

  // syslog.log(LOG_DEBUG, String(F("Heap = ")) + String(ESP.getFreeHeap()) + F(" bytes"));

  // read all data from server
  while (client.available() && remaining > 0)
  {
    // read up to buffer size
    received = client.readBytes(buff, ((remaining > sizeof(buff)) ? sizeof(buff) : remaining));

    // write it to file
    f.write(buff, received);

    if (remaining > 0)
    {
      remaining -= received;
    }
    yield();
  }

  // syslog.log(LOG_DEBUG, "[HTTP] connection closed or file end.");

  // Close SPIFFS file
  f.close();

  // Stop client
  client.stop();
  return contentLength;
}


int ScreenBuienRadar::getLocalForecast( double latitude, double longitude, String (&hours)[24], int (&forecasts)[24])
{

  int contentLength = -1;
  int httpCode;
  String host = F("gpsgadget.buienradar.nl");

  // HTTPS but dont verify certificates
  BearSSL::WiFiClientSecure client;
  client.setBufferSizes(1024, 256);
  client.setInsecure();

  // Connect
  client.connect(host, 443);

  // If not connected, return
  if (!client.connected())
  {
    client.stop();
    errLog("HTTPS: Can't connect");
    return -1;
  }

  // gpsgadget.buienradar.nl//data/raintext?lat=52&lon=5
  // HTTP GET
  String resource =  F("/data/raintext?lat=");
  resource += String(latitude, 2) +
              F("&lon=") +
              String(longitude, 2);

  client.print(F("GET "));
  client.print(resource);
  client.print(F(" HTTP/1.1\r\nHost: "));
  client.print(host);
  client.print(F("\r\nUser-Agent: ESP8266\r\n"));
  client.print(F("\r\n"));

  // Handle headers
  while (client.connected())
  {
    String header = client.readStringUntil('\n');
    if (header.startsWith(F("HTTP/1.")))
    {
      httpCode = header.substring(9, 12).toInt();

      if (httpCode != 200)
      {
        errLog("HTTPS: GET code " + String(httpCode));
        client.stop();
        return -1;
      }
    }

    if (header.startsWith(F("Content-Length: ")))
    {
      contentLength = header.substring(15).toInt();
    }
    if (header == F("\r"))
    {
      break;
    }
  }

  // syslog.log(LOG_DEBUG, "contentLength = " + String(contentLength));

  if (!(contentLength > 0))
  {
    errLog("contentLength=0");
    client.stop();
    return -1;
  }

  // Wait for body of reply
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      client.stop();
      return -1;
    }
  }

  // Download data
  char buff[300];

  // syslog.log(LOG_DEBUG, String(F("Heap = ")) + String(ESP.getFreeHeap()) + F(" bytes"));

  // Receive body
  client.readBytes(buff, contentLength < 300 ? contentLength : 300);

  // Stop client
  client.stop();

  // syslog.log(LOG_DEBUG, F("BEFORE TOKENIZATION"));

  // Prepare body for tokenization
  String response = String(buff);
  response.replace(F("|"), F(","));
  response.replace(F("\n"), F(","));
  response.replace(F("\r"), F(""));

  // Tokenize response
  StringTokenizer tokens(response, F(","));

  int dataPoints = 0;
  while (tokens.hasNext() && dataPoints < 24)
  {
    // Get the next token in the response
    forecasts[dataPoints] = tokens.nextToken().toInt();
    hours[dataPoints] = tokens.nextToken();
    dataPoints++;
  }

  syslog.log(LOG_DEBUG, "Datapoints = " + String(dataPoints));
  return dataPoints;
}

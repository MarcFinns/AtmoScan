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
  AUTHORS OR COPYBR_DATUM HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
  See more at http://blog.squix.ch

  Adapted by Bodmer to use the faster TFT_eSPI library:
  https://github.com/Bodmer/TFT_eSPI

  Plus:
  Minor changes to text placement and auto-blanking out old text with background colour padding
  Moon phase text added
  Forecast text lines are automatically split onto two lines at a central space (some are long!)
  Time is printed with colons aligned to tidy display
  Min and max forecast temperatures spaced out
  The ` character has been changed to a degree symbol in the 36 point font
  New smart WU splash startup screen and updated progress messages
  Display does not need to be blanked between updates
  Icons nudged about slightly to add wind direction
*/


#include "ScreenWeatherStation.h"
// check settings.h for adapting to your needs
#include "ScreenWeatherStationSettings.h"
#include "WundergroundClient.h"
#include "GlobalDefinitions.h"

// Fonts created by http://oleddisplay.squix.ch/
#include "ArialRoundedMTBold_14.h"
#include "ArialRoundedMTBold_36.h"
#include "Free_Fonts.h"

// Download helper
#include "WebResource.h"

#include <syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI




// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern GfxUi ui;
extern struct Configuration config;
extern struct ProcessContainer procPtr;
extern WebResource webResource;

// Prototypes
void errLog(String msg);
void setTurbo(bool setTurbo);
bool isTurbo();


ScreenWeatherStation::ScreenWeatherStation() {}


void  ScreenWeatherStation::activate()
{
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("ScreenWeatherStation::activate()"));
#endif

  LCD.fillScreen(TFT_BLACK);
  LCD.setFreeFont(&ArialRoundedMTBold_14);
  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);


  if (!procPtr.GeoLocation.isValid() || !config.connected)
  {
    LCD.setTextDatum(TC_DATUM);
    LCD.drawString(F("Weather Station needs"), 120, 120, GFXFF);
    LCD.drawString(F("network & geolocation"), 120, 140, GFXFF);
    isInitialised = false;
    return;
  }

  // Create Wunderground object if not existent
  if (!config.wunderValid)
  {

#ifdef DEBUG_SYSLOG 
    syslog.log(LOG_INFO, F("wunderground object did not exist, initialise it"));
#endif


    // TURBO mode
    setTurbo(true);

    config.wunderground = new WundergroundClient(IS_METRIC);
    config.wunderValid = true;

    // Print credits
    LCD.setTextDatum(BC_DATUM);
    LCD.setTextColor(TFT_DARKGREY, TFT_BLACK);
    LCD.drawString(F("By: blog.squix.org"), 120, 180);
    LCD.drawString(F("Adapted: Bodmer, MarcFinns"), 120, 195);

    LCD.setTextColor(TFT_ORANGE, TFT_BLACK);

    // Draw Splash
    if (SPIFFS.exists(F("/WU.jpg")) == true) ui.drawJpeg("/WU.jpg", 0, 10);

    if (SPIFFS.exists(F("/Earth.jpg")) == true) ui.drawJpeg("/Earth.jpg", 0, 320 - 56); // Image is 56 pixels high
    delay(1000);

    LCD.setTextPadding(240); // Pad next drawString() text to full width to over-write old text

    // download images from the net. If images already exist in SPIFFS don't download

    downloadResources();

    LCD.drawString(F("Fetching weather data..."), 120, 220);

    // load the weather information
    updateData();

    firstRun = false;

    // NORMAL mode
    setTurbo(false);

  }
  else
  {
#ifdef DEBUG_SYSLOG 
    syslog.log(LOG_INFO, F("wunderground object being reused"));
#endif

    // No need to show splash screen
    firstRun = false;

    // Redraw screen
    drawCurrentWeather();
    drawForecast();
    drawAstronomy();
  }

  isInitialised = true;
}


void ScreenWeatherStation::update()
{
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("ScreenWeatherStation update()"));
#endif

  // If not valid conditions, do nothing!
  if (!isInitialised || !procPtr.GeoLocation.isValid() || !config.connected)
  {
#ifdef DEBUG_SERIAL 
    syslog.log(LOG_DEBUG, F("WeatherStation - not valid preconditions, can't run " ));
#endif
    return;
  }

  // Check if we should update weather information
  updateData();
}

// callback called during download of files. Updates progress bar
void ScreenWeatherStation::downloadCallback(String filename, int16_t bytesDownloaded, int16_t bytesTotal)
{

  LCD.setTextDatum(BC_DATUM);
  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);
  LCD.setTextPadding(240);

  int percentage = 100 * bytesDownloaded / bytesTotal;
  if (percentage == 0) {
    LCD.drawString(filename, 120, 220);
  }
  if (percentage % 5 == 0) {
    LCD.setTextDatum(TC_DATUM);
    LCD.setTextPadding(LCD.textWidth(F(" 888% ")));
    LCD.drawString(String(percentage) + F("%"), 120, 245);
    ui.drawProgressBar(10, 225, 240 - 20, 15, percentage, TFT_WHITE, TFT_BLUE);
  }
}

// Download the bitmaps
void ScreenWeatherStation::downloadResources()
{
  // LCD.fillScreen(TFT_BLACK);
  LCD.setFreeFont(&ArialRoundedMTBold_14);

  LCD.drawString(F("Downloading resources..."), 120, 220);
  LCD.drawString(" ", 120, 240);  // Clear line
  LCD.drawString(" ", 120, 260);  // Clear line

  char id[5];

  // Download WU graphic jpeg first and display it, then the Earth view
  webResource.downloadFile(F("http://i.imgur.com/njl1pMj.jpg"), F("/WU.jpg"), _downloadCallback);
  if (SPIFFS.exists(F("/WU.jpg")) == true) ui.drawJpeg("/WU.jpg", 0, 10);

  webResource.downloadFile(F("http://i.imgur.com/v4eTLCC.jpg"), F("/Earth.jpg"), _downloadCallback);
  if (SPIFFS.exists(F("/Earth.jpg")) == true) ui.drawJpeg("/Earth.jpg", 0, 320 - 56);

  // Download resources

  char urlBuffer[100];
  char fileNameBuffer[100];

  for (int i = 0; i < 19; i++)
  {
    sprintf(id, "%02d", i);

    // Prepare URL
    strcpy_P(urlBuffer, URL1);
    strcat_P(urlBuffer, wundergroundIcons[i]);
    strcat_P(urlBuffer, FILETYPE);

    // Prepare filename
    strcpy_P(fileNameBuffer, wundergroundIcons[i]);
    strcat_P(fileNameBuffer, FILETYPE);

    // Download resource
    webResource.downloadFile(urlBuffer, fileNameBuffer, _downloadCallback);
  }

  for (int i = 0; i < 19; i++)
  {
    sprintf(id, "%02d", i);

    // Prepare URL
    strcpy_P(urlBuffer, URL2);
    strcat_P(urlBuffer, wundergroundIcons[i]);
    strcat_P(urlBuffer, FILETYPE);

    // Prepare filename
    strcpy_P(fileNameBuffer, MINI);
    strcat_P(fileNameBuffer, wundergroundIcons[i]);
    strcat_P(fileNameBuffer, FILETYPE);

    // Download resource
    webResource.downloadFile(urlBuffer, fileNameBuffer, _downloadCallback);

  }

  for (int i = 0; i < 24; i++)
  {

    // Prepare URL
    strcpy_P(urlBuffer, URL3);
    dtostrf(i, 1, 0, &urlBuffer[strlen(urlBuffer)]);
    strcat_P(urlBuffer, FILETYPE);

    // Prepare filename
    strcpy_P(fileNameBuffer, MOON);
    dtostrf(i, 1, 0, &fileNameBuffer[strlen(fileNameBuffer)]);
    strcat_P(fileNameBuffer, FILETYPE);

    // Download resource
    webResource.downloadFile(urlBuffer, fileNameBuffer, _downloadCallback);

    // webResource.downloadFile("http://www.squix.org/blog/moonphase_L" + String(i) + ".bmp", "/moon" + String(i) + ".bmp", _downloadCallback);
  }
}

// Update the internet based information and update screen
void ScreenWeatherStation::updateData()
{
  // firstRun = true;  // Test only
  // firstRun = false; // Test only
#ifdef DEBUG_SERIAL 
  Serial.println("WeatherStation - updating data");
#endif

  // Check if we should update weather information
  // Update only if:
  if ((firstRun) // First time
      || (config.wunderground->isValid && (millis() - config.wunderground->lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS)) // If valid, every given interval
      || (!config.wunderground->isValid && (millis() - config.wunderground->lastDownloadUpdate > 60000))) // if not valid, retry after 1 minute
  {
#ifdef DEBUG_SYSLOG 
    syslog.log(LOG_DEBUG, "#### WEATHER DATA NEED UPDATE, IS " + String((millis() - config.wunderground->lastDownloadUpdate ) / 1000) + " SECONDS OLD");
#endif


    // Set location only once (does not change)
    if (firstRun)
    {
      drawProgress(20, F("Setting location..."));
    }

    if (firstRun
        || !config.wunderground->isValid)
    {
      config.wunderground->isValid = config.wunderground->updateLocation(WUNDERGRROUND_API_KEY,
                                     procPtr.GeoLocation.getLatitude(),
                                     procPtr.GeoLocation.getLongitude());
    }

    // Conditions
    if (config.wunderground->isValid)
    {
      if (firstRun)
        drawProgress(40, F("Updating conditions..."));
      else
        ui.fillSegment(120, 160, 0, (int) (25 * 3.6), 24, TFT_RED);
      config.wunderground->isValid = config.wunderground->updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, config.wunderground->getCountryName(), config.wunderground->getCity());
    }

    // Forecast
    if (config.wunderground->isValid)
    {
      if (firstRun)
        drawProgress(60, F("Updating forecast..."));
      else
        ui.fillSegment(120, 160, 0, (int) (50 * 3.6), 24, TFT_RED);
      config.wunderground->isValid = config.wunderground->updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, config.wunderground->getCountryName(), config.wunderground->getCity());
    }

    // Astronomy
    if (config.wunderground->isValid)
    {

      if (firstRun)
        drawProgress(80, F("Updating astronomy..."));
      else
        ui.fillSegment(120, 160, 0, (int) (75 * 3.6), 24, TFT_RED);
      config.wunderground->isValid = config.wunderground->updateAstronomy(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, config.wunderground->getCountryName(), config.wunderground->getCity());
    }


    if (firstRun)
      drawProgress(100, F("Done..."));
    else
      ui.fillSegment(120, 160, 0, 360, 24, TFT_RED);

    if (firstRun) delay(100);

    // Clear screen
    if (firstRun) LCD.fillScreen(TFT_BLACK);
    else
      // Erase progress pie..
      ui.fillSegment(120, 160, 0, 360, 24, TFT_BLACK);


#ifdef DEBUG_SYSLOG 
    syslog.log(LOG_DEBUG, "1 = " + config.wunderground->getCountry());
    syslog.log(LOG_DEBUG, "2 = " + config.wunderground->getCountryName());
    syslog.log(LOG_DEBUG, "3 = " + config.wunderground->getCity());
    syslog.log(LOG_DEBUG, "4 = " + config.wunderground->getTZ_Short());
    syslog.log(LOG_DEBUG, "5 = " + config.wunderground->getTZ_Long());
#endif

    // Redraw all
    drawCurrentWeather();
    drawForecast();
    drawAstronomy();

    //if (firstRun) screenshotToConsole(); // Weather screen dump. Not supported in this sketch
    firstRun = false;

    config.wunderground->lastDownloadUpdate = millis();
  }
  else
  {

#ifdef DEBUG_SYSLOG 
    syslog.log(LOG_DEBUG, "#### WEATHER DATA _DOES_NOT_ NEED UPDATE, IS " + String((millis() - config.wunderground->lastDownloadUpdate) / 1000) + " SECONDS OLD");
#endif
    return;
  }
}

// Progress bar helper
void ScreenWeatherStation::drawProgress(uint8_t percentage, String text) {
  LCD.setFreeFont(&ArialRoundedMTBold_14);

  LCD.setTextDatum(BC_DATUM);
  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);
  LCD.setTextPadding(240);
  LCD.drawString(text, 120, 220);

  ui.drawProgressBar(10, 225, 240 - 20, 15, percentage, TFT_WHITE, TFT_BLUE);

  LCD.setTextPadding(0);
}


// draws current weather information
void ScreenWeatherStation::drawCurrentWeather()
{
  // Weather Icon
  String weatherIcon = getMeteoconIcon(config.wunderground->getTodayIcon());
  //uint32_t dt = millis();
  ui.drawBmp(weatherIcon + ".bmp", 0, 64);

  // Weather Text
  String weatherText = config.wunderground->getWeatherText();
  //weatherText = "Heavy Thunderstorms with Small Hail"; // Test line splitting with longest(?) string


#ifdef DEBUG_SERIAL 
  Serial.println(config.wunderground->getWeatherText());
#endif

  LCD.setFreeFont(&ArialRoundedMTBold_14);

  LCD.setTextDatum(BR_DATUM);
  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);

  int splitPoint = 0;
  int xpos = 230;
  splitPoint =  ui.splitIndex(weatherText);
  if (splitPoint > 16) xpos = 235;

  LCD.setTextPadding(LCD.textWidth(F(" Heavy Thunderstorms")));  // Max anticipated string width + margin
  if (splitPoint) LCD.drawString(weatherText.substring(0, splitPoint), xpos, 83);
  LCD.setTextPadding(LCD.textWidth(F(" with Small Hail")));  // Max anticipated string width + margin
  LCD.drawString(weatherText.substring(splitPoint), xpos, 98);

  LCD.setFreeFont(&ArialRoundedMTBold_36);

  LCD.setTextDatum(TR_DATUM);
  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);
  LCD.setTextPadding(LCD.textWidth(F("-88`")));

  // Font ASCII code 96 (0x60) modified to make "`" a degree symbol
  weatherText = config.wunderground->getCurrentTemp();
  if (weatherText.indexOf(".")) weatherText = weatherText.substring(0, weatherText.indexOf(".")); // Make it integer temperature
  if (weatherText == "") weatherText = F("?");  // Handle null return
  LCD.drawString(weatherText + "`", 221, 100);

  LCD.setFreeFont(&ArialRoundedMTBold_14);

  LCD.setTextDatum(TL_DATUM);
  LCD.setTextPadding(0);
  if (IS_METRIC) LCD.drawString(F("C "), 221, 100);
  else  LCD.drawString("F ", 221, 100);

  weatherText = config.wunderground->getWindDir() + " ";
  weatherText += String((int)(config.wunderground->getWindSpeed().toInt() * WIND_SPEED_SCALING)) + WIND_SPEED_UNITS;

  LCD.setTextPadding(LCD.textWidth(F("Variable 888 mph "))); // Max string length?
  LCD.drawString(weatherText, 114, 136);

  weatherText = config.wunderground->getWindDir();

  int windAngle = 0;
  String compassCardinal = "";
  switch (weatherText.length()) {
    case 1:
      compassCardinal = F("N E S W "); // Not used, see default below
      windAngle = 90 * compassCardinal.indexOf(weatherText) / 2;
      break;
    case 2:
      compassCardinal = F("NE SE SW NW");
      windAngle = 45 + 90 * compassCardinal.indexOf(weatherText) / 3;
      break;
    case 3:
      compassCardinal = F("NNE ENE ESE SSE SSW WSW WNW NNW");
      windAngle = 22 + 45 * compassCardinal.indexOf(weatherText) / 4; // Should be 22.5 but accuracy is not needed!
      break;
    default:
      if (weatherText == F("Variable")) windAngle = -1;
      else {
        compassCardinal = F("North East  South West"); // Possible strings
        windAngle = 90 * compassCardinal.indexOf(weatherText) / 6;
      }
      break;
  }

  LCD.fillCircle(128, 110, 23, TFT_BLACK); // Erase old plot, radius + 1 to delete stray pixels
  LCD.drawCircle(128, 110, 6, TFT_RED);
  if ( windAngle >= 0 ) ui.fillSegment(128, 110, windAngle - 15, 30, 22, TFT_GREEN); // Might replace this with a bigger rotating arrow
  //LCD.drawCircle(128, 110, 22, TFT_DARKGREY);    // Outer ring - optional

  ui.drawSeparator(153);

  LCD.setTextPadding(0); // Reset padding width to none
}

// draws the three forecast columns
void ScreenWeatherStation::drawForecast()
{
  drawForecastDetail(10, 171, 0);
  drawForecastDetail(95, 171, 2);
  drawForecastDetail(180, 171, 4);
  ui.drawSeparator(171 + 69);
}

// helper for the forecast columns
void ScreenWeatherStation::drawForecastDetail(uint16_t x, uint16_t y, uint8_t dayIndex)
{
  LCD.setFreeFont(&ArialRoundedMTBold_14);

  String day = config.wunderground->getForecastTitle(dayIndex).substring(0, 3);
  day.toUpperCase();

  LCD.setTextDatum(BC_DATUM);

  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);
  LCD.setTextPadding(LCD.textWidth(F("WWW")));
  LCD.drawString(day, x + 25, y);

  LCD.setTextColor(TFT_WHITE, TFT_BLACK);
  LCD.setTextPadding(LCD.textWidth("-88   -88"));
  LCD.drawString(config.wunderground->getForecastHighTemp(dayIndex) + "   " + config.wunderground->getForecastLowTemp(dayIndex), x + 25, y + 14);

  String weatherIcon = getMeteoconIcon(config.wunderground->getForecastIcon(dayIndex));

#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_DEBUG, "icon is = " + String("/mini/") + weatherIcon + ".bmp");
#endif

  ui.drawBmp("/mini/" + weatherIcon + ".bmp", x, y + 15);

  LCD.setTextPadding(0); // Reset padding width to none
}

// draw moonphase and sunrise/set and moonrise/set
void ScreenWeatherStation::drawAstronomy()
{
  LCD.setFreeFont(&ArialRoundedMTBold_14);

  LCD.setTextDatum(BC_DATUM);
  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);
  LCD.setTextPadding(LCD.textWidth(F(" Waxing Crescent ")));
  LCD.drawString(config.wunderground->getMoonPhase(), 120, 260 - 2);

  int moonAgeImage = 24 * config.wunderground->getMoonAge().toInt() / 30.0;
  ui.drawBmp("/moon" + String(moonAgeImage) + ".bmp", 120 - 30, 260);

  LCD.setTextDatum(BC_DATUM);
  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);
  LCD.setTextPadding(0); // Reset padding width to none
  LCD.drawString("Sun", 40, 280);

  LCD.setTextDatum(BR_DATUM);
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);
  LCD.setTextPadding(LCD.textWidth(F(" 88:88 ")));
  int dt = ui.rightOffset(config.wunderground->getSunriseTime(), ":"); // Draw relative to colon to them aligned
  LCD.drawString(config.wunderground->getSunriseTime(), 40 + dt, 300);

  dt = ui.rightOffset(config.wunderground->getSunsetTime(), ":");
  LCD.drawString(config.wunderground->getSunsetTime(), 40 + dt, 315);

  LCD.setTextDatum(BC_DATUM);
  LCD.setTextColor(TFT_ORANGE, TFT_BLACK);
  LCD.setTextPadding(0); // Reset padding width to none
  LCD.drawString(F("Moon"), 200, 280);

  LCD.setTextDatum(BR_DATUM);
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);
  LCD.setTextPadding(LCD.textWidth(F(" 88:88 ")));
  dt = ui.rightOffset(config.wunderground->getMoonriseTime(), ":"); // Draw relative to colon to them aligned
  LCD.drawString(config.wunderground->getMoonriseTime(), 200 + dt, 300);

  dt = ui.rightOffset(config.wunderground->getMoonsetTime(), ":");
  LCD.drawString(config.wunderground->getMoonsetTime(), 200 + dt, 315);

  LCD.setTextPadding(0); // Reset padding width to none
}


// Helper function, should be part of the weather station library and should disappear soon
String ScreenWeatherStation::getMeteoconIcon(String iconText)
{
  if (iconText == F("F")) return F("chanceflurries");
  if (iconText == F("Q")) return F("chancerain");
  if (iconText == F("W")) return F("chancesleet");
  if (iconText == F("V")) return F("chancesnow");
  if (iconText == F("S")) return F("chancestorms");
  if (iconText == F("B")) return F("clear");
  if (iconText == F("Y")) return F("cloudy");
  if (iconText == F("F")) return F("flurries");
  if (iconText == F("M")) return F("fog");
  if (iconText == F("E")) return F("hazy");
  if (iconText == F("Y")) return F("mostlycloudy");
  if (iconText == F("H")) return F("mostlysunny");
  if (iconText == F("H")) return F("partlycloudy");
  if (iconText == F("J")) return F("partlysunny");
  if (iconText == F("W")) return F("sleet");
  if (iconText == F("R")) return F("rain");
  if (iconText == F("W")) return F("snow");
  if (iconText == F("B")) return F("sunny");
  if (iconText == F("0")) return F("tstorms");

  return F("unknown");
}

void ScreenWeatherStation::deactivate()
{
#ifdef DEBUG_SYSLOG 
  syslog.log(LOG_INFO, F("ScreenWeatherStation::deactivate()"));
#endif

}


bool ScreenWeatherStation::onUserEvent(int event)
{
  return false;
}

long ScreenWeatherStation::getRefreshPeriod()
{
  return 5000;
}

String ScreenWeatherStation::getScreenName()
{
  return F("Weather Station");
}

bool ScreenWeatherStation::isFullScreen()
{
  return false;
}

bool ScreenWeatherStation::getRefreshWithScreenOff()
{
  return false;
}

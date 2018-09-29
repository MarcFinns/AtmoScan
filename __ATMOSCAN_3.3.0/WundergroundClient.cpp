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

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <Arduino.h>
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include "GlobalDefinitions.h"
#include "WundergroundClient.h"

// External variables
extern Syslog syslog;
extern WiFiClient wifiClient;

bool usePM = false; // Set to true if you want to use AM/PM time disaply
bool isPM = false; // JJG added ///////////

WundergroundClient::WundergroundClient(bool _isMetric) {
  isMetric = _isMetric;
}

// Added by fowlerk, 12/22/16, as an option to change metric setting other than at instantiation
void WundergroundClient::initMetric(bool _isMetric) {
  isMetric = _isMetric;
}
// end add fowlerk, 12/22/16

///////////////////  MarcFinns Additions ////////////////

bool WundergroundClient::updateLocation(String apiKey, float lat, float lon)
{
  return doUpdate(String(F("/api/")) + apiKey + F("/geolookup/q/") + String(lat)  + F(",") + String(lon) + F(".json"));
}

///////////////////////////////////////////////


bool WundergroundClient::updateConditions(String apiKey, String language, String country, String city) {
  isForecast = false;
  return doUpdate(String(F("/api/")) + apiKey + F("/conditions/lang:") + language + F("/q/") + country + F("/") + city + F(".json"));
}

// wunderground change the API URL scheme:
// http://api.wunderground.com/api/<API-KEY>/conditions/lang:de/q/zmw:00000.215.10348.json
bool WundergroundClient::updateConditions(String apiKey, String language, String zmwCode) {
  isForecast = false;
  return doUpdate(String(F("/api/")) + apiKey + F("/conditions/lang:") + language + F("/q/zmw:") + zmwCode + F(".json"));
}

bool WundergroundClient::updateForecast(String apiKey, String language, String country, String city) {
  isForecast = true;

  return doUpdate(String(F("/api/")) + apiKey + F("/forecast/lang:") + language + F("/q/") + country + F("/") + city + F(".json"));
}

// JJG added ////////////////////////////////
bool WundergroundClient::updateAstronomy(String apiKey, String language, String country, String city) {
  isForecast = true;
  return doUpdate(String(F("/api/"))   + apiKey + F("/astronomy/lang:") + language + F("/q/") + country + F("/") + city + F(".json"));
}
// end JJG add  ////////////////////////////////////////////////////////////////////
/*
  // fowlerk added
  void WundergroundClient::updateAlerts(String apiKey, String language, String country, String city) {
  currentAlert = 0;
  activeAlertsCnt = 0;
  isForecast = false;
  isSimpleForecast = false;
  isCurrentObservation = false;
  isAlerts = true;
  if (country == "US") {
    isAlertUS = true;
    isAlertEU = false;
  } else {
    isAlertUS = false;
    isAlertEU = true;
  }
  doUpdate("/api/" + apiKey + "/alerts/lang:" + language + "/q/" + country + "/" + city + ".json");
  }
  // end fowlerk add
*/

bool WundergroundClient::doUpdate(String url)
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("URL = ")) + url);
#endif
  url.replace(F(" "), F("%20"));

  JsonStreamingParser parser;
  parser.setListener(this);

  const int httpPort = 80;
  if (!wifiClient.connect(F("api.wunderground.com"), httpPort))
  {
#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, F("connection failed"));
#endif
    return false;
  }

  // This will send the request to the server
  wifiClient.print(F("GET "));
  wifiClient.print(url);
  wifiClient.print(F(" HTTP/1.1\r\nHost: api.wunderground.com\r\nConnection: close\r\n\r\n"));

  int retryCounter = 0;
  while (!wifiClient.available())
  {
    delay(1000);
    retryCounter++;
    if (retryCounter > 10)
    {
#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, F("Too many retries, giving up"));
#endif
      return false;
    }
  }

  int pos = 0;
  bool isBody = false;
  char c;

  int size = 0;
  wifiClient.setNoDelay(false);
  while (wifiClient.connected()) {
    while ((size = wifiClient.available()) > 0) {
      c = wifiClient.read();
      //response +=c;
      if (c == '{' || c == '[') {
        isBody = true;
      }
      if (isBody)
      {
        parser.parse(c);
      }
    }
  }
  wifiClient.stop();

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("Job done"));
#endif

  return true;
}


void WundergroundClient::whitespace(char c) {
  // #ifdef DEBUG_SERIAL Serial.println("whitespace");
}

void WundergroundClient::startDocument() {
  // #ifdef DEBUG_SERIAL Serial.println("start document");
}

void WundergroundClient::key(String key)
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("KEY =  ")) + key);
#endif


  currentKey = String(key);

  // #ifdef DEBUG_SERIAL Serial.println("Key = " + currentKey);


  //	Restructured following logic to accomodate the multiple types of JSON returns based on the API.  This was necessary since several
  //	keys are reused between various types of API calls, resulting in confusing returns in the original function.  Various bools
  //	now indicate whether the JSON stream being processed is part of the text forecast (txt_forecast), the first section of the 10-day
  //	forecast API that contains detailed text for the forecast period; the simple forecast (simpleforecast), the second section of the
  //	10-day forecast API that contains such data as forecast highs/lows, conditions, precipitation / probabilities; the current
  //	observations (current_observation), from the observations API call; or alerts (alerts), for the future) weather alerts API call.

  //    Added by MarcFinns 15 Feb 2017
  if (currentKey == F("geolookup"))
  {
    isGeolookup = true;
    isForecast = false;
    isCurrentObservation = false; // fowlerk
    isSimpleForecast = false;   // fowlerk
    /*
      isAlerts = false;       // fowlerk
    */
  }


  //		Added by fowlerk...18-Dec-2016
  if (currentKey == F("txt_forecast")) {
    isForecast = true;
    isGeolookup = false;
    isCurrentObservation = false;	// fowlerk
    isSimpleForecast = false;		// fowlerk
    //   isAlerts = false;				// fowlerk
  }
  if (currentKey == F("simpleforecast")) {
    isSimpleForecast = true;
    isGeolookup = false;
    isCurrentObservation = false;	// fowlerk
    isForecast = false;				// fowlerk
    //   isAlerts = false;				// fowlerk
  }
  //  Added by fowlerk...
  if (currentKey == F("current_observation")) {
    isCurrentObservation = true;
    isGeolookup = false;
    isSimpleForecast = false;
    isForecast = false;
    //   isAlerts = false;
  }
  if (currentKey == F("alerts")) {
    isCurrentObservation = false;
    isGeolookup = false;
    isSimpleForecast = false;
    isForecast = false;
    //    isAlerts = true;
  }
  // end fowlerk add
}

void WundergroundClient::value(String value)
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("VALUE = ")) + value);
#endif

  /*
    if (currentKey == F("local_epoch"))
    {
    localEpoc = value.toInt();
    localMillisAtUpdate = millis();
    }
  */
  // JJG added ... //////////////////////// search for keys /////////////////////////
  if (currentKey == F("percentIlluminated"))
  {
    moonPctIlum = value;
  }

  if (currentKey == F("ageOfMoon"))
  {
    moonAge = value;
  }

  if (currentKey == F("phaseofMoon"))
  {
    moonPhase = value;
  }

  if (currentParent == F("sunrise")) {      // Has a Parent key and 2 sub-keys
    if (currentKey == F("hour")) {
      int tempHour = value.toInt();    // do this to concert to 12 hour time (make it a function!)
      if (usePM && tempHour > 12) {
        tempHour -= 12;
        isPM = true;
      }
      else isPM = false;
      char tempHourBuff[3] = "";						// fowlerk add for formatting, 12/22/16
      sprintf(tempHourBuff, "%2d", tempHour);			// fowlerk add for formatting, 12/22/16
      sunriseTime = String(tempHourBuff);				// fowlerk add for formatting, 12/22/16
      //sunriseTime = value;
    }
    if (currentKey == F("minute")) {
      char tempMinBuff[3] = "";						// fowlerk add for formatting, 12/22/16
      sprintf(tempMinBuff, "%02d", value.toInt());	// fowlerk add for formatting, 12/22/16
      sunriseTime += ":" + String(tempMinBuff);		// fowlerk add for formatting, 12/22/16
      if (isPM) sunriseTime += F("pm");
      else if (usePM) sunriseTime += F("am");
    }
  }


  if (currentParent == F("sunset")) {      // Has a Parent key and 2 sub-keys
    if (currentKey == F("hour")) {
      int tempHour = value.toInt();   // do this to concert to 12 hour time (make it a function!)
      if (usePM && tempHour > 12) {
        tempHour -= 12;
        isPM = true;
      }
      else isPM = false;
      char tempHourBuff[3] = "";						// fowlerk add for formatting, 12/22/16
      sprintf(tempHourBuff, "%2d", tempHour);			// fowlerk add for formatting, 12/22/16
      sunsetTime = String(tempHourBuff);				// fowlerk add for formatting, 12/22/16
      // sunsetTime = value;
    }
    if (currentKey == F("minute")) {
      char tempMinBuff[3] = "";						// fowlerk add for formatting, 12/22/16
      sprintf(tempMinBuff, "%02d", value.toInt());	// fowlerk add for formatting, 12/22/16
      sunsetTime += ":" + String(tempMinBuff);		// fowlerk add for formatting, 12/22/16
      if (isPM) sunsetTime += F("pm");
      else if (usePM) sunsetTime += F("am");
    }
  }

  if (currentParent == F("moonrise")) {      // Has a Parent key and 2 sub-keys
    if (currentKey == F("hour")) {
      int tempHour = value.toInt();   // do this to concert to 12 hour time (make it a function!)
      if (usePM && tempHour > 12) {
        tempHour -= 12;
        isPM = true;
      }
      else isPM = false;
      char tempHourBuff[3] = "";						// fowlerk add for formatting, 12/22/16
      sprintf(tempHourBuff, "%2d", tempHour);			// fowlerk add for formatting, 12/22/16
      moonriseTime = String(tempHourBuff);			// fowlerk add for formatting, 12/22/16
      // moonriseTime = value;
    }
    if (currentKey == F("minute")) {
      char tempMinBuff[3] = "";						// fowlerk add for formatting, 12/22/16
      sprintf(tempMinBuff, "%02d", value.toInt());	// fowlerk add for formatting, 12/22/16
      moonriseTime = moonriseTime + F(":") + String(tempMinBuff);		// fowlerk add for formatting, 12/22/16
      if (isPM) moonriseTime += F("pm");
      else if (usePM) moonriseTime += F("am");
    }
  }

  if (currentParent == F("moonset")) {      // Not used - has a Parent key and 2 sub-keys
    if (currentKey == F("hour")) {
      char tempHourBuff[3] = "";						// fowlerk add for formatting, 12/22/16
      sprintf(tempHourBuff, "%2d", value.toInt());	// fowlerk add for formatting, 12/22/16
      moonsetTime = String(tempHourBuff);				// fowlerk add for formatting, 12/22/16
    }
    if (currentKey == F("minute")) {
      char tempMinBuff[3] = "";						// fowlerk add for formatting, 12/22/16
      sprintf(tempMinBuff, "%02d", value.toInt());	// fowlerk add for formatting, 12/22/16
      moonsetTime = moonsetTime + F(":") + String(tempMinBuff);		// fowlerk add for formatting, 12/22/16
    }
  }

  if (currentKey == F("wind_mph")) {
    windSpeed = value;
  }

  if (currentKey == F("wind_dir")) {
    windDir = value;
  }

  // end JJG add  ////////////////////////////////////////////////////////////////////
  /*
    if (currentKey == F("observation_time_rfc822")) {
      date = value.substring(0, 16);
    }

    // Begin add, fowlerk...04-Dec-2016
    if (currentKey == F("observation_time")) {
      observationTime = value;
    }
    // end add, fowlerk
  */
  if (currentKey == F("temp_f") && !isMetric) {
    currentTemp = value;
  }
  if (currentKey == F("temp_c") && isMetric) {
    currentTemp = value;
  }
  if (currentKey == F("icon")) {
    if (isForecast && !isSimpleForecast &&  currentForecastPeriod < MAX_FORECAST_PERIODS) {
      // #ifdef DEBUG_SERIAL Serial.println(String(currentForecastPeriod) + ": " + value + ":" + currentParent);
      forecastIcon[currentForecastPeriod] = value;
    }
    // if (!isForecast) {													// Removed by fowlerk
    if (isCurrentObservation && !(isForecast || isSimpleForecast)) {		// Added by fowlerk
      weatherIcon = value;
    }
  }
  if (currentKey == F("weather")) {
    weatherText = value;
  }

  /*
    if (currentKey == F("relative_humidity")) {
    humidity = value;
    }
    if (currentKey == F("pressure_mb") && isMetric) {
    pressure = value + "mb";
    }
    if (currentKey == F("pressure_in") && !isMetric) {
    pressure = value + "in";
    }
    // fowlerk added...
    if (currentKey == F("feelslike_f") && !isMetric) {
    feelslike = value;
    }

    if (currentKey == F("feelslike_c") && isMetric) {
    feelslike = value;
    }

    if (currentKey == F("UV")) {
      UV = value;
    }
    // Active alerts...added 18-Dec-2016
    if (currentKey == F("type") && isAlerts) {
      activeAlertsCnt++;
      currentAlert++;
      activeAlerts[currentAlert - 1] = value;
      // #ifdef DEBUG_SERIAL Serial.print("Alert type processed, value:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlerts[currentAlert - 1]);
    }

    if (currentKey == F("description") && isAlerts && isAlertUS) {
      activeAlertsText[currentAlert - 1] = value;
      // #ifdef DEBUG_SERIAL Serial.print("Alert description processed, value:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlertsText[currentAlert - 1]);
    }
    if (currentKey == F("wtype_meteoalarm_name") && isAlerts && isAlertEU) {
      activeAlertsText[currentAlert - 1] = value;
      // #ifdef DEBUG_SERIAL Serial.print("Alert description processed, value:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlertsText[currentAlert - 1]);
    }
    if (currentKey == F("message") && isAlerts) {
      activeAlertsMessage[currentAlert - 1] = value;
      // #ifdef DEBUG_SERIAL Serial.print("Alert msg length:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlertsMessage[currentAlert - 1].length());
      if (activeAlertsMessage[currentAlert - 1].length() >= 511) {
        activeAlertsMessageTrunc[currentAlert - 1] = true;
      } else {
        activeAlertsMessageTrunc[currentAlert - 1] = false;
      }
      // #ifdef DEBUG_SERIAL Serial.print("Alert message processed, value:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlertsMessage[currentAlert - 1]);
    }
    if (currentKey == ("date") && isAlerts) {
      activeAlertsStart[currentAlert - 1] = value;
      // Check last char for a "/"; the returned value sometimes includes this; if so, strip it (47 is a "/" char)
      if (activeAlertsStart[currentAlert - 1].charAt(activeAlertsStart[currentAlert - 1].length() - 1) == 47) {
        // #ifdef DEBUG_SERIAL Serial.println("...last char is a slash...");
        activeAlertsStart[currentAlert - 1] = activeAlertsStart[currentAlert - 1].substring(0, (activeAlertsStart[currentAlert - 1].length() - 1));
      }
      // For meteoalarms, the start field is returned with the UTC=0 by default (not used?)
      if (isAlertEU && activeAlertsStart[currentAlert - 1] == F("1970-01-01 00:00:00 GMT")) {
        activeAlertsStart[currentAlert - 1] = F("<Not specified>");
      }
      // #ifdef DEBUG_SERIAL Serial.print("Alert start processed, value:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlertsStart[currentAlert - 1]);
    }
    if (currentKey == F("expires") && isAlerts) {
      activeAlertsEnd[currentAlert - 1] = value;
      // #ifdef DEBUG_SERIAL Serial.print("Alert expiration processed, value:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlertsEnd[currentAlert - 1]);
    }
    if (currentKey == F("phenomena") && isAlerts) {
      activeAlertsPhenomena[currentAlert - 1] = value;
      // #ifdef DEBUG_SERIAL Serial.print("Alert phenomena processed, value:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlertsPhenomena[currentAlert - 1]);
    }
    if (currentKey == F("significance") && isAlerts && isAlertUS) {
      activeAlertsSignificance[currentAlert - 1] = value;
      // #ifdef DEBUG_SERIAL Serial.print("Alert significance processed, value:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlertsSignificance[currentAlert - 1]);
    }
    // Map meteoalarm level to the field for significance for consistency (used for European alerts)
    if (currentKey == F("level_meteoalarm") && isAlerts && isAlertEU) {
      activeAlertsSignificance[currentAlert - 1] = value;
      // #ifdef DEBUG_SERIAL Serial.print("Meteo alert significance processed, value:  "); // #ifdef DEBUG_SERIAL Serial.println(activeAlertsSignificance[currentAlert - 1]);
    }
    // For meteoalarms only (European alerts); attribution must be displayed according to the T&C's of use
    if (currentKey == F("attribution") && isAlerts) {
      activeAlertsAttribution[currentAlert - 1] = value;
      // Remove some of the markup in the attribution
      activeAlertsAttribution[currentAlert - 1].replace(" <a href='", " ");
      activeAlertsAttribution[currentAlert - 1].replace("</a>", "");
      activeAlertsAttribution[currentAlert - 1].replace("/'>", " ");
    }

    // end fowlerk add

    if (currentKey == F("dewpoint_f") && !isMetric) {
      dewPoint = value;
    }
    if (currentKey == F("dewpoint_c") && isMetric) {
      dewPoint = value;
    }
    if (currentKey == F("precip_today_metric") && isMetric) {
      precipitationToday = value + "mm";
    }
    if (currentKey == F("precip_today_in") && !isMetric) {
      precipitationToday = value + "in";
    }
  */
  if (currentKey == F("period")) {
    currentForecastPeriod = value.toInt();
  }
  // Modified below line to add check to ensure we are processing the 10-day forecast
  // before setting the forecastTitle (day of week of the current forecast day).
  // (The keyword title is used in both the current observation and the 10-day forecast.)
  //		Modified by fowlerk
  // if (currentKey ==F("title") && currentForecastPeriod < MAX_FORECAST_PERIODS) {				// Removed, fowlerk
  if (currentKey == F("title") && isForecast && currentForecastPeriod < MAX_FORECAST_PERIODS) {
    // #ifdef DEBUG_SERIAL Serial.println(String(currentForecastPeriod) + ": " + value);
    forecastTitle[currentForecastPeriod] = value;
  }

  /*
    // Added forecastText key following...fowlerk, 12/3/16
    if (currentKey == F("fcttext") && isForecast && !isMetric && currentForecastPeriod < MAX_FORECAST_PERIODS) {
      forecastText[currentForecastPeriod] = value;
    }
    // Added option for metric forecast following...fowlerk, 12/22/16
    if (currentKey == F("fcttext_metric") && isForecast && isMetric && currentForecastPeriod < MAX_FORECAST_PERIODS) {
      forecastText[currentForecastPeriod] = value;
    }
    // end fowlerk add, 12/3/16

        // Added PoP (probability of precipitation) key following...fowlerk, 12/22/16
        if (currentKey == F("pop") && isForecast && currentForecastPeriod < MAX_FORECAST_PERIODS) {
          PoP[currentForecastPeriod] = value;
        }
        // end fowlerk add, 12/22/16
  */
  // The detailed forecast period has only one forecast per day with low/high for both
  // night and day, starting at index 1.
  int dailyForecastPeriod = (currentForecastPeriod - 1) * 2;

  if (currentKey == F("fahrenheit") && !isMetric && dailyForecastPeriod < MAX_FORECAST_PERIODS) {

    if (currentParent == F("high")) {
      forecastHighTemp[dailyForecastPeriod] = value;
    }
    if (currentParent == F("low")) {
      forecastLowTemp[dailyForecastPeriod] = value;
    }
  }
  if (currentKey == F("celsius") && isMetric && dailyForecastPeriod < MAX_FORECAST_PERIODS) {

    if (currentParent == F("high")) {
      // #ifdef DEBUG_SERIAL Serial.println(String(currentForecastPeriod) + ": " + value);
      forecastHighTemp[dailyForecastPeriod] = value;
    }
    if (currentParent == F("low")) {
      forecastLowTemp[dailyForecastPeriod] = value;
    }
  }

  /*
    // fowlerk added...to pull month/day from the forecast period
    if (currentKey == F("month") && isSimpleForecast && currentForecastPeriod < MAX_FORECAST_PERIODS)
    {
    //	Added by fowlerk to handle transition from txtforecast to simpleforecast, as
    //	the key "period" doesn't appear until after some of the key values needed and is
    //	used as an array index.
    if (isSimpleForecast && currentForecastPeriod == 19)
    {
      currentForecastPeriod = 0;
    }
    forecastMonth[currentForecastPeriod] = value;
    }


    if (currentKey == F("day") && isSimpleForecast && currentForecastPeriod < MAX_FORECAST_PERIODS)  {
    //	Added by fowlerk to handle transition from txtforecast to simpleforecast, as
    //	the key "period" doesn't appear until after some of the key values needed and is
    //	used as an array index.
    if (isSimpleForecast && currentForecastPeriod == 19) {
      currentForecastPeriod = 0;
    }
    forecastDay[currentForecastPeriod] = value;
    }
    // end fowlerk add
  */

  // MarcFinns mods
  if (isGeolookup && currentParent == F("location" ))
  {
    if (currentKey == F("country"))
      country = value;
    else if (currentKey == F("country_name"))
      country_name = value;
    else if (currentKey == F("city"))
      city = value;
    else if (currentKey == F("tz_short"))
      tz_short = value;
    else if (currentKey == F("tz_long"))
      tz_long = value;
  }
  // end MarcFinns mods
}

void WundergroundClient::endArray() {
  // #ifdef DEBUG_SERIAL Serial.println("end array. ");
}


void WundergroundClient::startObject()
{
  // #ifdef DEBUG_SERIAL Serial.println("start object. " + currentKey);
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("start object =  ")) + currentKey);
#endif

  currentParent = currentKey;
}

void WundergroundClient::endObject()
{
  // #ifdef DEBUG_SERIAL Serial.println("end object. " + currentParent);
  currentParent = FPSTR(empty);
}

void WundergroundClient::endDocument()
{
  // #ifdef DEBUG_SERIAL Serial.println("end document. ");

}

void WundergroundClient::startArray()
{
  // #ifdef DEBUG_SERIAL Serial.println("start array. ");
}

/*
  String WundergroundClient::getHours() {
  if (localEpoc == 0) {
  return "--";
  }
  int hours = (getCurrentEpoch()  % 86400L) / 3600 + gmtOffset;
  if (hours < 10) {
  return "0" + String(hours);
  }
  return String(hours); // print the hour (86400 equals secs per day)

  }
  String WundergroundClient::getMinutes() {
  if (localEpoc == 0) {
  return "--";
  }
  int minutes = ((getCurrentEpoch() % 3600) / 60);
  if (minutes < 10 ) {
  // In the first 10 minutes of each hour, we'll want a leading '0'
  return "0" + String(minutes);
  }
  return String(minutes);
  }
  String WundergroundClient::getSeconds() {
  if (localEpoc == 0) {
  return "--";
  }
  int seconds = getCurrentEpoch() % 60;
  if ( seconds < 10 ) {
  // In the first 10 seconds of each minute, we'll want a leading '0'
  return "0" + String(seconds);
  }
  return String(seconds);
  }
  String WundergroundClient::getDate() {
  return date;
  }
  long WundergroundClient::getCurrentEpoch() {
  return localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  }
*/
// JJG added ... /////////////////////////////////////////////////////////////////////////////////////////
String WundergroundClient::getMoonPctIlum() {
  return moonPctIlum;
}

String WundergroundClient::getMoonAge() {
  return moonAge;
}

String WundergroundClient::getMoonPhase() {
  return moonPhase;
}

String WundergroundClient::getSunriseTime() {
  return sunriseTime;
}

String WundergroundClient::getSunsetTime() {
  return sunsetTime;
}

String WundergroundClient::getMoonriseTime() {
  return moonriseTime;
}

String WundergroundClient::getMoonsetTime() {
  return moonsetTime;
}

String WundergroundClient::getWindSpeed() {
  return windSpeed;
}

String WundergroundClient::getWindDir() {
  return windDir;
}

// end JJG add ////////////////////////////////////////////////////////////////////////////////////////////


String WundergroundClient::getCurrentTemp() {
  return currentTemp;
}

String WundergroundClient::getWeatherText() {
  return weatherText;
}

/*
  String WundergroundClient::getHumidity() {
  return humidity;
  }

  String WundergroundClient::getPressure() {
  return pressure;
  }

  String WundergroundClient::getDewPoint() {
  return dewPoint;
  }
  // fowlerk added...
  String WundergroundClient::getFeelsLike() {
  return feelslike;
  }

  String WundergroundClient::getUV() {
  return UV;
  }
*/

/*
  // Added by fowlerk, 04-Dec-2016
  String WundergroundClient::getObservationTime() {
  return observationTime;
  }

  // Active alerts...added 18-Dec-2016
  String WundergroundClient::getActiveAlerts(int alertIndex) {
  return activeAlerts[alertIndex];
  }

  String WundergroundClient::getActiveAlertsText(int alertIndex) {
  return activeAlertsText[alertIndex];
  }

  String WundergroundClient::getActiveAlertsMessage(int alertIndex) {
  return activeAlertsMessage[alertIndex];
  }

  bool WundergroundClient::getActiveAlertsMessageTrunc(int alertIndex) {
  return activeAlertsMessageTrunc[alertIndex];
  }

  String WundergroundClient::getActiveAlertsStart(int alertIndex) {
  return activeAlertsStart[alertIndex];
  }

  String WundergroundClient::getActiveAlertsEnd(int alertIndex) {
  return activeAlertsEnd[alertIndex];
  }

  String WundergroundClient::getActiveAlertsPhenomena(int alertIndex) {
  return activeAlertsPhenomena[alertIndex];
  }

  String WundergroundClient::getActiveAlertsSignificance(int alertIndex) {
  return activeAlertsSignificance[alertIndex];
  }

  String WundergroundClient::getActiveAlertsAttribution(int alertIndex) {
  return activeAlertsAttribution[alertIndex];
  }

  int WundergroundClient::getActiveAlertsCnt() {
  return activeAlertsCnt;
  }

  // end fowlerk add
*/

// start MarcFinns mods
String WundergroundClient::getCountry()
{
  return country;
}
String WundergroundClient::getCountryName()
{
  return country_name;
}

String WundergroundClient::getCity()
{
  return city;
}

String WundergroundClient::getTZ_Short()
{
  return tz_short;
}

String WundergroundClient::getTZ_Long()
{
  return tz_long;
}

// end MarcFinns mods

/*
  String WundergroundClient::getPrecipitationToday() {
  return precipitationToday;
  }
*/


String WundergroundClient::getTodayIcon() {
  return getMeteoconIcon(weatherIcon);
}

/*
  String WundergroundClient::getTodayIconText() {
  return weatherIcon;
  }
*/
String WundergroundClient::getForecastIcon(int period) {
  return getMeteoconIcon(forecastIcon[period]);
}

String WundergroundClient::getForecastTitle(int period) {
  return forecastTitle[period];
}

String WundergroundClient::getForecastLowTemp(int period) {
  return forecastLowTemp[period];
}

String WundergroundClient::getForecastHighTemp(int period) {
  return forecastHighTemp[period];
}

/*
  // fowlerk added...
  String WundergroundClient::getForecastDay(int period) {
  //  // #ifdef DEBUG_SERIAL Serial.print("Day period:  "); // #ifdef DEBUG_SERIAL Serial.println(period);
  return forecastDay[period];
  }

  String WundergroundClient::getForecastMonth(int period) {
  //  // #ifdef DEBUG_SERIAL Serial.print("Month period:  "); // #ifdef DEBUG_SERIAL Serial.println(period);
  return forecastMonth[period];
  }

  String WundergroundClient::getForecastText(int period) {
  //  // #ifdef DEBUG_SERIAL Serial.print("Forecast period:  "); // #ifdef DEBUG_SERIAL Serial.println(period);
  return forecastText[period];
  }

  // Added PoP...12/22/16
  String WundergroundClient::getPoP(int period) {
  return PoP[period];
  }
  // end fowlerk add
*/

String WundergroundClient::getMeteoconIcon(String iconText) {
  if (iconText == F("chanceflurries")) return F("F");
  if (iconText == F("chancerain")) return F("Q");
  if (iconText == F("chancesleet")) return F("W");
  if (iconText == F("chancesnow")) return F("V");
  if (iconText == F("chancestorms")) return F("S");
  if (iconText == F("clear")) return F("B");
  if (iconText == F("cloudy")) return F("Y");
  if (iconText == F("flurries")) return F("F");
  if (iconText == F("fog")) return F("M");
  if (iconText == F("hazy")) return F("E");
  if (iconText == F("mostlycloudy")) return F("Y");
  if (iconText == F("mostlysunny")) return F("H");
  if (iconText == F("partlycloudy")) return F("H");
  if (iconText == F("partlysunny")) return F("J");
  if (iconText == F("sleet")) return F("W");
  if (iconText == F("rain")) return F("R");
  if (iconText == F("snow")) return F("W");
  if (iconText == F("sunny")) return F("B");
  if (iconText == F("tstorms")) return F("0");

  if (iconText == F("nt_chanceflurries")) return F("F");
  if (iconText == F("nt_chancerain")) return F("7");
  if (iconText == F("nt_chancesleet")) return F("#");
  if (iconText == F("nt_chancesnow")) return F("#");
  if (iconText == F("nt_chancestorms")) return F("&");
  if (iconText == F("nt_clear")) return F("2");
  if (iconText == F("nt_cloudy")) return F("Y");
  if (iconText == F("nt_flurries")) return F("9");
  if (iconText == F("nt_fog")) return F("M");
  if (iconText == F("nt_hazy")) return F("E");
  if (iconText == F("nt_mostlycloudy")) return F("5");
  if (iconText == F("nt_mostlysunny")) return F("3");
  if (iconText == F("nt_partlycloudy")) return F("4");
  if (iconText == F("nt_partlysunny")) return F("4");
  if (iconText == F("nt_sleet")) return F("9");
  if (iconText == F("nt_rain")) return F("7");
  if (iconText == F("nt_snow")) return F("#");
  if (iconText == F("nt_sunny")) return F("4");
  if (iconText == F("nt_tstorms")) return F("&");

  return F(")");
}

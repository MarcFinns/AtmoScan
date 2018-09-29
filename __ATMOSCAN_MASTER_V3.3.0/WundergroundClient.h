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

#pragma once

#include <JsonStreamingParser.h> // https://github.com/squix78/json-streaming-parser

#define MAX_FORECAST_PERIODS 6  // Changed from 7 to 12 to support 6 day / 2 screen forecast (Neptune)
// Changed to 20 to support max 10-day forecast returned from 'forecast10day' API (fowlerk)

#define MAX_WEATHER_ALERTS 3  	 // The maximum number of concurrent weather alerts supported by the library

class WundergroundClient: public JsonListener
{
  private:
    String currentKey;
    String currentParent = "";
    long localEpoc = 0;
    int gmtOffset = 1;
    // long localMillisAtUpdate;
    String date = "-";
    bool isMetric = true;
    String currentTemp;
    // JJG added ... ////////////////////////////////// define returns /////////////////////////////////
    String moonPctIlum;  // not used
    String moonAge;      // make this a long?
    String moonPhase;
    String sunriseTime;
    String sunsetTime;
    String moonriseTime;
    String moonsetTime;
    String windSpeed;
    String windDir;
    // end JJG add ////////////////////////////////////////////////////////////////////////////////////
    String weatherIcon;
    String weatherText;

    //String humidity;
    //String pressure;
    //String dewPoint;

    // String precipitationToday;

    // fowlerk added...
    // String feelslike;
    // String UV;
    // String observationTime;					// fowlerk add, 04-Dec-2016
    // end fowlerk add

    // MarcFinns additions...
    String country;
    String city;
    String country_name;
    String tz_short;
    String tz_long;
    ////

    bool doUpdate(String url);

    // Status variables
    // bool isValid = false;
    bool isGeolookup = false;
    bool isForecast = false;
    bool isSimpleForecast = false;		// true;  fowlerk
    bool isCurrentObservation = false;	// Added by fowlerk
    /*
      bool isAlerts = false;				// Added by fowlerk
      bool isAlertUS = false;				// Added by fowlerk
      bool isAlertEU = false;				// Added by fowlerk
    */
    int currentForecastPeriod;
    String forecastIcon [MAX_FORECAST_PERIODS];
    String forecastTitle [MAX_FORECAST_PERIODS];
    String forecastLowTemp [MAX_FORECAST_PERIODS];
    String forecastHighTemp [MAX_FORECAST_PERIODS];

  public:
    bool isValid = false;
    WundergroundClient(bool isMetric);
    bool updateLocation(String apiKey, float lat, float lon);
    bool updateConditions(String apiKey, String language, String country, String city);
    bool updateConditions(String apiKey, String language, String zmwCode);
    bool updateForecast(String apiKey, String language, String country, String city);
    bool updateAstronomy(String apiKey, String language, String country, String city);

    void initMetric(bool isMetric);			// Added by fowlerk, 12/22/16, as an option to change metric setting other than at instantiation
    long lastDownloadUpdate = - 100000;


    // JJG added ... ///////////////////function name to string ////////////////////////////
    String getMoonPctIlum();
    String getMoonAge();
    String getMoonPhase();
    String getSunriseTime();
    String getSunsetTime();
    String getMoonriseTime();
    String getMoonsetTime();
    String getWindSpeed();
    String getWindDir();
    // end JJG add ///////////////////////////////////////////////////////////////////////
    // long getCurrentEpoch();

    String getCurrentTemp();

    String getTodayIcon();

    //    String getTodayIconText();

    String getMeteoconIcon(String iconText);

    String getWeatherText();


    String getForecastIcon(int period);

    String getForecastTitle(int period);

    String getForecastLowTemp(int period);

    String getForecastHighTemp(int period);

    String getCountry();
    String getCountryName();
    String getCity();
    String getTZ_Short();
    String getTZ_Long();

    // end MarcFinns mods

    virtual void whitespace(char c);

    virtual void startDocument();

    virtual void key(String key);

    virtual void value(String value);

    virtual void endArray();

    virtual void endObject();

    virtual void endDocument();

    virtual void startArray();

    virtual void startObject();
};

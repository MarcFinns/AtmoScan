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

  Adapted by Bodmer to use the faster TFT_eSPI library:
  https://github.com/Bodmer/TFT_eSPI

*/

#pragma once


// Setup
const int UPDATE_INTERVAL_SECS = 10 * 60; // Update every 10 minutes

// TimeClient settings
const float UTC_OFFSET = 0;

// Wunderground Settings, EDIT TO SUIT YOUR LOCATION
const bool IS_METRIC = true; // Temperature only? Wind speed units appear to stay in mph. To do: investigate <<<<<<<<<<<<<<<<<<<<<<<<<
const String WUNDERGRROUND_API_KEY = "937d424ec3abf8c0";
const String WUNDERGRROUND_LANGUAGE = "EN";

// Windspeed conversion, use 1 pair of #defines. To do: investigate a more convenient method <<<<<<<<<<<<<<<<<<<<<
//#define WIND_SPEED_SCALING 1.0      // mph
//#define WIND_SPEED_UNITS " mph"

//#define WIND_SPEED_SCALING 0.868976 // mph to knots
//#define WIND_SPEED_UNITS " kn"

#define WIND_SPEED_SCALING 1.60934  // mph to kph
#define WIND_SPEED_UNITS " kph"

// List of 19 items, so that the downloader knows what to fetch
const char * const wundergroundIcons[] PROGMEM = {"chanceflurries", "chancerain", "chancesleet", "chancesnow", "clear", "cloudy", "flurries", "fog", "hazy", "mostlycloudy", "mostlysunny", "partlycloudy", "partlysunny", "rain", "sleet", "snow", "sunny", "tstorms", "unknown"};

const char URL1[] PROGMEM = "http://www.squix.org/blog/wunderground/";
const char URL2[] PROGMEM = "http://www.squix.org/blog/wunderground/mini/";
const char URL3[] PROGMEM = "http://www.squix.org/blog/moonphase_L";
const char FILETYPE[] PROGMEM = ".bmp";
const char MOON[] PROGMEM = "/moon";
const char MINI[] PROGMEM = "/mini/";



/***************************
   End Settings
 **************************/

// #endif

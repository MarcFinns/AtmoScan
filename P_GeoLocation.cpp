
#include "P_GeoLocation.h"
#include "GlobalDefinitions.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <NtpClientLib.h>         // https://github.com/gmag11/NtpClient
#include <TimeSpace.h>            // https://github.com/MarcFinns/TimeSpaceLib

// External variables
extern Syslog syslog;
extern struct Configuration config;

// Prototypes
void errLog(String msg);

void Proc_GeoLocation::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, "Geolocation Setup");
#endif
}

void Proc_GeoLocation::service()
{

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, "Geolocation Service");
#endif
  // Service only if connected
  if (config.connected)
  {
    // invalidate current location
    valid = false;

    //  Geolocation -  Acquire coordinates

#ifdef DEBUG_SERIAL
    Serial.println("1 ------- Retrieving coordinates...");
#endif

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_INFO, "Geolocation 1 ------- Retrieving coordinates...");
#endif

    Geolocate geolocate;

    // Acquire coordinate
    if (!geolocate.acquire())
    {
      errLog(F("Geolocation Failure step 1"));

      // in case of failure, remember it
      valid = false;

      // Retry more frequently
      this->setPeriod(RETRY_INTERVAL);

      return;
    }

    // Save variables
    latitude = geolocate.getLatitude();
    longitude = geolocate.getLongitude();

    // Acquire timezone and daylight saving


#ifdef DEBUG_SYSLOG
    syslog.log(LOG_INFO, "Geolocation 2 ----------- Retrieving timezone...");
#endif

    // Acquire timezone

    Timezone timezone;

    if (!timezone.acquire(latitude, longitude))
    {
      errLog(F("Geolocation Failure step 2"));

      // in case of failure, remember it
      valid = false;

      // Retry more frequently
      this->setPeriod(RETRY_INTERVAL);

      // and stop here
      return;
    }

    // Save variables
    int  utcOffset = timezone.getUtcOffset();
    bool   dst = timezone.isDst();
    //    timeZoneId = timezone.getTimeZoneId();
    //    timeZoneName = timezone.getTimeZoneName();

    // Acquire location name
#ifdef DEBUG_SYSLOG
    syslog.log(LOG_INFO, "Geolocation 3 ----------- Acquiring locality...");
#endif

    Geocode geocode;

    if (!geocode.acquire(latitude, longitude))
    {
      errLog(F("Geolocation Failure step 3"));

      // in case of failure, remember it
      valid = false;

      // Retry more frequently
      this->setPeriod(RETRY_INTERVAL);

      // and stop here
      return;
    }

    // Save variables
    locality = geocode.getLocality();
    // country = geocode.getCountry();
    countryCode = geocode.getCountryCode();

    // All went well...
#ifdef DEBUG_SYSLOG
    syslog.log(LOG_INFO, "Geolocation 4 ----------- All went well...");
#endif

    // Retry less frequently
    //this->setPeriod(NORMAL_INTERVAL);

    // Dont retry anymore
    this->disable();

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_INFO, "Begin NTP sync");
#endif

    // Notify NTP of new timezone
    // NOTE: UTCOffset already contains DST offset!
    NTP.begin(FPSTR(ntpServerName), utcOffset / (3600 * (1 + dst)), dst);
    NTP.setInterval(10, 600);

    valid = true;
    /*
       // Wait until NTP time is synchronised
      #ifdef DEBUG_SYSLOG
       syslog.log(LOG_INFO, "Waiting for NTP sync");
      #endif

       // Set 10 sec timeout, not to be stuck indefinitely...
       int count = 0;
       while (NTP.getLastNTPSync() == 0 && count < 20)
       {
      #ifdef DEBUG_SYSLOG
         syslog.log(LOG_INFO, "Waiting for NTP sync");
      #endif
         delay(500);
         NTP.getTimeDateString();
         count ++;

       }

      #ifdef DEBUG_SYSLOG
       syslog.log(LOG_INFO, " retry count: " + String (count));
      #endif

       // If exited because of timeout, time and location are still invalid
       if (count > 0)
       {
         valid = true;
      #ifdef DEBUG_SYSLOG
         syslog.log(LOG_INFO, "Geospatial Valid");
      #endif
       }
       else
       {
      #ifdef DEBUG_SYSLOG
         syslog.log(LOG_INFO, "Geospatial NOT Valid");
      #endif
       }
    */

    //------------------ DEBUG -----------------------------------------------

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, F("======== TIME =================="));
    syslog.log(LOG_DEBUG, NTP.getTimeDateString());
    syslog.log(LOG_DEBUG, NTP.isSummerTime() ? F("Summer Time. ") : F("Winter Time. "));
    delay(500);
    syslog.log(LOG_DEBUG, "======== TIME ZONE ==================");
    syslog.log(LOG_DEBUG, "Raw Offset = " + String(utcOffset));
    syslog.log(LOG_DEBUG, "DST = " + String(dst));
    // syslog.log(LOG_DEBUG, "Time Zone ID = " + timeZoneId);
    // syslog.log(LOG_DEBUG, "Time Zone Name = " +  timeZoneName);
    delay(500);
    syslog.log(LOG_DEBUG, "======== COORDINATES ==================");
    syslog.log(LOG_DEBUG, "Latitude = " + String(latitude));
    syslog.log(LOG_DEBUG, "Longitude = " + String(longitude));
    delay(500);
    syslog.log(LOG_DEBUG, "======== ADDRESS ==================");
    syslog.log(LOG_DEBUG, "Locality = " + locality);
    // syslog.log(LOG_DEBUG, "country = " + country);
    syslog.log(LOG_DEBUG, "countryCode = " + countryCode);
#endif

  }
  else
  {
    // Disconnected, invalidate location
    valid = false;

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_INFO, "Geospatial NOT Valid");
#endif
  }

}


double Proc_GeoLocation::getLatitude()
{
  return latitude;
}

double Proc_GeoLocation::getLongitude()
{
  return longitude;
}

String Proc_GeoLocation::getLocality()
{
  return locality;
}

String Proc_GeoLocation::getCountryCode()
{
  return countryCode;
}

bool Proc_GeoLocation::isValid()
{
  return valid;
}



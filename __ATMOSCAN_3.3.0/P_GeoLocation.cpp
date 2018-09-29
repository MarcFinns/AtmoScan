/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#include "P_GeoLocation.h"
#include "GlobalDefinitions.h"
#include "TimeSpace.h"

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>

#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <NtpClientLib.h>         // https://github.com/gmag11/NtpClient


// External variables
extern Syslog syslog;
extern struct Configuration config;
extern struct ProcessContainer procPtr;

// Prototypes
void errLog(String msg);

void Proc_GeoLocation::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("Geolocation Setup"));
#endif

  // invalidate current location
  valid = false;

  // Initialize state machine
  step = 1;

  // Steps go fast, unless they fail...
  this->setPeriod(STEP_INTERVAL);

}

void Proc_GeoLocation::service()
{

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("Geolocation - Service()"));
#endif

  // Service only if connected
  if (config.connected)
  {
    // Make ongoing communications visible
    procPtr.UIManager.communicationsFlag(true);

    switch (step)
    {

      //  STEP 1 -  Acquire coordinates
      case 1:
        {
#ifdef DEBUG_SYSLOG
          syslog.log(LOG_INFO, F("Geolocation 1 - Retrieving coordinates..."));
#endif

          Geolocate geolocate;

          // Acquire coordinate
          if (geolocate.acquire())
          {
            // Save coordinates
            latitude = geolocate.getLatitude();
            longitude = geolocate.getLongitude();

            // Go to next step
            step = 2;

            // Quickly go to next step
            this->setPeriod(STEP_INTERVAL);

#ifdef DEBUG_SYSLOG
            syslog.log(LOG_INFO, F("Geolocation STEP 1 successful"));
#endif
          }
          else
          {
            errLog(F("Geolocation Failure step 1 (WiFi geolocation)"));

            // Wait a bit before retrying
            this->setPeriod(RETRY_INTERVAL);
          }
        }
        break;


      // STEP 2 - Acquire timezone and daylight saving
      case 2:
        {
#ifdef DEBUG_SYSLOG
          syslog.log(LOG_INFO, F("Geolocation 2 - Retrieving timezone..."));
#endif

          // Acquire timezone
          Timezone timezone;

          if (timezone.acquire(latitude, longitude))
          {
            // Save variables
            utcOffset = timezone.getUtcOffset();
            dst = timezone.isDst();
            //    timeZoneId = timezone.getTimeZoneId();
            //    timeZoneName = timezone.getTimeZoneName();

            // Go to next step
            step = 3;


            // Quickly go to next step
            this->setPeriod(STEP_INTERVAL);

#ifdef DEBUG_SYSLOG
            syslog.log(LOG_INFO, F("Geolocation STEP 2 successful"));
#endif
          }
          else
          {
            errLog(F("Geolocation Failure step 2 - Retrieving timezone"));

            // Wait a bit before retrying
            this->setPeriod(RETRY_INTERVAL);
          }
        }
        break;


      // STEP 3 - Acquire location name
      case 3:
        {
#ifdef DEBUG_SYSLOG
          syslog.log(LOG_INFO, F("Geolocation Step 3 - Acquiring locality..."));
#endif
          Geocode geocode;

          if (geocode.acquire(latitude, longitude))
          {
            // Save variables
            locality = geocode.getLocality();
            // country = geocode.getCountry();
            countryCode = geocode.getCountryCode();

            // Go to next step
            step = 4;

#ifdef DEBUG_SYSLOG
            syslog.log(LOG_INFO, F("Geolocation STEP 3 successful"));
#endif
          }
          else
          {
            errLog(F("Geolocation Failure step 3 (Geocoding)"));

            // Wait a bit before retrying
            this->setPeriod(RETRY_INTERVAL);
          }
        }
        break;
    }

    // Clear visible communications flag
    procPtr.UIManager.communicationsFlag(false);

    // If all previous steps were successful, enable NTP
    if (step == 4)
    {
      // All went well...
#ifdef DEBUG_SYSLOG
      syslog.log(LOG_INFO, F("Geolocation 4 - enabling NTP sync..."));
#endif

      // Dont retry anymore
      this->disable();

      // Notify NTP of new timezone
      // NOTE: UTCOffset already contains DST offset!
      NTP.begin(FPSTR(ntpServerName), utcOffset / (3600 * (1 + dst)), dst);
      NTP.setInterval(10, 600);

      // Location is now valid
      valid = true;

      //------------------ DEBUG LOG -----------------------------------------------

#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, F("======== TIME ZONE =================="));
      syslog.log(LOG_DEBUG, String(F("Raw Offset = ")) + String(utcOffset));
      syslog.log(LOG_DEBUG, String(F("DST = ")) + String(dst));
      // syslog.log(LOG_DEBUG, "Time Zone ID = " + timeZoneId);
      // syslog.log(LOG_DEBUG, "Time Zone Name = " +  timeZoneName);
      delay(500);
      syslog.log(LOG_DEBUG, F("======== COORDINATES =================="));
      syslog.log(LOG_DEBUG, String(F("Latitude = ")) + String(latitude));
      syslog.log(LOG_DEBUG, String(F("Longitude = ")) + String(longitude));
      delay(500);
      syslog.log(LOG_DEBUG, F("======== ADDRESS =================="));
      syslog.log(LOG_DEBUG, String(F("Locality = ")) + locality);
      // syslog.log(LOG_DEBUG, "country = " + country);
      syslog.log(LOG_DEBUG, String(F("countryCode = ")) + countryCode);
#endif

    }
  }
  else
  {
    // Disconnected, invalidate location and start sequence from scratch
    valid = false;
    step = 1;

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_INFO, F("Geolocation NOT Valid"));
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

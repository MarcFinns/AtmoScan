#pragma once

#include <ProcessScheduler.h>     // https://github.com/wizard97/ArduinoProcessScheduler

#define RETRY_INTERVAL 30000
#define NORMAL_INTERVAL 3600000

// Process definition
class Proc_GeoLocation : public Process
{
  public:
    // Call the Process constructor
    Proc_GeoLocation(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
      :  Process(manager, pr, period, iterations) {}

    double getLatitude();
    double getLongitude();
    // bool isDst();
    // int getUtcOffset();
    // String getTimeZoneId();
    // String getTimeZoneName();
    String getLocality();
    // String getCountry();
    String getCountryCode();
    bool isValid();
    // void makeInvalid();

  protected:
    virtual void setup();
    virtual void service();

    double latitude;
    double longitude;
    // bool dst = false;
    // int utcOffset = 0;
    // String timeZoneId;
    // String timeZoneName;
    String locality;
    // String country;
    String countryCode;
    bool valid;
};



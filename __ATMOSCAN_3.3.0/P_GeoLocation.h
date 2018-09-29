/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include <ProcessScheduler.h>     // https://github.com/wizard97/ArduinoProcessScheduler

#define RETRY_INTERVAL 15000
#define STEP_INTERVAL 1000

// Process definition
class Proc_GeoLocation : public Process
{
  public:
    // Call the Process constructor
    Proc_GeoLocation(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
      :  Process(manager, pr, period, iterations) {}

    double getLatitude();
    double getLongitude();
    String getLocality();
    String getCountryCode();
    bool isValid();

  protected:
    virtual void setup();
    virtual void service();
    double latitude;
    double longitude;
    bool dst = false;
    int utcOffset = 0;
    String locality;
    String countryCode;
    bool valid;
    int step;
};

/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include "ESP8266WiFi.h"
#include "JsonListener.h"
#include "WsClient.h"


#define MAX_SSIDS 8

#define min(a,b) ((a)<(b)?(a):(b))

// Geolocate
const char PROGMEM  geoLocateHost[] = "api.mylnikov.org";
//const char PROGMEM  geoLocateURL[] = "/geolocation/wifi?v=1.2&data=open&search=";
const char PROGMEM  geoLocateURL[] = "/geolocation/wifi?v=1.2&search=";

// Timezone
const char PROGMEM  timeZoneHost[] = "api.timezonedb.com";
const char PROGMEM  timeZoneURL[] = "/v2/get-time-zone?format=json&by=position&";

// GeoCode
const char PROGMEM  geoCodeHost[] = "api.geonames.org";
const char PROGMEM  geoCodeURL[] = "/findNearbyPlaceNameJSON?";

// Constants
const char PROGMEM empty[] = "";



class Timezone : public JsonListener, public WsClient
{
  public:
    Timezone()
    {
#ifdef DEBUG_LOG
      Serial.println("Timezone constructor");
#endif
      hostName = FPSTR(timeZoneHost);
    };

    ~Timezone()
    {
#ifdef DEBUG_LOG
      Serial.println("Timezone destructor");
#endif
    };

    bool acquire(double latitude, double longitude);
    virtual void whitespace(char c);
    virtual void startDocument();
    virtual void key(String key);
    virtual void value(String value);
    virtual void endArray();
    virtual void endObject();
    virtual void endDocument();
    virtual void startArray();
    virtual void startObject();
    String getTimeZoneId();
    String getTimeZoneName();
    bool isDst();
    int getUtcOffset();

  private:
    String currentKey;
    String currentParent;
    bool dst;
    int utcOffset;
    String timeZoneId;
    String timeZoneName;
};

class Geolocate : public JsonListener, public WsClient
{
  public:
    Geolocate() {
#ifdef DEBUG_LOG
      Serial.println("Geolocate constructor");
#endif
      hostName = FPSTR(geoLocateHost);
    };
    ~Geolocate() {
#ifdef DEBUG_LOG
      Serial.println("Geolocate destructor");
#endif
    };

    bool acquire();
    virtual void whitespace(char c);
    virtual void startDocument();
    virtual void key(String key);
    virtual void value(String value);
    virtual void endArray();
    virtual void endObject();
    virtual void endDocument();
    virtual void startArray();
    virtual void startObject();
    double getLatitude();
    double getLongitude();

  private:
    String encodeBase64(char* bytes_to_encode, unsigned int in_len);
    double latitude    = 0.0;
    double longitude   = 0.0;
    String currentKey;
    String currentParent;
    String base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
};


class Geocode : public JsonListener, public WsClient
{
  public:
    Geocode() {
#ifdef DEBUG_LOG
      Serial.println("Geocode constructor");
#endif
      hostName = FPSTR(geoCodeHost);
    };
    ~Geocode() {
#ifdef DEBUG_LOG
      Serial.println("Geocode destructor");
#endif
    };

    bool acquire(double latitude, double longitude);
    String getLocality();
    String getCountry();
    String getCountryCode();


    virtual void whitespace(char c);
    virtual void startDocument();
    virtual void key(String key);
    virtual void value(String value);
    virtual void endArray();
    virtual void endObject();
    virtual void endDocument();
    virtual void startArray();
    virtual void startObject();

  private:
    String currentKey;
    String locality;
    String country;
    String countryCode;


};


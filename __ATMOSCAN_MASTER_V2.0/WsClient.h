/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include "ESP8266WiFi.h"

class WsClient
{
  public:
    WsClient()
    {
#ifdef DEBUG_LOG
      Serial.println("WsClient constructor");
#endif
    };

    ~WsClient()
    {
#ifdef DEBUG_LOG
      Serial.println("WsClient destructor");
#endif
    };

  public:
    String hostName;
    bool httpConnect();
    bool httpGet(String resource);
    bool skipResponseHeaders();
    void disconnect();

    WiFiClient client;
};

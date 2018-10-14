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
    {};

    ~WsClient()
    { };

  public:
    String hostName;
    bool httpConnect();
    bool httpGet(String resource);
    bool skipResponseHeaders();
    void disconnect();

    WiFiClient client;
};

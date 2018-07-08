/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


#include "WsClient.h"


const int httpPort = 80;

// Skip HTTP headers so that we are at the beginning of the response's body
bool WsClient::skipResponseHeaders()
{
  bool outcome = false;
  while (client.connected())
  {
    String line = client.readStringUntil('\n');

#ifdef DEBUG_LOG
    Serial.println("HEADERS: " + line);
#endif

    if (line == "\r")
    {
#ifdef DEBUG_LOG
      Serial.println(F("headers received"));
#endif
      outcome = true;
      break;
    }
  }
  return outcome;
}


// Open connection to the HTTP server
bool WsClient::httpConnect()
{
  return client.connect(hostName.c_str(), httpPort);
}


// Send the HTTP GET request to the server
bool WsClient::httpGet(String url)
{
  client.print(F("GET "));
  client.print(url);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: "));
  client.println(hostName);
  client.println(F("User-Agent: ATMOSCAN"));
  client.println(F("Content-Type: application/json; charset=UTF-8"));
  client.println(F("Connection: close"));
  client.println();

  return true;
}


// Close the connection with the HTTP server
void WsClient::disconnect()
{
#ifdef DEBUG_LOG
  Serial.println(F("Disconnect"));
#endif
  client.stop();
}


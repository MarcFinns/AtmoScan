/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once


#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient
#include <ProcessScheduler.h>     // https://github.com/wizard97/ArduinoProcessScheduler
#include <ESP8266HTTPClient.h>

extern WiFiClient wifiClient;

// Process definition
class Proc_MQTTUpdate : public Process
{
  public:
    Proc_MQTTUpdate(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
      :  mqttClient (wifiClient), Process(manager, pr, period, iterations) {}

    char* getLastMqttUpdate();

  protected:
    virtual void setup();
    virtual void service();

  private:
    PubSubClient mqttClient;
    bool mqttReconnect();
    int mqttSend(char *mqttTopic, char *mqttData);
    char lastMqttUpdate[25];
};



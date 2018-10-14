/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


// Override PubSub defaults

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#define MQTT_KEEPALIVE 5

// MQTT_SOCKET_TIMEOUT: socket timeout interval in Seconds
#define MQTT_SOCKET_TIMEOUT 7

#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient
#include <NtpClientLib.h>         // https://github.com/gmag11/NtpClient
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include "P_MQTT.h"
#include "P_AirSensors.h"
#include "GlobalDefinitions.h"


// External variables
extern struct ProcessContainer procPtr;
extern struct Configuration config;
extern Syslog syslog;
extern String systemID;
extern WiFiClient wifiClient;

// Prototypes
void errLog(String msg);

// Tags
const char PARAM_1[] PROGMEM = "1=";
const char PARAM_2[] PROGMEM = "&2=";
const char PARAM_3[] PROGMEM = "&3=";
const char PARAM_4[] PROGMEM = "&4=";
const char PARAM_5[] PROGMEM = "&5=";
const char PARAM_6[] PROGMEM = "&6=";
const char PARAM_7[] PROGMEM = "&7=";
const char PARAM_8[] PROGMEM = "&8=";


// Process Setup
void Proc_MQTTUpdate::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("Proc_MQTTUpdate::setup()"));
#endif

  // Set the MQTT broker
  mqttClient.setServer(config.mqtt_server, 1883);

  mqttClient.loop();

}

// Process Service
void Proc_MQTTUpdate::service()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("Proc_MQTTUpdate::service()"));
#endif

  // Update MQTT  only if WiFi is connected
  if (config.connected)
  {


#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, F("Connect to MQTT server..."));
#endif

    // Make ongoing MQTT visible
    procPtr.UIManager.communicationsFlag(true);

    bool isConnected = mqttReconnect();

    if (isConnected)
    {

      // Reusable buffer
      char mqttData[100];

      // Create data string - Topic 1
      strcpy_P(mqttData, PARAM_1);
      dtostrf(procPtr.ComboTemperatureHumiditySensor.getTemperature(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_2);
      dtostrf(procPtr.ComboTemperatureHumiditySensor.getHumidity(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_3);
      dtostrf(procPtr.ComboPressureHumiditySensor.getPressure(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_4);
      dtostrf(procPtr.ParticleSensor.getPM01(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_5);
      dtostrf(procPtr.ParticleSensor.getPM2_5(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_6);
      dtostrf(procPtr.ParticleSensor.getPM10(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_7);
      dtostrf(procPtr.GeigerSensor.getCPM(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_8);
      dtostrf(procPtr.GeigerSensor.getRadiation(), 2, 2, &mqttData[strlen(mqttData)]);

      // Update topic 1
      mqttSend(config.mqtt_topic1, mqttData);

#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, String(F("mqttData1 ")) + String(mqttData));
#endif

      // Create data string - Topic 2
      strcpy_P(mqttData, PARAM_1);
      dtostrf(procPtr.MultiGasSensor.getCO(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_2);
      dtostrf(procPtr.CO2Sensor.getCO2(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_3);
      dtostrf(procPtr.MultiGasSensor.getNO2(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_4);
      dtostrf(procPtr.VOCSensor.getVOC(), 2, 2, &mqttData[strlen(mqttData)]);

      // Update topic 2
      mqttSend(config.mqtt_topic2, mqttData);


#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, String(F("mqttData2 ")) + String(mqttData));
#endif

      // Create data string - Topic 3 (SYSTEM)
      strcpy_P(mqttData, PARAM_1);
      dtostrf(millis() / (60000L), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_2);
      dtostrf(ESP.getFreeHeap(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_3);
      dtostrf(WiFi.RSSI(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_4);
      dtostrf(procPtr.UIManager.getVolt(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_5);
      dtostrf(procPtr.UIManager.getSoC(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_6);
      dtostrf(procPtr.UIManager.getNativeSoC(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_7);
      dtostrf(procPtr.ComboPressureHumiditySensor.getTemperature(), 2, 2, &mqttData[strlen(mqttData)]);

      strcat_P(mqttData, PARAM_8);
      dtostrf(procPtr.ComboPressureHumiditySensor.getHumidity(), 2, 2, &mqttData[strlen(mqttData)]);

      // Update topic 3
      mqttSend(config.mqtt_topic3, mqttData);


#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, String(F("mqttData3 ")) + String(mqttData));
#endif

      // Remember last update
      sprintf(lastMqttUpdate, "%d/%d/%d %d:%02d.%02d   ", day(), month(), year(), hour(), minute(), second());

      // Experimental
      /*
            if (mqttClient.connected())
            {
              // Disconnect from server
              mqttClient.disconnect();
            }
            else
            {
              // TEMP log
              syslog.log(LOG_DEBUG, "MQTT not connected, so no need to disconnect");
            }
      */
    }
  }

  // Reset visual communications flag
  procPtr.UIManager.communicationsFlag(false);

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("END Proc_MQTTUpdate::service()"));
#endif
}


bool Proc_MQTTUpdate::mqttReconnect()
{
  // If already connected, exit
  if (mqttClient.connected())
    return true;
  else
  {
    for (int attempt = 1; attempt < 5; attempt++)
    {
#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, String(F("MQTT connection, attempt ")) + attempt);
#endif

      // Note: to avoid thingspeak occasional lockout
      String randomID = systemID + String(random(999999));

      // Connect to the MQTT broker
      if (mqttClient.connect(randomID.c_str(), String(F("username")).c_str(), String(F("password")).c_str()))
      {
        if (attempt > 1)
          errLog(String(F("MQTT retries:")) + String(attempt));

        return true;
      }
      else
      {

#ifdef DEBUG_SYSLOG
        errLog(String(F("MQTT fail,err ")) + String(mqttClient.state()));
#endif
        // Wait a bit and retry (2000 ms)
        for (int wait = 1 ; wait < 40; wait++)
        {
          // Wait before retrying
          // unless a userEvent needs to be serviced
          if (!procPtr.UIManager.eventPending())
          {
            delay(50);
          }
          else
          {
            // Cannot update MQTT as UserEvent is pending...
            // ...so, force scheduling so to retry asap
            errLog(F("MQTT connect - giving up as user event pending"));

            this->force();
            return false;
          }
        }
      }
    }

    // All attempts were exausted, giving up..
    errLog(String(F("MQTT giveup,err ")) + String(mqttClient.state()));
    return false;
  }
}

int Proc_MQTTUpdate::mqttSend(char *mqttTopic, char *mqttData)
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("Updating MQTT with ")) + String(mqttData));
#endif

  // Publish data to ThingSpeak
  int rc = mqttClient.publish(mqttTopic, mqttData);

#ifdef DEBUG_SYSLOG
  syslog.logf(LOG_DEBUG, "MQTT outcome =  % d ", rc);
#endif
}

char* Proc_MQTTUpdate::getLastMqttUpdate()
{
  return lastMqttUpdate;
}

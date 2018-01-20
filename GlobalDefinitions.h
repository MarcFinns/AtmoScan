#pragma once

#include "P_UIManager.h"
#include "P_MQTT.h"
#include "P_AirSensors.h"
#include "P_GeoLocation.h"
#include "WundergroundClient.h"
#include <RingBufCPP.h>           //https://github.com/wizard97/Embedded_RingBuf_CPP

// -------------------------------------------------------
// Definitions
// -------------------------------------------------------

// #define DEBUG_SYSLOG
// #define DEBUG_SERIAL

#define ATMOSCAN_VERSION "v1.2.1"

// Syslog server connection info (IP is from configuration portal)
#define SYSLOG_PORT 514

// This system info
#define APP_NAME "ATMOSCAN"

// I/O pins assignments
#define VOC_PIN A0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define GESTURE_INTERRUPT_PIN 10
#define GEIGER_INTERRUPT_PIN 15
#define CO2_RX_PIN 0
#define CO2_TX_PIN 2
#define BACKLIGHT_PIN 12 // Active low

#define BACKLIGHT_TIMEOUT 60000 // millisec
#define CONFIG_PORTAL_TIMEOUT 600 // sec

#define START_SCREEN 1
#define SETUP_SCREEN 0
#define LOWBATT_SCREEN 99

// RUNTIME definitions
#define VOLT_LOW 3.2
#define VOLT_HIGH 3.9

#define AVERAGING_WINDOW 12

#define FAST_SAMPLE_PERIOD 2000
#define SLOW_SAMPLE_PERIOD 5000
#define MQTT_UPDATE_PERIOD 60000
#define GEOLOC_RETRY_PERIOD 10000

// -------------------------------------------------------
//  Global constants
// -------------------------------------------------------


// NTP Server:
const char PROGMEM ntpServerName[] = "pool.ntp.org";


// -------------------------------------------------------
//  Types
// -------------------------------------------------------


struct ProcessContainer
{
  // Members

  Proc_ComboTemperatureHumiditySensor ComboTemperatureHumiditySensor ;
  Proc_ComboPressureHumiditySensor ComboPressureHumiditySensor;
  Proc_CO2Sensor CO2Sensor;
  Proc_ParticleSensor ParticleSensor;
  Proc_VOCSensor VOCSensor;
  Proc_MultiGasSensor MultiGasSensor;
  Proc_GeigerSensor GeigerSensor;
  Proc_UIManager UIManager;
  Proc_MQTTUpdate MQTTUpdate;
  Proc_GeoLocation GeoLocation;

};

struct Configuration
{
  RingBufCPP<String, 18> lastErrors;
  bool connected = false;
  int startScreen = START_SCREEN;
  char mqtt_topic1[64];
  char mqtt_topic2[64];
  char mqtt_topic3[64];
  char mqtt_server[40];
  char syslog_server[20];
  WundergroundClient *wunderground;
  bool wunderValid = false;
  bool configValid = false;
};



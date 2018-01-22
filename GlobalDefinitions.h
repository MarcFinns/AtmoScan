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

// Used to turn on debug log over syslog
// #define DEBUG_SYSLOG

// Used to turn on SOME log over serial
// WARNING - this was used during development but can't be used in the fulluy assemled system, as serial port is used for a sensor
// #define DEBUG_SERIAL

// Firmware revision
#define ATMOSCAN_VERSION "v1.2.2"

// This system name
#define APP_NAME "ATMOSCAN"

// Syslog server connection info (IP is from configuration portal)
#define SYSLOG_PORT 514

// I/O pins assignments
#define VOC_PIN A0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define GESTURE_INTERRUPT_PIN 10
#define GEIGER_INTERRUPT_PIN 15
#define CO2_RX_PIN 0
#define CO2_TX_PIN 2
#define BACKLIGHT_PIN 12 // NOTE: Active low

#define BACKLIGHT_TIMEOUT 60000 // millisec
#define CONFIG_PORTAL_TIMEOUT 600 // sec

#define START_SCREEN 1
#define SETUP_SCREEN 0
#define LOWBATT_SCREEN 99

// RUNTIME definitions (NOTE: ampirical)
#define VOLT_LOW 3.2
#define VOLT_HIGH 3.9

#define AVERAGING_WINDOW 12         // NOTE: 12 * 5 sec sensor sampling rate = 1 minute

#define FAST_SAMPLE_PERIOD 2000     // (ms) Used for Geiger sensor 
#define SLOW_SAMPLE_PERIOD 5000     // (ms) Used for other sensors 
#define MQTT_UPDATE_PERIOD 60000    // (ms)
#define GEOLOC_RETRY_PERIOD 10000   // (ms)

// -------------------------------------------------------
//  Global constants
// -------------------------------------------------------

// NTP Server - TODO: move into configuration?
const char PROGMEM ntpServerName[] = "pool.ntp.org";


// -------------------------------------------------------
//  Types
// -------------------------------------------------------

// Holds pointers to processes
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

// Holds various items related to the current configuration
struct Configuration
{
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



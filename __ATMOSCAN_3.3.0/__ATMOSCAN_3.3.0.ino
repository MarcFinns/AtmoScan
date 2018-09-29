/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/* Author: Marc Finns, 2017                             */
/*                                                      */
/*  Components:                                         */
/*      - ESP8266 ESP-12E                               */
/*      - HDC1080 Humidity & Temperature Sensor         */
/*      - Plantower PMS7003 Particle sensor             */
/*      - MZ-H19 CO2 sensor                             */
/*      - Air Quality Sensor (based on Winsen MP503)    */
/*      - Multigas sensor (Based on MiCS6814)           */
/*      - BME 280 Pressure & humidity sensor            */
/*                                                      */
/*  Wiring:                                             */
/*                                                      */
/*  Analog:                                             */
/*      - AirQuality out    => ADC A0                   */
/*  I2C:                                                */
/*      - BMP280    SCL     => GPIO5                    */
/*      - BMP280    SDA     => GPIO4                    */
/*      - HDC1080   SCL     => GPIO5                    */
/*      - HDC1080   SDA     => GPIO4                    */
/*      - PAJ7620U  SCL     => GPIO5                    */
/*      - PAJ7620U  SDA     => GPIO4                    */
/*      - MiCS6814  SCL     => GPIO5                    */
/*      - MiCS6814  SDA     => GPIO4                    */
/*  SPI:                                                */
/*      - ILI9341   SCK     => GPIO14                   */
/*      - ILI9341   MISO    NC                          */
/*      - ILI9341   MOSI    => GPIO13                   */
/*      - ILI9341   D/C     => GPIO16                   */
/*      - ILI9341   RST     => RST                      */
/*      - ILI9341   CS      => GND                      */
/*      - ILI9341   LED     => GPIO12 (via MOS driver)  */
/* Serial:                                              */
/*      - PMS7003   TX      => RXD (GPIO3)              */
/*      - PMS7003   RX      => TXD (GPIO1)              */
/*      - MH-Z19    TX      => GPIO0 (SW RX)            */
/*      - MH-Z19    RX      => GPIO2 (SW TX)            */
/* Interrupts:                                          */
/*      - _Gesture          => GPIO10                   */
/*      - Geiger            => GPIO15                   */
/********************************************************/

// -------------------------------------------------------
// Libraries
// -------------------------------------------------------
#define FS_NO_GLOBALS
#include <FS.h>                   //this needs to be first
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <DNSServer.h>
#include <SPI.h>
#include <Wire.h>

#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog
#include <Average.h>              // https://github.com/MajenkoLibraries/Average
#include <PubSubClient.h>         // https://github.com/knolleary/pubsubclient
#include <ProcessScheduler.h>     // https://github.com/wizard97/ArduinoProcessScheduler  NOTE: Requires https://github.com/wizard97/ArduinoRingBuffer
#include <NtpClientLib.h>         // https://github.com/gmag11/NtpClient                  NOTE: Requires https://github.com/PaulStoffregen/Time
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson
#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI

// Project includes
#include "GlobalDefinitions.h"
#include "ScreenFactory.h"
#include "TimeSpace.h"

// Screens
#include "ScreenSensors.h"
#include "ScreenStatus.h"
#include "ScreenSetup.h"
#include "ScreenGeiger.h"
#include "ScreenPlaneSpotter.h"
#include "ScreenWeatherStation.h"
#include "ScreenErrLog.h"
#include "ScreenBuienRadar.h"

// Graphics & fonts
#include "GlobalBitmaps.h"
#include "Fonts.h"
#include "Free_Fonts.h"

// Allows to change the clock speed at runtime
extern "C" {
#include "user_interface.h"
}


// -------------------------------------------------------
//  Globals
// -------------------------------------------------------


// Unique board ID
String systemID;

// Turbo flag
bool turbo = false;

// LCD Screen
TFT_eSPI LCD = TFT_eSPI();

// Global UI management
GfxUi ui(&LCD);

WiFiClient wifiClient;

// UDP instance to send and receive packets over UDP
WiFiUDP udpClient;

// Global syslog instance
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);

// Download manager
WebResource webResource;

// Register screens with Screen Factory
ScreenCreatorImpl<ScreenSetup> creator0;
ScreenCreatorImpl<ScreenSensors> creator1;
ScreenCreatorImpl<ScreenStatus> creator2;
ScreenCreatorImpl<ScreenErrLog> creator3;
ScreenCreatorImpl<ScreenGeiger> creator4;
ScreenCreatorImpl<ScreenPlaneSpotter> creator5;
ScreenCreatorImpl<ScreenWeatherStation> creator6;
ScreenCreatorImpl<ScreenBuienRadar> creator7;

// Global Scheduler object
Scheduler sched;

// Last errors list
RingBufCPP<String, 18> lastErrors;

// Configuration container structure
Configuration config;

//  Processes container structure
ProcessContainer procPtr =
{
  Proc_ComboTemperatureHumiditySensor(sched,
  MEDIUM_PRIORITY,
  SLOW_SAMPLE_PERIOD,
  RUNTIME_FOREVER),

  Proc_ComboPressureHumiditySensor(sched,
  MEDIUM_PRIORITY,
  SLOW_SAMPLE_PERIOD,
  RUNTIME_FOREVER),

  Proc_CO2Sensor(sched,
  MEDIUM_PRIORITY,
  SLOW_SAMPLE_PERIOD,
  RUNTIME_FOREVER),

  Proc_ParticleSensor(sched,
  MEDIUM_PRIORITY,
  SLOW_SAMPLE_PERIOD,
  RUNTIME_FOREVER),

  Proc_VOCSensor(sched,
  MEDIUM_PRIORITY,
  SLOW_SAMPLE_PERIOD,
  RUNTIME_FOREVER),

  Proc_MultiGasSensor(sched,
  MEDIUM_PRIORITY,
  SLOW_SAMPLE_PERIOD,
  RUNTIME_FOREVER),

  Proc_GeigerSensor(sched,
  MEDIUM_PRIORITY,
  FAST_SAMPLE_PERIOD,
  RUNTIME_FOREVER),

  Proc_UIManager(sched,
  HIGH_PRIORITY,
  SLOW_SAMPLE_PERIOD,
  RUNTIME_FOREVER),

  Proc_MQTTUpdate(sched,
  MEDIUM_PRIORITY,
  MQTT_UPDATE_PERIOD,
  RUNTIME_FOREVER),

  Proc_GeoLocation(sched,
  MEDIUM_PRIORITY,
  GEOLOC_RETRY_PERIOD,
  RUNTIME_FOREVER)

};


void setup()
{

#ifdef DEBUG_SERIAL
  Serial.begin(115200);
#endif

  // Wait for electronics to settle
  delay(3000);

  // Dynamically create systemID based on MAC address
  systemID = F("ATMOSCAN-");
  systemID += WiFi.macAddress();
  systemID.replace(F(":"), F(""));

  // Initiatlise the LCD
  LCD.begin();
  LCD.setRotation(2);
  LCD.fillScreen(TFT_BLACK);


  // Initialise text engine
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);
  LCD.setTextWrap(false);
  LCD.setFreeFont(FSS9);
  int  xpos = 0;
  int  ypos = 20;
  LCD.setTextDatum(BR_DATUM);

  // **********************  Draw splash screen
  ui.drawBitmap(Splash_Screen, (LCD.width() - splashWidth) / 2, 20, splashWidth, splashHeight);

  LCD.drawString(F(ATMOSCAN_VERSION), xpos, ypos, GFXFF);

  xpos = 240;
  LCD.setTextDatum(BL_DATUM);
  LCD.drawString(F("(c) 2018 MarcFinns"), xpos, ypos, GFXFF);

  // **********************  Credits: Libraries
  ypos = 215;

  LCD.drawRect(0, ypos, 240, 105, TFT_WHITE);
  LCD.setFreeFont(&Dialog_plain_9);

  ypos += 13;
  int lineSpacing = 9;

  LCD.setTextDatum(BC_DATUM);
  LCD.drawString(F("INCLUDES LIBRARIES FROM:"), 120, ypos, GFXFF);

  LCD.setTextDatum(BL_DATUM);

  ypos +=  lineSpacing + 6;
  LCD.drawString(F("Adafruit"), 6, ypos, GFXFF);
  LCD.drawString(F("ClosedCube"), 85, ypos, GFXFF);
  LCD.drawString(F("Seeed"), 170, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Arcao"), 6, ypos, GFXFF);
  LCD.drawString(F("Gmag11"), 85, ypos, GFXFF);
  LCD.drawString(F("Squix78"), 170, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Bblanchon"), 6, ypos, GFXFF);
  LCD.drawString(F("Knolleary"), 85, ypos, GFXFF);
  LCD.drawString(F("Tzapu"), 170, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Bodmer"), 6, ypos, GFXFF);
  LCD.drawString(F("Lucadentella"), 85, ypos, GFXFF);
  LCD.drawString(F("Wizard97"), 170, ypos, GFXFF);


  // ********************* Credits: web services
  ypos +=  lineSpacing + 6;

  LCD.setTextDatum(BC_DATUM);
  LCD.drawString(F("INTEGRATES WEB SERVICES FROM:"), 120, ypos, GFXFF);

  LCD.setTextDatum(BL_DATUM);

  ypos +=  lineSpacing + 6;
  LCD.drawString(F("Adsbexchange.com"), 6, ypos, GFXFF);
  LCD.drawString(F("GeoNames.org"), 122, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Google.com"), 6, ypos, GFXFF);
  LCD.drawString(F("Wunderground.com"), 122, ypos, GFXFF);

  ypos +=  lineSpacing;
  LCD.drawString(F("Timezonedb.com"), 6, ypos, GFXFF);
  LCD.drawString(F("Mylnikov.org"), 122, ypos, GFXFF);

  /////////////////////////////////////////////////////
  // clean SPIFFS FS, used for testing
  // SPIFFS.format();
  /////////////////////////////////////////////////////

  // Initialise I2C bus
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Add process objects to scheduler
  addProcesses();

  // Turn on LCD and wait a bit on splash screen...
  procPtr.UIManager.displayOn();
  delay (1000);

  // **********************  Initialization with progress bar...
  LCD.setFreeFont(FSS9);
  LCD.setTextDatum(TL_DATUM);
  LCD.setTextColor(TFT_YELLOW, TFT_BLACK);
  xpos = 6;
  ypos = 195;

  // Retrieve dynamic config parameters from SPIFSS (such as MQTT Topics and servers)

  // Draw progress bar...
  ui.drawProgressBar(10, 175, 240 - 20, 15, 0, TFT_YELLOW, TFT_BLUE);

  // Show init step...
  LCD.drawString(F("Retrieving configuration...       "), xpos, ypos, GFXFF);
  ui.drawProgressBar(10, 175, 240 - 20, 15, 30, TFT_YELLOW, TFT_BLUE);

  delay(500);

  // Retrieve configuration from SPIFFS, if existent
  if (retrieveConfig())
  {
    config.configValid = true;

    // Show init step...
    LCD.drawString(F("Connecting to network...    "), xpos, ypos, GFXFF);
    ui.drawProgressBar(10, 175, 240 - 20, 15, 60, TFT_YELLOW, TFT_BLUE);

    delay(500);

    // Handle Wifi connection, including dynamic config parameters such as MQTT Topic and server
    config.connected = wifiConnect();

    // Initialize NTP library
    initNTP();

    //  Configure syslog instance
    syslog.server(config.syslog_server, SYSLOG_PORT);
    syslog.deviceHostname(systemID.c_str());
    syslog.appName(APP_NAME);
    syslog.defaultPriority(LOG_KERN);

    // Start logging
    String strBuffer = F("******* BOOTING FIRMWARE ") ;
    syslog.log(LOG_INFO, strBuffer + F(ATMOSCAN_VERSION) + F(", BUILT ") + String(__DATE__ " " __TIME__) + F(" ******* "));

    // Log current configuration
    strBuffer = F("Connected to network ");
    syslog.log(LOG_INFO, strBuffer + WiFi.SSID() + F(" with address ") + WiFi.localIP().toString());

    // Scan I2C bus and log devices found
#ifdef DEBUG_SYSLOG
    delay(1000); // Syslog does not like too many messages at a time...
    i2cScan();
#endif

    // Log current AtmoScan configuration
#ifdef DEBUG_SYSLOG
    delay(1000); // Syslog does not like too many messages at a time...
    syslog.log(LOG_DEBUG, F("Configuration is:"));
    syslog.log(LOG_DEBUG, config.mqtt_server);
    syslog.log(LOG_DEBUG, config.mqtt_topic1);
    syslog.log(LOG_DEBUG, config.mqtt_topic2);
    syslog.log(LOG_DEBUG, config.syslog_server);
    syslog.log(LOG_DEBUG, config.google_key);
    syslog.log(LOG_DEBUG, config.wunderground_key);
    syslog.log(LOG_DEBUG, config.geonames_user);
    syslog.log(LOG_DEBUG, config.timezonedb_key);
#endif

#ifdef DEBUG_SERIAL
    Serial.println(F("Configuration is:"));
    Serial.println(config.mqtt_server);
    Serial.println(config.mqtt_topic1);
    Serial.println(config.mqtt_topic2);
    Serial.println(config.syslog_server);
    Serial.println(config.google_key);
    Serial.println(config.wunderground_key);
    Serial.println(config.geonames_user);
    Serial.println(config.timezonedb_key);

#endif

    // Log ESP configuration
#ifdef DEBUG_SYSLOG
    logESPconfig();
#endif

  }
  else
  {
    // If no valid config found, force Setup screen
    config.configValid = false;
    config.startScreen = SETUP_SCREEN;
  }

  // Initialise OTA
  initOTA();

  // Show init step...
  LCD.drawString(F("Starting processes...         "), xpos, ypos, GFXFF);
  ui.drawProgressBar(10, 175, 240 - 20, 15, 100, TFT_YELLOW, TFT_BLUE);

  // Start processes
  startProcesses();

  // Wait a bit (optical)
  delay(500);

  // Cleanup old maps from SPIFFS, if present
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "SPIFFS dir listing:");
#endif
  String fileName;
  fs::Dir dir = SPIFFS.openDir(F("/"));
  while (dir.next())
  {
    fileName = dir.fileName();
    if (fileName.startsWith(F("/map")))
    {
#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, " FOUND " + fileName);
#endif
      bool outcome = SPIFFS.remove(fileName);

#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, " FILE REMOVAL " + String(outcome));
#endif

    }
  }

  // Configuration completed!
  syslog.log(LOG_INFO, F("************ BOOT SEQUENCE COMPLETE *************"));

  ESP.wdtEnable(0);
}


void loop()
{

  // Handle OTA
  ArduinoOTA.handle();

  // Invoke scheduler
  sched.run();

  // Feed the WatchDog
  ESP.wdtFeed();

}


// OTA firmware update management

int lastOTAprogressPercentual = 0;

void initOTA()
{
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Set Hostname
  ArduinoOTA.setHostname(systemID.c_str());

  // Default password
  ArduinoOTA.setPassword((const char *)"ATMOSCAN");

  ArduinoOTA.onStart([]()
  {
    // Precautionary display reset
    LCD.init();

    delay(100);

    // Turn on backlight
    procPtr.UIManager.displayOn();

    // Reset screen settings
    LCD.setRotation(2);
    LCD.fillScreen(TFT_BLUE);
    LCD.setTextColor(TFT_YELLOW, TFT_BLUE);
    LCD.setFreeFont(FSS9);
    LCD.setTextDatum(BC_DATUM);
    LCD.drawString(F("OTA Firmware update"), 120, 20, GFXFF);
    syslog.log(LOG_INFO, F("OTA Update Start"));
  });

  ArduinoOTA.onEnd([]()
  {
    LCD.setTextDatum(BL_DATUM);
    LCD.drawString(F(" Rebooting..."), 0, 120, GFXFF);
    syslog.log(LOG_INFO, F("OTA Update End - Rebooting"));
    delay(500);
    ESP.restart();
  });


  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {

    int progressPercentual = progress / (total / 100.0);

    // Print OTA progress only if incremented
    if (progressPercentual != lastOTAprogressPercentual)
    {
      // Print OTA progress

      LCD.drawString(String(progress) + F("/") + String (total) + F(" bytes (") + String(progressPercentual) + F("%)"), 120, 50, GFXFF);
      syslog.logf(LOG_INFO, "OTA Progress = %u%%\r", progressPercentual);

      ui.drawProgressBar(10, 60, 240 - 20, 15, progressPercentual, TFT_YELLOW, TFT_YELLOW);
      lastOTAprogressPercentual = progressPercentual;
    }
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    //String strbuffer = F("OTA Update Error ");
    syslog.log(LOG_ERR, "OTA Update Error " +   String(error));
    if (error == OTA_AUTH_ERROR)
    {
      LCD.drawString(F("OTA Auth Failed"), 0, 100, GFXFF);
      errLog(F("OTA Auth Failed"));
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      LCD.drawString(F("OTA Update Begin Failed"), 0, 100, GFXFF);
      errLog( F("OTA Update Begin Failed"));
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      LCD.drawString(F("OTA Update Connect Failed"), 0, 100, GFXFF);
      errLog( F("OTA Update Connect Failed"));
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      LCD.drawString(F("OTA Update Receive Failed"), 0, 100, GFXFF);
      errLog( F("OTA Update Receive Failed"));
    }
    else if (error == OTA_END_ERROR)
    {
      LCD.drawString(F("OTA Update End Failed"), 0, 100, GFXFF);
      errLog( F("OTA Update End Failed"));
    }

    delay(2000);
    ESP.restart();
  });

  // OTA name Setup
  ArduinoOTA.setHostname((const char *)systemID.c_str());

  // Begin listening
  ArduinoOTA.begin();
}

// Manage network disconnection
void onSTADisconnected(WiFiEventStationModeDisconnected event_info)
{

#ifdef DEBUG_SERIAL
  Serial.println("WiFi disconnected");
#endif

  config.connected = false;
}


// Manage network reconnection
void onSTAGotIP(WiFiEventStationModeGotIP ipInfo)
{
#ifdef DEBUG_SERIAL
  Serial.println("WiFi Connected");
#endif

  // Remember current connection status in configuration
  config.connected =  true;
}


// Network time management
void initNTP()
{
  static WiFiEventHandler e1, e2;

  // NTP start
  NTP.onNTPSyncEvent([](NTPSyncEvent_t error)
  {
    if (error)
    {
      if (error == noResponse)
      {
        errLog(F("NTP svr unreachable"));
      }
      else if (error == invalidAddress)
      {
        errLog(F("Invalid NTP server address"));
      }
    }
    else
    {
      String strBuffer = F("Got NTP time - ") ;
      syslog.log(LOG_INFO, strBuffer + NTP.getTimeDateString(NTP.getLastNTPSync()));
    }
  });

  e1 = WiFi.onStationModeGotIP(onSTAGotIP);
  e2 = WiFi.onStationModeDisconnected(onSTADisconnected);
}

// Add processes to scheduler
void addProcesses()
{

#ifdef ENABLE_SENSORS
  procPtr.ComboTemperatureHumiditySensor.add();
  procPtr.ComboPressureHumiditySensor.add();
  procPtr.CO2Sensor.add();
  procPtr.ParticleSensor.add();
  procPtr.VOCSensor.add();
  procPtr.MultiGasSensor.add();
  procPtr.GeigerSensor.add();
  procPtr.MQTTUpdate.add();
#endif

  procPtr.UIManager.add();
  procPtr.GeoLocation.add();
}

// Enable Process scheduling
void startProcesses()
{
  // if current configuration is not valid, start UI only
  if (config.configValid)
  {
#ifdef ENABLE_SENSORS
    procPtr.ComboTemperatureHumiditySensor.enable();
    procPtr.ComboPressureHumiditySensor.enable();
    procPtr.ParticleSensor.enable();
    procPtr.CO2Sensor.enable();
    procPtr.VOCSensor.enable();
    procPtr.MultiGasSensor.enable();
    procPtr.GeigerSensor.enable();
    procPtr.MQTTUpdate.enable();
#endif
    procPtr.GeoLocation.enable();
  }
  else
  {
#ifdef DEBUG_SYSLOG
    syslog.log(LOG_INFO, F("Invalid configuration, starting only UI process"));
#endif
  }

  procPtr.UIManager.enable();

}

// Retrieve previously saved configuration from SPIFFS
bool retrieveConfig()
{
  // Read configuration from FS json
  if (SPIFFS.begin())
  {
    if (SPIFFS.exists(F("/config.json")))
    {
#ifdef DEBUG_SERIAL
      Serial.println("Configuration found");
#endif

      //file exists, read and load it
      fs::File configFile = SPIFFS.open(F("/config.json"), "r");
      if (configFile)
      {
        size_t size = configFile.size();

        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());

        if (json.success())
        {

#ifdef DEBUG_SERIAL
          Serial.println("JSON parse success ");
#endif

          // Gracefully handle upgrade from firmware v1.x
          if (!json.containsKey(F("google_key")))
          {
            syslog.log(LOG_ERR, F("Incompatible configuration found"));
            return false;
          }

          strcpy(config.mqtt_server, json[F("mqtt_server")]);
          strcpy(config.mqtt_topic1, json[F("mqtt_topic1")]);
          strcpy(config.mqtt_topic2, json[F("mqtt_topic2")]);
          strcpy(config.mqtt_topic3, json[F("mqtt_topic3")]);
          strcpy(config.syslog_server, json[F("syslog_server")]);
          strcpy(config.google_key, json[F("google_key")]);
          strcpy(config.wunderground_key, json[F("wunderground_key")]);
          strcpy(config.geonames_user, json[F("geonames_user")]);
          strcpy(config.timezonedb_key, json[F("timezonedb_key")]);
          return true;

        }
        else
        {
#ifdef DEBUG_SERIAL
          Serial.println("JSON parse failed ");
#endif
          // LCD.print(F(" Error parsing"));
          delay(2000);
          SPIFFS.format();
          ESP.eraseConfig();
          return false;
        }
      }
    }
    else
    {
#ifdef DEBUG_SERIAL
      Serial.println(F("JSON File does not exist!"));
#endif
      delay(2000);
      ESP.eraseConfig();
      return false;
    }
  }
  else
  {
    LCD.println(F(" > Could not access file system"));
#ifdef DEBUG_SERIAL
    Serial.println(F("Could access file system!"));
#endif
    return false;
  }
}


// Connect to WiFi
bool wifiConnect()
{
  // Connect using last good credentials
  WiFi.mode(WIFI_STA);
  WiFi.begin();

  // Set hostname
  WiFi.hostname(systemID);

  // Wait for connection...
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED && counter++ < 30)
  {
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED )
  {
    return true;
  }
  else
  {
    return false;
  }
}


#ifdef DEBUG_SYSLOG
void logESPconfig()
{

  syslog.log(LOG_DEBUG, F("FLASH CONFIGURATION LOG"));
  // Log ESP configuration
  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();
  delay(1000);
  syslog.logf(LOG_DEBUG, "Flash real id:   %08X\n", ESP.getFlashChipId());
  delay(1000);
  syslog.logf(LOG_DEBUG, "Flash real size: %u\n\n", realSize);
  delay(1000);
  syslog.logf(LOG_DEBUG, "Flash ide  size: %u\n", ideSize);
  delay(1000);
  syslog.logf(LOG_DEBUG, "Flash ide speed: %u\n", ESP.getFlashChipSpeed());
  delay(1000);
  syslog.logf(LOG_DEBUG, "Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
  delay(1000);
  if (ideSize != realSize)
  {
    errLog( F("Flash Chip configuration wrong!"));
  }
  else
  {
    syslog.log(LOG_DEBUG, F("Flash Chip configuration ok."));
  }
}
#endif

// Allows to dynamically set CPU clock (80 or 160 MHz)
void setTurbo(bool setTurbo)
{
  if (setTurbo)
  {
#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, F("Set TURBO mode"));
#endif
    system_update_cpu_freq(160);
    turbo = true;
  }
  else
  {
#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, F("Set NORMAL mode"));
#endif
    system_update_cpu_freq(80);
    turbo = false;
  }

}

// Check current CPU clock status
bool isTurbo()
{
  return turbo;
}

// Log error on both syslog and error screen, managing a circular buffer
void errLog(String msg)
{
  // Log time
  char logTime[25];
  sprintf(logTime, "[%d/%d%d:%02d.%02d] ", day(), month(), hour(), minute(), second());

  // Manage buffer
  if (lastErrors.isFull())
  {
    // Remove oldest (outgoing) element
    String first;
    lastErrors.pull(&first);
  }

  lastErrors.add(String(logTime) + msg);
  syslog.log(LOG_ERR, msg);
}

#ifdef KILL_INSTALLED
void turnOff()
{
  pinMode(KILL_PIN, OUTPUT);
  digitalWrite(KILL_PIN, LOW);
}
#endif

#ifdef DEBUG_SYSLOG
void i2cScan()
{
  byte error, address;
  int nDevices;

  syslog.log(LOG_DEBUG, F("Scanning I2C bus..."));

  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      syslog.log(LOG_DEBUG, "I2C device found at address 0x" + byte2hex(address));
      nDevices++;
    }
    else if (error == 4)
    {
      syslog.log(LOG_DEBUG, "Unknown error at address 0x" + byte2hex(address));
    }
  }

  syslog.log(LOG_DEBUG, "I2C devices found: " + String(nDevices));

}

String byte2hex(unsigned char buf)
{
  char onebyte[2];
  sprintf(onebyte, "%02X", buf);
  return  String(onebyte) ;

}
#endif

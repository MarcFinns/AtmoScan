
#include "P_UIManager.h"
#include "ESP8266WiFi.h"
#include "GlobalDefinitions.h"
#include "Free_Fonts.h"
#include "ArialRoundedMTBold_14.h"
#include "ArialRoundedMTBold_36.h"
#include "ScreenLowbatt.h"

#include <TFT_eSPI.h>             // https://github.com/Bodmer/TFT_eSPI
#include <NtpClientLib.h>         // https://github.com/gmag11/NtpClient
#include <MAX17043.h>             // https://github.com/lucadentella/ArduinoLib_MAX17043
#include <Syslog.h>               // https://github.com/arcao/ESP8266_Syslog

#

// External variables
extern Syslog syslog;
extern TFT_eSPI LCD;
extern struct Configuration config;
extern struct ProcessContainer procPtr;
extern GfxUi ui;

// Prototypes
void errLog(String msg);

Proc_UIManager * Proc_UIManager::instance = nullptr;


Proc_UIManager::Proc_UIManager(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
  :  Process(manager, pr, period, iterations), avgSOC(AVERAGING_WINDOW) {}


void Proc_UIManager::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("Proc_DisplayUpdate::setup()"));
#endif

  // Initialise battery meter
  batterySetup();

  // Initialise interrup redirection mechanism
  instance = this;

  // Initialize gesture sensor
  initSuccess = initGesture();

  // Clear event queue, in case
  getUserEvent();

  // Set interrupt pin as input
  pinMode(GESTURE_INTERRUPT_PIN, INPUT);

  // attach interrupt handler
  attachInterrupt(digitalPinToInterrupt(GESTURE_INTERRUPT_PIN), onGestureISR, FALLING);

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("registering screen"));
#endif

  // Initialise first screen
  currentScreenID = config.startScreen;
  currentScreen = ScreenFactory::getInstance()->createScreen(currentScreenID);

  // Activate screen
  currentScreen->activate();

  // Refresh screen for first time
  currentScreen->update();

  // Set screen refresh interval appropriate for current screen
  this->setPeriod(currentScreen->getRefreshPeriod());

  // Force redraw of top bar if required
  if (!currentScreen->isFullScreen())
    drawBar(true);

  // Backlit control
  eventTime = millis();
}


void Proc_UIManager::service()
{

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("Proc_DisplayUpdate::service()"));
#endif

  if (!initSuccess)
  {
    errLog(F("Gesture sensor was not initialised - retrying"));

    initSuccess = initGesture();
  }

  long starttime = millis();

  // Calculate state of charge (linear approximation on voltage)
  float SoC = 100 / (VOLT_HIGH - VOLT_LOW) * (getVolt() - VOLT_LOW);
  avgSOC.push(SoC > 100 ? 100 : SoC); // 3.6v = 100 %; 3v = 0 %


  // Handle low battery condition
  // If battery depleted, force LOWBAT screen and move on
  if (getVolt() <= VOLT_LOW)
  {
    // Was already at the lowbatt screen?
    if ( currentScreenID != LOWBATT_SCREEN)
    {
      // NO: Force LowBattery screen

      syslog.log(LOG_CRIT, F("BATTERY LOW - HALTING SYSTEM"));

      // Deactivate & Deallocate previous screen
      currentScreen->deactivate();
      delete currentScreen;

      // LOWBATT becomes the current screen
      currentScreenID = LOWBATT_SCREEN;

      // Allocate & Activate LOWBATT screen
      currentScreen = new ScreenLowbatt();
      currentScreen->activate();

      // Force redraw of top bar if required
      if (!currentScreen->isFullScreen())
        drawBar(true);

      // Set screen refresh interval appropriate for current screen
      this->setPeriod(currentScreen->getRefreshPeriod());

      // Switch off processes
      // TODO: now hardcoded... make it generic and implement global stopProcesses
      procPtr.ComboTemperatureHumiditySensor.disable();
      procPtr.ComboPressureHumiditySensor.disable();
      procPtr.ParticleSensor.disable();
      procPtr.CO2Sensor.disable();
      procPtr.VOCSensor.disable();
      procPtr.MultiGasSensor.disable();
      procPtr.MQTTUpdate.disable();
      procPtr.GeigerSensor.disable();
      procPtr.GeoLocation.disable();

    }
    // Already in lowbatt screen, nothing to do
  }

  // If in LOWBATT mode and battery is recharging, reset
  else if (getVolt() > VOLT_HIGH &&  currentScreenID == LOWBATT_SCREEN)
  {
    syslog.log(LOG_INFO, F("BATTERY HIGH - RESTARTING SYSTEM") );
    delay(1000);
    ESP.restart();
  }


  // Event processing
  bool wasEvent =  false;

  // Is there an event to be handled?
  if (eventFlag)
  {
    if (millis() - lastEventProcessing > 1000)
    {
      lastEventProcessing = millis();

#ifdef DEBUG_SYSLOG
      syslog.logf(LOG_INFO, "User event serviced with delay of %d ms", (millis() - eventTime) );
#endif

      // reset event flags
      eventFlag = false;

      // Remember that an event was processed
      wasEvent =  true;

      // get event from sensor
      int eventID = getUserEvent();

      // If screen was off, just turn it on and no further actions no matter what the event is
      if (!isDisplayOn)
      {
        displayOn();
      }

      // Display was on, process event

      // If we are in low batt mode, ignore all events, apart from screen switch off
      else if (currentScreenID == LOWBATT_SCREEN)
      {
        if (eventID != GES_FORWARD)
          eventID = GES_NONE;
      }

      // If FORWARD, switch off screen and exit
      else if (eventID == GES_FORWARD)
      {
        // Switch off screen
        displayOff();

        // Clear spurious events, if any
        delay(500);
        if (eventFlag)
        {

#ifdef DEBUG_SERIAL
          Serial.println("++++ Spurious event!");
#endif
          gestureSensor.cancelGesture();
          eventFlag = false;
        }
      }
      // in case of valid event, execute the corresponding action
      else if (eventID != GES_NONE)
      {
        // pass event to current screen
        bool cancelEvent = currentScreen->onUserEvent(eventID);

        // if current screen does not consume the event, process screen transition...
        if (!cancelEvent)
        {

          // Is it Setup event?
          if (eventID == GES_CNTRCLOCKWISE)
          {

            // Draw rotation icon
            ui.fillArc(120, 160, 0, 45, 70, 70, 30, TFT_RED);

            LCD.fillTriangle(120, 75,  // top
                             120, 135, // bottom
                             80, 105, // middle
                             TFT_RED);

            // Wait a bit
            delay(250);

            // Deactivate & Deallocate previous screen
            currentScreen->deactivate();
            delete currentScreen;

            // Setup becomes the current screen
            currentScreenID = SETUP_SCREEN;

            // Allocate & Activate setup screen
            currentScreen = ScreenFactory::getInstance()->createScreen(currentScreenID);
            currentScreen->activate();

            // Force redraw of top bar if required
            if (!currentScreen->isFullScreen())
              drawBar(true);

            // Set screen refresh interval appropriate for current screen
            this->setPeriod(currentScreen->getRefreshPeriod());

          }
          else
            // Is it screen rotation event?
            if (eventID == GES_CLOCKWISE)
            {

              // Draw rotation icon
              ui.fillArc(120, 160, 90, 45, 70, 70, 30, TFT_RED);

              LCD.fillTriangle(120, 75,  // top
                               120, 135, // bottom
                               160, 105, // middle
                               TFT_RED);

              // Wait a bit
              delay(250);

              // Determine new screen rotation
              if (currentScreenRotation == 2)
                currentScreenRotation = 0;
              else
                currentScreenRotation = 2;

              // Deactivate current screen
              currentScreen->deactivate();

              // Rotate screen
              LCD.setRotation(currentScreenRotation);

              // Wipe Screen
              LCD.fillScreen(TFT_BLACK);

              // Reactivate current screen to redraw it
              currentScreen->activate();

              // Forget previous screen update, so to force immediate refresh
              currentScreen->lastUpdate = 0;

              // Force redraw of top bar if required
              if (!currentScreen->isFullScreen())
                drawBar(true);

            }
            else
            {

              // Determine new screen ID, based on user gesture
              int newScreenID = handleSwipe(eventID, currentScreenID);

#ifdef DEBUG_SYSLOG
              syslog.log(LOG_INFO, "SCREEN TRANSITION " + String(currentScreenID) + " --> " + String(newScreenID));
#endif

              // If screen has changed...
              if (newScreenID != currentScreenID)
              {
                // Show arrows on swipe
                switch (eventID)
                {
                  case GES_RIGHT:
                    LCD.fillTriangle(190, 80,  // top
                                     190, 240, // bottom
                                     230, 160, // middle
                                     TFT_RED);
                    break;

                  case GES_LEFT:
                    LCD.fillTriangle(50, 80,   // top
                                     50, 240,  // bottom
                                     10, 160,  // middle
                                     TFT_RED);
                    break;
                }

                // Wait a bit
                delay(250);

                // Deactivate & Deallocate previous screen
                currentScreen->deactivate();
                delete currentScreen;

                // Allocate & Activate selected screen
                currentScreen = ScreenFactory::getInstance()->createScreen(newScreenID);
                currentScreen->activate();

                // Force redraw of top bar if required
                if (!currentScreen->isFullScreen())
                  drawBar(true);

                // Set screen refresh interval appropriate for current screen
                this->setPeriod(currentScreen->getRefreshPeriod());

                // make it the current screen
                currentScreenID = newScreenID;
              }
            }
        }
      }
    } // end event processing
  }
  else     // Service with no event
  {

    // If timeout, switch off backlight
    if  (isDisplayOn && millis() - eventTime > BACKLIGHT_TIMEOUT * (1 + (getSoC() > 95)) ? 1 : 0) // Over 94% we assume we are on power, longer timeout
    {
#ifdef DEBUG_SERIAL
      Serial.println("Timeout - switching off display");
#endif
      displayOff();
    }
  }

  // Update screen only if backlight is on OR if s=current screen requires to be refreshed anyway
  if (isDisplayOn || currentScreen->getRefreshWithScreenOff())
  {
    // If screen needs refresh at this time, do it
    if (millis() - currentScreen->lastUpdate >= currentScreen->getRefreshPeriod() - 50)  // make some allowance for the Scheduler delay...
    {
      // Refresh top bar unless we are full screen
      if (!currentScreen->isFullScreen())
        drawBar();

      // refresh screen
      currentScreen->lastUpdate = millis();
      currentScreen->update();
    }
  }

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, F("END Proc_DisplayUpdate::service()"));
#endif

}

String Proc_UIManager:: getCurrentScreenName()
{
  return currentScreen->getScreenName();
}

// Static relay
void Proc_UIManager::onGestureISR()
{
  instance->onGesture();
}

// Called by ISR to signal user iteraction
void Proc_UIManager::onGesture()
{

  // Dont flag interrupt if previous one has not been serviced yet
  if (!eventFlag)
  {

    // Remember event
    eventFlag = true;

    // Force scheduling
    this->force();

    // Remember last user event
    eventTime = millis();

  }
}


int Proc_UIManager::handleSwipe(int evt, int curScrn)
{
  if (evt == GES_RIGHT)
  {
    curScrn ++;
    if (curScrn >= ScreenFactory::getInstance()->getScreenCount())
      curScrn = 1;
  }
  else if (evt == GES_LEFT)
  {
    curScrn --;
    if (curScrn < 1)
      curScrn = ScreenFactory::getInstance()->getScreenCount() - 1;
  }
  return curScrn;
}



int Proc_UIManager::getUserEvent()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("Reading event..."));
#endif

  int gesture = gestureSensor.readGesture() ;

  // NOTE: Gesture have been remaped to accommodate for the sensor positioning in the case!!
  switch (gesture)
  {
    case GES_UP:

      // Remap
      if (currentScreenRotation == 0)
        gesture = GES_LEFT;
      else
        gesture = GES_RIGHT;
      break;

    case GES_DOWN:

      // Remap
      if (currentScreenRotation == 0)
        gesture = GES_RIGHT;
      else
        gesture = GES_LEFT;

      break;

    case GES_LEFT:

      // Remap
      if (currentScreenRotation == 0)
        gesture = GES_DOWN;
      else
        gesture = GES_UP;
      break;

    case GES_RIGHT:

      // Remap
      if (currentScreenRotation == 0)
        gesture = GES_UP;
      else
        gesture = GES_DOWN;
      break;

    case GES_FORWARD:

      break;

    case GES_BACKWARD:

      break;

    case GES_CLOCKWISE:

      break;

    case GES_CNTRCLOCKWISE:

      break;

    case GES_WAVE:

      break;

#if defined(DEBUG_SYSLOG ) || defined(DEBUG_SERIAL )

    default:
#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, F("NONE"));
#endif
#ifdef DEBUG_SERIAL
      Serial.println(F("NONE"));
#endif
#endif
  }

  return gesture;
}

/*
   Draw the top information bar
*/

void Proc_UIManager::drawBar(bool forceDraw)
{

  String lineBuffer;

  // ********* Date display

  // set small font
  LCD.setFreeFont(&ArialRoundedMTBold_14);
  LCD.setTextDatum(BC_DATUM);
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);

  if (config.connected && NTP.getLastNTPSync() > 0)
  {
    // Display date
    lineBuffer = String(dayStr(weekday()));
    lineBuffer += F(" ");
    lineBuffer += String(day());
    lineBuffer += F(" ");
    lineBuffer +=  String(monthStr(month()));
    lineBuffer += F(" ");
    lineBuffer += String(year());
  }
  else
  {
    // Print BLANK
    lineBuffer = "                        ";
  }

  // Draw it only if it changed
  if (forceDraw || lineBuffer != topBar.dateLine )
  {
    // Remember last printed value
    topBar.dateLine = lineBuffer;

    // Erase current line
    LCD.fillRect(0, 0, LCD.width(), LCD.fontHeight(GFXFF), TFT_BLACK);

    LCD.setTextPadding(LCD.textWidth(F("  Saturday, 44 November 4444  ")));  // String width + margin
    LCD.drawString(lineBuffer, 120, 14);
  }

  // ********* Location display

  if (procPtr.GeoLocation.isValid() && config.connected)
  {
    lineBuffer = procPtr.GeoLocation.getLocality()  + " " + procPtr.GeoLocation.getCountryCode();
  }
  else
  {
    lineBuffer = "                                  ";
  }

  // Draw it only if it changed
  if (forceDraw || lineBuffer != topBar.locationLine )
  {
    // Remember last printed value
    topBar.locationLine = lineBuffer;
    LCD.setTextPadding(LCD.textWidth(F("                          ")));  // String width + margin
    LCD.drawString(lineBuffer, 120, 63); // was 65
  }

#ifdef DEBUG_SYSLOG
  // print uptime
  LCD.setTextDatum(TC_DATUM);
  LCD.drawString(upTime(), 120, 320);
#endif

  // ********* Time display

  LCD.setFreeFont(&ArialRoundedMTBold_36);
  LCD.setTextDatum(BC_DATUM);
  LCD.setTextColor(TFT_YELLOW, TFT_BLACK);


  if (config.connected && NTP.getLastNTPSync() > 0)
  {
    // Print time
    char timeNow[10];
    sprintf(timeNow, "%d:%02d", hour(), minute());
    lineBuffer = String(timeNow);
  }
  else
  {
    // Print ATMOSCAN
    lineBuffer = "AtmoScan";
  }

  // Draw it only if it changed
  if (forceDraw || lineBuffer != topBar.timeLine )
  {
    // Remember last printed value
    topBar.timeLine = lineBuffer;
    LCD.setTextPadding(LCD.textWidth(F("     44:44     ")));  // String width + margin
    LCD.drawString(lineBuffer, 120, 50);
  }

  // ************ Draw WiFi radio gauge
  drawWifiGauge(220, 17, WiFi.RSSI(), forceDraw);


  // ************ Draw battery gauge
  drawBatteryGauge(5, 17, getSoC(), 30, forceDraw);


  // Draw separator between upper bar and application screen
  ui.drawSeparator(64);

  LCD.setTextPadding(0);

}

/*
 * *
 * * Graphics helper functions
 * *
*/

void Proc_UIManager::drawWifiGauge(int topX, int topY, int dBm, bool forceDraw)
{

  // Draw it only if it changed
  if (dBm != topBar.dBm || forceDraw)
  {
    // Draw it
    int spacing = 5;
    int thick = 4;
    int radius = 3;
    int count = 5;

    int quality, bars;

    // dBm to Quality:
    if (dBm <= -100 || dBm == 31)
      quality = 0;
    else if (dBm >= -60)
      quality = 100;
    else
      quality = 3.3 * dBm + 330;

    // If previous was 31 (disconnected) erase the whole icon and start over
    if (topBar.dBm == 31)
    {
      LCD.fillRect(topX, topY, 15, 5 * (thick + spacing), TFT_BLACK);
    }

    if (quality == 0)
    {
      bars = 0;
    }
    else if (quality < 20)
    {
      bars = 1;
    }
    else if (quality < 40)
    {
      bars = 2;
    }
    else if (quality < 60)
    {
      bars = 3;
    }
    else if (quality < 80)
    {
      bars = 4;
    }
    else if (quality >= 80)
    {
      bars = 5;
    }

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_INFO, "RSSI = " + String(WiFi.RSSI()) + "dbm");
    syslog.log(LOG_INFO, "WiFI quality = " + String(quality));
    syslog.log(LOG_INFO, "WiFI bars = " + String(bars));
#endif

    for (int i = 0; i < count; i++)
    {
      int color;
      if (i  >= (5 - bars))
        color = TFT_GREEN;
      else
        color = 0xEF5D; // GREY 90%

      LCD.fillRoundRect(topX + i * 2, topY + i * spacing, 15 - i * 2, thick, radius, color);
    }

    // If disconnected, red X over bars
    if (dBm == 31)
    {
      LCD.setFreeFont(FSSB12);
      LCD.setTextDatum(MC_DATUM);
      LCD.setTextColor(TFT_RED);
      LCD.drawString("X", topX + 9, topY + 10);
    }

    // Remember last displayed value
    topBar.dBm = dBm;

  }
}

void Proc_UIManager::drawBatteryGauge(int topX, int topY, int batLevel, int redLevel, bool forceDraw)
{
  // Draw it only if it changed, unless forced
  if (batLevel != topBar.batLevel || forceDraw)
  {
    // Remember last printed value
    topBar.batLevel = batLevel;

    // Draw it
    int batHeight = 24;
    int batWidth = 10;
    int tipHeight = 2;
    int tipWidth = 4;

    // Draw battery outline
    LCD.fillRect(topX + batWidth / 2 - tipWidth / 2, topY, tipWidth, tipHeight, TFT_WHITE); // tip
    LCD.drawRect(topX, topY + tipHeight, batWidth, batHeight, TFT_WHITE); // battery body

    // Decide fill color
    int batfillColor = batLevel > 30 ? TFT_GREEN : TFT_RED;
    int batFillHeight = batLevel * (batHeight - 2) / 100;

    // Fill battery + complement
    LCD.fillRect(topX + 1, topY + batHeight + tipHeight - batFillHeight - 1, batWidth - 2, batFillHeight , batfillColor); // Fill
    LCD.fillRect(topX + 1, topY + tipHeight + 1, batWidth - 2, batHeight - batFillHeight - 2, TFT_BLACK);                 // Complement
  }
}


bool Proc_UIManager::eventPending()
{
  return eventFlag;
}


void Proc_UIManager::displayOn()
{

#ifdef DEBUG_SERIAL
  Serial.println("Display: turning ON");
#endif

  initDisplay();
  digitalWrite(BACKLIGHT_PIN, HIGH);
  isDisplayOn = true;
}

void Proc_UIManager::displayOff()
{

#ifdef DEBUG_SERIAL
  Serial.println("Display: turning OFF");
#endif

  initDisplay();
  digitalWrite(BACKLIGHT_PIN, LOW);
  isDisplayOn = false;
}

bool Proc_UIManager::initDisplay()
{
  if (!displayInitialized)
  {
    // Init display
    pinMode(BACKLIGHT_PIN, OUTPUT);
    displayInitialized = true;
  }
}


/*
     Battery management functions
*/

void Proc_UIManager::batterySetup()
{
  batteryMonitor.reset();
  batteryMonitor.quickStart();
  delay(1000);
}

//#ifdef DEBUG_SYSLOG
String Proc_UIManager::batteryStats()
{
  float cellVoltage = getVolt();
  String stats = "Voltage \t\t";
  stats += String(cellVoltage, 4);
  stats += "V \r\n";

  float stateOfCharge = getSoC();
  stats += ", State of charge \t";
  stats += String(stateOfCharge, 4);
  stats += "% ";
  return stats;
}



/* Useful Constants */
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)


String Proc_UIManager::upTime()
{
  long val = millis() / 1000;
  int days = elapsedDays(val);
  int hours = numberOfHours(val);
  int minutes = numberOfMinutes(val);
  int seconds = numberOfSeconds(val);

  // digital clock display of current time

  return  (printDigits(days) + "d " + printDigits(hours) + "h " + printDigits(minutes) + "m ");// + printDigits(seconds));
}

String Proc_UIManager::printDigits(int digits)
{
  String buffer;
  // utility function for digital clock display: prints colon and leading 0
  if (digits < 10)
    buffer = "0" + buffer;
  return buffer + digits;
}

float Proc_UIManager::getVolt()
{
  return batteryMonitor.getVCell();
}

float Proc_UIManager::getNativeSoC()
{
  float SoC = batteryMonitor.getSoC();
  if (SoC > 100)
    SoC = 100;
  return SoC;
}



float Proc_UIManager::getSoC()
{
  return avgSOC.mean();
}

bool Proc_UIManager::initGesture()
{
  for (int i = 0; i < 3; i++)
  {
    // Gesture sensor initialization
    gestureSensor = PAJ7620U();

    uint8_t error = gestureSensor.begin();
    if (!error)
    {
      syslog.log(LOG_DEBUG, F("PAJ7620U initialization successful"));
      return true;
      break;
    }
    else
    {
      errLog("PAJ7620U init error " + String (error));

      delay(2000);
    }
  }
  return false;
}

/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#include <ProcessScheduler.h>     // https://github.com/wizard97/ArduinoProcessScheduler
#include <Syslog.h>                 // https://github.com/arcao/ESP8266_Syslog
#include <Adafruit_BME280.h>        // https://github.com/adafruit/Adafruit_BME280_Library NOTE: Requires https://github.com/adafruit/Adafruit_Sensor
#include <MutichannelGasSensor.h>   // https://github.com/MarcFinns/Mutichannel_Gas_Sensor

#include "P_AirSensors.h"
#include "GlobalDefinitions.h"

// External variables
extern Syslog syslog;
extern struct ProcessContainer procPtr;
extern struct Configuration config;

extern TFT_eSPI LCD;


// Prototypes
void errLog(String msg);

// Geiger tube definitions
#define LND712_CONV_FACTOR  123 // CPS * 1/123 = uSv/h

// Particle sensor PMS7003 definitions
#define PMS7003_COMMAND_SIZE 7
#define PMS7003_RESPONSE_SIZE 32

// CO2 Sensor MH-Z19 definitions
#define MHZ19_COMMAND_SIZE 9
#define MHZ19_RESPONSE_SIZE 9

// Temperature sensor definitions
#define TEMPERATURE_ADJUSTMENT_FACTOR -0.4 // NOTE: empirical correction based on observations, TBC

// Particle sensor PMS7003 definitions
static const byte PMS7003_cmdPassiveEnable[] = {0x42, 0x4d, 0xe1, 0x00, 0x00, 0x01, 0x70};
static const byte PMS7003_cmdPassiveRead[] = {0x42, 0x4d, 0xe2, 0x00, 0x00, 0x01, 0x71};
static const byte PMS7003_cmdSleep[] = {0x42, 0x4d, 0xe4, 0x00, 0x00, 0x01, 0x73};
static const byte PMS7003_cmdWakeup[] = {0x42, 0x4d, 0xe4, 0x00, 0x01, 0x01, 0x74};

// CO2 Sensor MH-Z19 definitions
static const byte MHZ19_cmdRead[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

// Sensor processes implementation

// -------------------------------------------------------
// Combo Temperature & Umidity Sensor wrapper (HDC1080)
// -------------------------------------------------------

Proc_ComboTemperatureHumiditySensor::Proc_ComboTemperatureHumiditySensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
  :  Process(manager, pr, period, iterations),
     avgTemperature(AVERAGING_WINDOW),
     avgHumidity(AVERAGING_WINDOW) {}

void Proc_ComboTemperatureHumiditySensor::setup()
{

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "Proc_ComboTemperatureHumiditySensor::setup()");
#endif

  //  Sensor begin
  hdc1080.begin(0x40);

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, "Manufacturer ID=" + String((hdc1080.readManufacturerId(), HEX)));  // 0x5449 ID of Texas Instruments
  syslog.log(LOG_INFO, "Device ID=" + String((hdc1080.readDeviceId(), HEX)));  // 0x1050 ID of the device
#endif

  // Is the device reachable?
  if (!(hdc1080.readDeviceId() == 0x1050))
  {
    // There was a problem detecting the sensor
    errLog(F("Err HDC1080 - disabled"));

    // Set invalid reading
    avgTemperature.push(0);
    avgHumidity.push(0);

    // Disable process
    this->disable();
  }

  // first reading, to initialise averages

  // Get temperature value
  float temp = hdc1080.readTemperature();

  // initialize average
  avgTemperature.push(temp + TEMPERATURE_ADJUSTMENT_FACTOR);


  // Get humidity value
  float hum = hdc1080.readHumidity();

  // initialize average
  avgHumidity.push(hum);

}

void Proc_ComboTemperatureHumiditySensor::service()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "Proc_ComboTemperatureHumiditySensor::service()");
#endif

  // Get temperature value
  float temp = hdc1080.readTemperature();

  // If sensor misbehave (might happen when battery is drained), reset it and skip a reading cycle
  // Misbehavior is define as a jump in value of more than 5 degree in 5 seconds with respect to the moving average
  if (abs(temp - avgTemperature.mean()) > 5 )
  {
    // Log error
    errLog(F("TemperatureHumiditySensor - Temp error - resetting sensor"));
    syslog.log(LOG_ERR, String(F("Temp reading was =")) + String(temp));
    syslog.log(LOG_ERR, String(F("Battery voltage was =")) + String(procPtr.UIManager.getVolt()));

    //  Reset Sensor
    hdc1080.begin(0x40);

    // Skip cycle
    return;
  }


  // Get humidity value
  float humidity = hdc1080.readHumidity();

  // If sensor misbehave (might happen when battery is drained), reset it and skip a reading cycle
  // Misbehavior is define as a jump in value of more than 10% in 5 seconds with respect to the moving average
  if (abs(humidity - avgHumidity.mean()) > 10 )
  {
    // Log error
    errLog(F("TemperatureHumiditySensor - Humidity error - resetting sensor"));
    syslog.log(LOG_ERR, String(F("Hum reading was =")) + String(humidity));
    syslog.log(LOG_ERR, String(F("Battery voltage was =")) + String(procPtr.UIManager.getVolt()));

    //  Reset Sensor
    hdc1080.begin(0x40);

    // Skip cycle
    return;
  }

  // averages
  avgTemperature.push(temp + TEMPERATURE_ADJUSTMENT_FACTOR);
  avgHumidity.push(humidity);

}

float Proc_ComboTemperatureHumiditySensor::getTemperature()
{
  return avgTemperature.mean();
}

float Proc_ComboTemperatureHumiditySensor::getHumidity()
{
  return avgHumidity.mean();
}
// END Combo Temperature & Umidity Sensor wrapper (HDC1080)


// -------------------------------------------------------
// Combo Pressure & humidity Sensor process (BME280)
// -------------------------------------------------------

Proc_ComboPressureHumiditySensor::Proc_ComboPressureHumiditySensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
  :  Process(manager, pr, period, iterations),
     avgPressure(AVERAGING_WINDOW),
     avgHumidity(AVERAGING_WINDOW),
     avgTemperature(AVERAGING_WINDOW)
{
}

void Proc_ComboPressureHumiditySensor::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "Proc_ComboPressureHumiditySensor::setup()");
#endif

  // Initialise the sensor
  if (!bme.begin(0x76))
  {
    // There was a problem detecting the sensor
    errLog(F("Err BME280 - disabled"));

    // Set invalid reading
    avgPressure.push(0);
    avgTemperature.push(0);
    avgHumidity.push(0);

    // Disable process
    this->disable();
  }
}

void Proc_ComboPressureHumiditySensor::service()
{
  // syslog.log(LOG_DEBUG, "2 - BME280");
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "Proc_ComboPressureHumiditySensor::service()");
#endif

  // Get values
  float pressure = bme.readPressure() / 100.0F;
  float humidity = bme.readHumidity();
  float temperature = bme.readTemperature();

  avgPressure.push(pressure);
  avgHumidity.push(humidity);
  avgTemperature.push(temperature);

}


float Proc_ComboPressureHumiditySensor::getPressure()
{
  return avgPressure.mean();
}

float Proc_ComboPressureHumiditySensor::getHumidity()
{
  return avgHumidity.mean();
}

float Proc_ComboPressureHumiditySensor::getTemperature()
{
  return avgTemperature.mean();
}
// END Pressure Sensor process (BME280)


// -------------------------------------------------------
// CO2 Sensor process (MH-Z19)
// -------------------------------------------------------

Proc_CO2Sensor::Proc_CO2Sensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
  :  Process(manager, pr, period, iterations),
     avgCO2(AVERAGING_WINDOW),
     co2Serial(CO2_RX_PIN, CO2_TX_PIN, false, 256)
{
}

void Proc_CO2Sensor::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "Proc_CO2Sensor::setup()");
#endif


  // Test whether sensor is readable
  for (int attempts = 0; attempts < 5; attempts++)
  {

    // Initialise SoftwareSerial for CO2 sensor MH-Z19
    co2Serial.begin(9600);
    co2Serial.enableIntTx(false);

    // Wait a bit to make SURE a sampling cycle (5s) elapses
    // delay (6000);

    // Discard spurious data
    co2Serial.flush();

    // Test sensor functionality
    service();

    // If data is read correctly, we are done
    if (!readError)
      return;

    errLog(F("Init err CO2 sensor, retrying"));

    // Wait a bit and retry
    delay(1000 + 2000 * attempts);
  }

  // Attempts exausted without a successful read
  // There was a problem detecting the sensor
  errLog(F("Err MH-Z19 - disabled"));

  // Set invalid reading
  avgCO2.push(0);

  // Disable process
  this->disable();
}

void Proc_CO2Sensor::service()
{

  // syslog.log(LOG_DEBUG, "3 - MH-Z19");
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "Proc_CO2Sensor::service()");
#endif

  unsigned char Buffer[MHZ19_RESPONSE_SIZE];

  // Reset error conditions
  readError = false;

  // Clear buffer for any spurious data received since last reading
  co2Serial.flush();

  //request PPM CO2
  co2Serial.write(MHZ19_cmdRead, MHZ19_COMMAND_SIZE);

  // Read response
  co2Serial.readBytes(Buffer, MHZ19_RESPONSE_SIZE);

  //  PRINT BUFFER
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "CO2 sensor response - " + bytes2hex(Buffer, MHZ19_RESPONSE_SIZE));
#endif

  if (Buffer[0] != 0xFF)
  {
    errLog(F("CO2 Sensor - Wrong starting byte"));

    // empty buffer
    //co2Serial.readBytes(Buffer, MHZ19_RESPONSE_SIZE * 2);

    readError = true;

    return ;
  }

  if (Buffer[1] != 0x86)
  {
    delay(1000);
    errLog(F("CO2 Sensor - Wrong command"));

    // empty buffer
    // co2Serial.readBytes(Buffer, MHZ19_RESPONSE_SIZE * 2);

    readError = true;

    return ;
  }

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "CO2 Sensor - header OK");
#endif

  // Get value
  int responseHigh = (int) Buffer[2];
  int responseLow = (int) Buffer[3];
  float co2 = (256 * responseHigh) + responseLow;

  // UpdateAverage
  avgCO2.push(co2);

  readError = false;
}

float Proc_CO2Sensor::getCO2()
{
  return avgCO2.mean();
}
// END CO2 Sensor process (MH-Z19)



// -------------------------------------------------------
// Particle Sensor process (PMS7003)
// -------------------------------------------------------

Proc_ParticleSensor::Proc_ParticleSensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations)
  :  Process(manager, pr, period, iterations),
     avgPM01(AVERAGING_WINDOW),
     avgPM2_5(AVERAGING_WINDOW),
     avgPM10(AVERAGING_WINDOW)
{
}

void Proc_ParticleSensor::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "Proc_ParticleSensor::setup()");
#endif

  unsigned char Buffer[256];

  // Setup HW serial for particle sensor PMS7003
  Serial.begin(9600);
  Serial.setTimeout(3000);


  // Test whether sensor is readable
  for (int attempts = 0; attempts < 3; attempts++)
  {

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, "PMS7003 SETTING PASSIVE MODE");
#endif

    // Set passive mode command
    Serial.write(PMS7003_cmdPassiveEnable, PMS7003_COMMAND_SIZE);
    Serial.flush();

    // Wait for command to take effect
    delay(1000);

    // Test sensor functionality
    service();

    // If data is read correctly, we are done
    if (!readError)
    {
      return;
    }

    errLog(F("Init err PMS7003 sensor, retrying"));

    // Wait a bit and retry
    delay(1000 + 1000 * attempts);
  }

  // Attempts exausted without a successful read
  // There was a problem detecting the sensor
  errLog(F("Err PMS7003 - disabled"));

  // Set invalid reading
  avgPM01.push(0);
  avgPM2_5.push(0);
  avgPM10.push(0);

  // Disable process
  this->disable();

}

void Proc_ParticleSensor::service()
{
  // syslog.log(LOG_DEBUG, "4 - PMS7003");
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("Proc_ParticleSensor::service()"));
#endif

  unsigned char Buffer[PMS7003_RESPONSE_SIZE];

  // Reset error conditions
  readError = false;

  // Clear serial input buffer from spurious characters
  while (Serial.available() > 0)
  {

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, F("Clearing PMS serial buffer"));
#endif

    Serial.read();
  }

  // Send READ command
  Serial.write(PMS7003_cmdPassiveRead, PMS7003_COMMAND_SIZE);
  Serial.flush();

  // Receive response
  Serial.readBytes(Buffer, PMS7003_RESPONSE_SIZE);

  // PRINT BUFFER
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, "Particle sensor response - " + bytes2hex(Buffer, PMS7003_RESPONSE_SIZE));
#endif

  //start to read when detect 0x42 0x4d
  if (Buffer[0] == 0x42 && Buffer[1] == 0x4d)
  {
#ifdef DEBUG_SYSLOG
    syslog.log(LOG_DEBUG, F("Particle sensor - header OK"));
#endif

    // Is checksum ok?
    if (verifyChecksum(Buffer, PMS7003_RESPONSE_SIZE))
    {
#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, F("Buffer valid"));
#endif
      // Get values
      int PM01 = extractPM01(Buffer);
      int PM2_5 = extractPM2_5(Buffer);
      int PM10 = extractPM10(Buffer);

      // Average
      avgPM01.push(PM01);    //count PM1.0 value of the air detector module
      avgPM2_5.push(PM2_5);  //count PM2.5 value of the air detector module
      avgPM10.push(PM10);    //count PM10 value of the air detector module

      readError = false;

    }
    else
    {
      errLog(F("Particle sensor - Checksum wrong"));
      readError = true;
    }
  }
  else
  {
    errLog(F("Particle sensor -  timeout"));
    readError = true;
  }
}


float Proc_ParticleSensor::getPM01()
{
  return avgPM01.mean();
}

float Proc_ParticleSensor::getPM2_5()
{
  return avgPM2_5.mean();
}

float Proc_ParticleSensor::getPM10()
{
  return avgPM10.mean();
}

char Proc_ParticleSensor::verifyChecksum(unsigned char *thebuf, int leng)
{
  char receiveflag = 0;
  int receiveSum = 0;

  for (int i = 0; i < (leng - 2); i++)
  {
    receiveSum = receiveSum + thebuf[i];
  }
  // + 0x42 + 0x4d;

  if (receiveSum == ((thebuf[leng - 2] << 8) + thebuf[leng - 1])) //check the  data
  {
    receiveSum = 0;
    receiveflag = 1;
  }
  return receiveflag;
}

int Proc_ParticleSensor::extractPM01(unsigned char *thebuf)
{
  int PM01Val;
  PM01Val = ((thebuf[4] << 8) + thebuf[5]); //count PM1.0 value of the air detector module
  return PM01Val;
}

//extract PM Value to PC
int Proc_ParticleSensor::extractPM2_5(unsigned char *thebuf)
{
  int PM2_5Val;
  PM2_5Val = ((thebuf[6] << 8) + thebuf[7]); //count PM2.5 value of the air detector module
  return PM2_5Val;
}

//extract PM Value to PC
int Proc_ParticleSensor::extractPM10(unsigned char *thebuf)
{
  int PM10Val;
  PM10Val = ((thebuf[8] << 8) + thebuf[9]); //count PM10 value of the air detector module
  return PM10Val;
}

// END Particle Sensor process (PMS7003)


// -------------------------------------------------------
// VOC Sensor process (Grove - Air quality sensor v1.3)
// -------------------------------------------------------

Proc_VOCSensor::Proc_VOCSensor(Scheduler & manager, ProcPriority pr, unsigned int period, int iterations)
  :  Process(manager, pr, period, iterations),
     //avgVOC(AVERAGING_WINDOW)
     avgVOC(60)
{
}

void Proc_VOCSensor::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("Proc_VOCSensor::setup()"));
#endif
}

void Proc_VOCSensor::service()
{
  // syslog.log(LOG_DEBUG,F("5 - VOC");
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("Proc_VOCSensor::service()"));
#endif

  // Air Quality reading
  float voc = analogRead(VOC_PIN);

  // Average
  avgVOC.push(voc);
}

float Proc_VOCSensor::getVOC()
{
  return avgVOC.mean();
}
// END VOC Sensor process (Grove - Air quality sensor v1.3)

// -------------------------------------------------------
// Geiger Sensor process (LND712)
// -------------------------------------------------------

Proc_GeigerSensor *Proc_GeigerSensor::instance = nullptr;

Proc_GeigerSensor::Proc_GeigerSensor(Scheduler & manager, ProcPriority pr, unsigned int period, int iterations)
  :  Process(manager, pr, period, iterations),
     avgCPM(AVERAGING_WINDOW * 2),  // moving average over 1 minute
     avgRAD(15)                     // moving average over 15 minutes

{
}

void Proc_GeigerSensor::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("Proc_GeigerSensor::setup()"));
#endif

  instance = this;

  // Set interrupt pin as input and attach interrupt
  pinMode(GEIGER_INTERRUPT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(GEIGER_INTERRUPT_PIN), onTubeEventISR, RISING);
  counts = 0;
}

void Proc_GeigerSensor::service()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("Proc_GeigerSensor::service()"));
#endif

  unsigned long interval = millis() - lastCountReset;

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, String(F("Geiger: counts = ")) + String(counts));
#endif

  // SPURIOUS INTERVAL GUARD - If interval is too short or too long we skip the reading (spurious activation of process due to starvation)
  if ((interval < FAST_SAMPLE_PERIOD * 0.9) || (interval > FAST_SAMPLE_PERIOD * 2))
  {

#ifdef DEBUG_SYSLOG
    syslog.log(LOG_WARNING, String(F("Geiger: skipping this reading as interval is out of range ")) + String(interval));
#endif
  }

  else
  {
    // INTERVAL OK, Measure normally
    float thisCPM = (float(counts) * 60000.0 / float(interval));

    // SPURIOUS MEASUREMENT GUARD - If cpm is obviously out of range, discard it
    if (thisCPM > 100000 || thisCPM < 0)
    {
      // Spurious run
#ifdef DEBUG_SYSLOG
      syslog.log(LOG_WARNING, String(F("WARNING - Geiger thisCPM = ")) + String(thisCPM));
#endif
    }
    else
    {
      // Good run, record it
      avgCPM.push(thisCPM);

      // Smoothen Radiation measurement
      if (radAvgDelay == 0) // every minute
      {
        avgRAD.push(avgCPM.mean());
        radAvgDelay = 60000 / FAST_SAMPLE_PERIOD;
      }
      radAvgDelay--;

#ifdef DEBUG_SYSLOG
      syslog.log(LOG_DEBUG, String(F("Geiger last CPM = ")) + String(thisCPM));
      syslog.log(LOG_DEBUG, String(F("Geiger mean CPM = ")) + String(avgCPM.mean()));
#endif
    }

  }

  // In any case reset counters
  lastCountReset = millis();
  counts = 0;
}

float Proc_GeigerSensor::getCPM()
{
  return avgCPM.mean();
}

float Proc_GeigerSensor::getRadiation()
{
  return avgRAD.mean() / LND712_CONV_FACTOR;
}

void Proc_GeigerSensor::onTubeEventISR()
{
  instance->onTubeEvent();
}

void Proc_GeigerSensor::onTubeEvent()
{
  counts++;
}

// END Geiger Sensor wrapper (LND712)



// -------------------------------------------------------
// MultiGas Sensor process (Grove - MiCS6814)
// -------------------------------------------------------


Proc_MultiGasSensor::Proc_MultiGasSensor(Scheduler & manager, ProcPriority pr, unsigned int period, int iterations)
  :  Process(manager, pr, period, iterations),
     avgCO(AVERAGING_WINDOW),
     avgNO2(AVERAGING_WINDOW)
     //     avgNH3(AVERAGING_WINDOW),
     //     avgC3H8(AVERAGING_WINDOW),
     //     avgC4H10(AVERAGING_WINDOW),
     //     avgCH4(AVERAGING_WINDOW),
     //     avgH2(AVERAGING_WINDOW),
     //     avgC2H5OH(AVERAGING_WINDOW)
{
}

void Proc_MultiGasSensor::setup()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("Proc_MultiGasSensor::setup()"));
#endif

  // Check for sensor using the return value of
  // the Write.endTransmission to see if
  // the device did acknowledge to the address. 
  Wire.beginTransmission(0x04);
  if (Wire.endTransmission() != 0)
  {
    // There was a problem detecting the sensor
    errLog(F("Err Multigas - disabled"));

    // Set invalid reading
    avgCO.push(0);
    avgNO2.push(0);

    // Disable process
    this->disable();

    return;
  }


  // Init sensor
  gas.begin(0x04); //the default I2C address of the slave is 0x04

  gas.powerOn();
  delay(1000);
  unsigned char firmwareVersion = gas.getVersion();

#ifdef DEBUG_SYSLOG
  syslog.log(LOG_INFO, String(F("MultiGas firmware Version = " )) + String(firmwareVersion));
#endif
  if (firmwareVersion != 2)
  {
    errLog(F("MultiGas firmware mismatch - Sensor disabled"));

    // Set invalid reading
    //    avgNH3.push(0);
    avgCO.push(0);
    avgNO2.push(0);
    //    avgC3H8.push(0);
    //    avgC4H10.push(0);
    //    avgCH4.push(0);
    //    avgH2.push(0);
    //    avgC2H5OH.push(0);

    // Disable process
    this->disable();
  }
}

void Proc_MultiGasSensor::service()
{
#ifdef DEBUG_SYSLOG
  syslog.log(LOG_DEBUG, F("Proc_MultiGasSensor::service()"));
#endif

  // float nh3;
  float co;
  float no2;
  // float c3h8;
  // float c4h10;
  // float ch4;'
  // float h2;
  // float c2h5oh;

  // Get values
  // nh3 = gas.measure_NH3();
  co = gas.measure_CO();
  no2 = gas.measure_NO2();
  // c3h8 = gas.measure_C3H8();
  // c4h10 = gas.measure_C4H10();
  // ch4 = gas.measure_CH4();
  // h2 = gas.measure_H2();
  // c2h5oh = gas.measure_C2H5OH();

  // Average
  if (co >= 0)
    avgCO.push(co);

#ifdef DEBUG_SYSLOG
  else
    errLog(String(F("CO = ")) + String(co));
#endif

  if (no2 >= 0)
    avgNO2.push(no2);

  /*
    #ifdef DEBUG_SYSLOG
    else
    errLog( "NO2 = " + String(no2));
    #endif

      if (nh3 >= 0)
      avgNH3.push(nh3);

    #ifdef DEBUG_SYSLOG
      else
      errLog( "NH3 = " + String(nh3));
    #endif

          if (c3h8 >= 0)
          avgC3H8.push(c3h8);

    #ifdef DEBUG_SYSLOG
          else
          errLog( "c3h8 = " + String(c3h8));
    #endif

              if (c4h10 >= 0)
              avgC4H10.push(c4h10);

    #ifdef DEBUG_SYSLOG
              else
              errLog( "c4h10 = " + String(c4h10));
    #endif

                  if (ch4 >= 0)
                  avgCH4.push(ch4);

    #ifdef DEBUG_SYSLOG
                  else
                  errLog( "ch4 = " + String(ch4));
    #endif

                      if (h2 >= 0)
                      avgH2.push(h2);

    #ifdef DEBUG_SYSLOG
                      else
                      errLog( "h2 = " + String(h2));
    #endif

                          if (c2h5oh >= 0)
                          avgC2H5OH.push(c2h5oh);
    #ifdef DEBUG_SYSLOG
                          else
                          errLog( "c2h5oh = " + String(c2h5oh));
    #endif
  */

}


float Proc_MultiGasSensor::getCO()
{
  return avgCO.mean();
}

float Proc_MultiGasSensor::getNO2()
{
  return avgNO2.mean();
}

/*
  float Proc_MultiGasSensor::getNH3()
  {
  return avgNH3.mean();
  }

  float Proc_MultiGasSensor::getC3H8()
  {
  return avgC3H8.mean();
  }


  float Proc_MultiGasSensor::getC4H10()
  {
  return avgC4H10.mean();
  }

  float Proc_MultiGasSensor::getCH4()
  {
  return avgCH4.mean();
  }

  float Proc_MultiGasSensor::getH2()
  {
  return avgH2.mean();
  }

  float Proc_MultiGasSensor::getC2H5OH()
  {
  return avgC2H5OH.mean();
  }

*/

// END MultiGas Sensor wrapper (Grove - MiCS6814)


// -------------------------------------------------------
// Shared methods
// -------------------------------------------------------

String BaseSensor::bytes2hex(unsigned char buf[], int len)
{
  char onebyte[2];
  String output;
  for (int i = 0; i < len; i++)
  {
    sprintf(onebyte, "%02X", buf[i]);
    output = output  + String(onebyte) + String(F(": "));
  }
  return output;
}

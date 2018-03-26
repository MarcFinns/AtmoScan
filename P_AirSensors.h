/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include <Average.h>                // https://github.com/MajenkoLibraries/Average
#include <ProcessScheduler.h>       // https://github.com/wizard97/ArduinoProcessScheduler
#include <ClosedCube_HDC1080.h>     // https://github.com/MarcFinns/ClosedCube_HDC1080_Arduino
#include <Adafruit_BME280.h>        // https://github.com/adafruit/Adafruit_BME280_Library
#include <SoftwareSerial.h>         // https://github.com/plerup/espsoftwareserial

// -------------------------------------------------------
// BASE Sensor
// -------------------------------------------------------

class BaseSensor
{
  protected:
    // methods
    String bytes2hex(unsigned char buf[], int len);
};
// END BASE Sensor

// -------------------------------------------------------
//  Combo Temperature & Umidity Sensor wrapper (HDC1080)
// -------------------------------------------------------


class Proc_ComboTemperatureHumiditySensor: public Process, public BaseSensor
{
  public:
    Proc_ComboTemperatureHumiditySensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations);
    float getTemperature();
    float getHumidity();


  protected:
    virtual void setup();
    virtual void service();

  private:
    // Properties
    Average<float> avgTemperature;
    Average<float> avgHumidity;
    ClosedCube_HDC1080 hdc1080;


    // methods
};
// END Combo Temperature & Umidity Sensor wrapper (HDC1080)


// -------------------------------------------------------
//  Combo Pressure & Umidity Sensor wrapper (BME280)
// -------------------------------------------------------

class Proc_ComboPressureHumiditySensor: public Process, public BaseSensor
{
  public:
    Proc_ComboPressureHumiditySensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations);
    float getPressure();
    float getHumidity();
    float getTemperature();


  protected:
    virtual void setup();
    virtual void service();

  private:
    // Properties
    Average<float> avgPressure;
    Average<float> avgHumidity;
    Average<float> avgTemperature;
    Adafruit_BME280 bme;

    // methods

};
// END Pressure Sensor wrapper (BMP180)



// -------------------------------------------------------
// CO2 Sensor wrapper (MH-Z19)
// -------------------------------------------------------

class Proc_CO2Sensor: public Process, public BaseSensor
{
  public:
    Proc_CO2Sensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations);
    float getCO2();


  protected:
    virtual void setup();
    virtual void service();


  private:
    // Properties
    Average<float> avgCO2;
    SoftwareSerial co2Serial;
    bool readError =  false;

};
// END CO2 Sensor wrapper (MH-Z19)




// -------------------------------------------------------
// Particle Sensor wrapper (PMS7003)
// -------------------------------------------------------

class Proc_ParticleSensor: public Process, public BaseSensor
{
  public:
    Proc_ParticleSensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations);
    float getPM01();
    float getPM2_5();
    float getPM10();


  protected:
    virtual void setup();
    virtual void service();

  private:
    // Properties
    Average<float> avgPM01;
    Average<float> avgPM2_5;
    Average<float> avgPM10;
    bool readError =  false;

    // methods
    char verifyChecksum(unsigned char *thebuf, int leng);
    int extractPM01(unsigned char *thebuf);
    int extractPM2_5(unsigned char *thebuf);
    int extractPM10(unsigned char *thebuf);
};
// END Particle Sensor wrapper (PMS7003)

// -------------------------------------------------------
// VOC Sensor wrapper (Grove - Air quality sensor v1.3)
// -------------------------------------------------------

class Proc_VOCSensor : public Process, public BaseSensor
{
  public:
    Proc_VOCSensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations);
    float getVOC();


  protected:
    virtual void setup();
    virtual void service();

  private:
    // Properties
    Average<float> avgVOC;
};
// END VOC Sensor wrapper (Grove - Air quality sensor v1.3)


// -------------------------------------------------------
// Geiger Sensor process (LND712)
// -------------------------------------------------------

class Proc_GeigerSensor : public Process, public BaseSensor
{
  public:
    Proc_GeigerSensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations);
    // unsigned long getLastCPM();
    float getCPM();
    float getRadiation();
    static void onTubeEventISR();
    void onTubeEvent();

  protected:
    virtual void setup();
    virtual void service();

  private:
    // Properties
    volatile unsigned long counts = 0;  // variable for GM Tube events
    float radiationValue = 0.0;         // Radiation energy in uSv/h
    unsigned long lastCountReset = 0;
    static Proc_GeigerSensor * instance;
    Average<float> avgCPM;
};
// END Geiger Sensor wrapper (LND712)


// -------------------------------------------------------
// MultiGas Sensor process (Grove - MiCS6814)
// -------------------------------------------------------

class Proc_MultiGasSensor : public Process, public BaseSensor
{
  public:
    Proc_MultiGasSensor(Scheduler &manager, ProcPriority pr, unsigned int period, int iterations);
    float getNH3();
    float getCO();
    float getNO2();
    float getC3H8();
    float getC4H10();
    float getCH4();
    float getH2();
    float getC2H5OH();


  protected:
    virtual void setup();
    virtual void service();

  private:
    // Properties
    Average<float> avgNH3;
    Average<float> avgCO;
    Average<float> avgNO2;
    Average<float> avgC3H8;
    Average<float> avgC4H10;
    Average<float> avgCH4;
    Average<float> avgH2;
    Average<float> avgC2H5OH;

};
// END MultiGas Sensor wrapper (Grove - MiCS6814)

//#endif

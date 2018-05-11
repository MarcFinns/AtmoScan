/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include <Arduino.h>

#define TFT_GREY 0x5AEB

// Screen Handler definition
class AnalogMeter
{
  public:
    AnalogMeter(int offsetY, int decades, int orangeValue, int redValue, String measurement, String units);
    void begin();
    void drawNeedle(float value);

  private:
    String units, measurement;
    int offsetY;
    int currentValue = 0;
    int decades, orangeValue, redValue, minValue, maxValue;
    float ltx = 0;
    uint16_t osx = 120, osy = 120; // Saved x & y coords

    float mapf(float x, float in_min, float in_max, float out_min, float out_max);
    void fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour);
    void drawArc(int x, int y, int start_angle, int end_angle, int r,  unsigned int colour);

};



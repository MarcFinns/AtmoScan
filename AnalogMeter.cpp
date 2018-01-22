
/********************************************************/
/*                                                      */
/*  Marc Finns 2017, based on Bodmer TFT examples       */
/*                                                      */
/********************************************************/

#include "AnalogMeter.h"
#include "TFT_eSPI.h"

extern TFT_eSPI LCD;

AnalogMeter::AnalogMeter(int offsetY, int decades, int orangeValue, int redValue, String measurement, String units)
{
  this->units = units;
  this->offsetY = offsetY;
  this->decades = decades;
  this->orangeValue = orangeValue;
  this->redValue = redValue;
  this->measurement = measurement;
  this->units = units;

  this->minValue = 1;
  this->maxValue = pow(10, decades);
}

void AnalogMeter::begin()
{
  int label[5];

  label[0] = 0;
  for (int i = 1; i <= decades; i++)
  {
    label[i] = pow(10, i);
  }

  // Meter outline
  LCD.fillRect(0, offsetY, 239, 126, TFT_GREY);
  LCD.fillRect(5, offsetY + 3, 230, 119, TFT_WHITE);
  LCD.setTextColor(TFT_BLACK);  // Text colour

  // Draw scale

  // Draw ticks every 5 degrees from -50 to +50 degrees (100 deg. FSD swing)
  for (int i = -50; i < 51; i += 5) {
    // Long scale tick length
    int tl = 15;

    // Coodinates of tick to draw
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (100 + tl) + 120 ;
    uint16_t y0 = sy * (100 + tl) + 140 ;
    uint16_t x1 = sx * 100 + 120 ;
    uint16_t y1 = sy * 100 + 140 ;

    // Coordinates of next tick for zone fill
    float sx2 = cos((i + 5 - 90) * 0.0174532925);
    float sy2 = sin((i + 5 - 90) * 0.0174532925);
    int x2 = sx2 * (100 + tl) + 120 ;
    int y2 = sy2 * (100 + tl) + 140 ;
    int x3 = sx2 * 100 + 120 ;
    int y3 = sy2 * 100 + 140 ;

    // ORANGE zone limits
    if (i >= mapf(log10(orangeValue), log10(minValue), log10(maxValue), -50, 50) && i < mapf(log10(redValue), log10(minValue), log10(maxValue), -50, 50))
    {
      LCD.fillTriangle(x0 , y0 + offsetY, x1 , y1 + offsetY, x2, y2 + offsetY, TFT_ORANGE);
      LCD.fillTriangle(x1 , y1 + offsetY, x2 , y2 + offsetY, x3, y3 + offsetY, TFT_ORANGE);
    }

    // RED zone limits
    if (i >= mapf(log10(redValue), log10(minValue), log10(maxValue), -50, 50) && i < 50)
    {
      LCD.fillTriangle(x0, y0 + offsetY, x1 , y1 + offsetY, x2 , y2 + offsetY, TFT_RED);
      LCD.fillTriangle(x1, y1 + offsetY, x2 , y2 + offsetY, x3 , y3 + offsetY, TFT_RED);
    }

    /////////////////////
    if (i < -26)
    {
      // Short scale tick length
      if (i % 25 != 0) tl = 8;

      // Recalculate coords incase tick lenght changed
      x0 = sx * (100 + tl) + 120 ;
      y0 = sy * (100 + tl) + 140 ;
      x1 = sx * 100 + 120 ;
      y1 = sy * 100 + 140 ;

      // Draw tick
      LCD.drawLine(x0 , y0 + offsetY, x1 , y1 + offsetY, TFT_BLACK);
    }


    // Check if labels should be drawn, with position tweaks
    if (i % 25 == 0)
    {
      // Calculate label positions
      x0 = sx * (100 + tl + 10) + 117 ;
      y0 = sy * (100 + tl + 9) + 135;
      switch (i / 25) {
        case -2: LCD.drawCentreString(String(label[0]), x0 , y0 - 12 + offsetY, 2); break;
        case -1: LCD.drawCentreString(String(label[1]), x0 , y0 - 9 + offsetY, 2); break;
        case 0: LCD.drawCentreString(String(label[2]), x0 , y0 - 6 + offsetY, 2); break;
        case 1: LCD.drawCentreString(String(label[3]), x0 , y0 - 9 + offsetY, 2); break;
        case 2: LCD.drawCentreString(String(label[4]), x0 , y0 - 12 + offsetY, 2); break;
      }
    }

    /////////////////////

    // Now draw the arc of the scale
    sx = cos((i + 5 - 90) * 0.0174532925);
    sy = sin((i + 5 - 90) * 0.0174532925);
    x0 = sx * 100 + 120;
    y0 = sy * 100 + 140;

    // Draw scale arc, don't draw the last part
    if (i < 50) LCD.drawLine(x0, y0 + offsetY, x1, y1 + offsetY, TFT_BLACK);
  }

  //---------------------------------------------------------

  //  Draw logarithmic ticks

  int div, color, dec ;
  float divCoord;

  for (dec = 1; dec < decades; dec ++)
  {
    for (div = 1; div <= 10; div++)
    {
      divCoord = log10(div);

      int i = mapf((divCoord  +  dec), 0, 4, -50, 50);

      // Long scale tick length
      int tl = 15;

      // Coodinates of tick to draw
      float sx = cos((i - 90) * 0.0174532925);
      float sy = sin((i - 90) * 0.0174532925);
      uint16_t x0 = sx * (100 + tl) + 120 ;
      uint16_t y0 = sy * (100 + tl) + 140 ;
      uint16_t x1 = sx * 100 + 120 ;
      uint16_t y1 = sy * 100 + 140 ;


      // Short scale tick length
      if (i % 25 != 0) tl = 8;

      // Recalculate coords incase tick lenght changed
      x0 = sx * (100 + tl) + 120 ;
      y0 = sy * (100 + tl) + 140 ;
      x1 = sx * 100 + 120 ;
      y1 = sy * 100 + 140 ;

      // Draw tick
      LCD.drawLine(x0 , y0 + offsetY, x1 , y1 + offsetY, TFT_BLACK);

      // Check if labels should be drawn, with position tweaks
      if (i % 25 == 0)
      {
        // Calculate label positions
        x0 = sx * (100 + tl + 10) + 117 ;
        y0 = sy * (100 + tl + 9) + 135;
        switch (i / 25) {
          case -2: LCD.drawCentreString(String(label[0]), x0 , y0 - 12 + offsetY, 2); break;
          case -1: LCD.drawCentreString(String(label[1]), x0 , y0 - 9 + offsetY, 2); break;
          case 0: LCD.drawCentreString(String(label[2]), x0 , y0 - 6 + offsetY, 2); break;
          case 1: LCD.drawCentreString(String(label[3]), x0 , y0 - 9 + offsetY, 2); break;
          case 2: LCD.drawCentreString(String(label[4]), x0 , y0 - 12 + offsetY, 2); break;
        }
      }
    }
  }

  LCD.setTextDatum(TR_DATUM);
  LCD.drawString(measurement, 235 , 119 - 20 + offsetY, 2); // measurement at bottom right

  //  LCD.drawString(measurement, 5 + 230 - 40 , 119 - 20 + offsetY, 2); // measurement at bottom right
  LCD.drawCentreString(units, 120 , 70 + offsetY, 4); // Comment out to avoid font 4
  LCD.drawRect(5 , 3 + offsetY, 230, 119, TFT_BLACK); // Draw bezel line

  drawNeedle(1); // Put meter needle at 0
}

void AnalogMeter::drawNeedle(float value)
{
  int scaledValue;
  if (value <= 10)
    scaledValue = int(mapf(value, 0, 10 * decades, 0, 100));
  else
    scaledValue = int(mapf(log10(value), log10(minValue), log10(maxValue), 0, 100));

  int us_delay = 100;

  LCD.setTextColor(TFT_BLACK, TFT_WHITE);
  char buf[8]; dtostrf(value, 4, 0, buf);
  LCD.drawRightString(buf, 40 , 119 - 20 + offsetY, 2);

  if (scaledValue < -10) scaledValue = -10; // Limit value to emulate needle end stops
  if (scaledValue > 110) scaledValue = 110;

  // Move the needle util new value reached
  while (!(scaledValue == currentValue))
  {
    int increment = abs(scaledValue - currentValue) > 10 ? 5 : 1;

    if (currentValue < scaledValue)
      currentValue += increment;
    else
      currentValue -= increment;

    // if (us_delay == 0) currentValue = scaledValue; // Update immediately id delay is 0

    float sdeg = mapf(currentValue, -10, 110, -150, -30); // Map value to angle
    // Calcualte tip of needle coords
    float sx = cos(sdeg * 0.0174532925);
    float sy = sin(sdeg * 0.0174532925);

    // Calculate x delta of needle start (does not start at pivot point)
    float tx = tan((sdeg + 90) * 0.0174532925);

    // Erase old needle image
    LCD.drawLine(120 + 20 * ltx - 1 , 140 - 20 + offsetY, osx - 1 , osy + offsetY, TFT_WHITE);
    LCD.drawLine(120 + 20 * ltx , 140 - 20 + offsetY, osx , osy + offsetY, TFT_WHITE);
    LCD.drawLine(120 + 20 * ltx + 1 , 140 - 20 + offsetY, osx + 1 , osy + offsetY, TFT_WHITE);

    // Re-plot text under needle
    LCD.setTextColor(TFT_BLACK);
    LCD.drawCentreString(units, 120 , 70 + offsetY, 4); // // Comment out to avoid font 4

    // Store new needle end coords for next erase
    ltx = tx;
    osx = sx * 98 + 120;
    osy = sy * 98 + 140;

    // Draw the needle in the new postion, magenta makes needle a bit bolder
    // draws 3 lines to thicken needle
    LCD.drawLine(120 + 20 * ltx - 1 , 140 - 20 + offsetY, osx - 1 , osy + offsetY, TFT_RED);
    LCD.drawLine(120 + 20 * ltx , 140 - 20 + offsetY, osx , osy + offsetY, TFT_MAGENTA);
    LCD.drawLine(120 + 20 * ltx + 1 , 140 - 20 + offsetY, osx + 1 , osy + offsetY, TFT_RED);

    // Slow needle down slightly as it approaches new postion
    if (abs(currentValue - scaledValue) < 7) us_delay += us_delay / 5;

    // Wait before next update
    delayMicroseconds(us_delay);
  }
}


float AnalogMeter::mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/////////////
#define DEG2RAD 0.0174532925 // Degrees to Radians conversion factor
#define INC 2 // Minimum segment subtended angle and plotting angle increment (in degrees)

void AnalogMeter::fillArc(int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour)
{

  byte seg = 6; // Segments are 3 degrees wide = 120 segments for 360 degrees
  byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Calculate first pair of coordinates for segment start
  float sx = cos((start_angle - 90) * DEG2RAD);
  float sy = sin((start_angle - 90) * DEG2RAD);
  uint16_t x0 = sx * (rx - w) + x;
  uint16_t y0 = sy * (ry - w) + y;
  uint16_t x1 = sx * rx + x;
  uint16_t y1 = sy * ry + y;

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc) {

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (rx - w) + x;
    int y2 = sy2 * (ry - w) + y;
    int x3 = sx2 * rx + x;
    int y3 = sy2 * ry + y;

    if (w > 0)
    {
      LCD.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      LCD.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
    }
    else
    {
      LCD.drawLine(x0, y0, x2, y2, TFT_BLACK);

    }
    // Copy segment end to segment start for next segment
    x0 = x2;
    y0 = y2;
    x1 = x3;
    y1 = y3;
  }
}

void AnalogMeter::drawArc(int x, int y, int start_angle, int end_angle, int r,  unsigned int colour)
{
  for (float i = start_angle * DEG2RAD; i < end_angle * DEG2RAD; i = i + 0.05)
  {
    LCD.drawPixel(x + cos(i) * r, y + sin(i) * r, colour);
  }
}


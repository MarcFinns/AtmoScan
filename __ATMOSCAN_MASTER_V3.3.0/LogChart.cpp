/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#include "LogChart.h"
#include <TFT_eSPI.h>
#include "Fonts.h"

// External variables
extern TFT_eSPI LCD;

LogChart::LogChart(int topY, int height, int numDecades)
{

  _topY = topY;
  _height = height;
  _decadeHeight = float(height) / float(numDecades);
  _numDecades = numDecades;

}

void LogChart::begin()
{
  drawDiv(true);
  // LCD.drawRect(0, _topY - 1, LCD.width(), _numDecades * _decadeHeight + 2, TFT_RED);

  // Loop through all existing elements (if any) to redraw element in new positions
  // This is to accommodate screen rotation...
  for ( int x = buf.numElements(); x > 0; x--)
  {
    // Retrieve element
    int y = *buf.peek(x - 1);

    // If not negative...
    if (y > -1)
      // Draw it in new position
      LCD.drawPixel(HOFFSET + x - 1, _topY + _height -  y , TFT_YELLOW);
  }
}


void LogChart::drawPoint(int value)
{
  int chartValue, num, first, x, y;

  chartValue = -1;

  // Handle linear and log portions of the chart
  if (value >= 10)
    chartValue = log10(value) * _decadeHeight;
  else
    chartValue = value * _decadeHeight / 10.0 ;

  // If off chart, dont draw it
  if (chartValue > pow(10, _numDecades))
    chartValue = -1;

  // If still not full, just draw point
  num = buf.numElements();
  if (num < BUFFER_DEPTH)
  {
    // If not negative, draw it
    if (chartValue > -1)
      LCD.drawPixel(HOFFSET + num, _topY + _height - chartValue, TFT_YELLOW);
  }

  // if full
  else
  {

    // Remove oldest (outgoing) element on leftmost position
    //first;
    buf.pull(&first);

    // If not negative, Erase it from screen
    if (first > -1)
    {
      LCD.drawPixel(HOFFSET, _topY + _height -  first, first = 0 ? TFT_RED : TFT_BLACK);
    }

    // Loop through all elements to erase old
    for (x = buf.numElements(); x > 0; x--)
    {
      // Retrieve element
      y = *buf.peek(x - 1);

      // If not negative...
      if (y > -1)
        // Erase it from prior position
        if ((x >= 0) && (x < BUFFER_DEPTH))
          LCD.drawPixel(HOFFSET + x, _topY + _height -  y , y = 0 ? TFT_RED : TFT_BLACK);
    }

    // Draw grid
    drawDiv (false);

    // Draw new point
    LCD.drawPixel(HOFFSET + BUFFER_DEPTH - 1, _topY + _height - chartValue , TFT_YELLOW);

    // Loop through all elements to redraw element in new positions
    for ( x = buf.numElements(); x > 0; x--)
    {
      // Retrieve element
      y = *buf.peek(x - 1);

      // If not negative...
      if (y > -1)
        // Draw it in new position
        LCD.drawPixel(HOFFSET + x - 1, _topY + _height -  y , TFT_YELLOW);
    }
  }

  // Add new element to buffer
  buf.add(chartValue);
}


void LogChart::drawDiv(bool axis)
{
  int div, color, dec ;
  float divCoord;

  LCD.setFreeFont(&Dialog_plain_9);
  LCD.setTextColor(TFT_WHITE, TFT_BLACK);
  LCD.setTextDatum(BL_DATUM);


  // linear portion
  LCD.drawFastHLine(HOFFSET, _topY + _height, BUFFER_DEPTH, TFT_RED);

  for (int i = 1; i < 10; i ++)
  {
    LCD.drawFastHLine(HOFFSET, _topY + _height - _decadeHeight * i / 10.0, BUFFER_DEPTH, TFT_GREY);
  }
  if (axis)
    LCD.drawString(F("0"), 2, _topY + _height);

  // Logarithmic portion
  for (div = 1; div <= 9; div++)
  {
    // float divCoord = log10(div);
    divCoord = logs[div];
    color = (div == 1) ? TFT_RED : TFT_GREY;

    for (dec = 1; dec < _numDecades; dec ++)
    {
      // x, y , w, c
      LCD.drawFastHLine(HOFFSET, _topY + _height - _decadeHeight * (divCoord  +  dec), BUFFER_DEPTH, color);
      if (axis && div == 1)
      {
        LCD.drawString(String(div * pow(10, dec), 0), 2, _topY + _height - _decadeHeight * (divCoord  +  dec));
      }
    }
  }
  // Handle highest line
  LCD.drawFastHLine(HOFFSET, _topY + _height - _decadeHeight * _numDecades, BUFFER_DEPTH, TFT_RED);
  if (axis)
    LCD.drawString(String(pow(10, _numDecades), 0), 2, _topY + _height - _decadeHeight * _numDecades);
}

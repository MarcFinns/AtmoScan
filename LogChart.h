
#pragma once

#include <RingBufCPP.h> //https://github.com/wizard97/Embedded_RingBuf_CPP

#define TFT_GREY 0x5AEB
#define BUFFER_DEPTH 195
#define SCREEN_WIDTH 240
#define HOFFSET 44

class LogChart
{
  public:
    LogChart(int topY, int height, int numDecades);
    void begin();
    void drawPoint(int value);

  private:
    // Stack allocate the buffer to hold event structs
    RingBufCPP<int, BUFFER_DEPTH> buf;

    void drawDiv(bool axis = false );

    int _topY;
    int _height;
    int _decadeHeight;
    int _numDecades;

    const float logs[11] = { -1,                   // 0
                             0,                    // 1
                             0.301029995663981,    // 2
                             0.477121254719662,    // 3
                             0.602059991327962,    // 4
                             0.698970004336019,    // 5
                             0.778151250383644,    // 6
                             0.845098040014257,    // 7
                             0.903089986991944,    // 8
                             0.954242509439325,    // 9
                             1                     // 10
                           };

};



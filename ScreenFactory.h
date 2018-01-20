#pragma once

#include "Screen.h"
#include <vector>
#include "Arduino.h"

//-- CREATOR

class ScreenCreator
{
  public:
    ScreenCreator();
    ~ScreenCreator() {
      //-#ifdef DEBUG_SERIAL Serial.println("creator destructor");
    };
    virtual Screen* create() = 0;
};


template <class T> class ScreenCreatorImpl : public ScreenCreator
{
  public:

    ScreenCreatorImpl() : ScreenCreator() {};

    virtual Screen* create()
    {
      return new T;
    }
};



//-- FACTORY


class ScreenFactory
{
  public:
    ScreenFactory() {}
    void registerScreen(ScreenCreator* creator);
    Screen* createScreen(int ScreenID);
    int getScreenCount();

    static ScreenFactory* getInstance();

  private:
    static ScreenFactory* instance;
    std::vector<ScreenCreator*> screenCreators;

};




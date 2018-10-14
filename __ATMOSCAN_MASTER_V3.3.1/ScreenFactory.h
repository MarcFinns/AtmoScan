/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/

#pragma once

#include <vector>

#include <unordered_map>

#include <Arduino.h>

#include "Screen.h"

//-- CREATOR

class ScreenCreator
{
  public:
    ScreenCreator();
    ~ScreenCreator() {};
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

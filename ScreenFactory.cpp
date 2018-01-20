
#include "ScreenFactory.h"


// have the creator's constructor do the registration
ScreenCreator::ScreenCreator()
{
  //-#ifdef DEBUG_SERIAL Serial.println("creator constructor");
  ScreenFactory::getInstance()->registerScreen(this);
}


ScreenFactory* ScreenFactory::instance = nullptr;

void ScreenFactory::registerScreen(ScreenCreator* creator)
{
  screenCreators.push_back(creator);
}


Screen* ScreenFactory::createScreen(int ScreenID)
{
  if (ScreenID >= 0 && ScreenID < screenCreators.size())
  {
    return screenCreators[ScreenID]->create();
  }
  else
  {
    return (Screen*)nullptr;
  }
}



ScreenFactory* ScreenFactory::getInstance()
{
  if (!instance)
  {
    instance = new ScreenFactory;
  }
  return instance;
}

int ScreenFactory::getScreenCount()
{
  return screenCreators.size();
}




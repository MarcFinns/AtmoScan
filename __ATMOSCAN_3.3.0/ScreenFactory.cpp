/********************************************************/
/*                    ATMOSCAN                          */
/*                                                      */
/*            Author: Marc Finns 2017                   */
/*                                                      */
/********************************************************/


#include "ScreenFactory.h"


// have the creator's constructor do the registration
ScreenCreator::ScreenCreator()
{
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

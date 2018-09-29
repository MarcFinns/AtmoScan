/*
  StringTokenizer.h - Library for spliting strings into tokens using delimiters.
  Created by Ujjwal Syal, February 22, 2016.
  Released into the public domain.
*/
#ifndef StringTokenizer_h
#define StringTokenizer_h

#include "Arduino.h"

class StringTokenizer
{
  public:
    StringTokenizer(String str, String del);
    boolean hasNext();
    String nextToken();
  private:
    String _str;
	String _del;
	int ptr;
};

#endif

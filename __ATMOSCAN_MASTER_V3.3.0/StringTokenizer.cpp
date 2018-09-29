/*
  StringTokenizer.h - Library for spliting strings into tokens using delimiters.
  Created by Ujjwal Syal, February 22, 2016.
  Released into the public domain.
*/

#include "Arduino.h"
#include "StringTokenizer.h"

StringTokenizer::StringTokenizer(String str, String del)
{
  _str = str;
  _del = del;
  ptr = 0;
}

boolean StringTokenizer::hasNext()
{
  if(ptr < _str.length()){
	return true;
  }else{
	return false;
  }
}

String StringTokenizer::nextToken()
{
	if(ptr >= _str.length()){
		ptr = _str.length();
		return "";
	}
	
	String result = "";
	int delIndex = _str.indexOf(_del, ptr);
	
	if(delIndex == -1){
	  result = _str.substring(ptr);
	  ptr = _str.length();
	  return result;
	}else{
		result = _str.substring(ptr, delIndex);
		ptr = delIndex + _del.length();
		return result;
	}
}

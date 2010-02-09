/* 
 * strhacks.h
 * by WN @ Dec. 13; 2009
 */

#ifndef __STRHACKS_H
#define __STRHACKS_H

#include <ctype.h>

inline static void
str_toupper(char * str)
{
	char * p = str;
	while (*p != '\0') {
		*p = islower(*p) ? toupper(*p) : *p;
		p++;
	}
}

inline static void
str_tolower(char * str)
{
	char * p = str;
	while (*p != '\0') {
		*p = isupper(*p) ? tolower(*p) : *p;
		p++;
	}
}

#endif

// vim:ts=4:sw=4

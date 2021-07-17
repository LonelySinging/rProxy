#ifndef __TYPES_H
#define __TYPES_H
#include <iostream>

#ifdef __linux

#else	// windows
void perror(const char* str) {
	printf("%s",str);
}

#endif

#endif
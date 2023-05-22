#ifndef __HEHEPIG_UTILS_H__
#define __HEHEPIG_UTILS_H__


#include <string>
#include <iostream>
#include <stdlib.h>

void logError(const char *str);

void logError(const char *str, std::pair<int, int> pos);

void logError(const char *pre, const char *str, std::pair<int, int> pos);


#endif
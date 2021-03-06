#ifndef _UTIL_H_
#define _UTIL_H_

#include <string>
#include <psputils.h>

void uRandomInit(int seed);

u32 uRandomUIntBetween(int min, int max);
float uRandomFloatBetween(float min, float max);
bool uRandomBool(float probability = 0.5);

#endif

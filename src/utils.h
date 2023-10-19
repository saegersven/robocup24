#pragma once

#include <stdlib.h>
#include <stdint.h>

#define PI 3.141592653f

#define FOR_MILLIS_VAR(ms, var) long long var = millis(); \
    while(millis() - var < ms)

#define FOR_MILLIS(ms) FOR_MILLIS_VAR(ms, start_time)

long long millis();

void delay(unsigned int milliseconds);
#pragma once

#include <stdlib.h>
#include <stdint.h>

// Constants
#define PI 3.141592653f

// Macros
#define FOR_MILLIS_VAR(ms, var) long long var = millis(); \
    while(millis() - var < ms)

#define FOR_MILLIS(ms) FOR_MILLIS_VAR(ms, start_time)

#define CLEAR(x) memset(&(x), 0, sizeof(x))

// Time functions
long long millis();
void delay(unsigned int milliseconds);

// Compares two floats (passed to qsort)
int float_comparison(const void *elem1, const void *elem2);
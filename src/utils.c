#include "utils.h"

#include <time.h>
#include <unistd.h>

long long milliseconds() {
    long            ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);

    s  = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }

    return ms;
}

long long microseconds() {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec)*1000000 + tv.tv_usec);
}

void delay(unsigned int milliseconds) {
    usleep(milliseconds * 1000);
}

int clamp(int value, int min, int max) {
    return value > min ? (value < max ? value : max) : min;
}

float clampf(float value, float min, float max) {
    return value > min ? (value < max ? value : max) : min;
}

int compare_float(const void *elem1, const void *elem2) {
    float f = *((float*)elem1);
    float s = *((float*)elem2);
    if(f > s) return 1;
    if(f < s) return -1;
    return 0;
}
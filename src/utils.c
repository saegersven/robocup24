#include "utils.h"

#include <sys/time.h>
#include <unistd.h>

long long millis() {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec)*1000+(tv.tv_usec/1000));
}

void delay(unsigned int milliseconds) {
    usleep(milliseconds * 1000);
}

int compare_float(const void *elem1, const void *elem2) {
    float f = *((float*)elem1);
    float s = *((float*)elem2);
    if(f > s) return 1;
    if(f < s) return -1;
    return 0;
}
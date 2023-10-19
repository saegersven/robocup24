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
#pragma once

#include <stdlib.h>

#include "vision.h"

/* Image data buffer */
struct image_data_buffer {
    void* start;
    size_t length;
};

/* Struct to pass to capture_loop */
struct image_size {
    int width;
    int height;
};

void start_capture(int width, int height);
void stop_capture();

Image grab_frame();

void *capture_loop(void *size);
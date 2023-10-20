#pragma once

#include <stdlib.h>

#include "vision.h"

// Image data buffer
struct image_data_buffer {
    void* start;
    size_t length;
};

// Struct to pass to capture loop
struct image_size {
    int width;
    int height;
};

void camera_start_capture(int width, int height);
void camera_stop_capture();

Image camera_grab_frame();

void *camera_capture_loop(void *size);
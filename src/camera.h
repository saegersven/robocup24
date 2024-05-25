/****************************************************************************************************************
 *                                                                                                              *
 * CAMERA                                                                                                       *
 *                                                                                                              *
 * Controls camera at /dev/video0 via V4L2 (+libv4l). Continuously captures frames in a separate thread.        *
 *                                                                                                              *
 * camera_start_capture(width, height) - Start capture thread. Requests resolution width x height.              *
 * camera_stop_capture()               - Stop capture. Closes camera and waits for capture thread to stop.      *
 * camera_grab_frame()                 - Copy frame from camera buffer to memory. Returns captured image.       *
 *                                                                                                              *
 ****************************************************************************************************************/

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

void camera_grab_frame(uint8_t *frame, uint32_t width, uint32_t height);

void *camera_capture_loop(void *size);

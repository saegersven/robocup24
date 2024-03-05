#pragma once

#include "line_private.h"

#define LINE_RED_MIN_PERCENTAGE 0.23f

void line_red() {
    num_red_pixels = 0;
    image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(red), LINE_IMAGE_TO_PARAMS(frame), &num_red_pixels, is_red);

    float red_percentage = (float)num_red_pixels / (LINE_FRAME_WIDTH * LINE_FRAME_HEIGHT);

    if(red_percentage > LINE_RED_MIN_PERCENTAGE) {
        printf("Red percentage %f\n", red_percentage);
        robot_stop();
        delay(8000);
    }
}
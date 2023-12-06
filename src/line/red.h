#pragma once

#include "line_private.h"

#define LINE_RED_MIN_PERCENTAGE 0.27f

void line_red() {
    uint32_t num_red_pixels = 0;
    Image red = image_threshold(frame, &num_red_pixels, is_red);
    free_image(red);

    float red_percentage = (float)num_red_pixels / (LINE_FRAME_WIDTH * LINE_FRAME_HEIGHT);

    if(red_percentage > LINE_RED_MIN_PERCENTAGE) {
        robot_stop();
        delay(8000);
    }
}
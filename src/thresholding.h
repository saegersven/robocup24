#pragma once

#include <stdint.h>

#include "vision.h"

#define BLACK_MAX_SUM 200

#define GREEN_RATIO_THRESHOLD 0.7f
#define GREEN_MIN_VALUE 14

#define RED_RATIO_THRESHOLD 1.3f
#define RED_MIN_VALUE 20

int is_black(uint8_t b, uint8_t g, uint8_t r) {
    return (uint16_t)b + (uint16_t)g + (uint16_t)r < BLACK_MAX_SUM;
}

int is_green(uint8_t b, uint8_t g, uint8_t r) {
    return 1.0f / GREEN_RATIO_THRESHOLD * g > b + r && g > GREEN_MIN_VALUE;
}

int is_red(uint8_t b, uint8_t g, uint8_t r) {
    return 1.0f / RED_RATIO_THRESHOLD * r > g + b && r > RED_MIN_VALUE;
}

Image image_threshold(Image in, uint32_t *num_pixels, int(*threshold_fun)(uint8_t, uint8_t, uint8_t)) {
    Image out;
    out.channels = 1;
    out.height = in.height;
    out.width = in.width;
    alloc_image(&out);

    for(int i = 0; i < in.height; i++) {
        for(int j = 0; j < in.width; j++) {
            int idx = i * in.width + j;
            out.data[idx] = threshold_fun(in.data[3*idx], in.data[3*idx+1], in.data[3*idx+2]) ? 255 : 0;

            if(out.data[idx] && num_pixels != NULL) {
                ++(*num_pixels);
            }
        }
    }

    return out;
}
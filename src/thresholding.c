#pragma once
#include "thresholding.h"

#include <stdint.h>
#include <stdio.h>

#include "vision.h"

int is_black(uint8_t b, uint8_t g, uint8_t r) {
    return (uint16_t)b + (uint16_t)g + (uint16_t)r < BLACK_MAX_SUM;
}

int is_green(uint8_t b, uint8_t g, uint8_t r) {
    return 1.0f / GREEN_RATIO_THRESHOLD * g > b + r && g > GREEN_MIN_VALUE;
}

int is_red(uint8_t b, uint8_t g, uint8_t r) {
    return 1.0f / RED_RATIO_THRESHOLD * r > g + b && r > RED_MIN_VALUE;
}

void image_threshold(S_IMAGE(out), S_IMAGE(in), uint32_t *num_pixels, int(*threshold_fun)(uint8_t, uint8_t, uint8_t)) {
    if(out_c != 1 || out_h != in_h || out_w != in_w) {
        fprintf(stderr, "image_threshold: mismatched sizes");
        return;
    }

    for(int i = 0; i < in_h; i++) {
        for(int j = 0; j < in_w; j++) {
            int idx = i * in_w + j;
            out_d[idx] = threshold_fun(in_d[3*idx], in_d[3*idx+1], in_d[3*idx+2]) ? 255 : 0;

            if(out_d[idx] && num_pixels != NULL) {
                ++(*num_pixels);
            }
        }
    }
}
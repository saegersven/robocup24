#pragma once

#include <stdint.h>
#include <stdio.h>

#include "vision.h"

#define BLACK_MAX_SUM 200

#define GREEN_RATIO_THRESHOLD 0.56f
#define GREEN_MIN_VALUE 13

#define RED_RATIO_THRESHOLD 1.3f
#define RED_MIN_VALUE 20

int is_black(uint8_t b, uint8_t g, uint8_t r);

int is_green(uint8_t b, uint8_t g, uint8_t r);

int is_red(uint8_t b, uint8_t g, uint8_t r);

void image_threshold(S_IMAGE(out), S_IMAGE(in), uint32_t *num_pixels, int(*threshold_fun)(uint8_t, uint8_t, uint8_t));
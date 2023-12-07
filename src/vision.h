#pragma once

#include <stdint.h>

#define MAX_IMAGE_WIDTH 320
#define MAX_IMAGE_HEIGHT 320

#define S_IMAGE(identifier) uint8_t *identifier##_d, uint32_t identifier##_w, uint32_t identifier##_h, uint32_t identifier##_c
#define DECLARE_S_IMAGE(identifier, width, height, channels) uint8_t identifier##[width * height * channels]

#define S_IMAGE_PARAMS(identifier, width, height, channels) identifier, width, height, channels

#define LINE_IMAGE_TO_PARAMS(identifier) S_IMAGE_PARAMS(identifier, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 3)
#define LINE_IMAGE_TO_PARAMS_GRAY(identifier) S_IMAGE_PARAMS(identifier, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 1)

void copy_image(S_IMAGE(src), S_IMAGE(dest));

void write_image(const char* path, S_IMAGE(img));

void resize_image(S_IMAGE(src), S_IMAGE(dest), int new_width, int new_height);
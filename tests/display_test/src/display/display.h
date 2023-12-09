#pragma once

#include <stdint.h>

#define DISPLAY_ENABLE

#define NUMBER_FPS      0
#define NUMBER_ANGLE    1
#define NUMBER_ANGLE_D  2
#define NUMBER_SPEED_L  3
#define NUMBER_SPEED_R  4
#define NUMBER_SILVER_CONFIDENCE 5

#define NUM_NUMBERS     16

#define IMAGE_FRAME     0
#define IMAGE_BLACK     1
#define IMAGE_GREEN     2

#define NUM_IMAGES      16

#define MODE_IDLE           0
#define MODE_LINE_FOLLOW    1
#define MODE_LINE_OBSTACLE  2

#define LINE_FRAME_WIDTH 80
#define LINE_FRAME_HEIGHT 48

void display_create(int fullscreen);

int display_loop();

void display_set_number(int number_id, float value);
void display_set_image(int image_id, uint8_t *ptr);

void display_set_mode(int mode_id);

void display_destroy();
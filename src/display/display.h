#pragma once

#include <stdint.h>

#define DISPLAY_ENABLE

#define NUMBER_FPS      0
#define NUMBER_ANGLE    1
#define NUMBER_ANGLE_D  2
#define NUMBER_SPEED_L  3
#define NUMBER_SPEED_R  4
#define NUMBER_SILVER_CONFIDENCE 5
#define NUMBER_PERCENTAGE 6
#define NUMBER_RESCUE_OBJECTIVE 7
#define NUMBER_RESCUE_POS_X 8
#define NUMBER_RESCUE_POS_Y 9
#define NUMBER_RESCUE_NUM_VICTIMS 10
#define NUMBER_RESCUE_IS_DEAD 11
#define NUMBER_BAT_VOLTAGE 12

#define NUM_NUMBERS     16

#define IMAGE_FRAME     0
#define IMAGE_BLACK     1
#define IMAGE_GREEN     2
#define IMAGE_RESCUE_FRAME 3
#define IMAGE_RESCUE_THRESHOLD 4

#define NUM_IMAGES      16

#define MODE_IDLE           0
#define MODE_LINE_FOLLOW    1
#define MODE_LINE_OBSTACLE  2
#define MODE_RESCUE 3

#define RESCUE_OBJECTIVE_CORNER 0
#define RESCUE_OBJECTIVE_VICTIM 1
#define RESCUE_OBJECTIVE_EXIT   2

void display_create(int fullscreen);

int display_loop();

void display_set_number(int number_id, float value);
void display_set_image(int image_id, uint8_t *ptr);

void display_set_mode(int mode_id);

void display_destroy();
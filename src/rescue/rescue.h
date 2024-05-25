#pragma once

#define RESCUE_FRAME_WIDTH 320
#define RESCUE_FRAME_HEIGHT 240

#define RESCUE_CAPTURE_WIDTH 1280
#define RESCUE_CAPTURE_HEIGHT 960

#include <stdbool.h>


void rescue();
void rescue_cleanup();
float get_angle_to_right_wall();
void rescue_realign_wall();
bool rescue_is_corner();
void rescue_reposition();
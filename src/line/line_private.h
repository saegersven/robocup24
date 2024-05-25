#pragma once

#include <math.h>

#include "line.h"

#include "../vision.h"
#include "../camera.h"
#include "../thresholding.h"
#include "../utils.h"
#include "../robot.h"

// Global images as they are required by multiple functions
static DECLARE_S_IMAGE(frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 3);

static DECLARE_S_IMAGE(black, LINE_FRAME_HEIGHT, LINE_FRAME_WIDTH, 1);
static uint32_t num_black_pixels;

static DECLARE_S_IMAGE(green, LINE_FRAME_HEIGHT, LINE_FRAME_WIDTH, 1);
static uint32_t num_green_pixels;

static DECLARE_S_IMAGE(red, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 1);
static uint32_t num_red_pixels;

static int obstacle_enabled = 1;    // This is here because green must be able to disable obstacle
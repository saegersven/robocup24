#pragma once

#include <math.h>

#include "vision.h"
#include "camera.h"
#include "thresholding.h"
#include "utils.h"
#include "robot.h"

#define LINE_FRAME_WIDTH  80
#define LINE_FRAME_HEIGHT 48

// Global images as they are required by multiple functions
static Image frame;

static Image black;
static uint32_t num_black_pixels;

static Image green;
static uint32_t num_green_pixels;
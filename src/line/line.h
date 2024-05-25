#pragma once

#define LINE_FRAME_WIDTH  80
#define LINE_FRAME_HEIGHT 48

#define LINE_CAPTURE_WIDTH 320
#define LINE_CAPTURE_HEIGHT 192
//#define LINE_CAPTURE_MODE // turn off motors and save a frame every 200 ms

static int line_found_silver;

int line();
void line_stop();
void line_start();
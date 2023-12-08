#pragma once

#define LINE_FRAME_WIDTH  80
#define LINE_FRAME_HEIGHT 48

#define LINE_CAPTURE_WIDTH 320
#define LINE_CAPTURE_HEIGHT 192

static int line_found_silver;

void line();
void line_stop();
void line_start();
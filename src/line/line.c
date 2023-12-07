#include "line.h"

#include "line_private.h"

#include "follow.h"
#include "green.h"
#include "red.h"

void line_start() {
    line_found_silver = 0;

    camera_start_capture(LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);
}

void line_stop() {
    robot_stop();
    camera_stop_capture();
}

void line() {
    camera_grab_frame(frame);

    // Thresholding in here as some images are required by multiple functions
    num_black_pixels = 0;
    image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(black), LINE_IMAGE_TO_PARAMS(frame), &num_black_pixels, is_black);

    num_green_pixels = 0;
    image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(green), LINE_IMAGE_TO_PARAMS(frame), &num_green_pixels, is_green);

    line_follow();
    line_green();
}
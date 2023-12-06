#include "line.h"

#include "line_private.h"

#include "follow.h"
#include "green.h"
#include "red.h"

void line_start() {
    found_silver = 0;

    camera_start_capture(LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);
}

void line_stop() {
    robot_stop();
    camera_stop_capture();
}

void line() {
    frame = camera_grab_frame();

    // Thresholding in here as some images are required by multiple functions
    num_black_pixels = 0;
    black = image_threshold(frame, &num_black_pixels, is_black);

    num_green_pixels = 0;
    green = image_threshold(frame, &num_green_pixels, is_green);

    line_follow();
    line_green();

    free_image(black);
    free_image(green);

    free_image(frame);
}
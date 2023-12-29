#include "line.h"

#include "line_private.h"

#include "follow.h"
#include "green.h"
#include "red.h"
#include "silver.h"
#include "../vision.h"
#include "../utils.h"
#include "../display/display.h"

static DECLARE_S_IMAGE(line_correction, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 3);

void line_start() {
    line_found_silver = 0;

    line_create_maps();

    camera_start_capture(LINE_CAPTURE_WIDTH, LINE_CAPTURE_HEIGHT);

    silver_init();

    read_raw_image("/home/pi/robocup24/runtime_data/line_correction.bin", line_correction);

    robot_servo(SERVO_CAM, CAM_POS_DOWN, false);

#ifdef DISPLAY_ENABLE
    display_set_mode(MODE_LINE_FOLLOW);
    display_set_image(IMAGE_FRAME, frame);
    display_set_image(IMAGE_BLACK, black);
    display_set_image(IMAGE_GREEN, green);
#endif
}

void line_stop() {
    robot_stop();
    silver_destroy();
    camera_stop_capture();
}

void line_correct_image() {
    for(int i = 0; i < LINE_FRAME_HEIGHT; i++) {
        if(frame[i] < )
    }
}

void line() {
    camera_grab_frame(frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);

    line_correct_image();

    // Thresholding in here as some images are required by multiple functions
    num_black_pixels = 0;
    image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(black), LINE_IMAGE_TO_PARAMS(frame), &num_black_pixels, is_black);

    num_green_pixels = 0;
    image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(green), LINE_IMAGE_TO_PARAMS(frame), &num_green_pixels, is_green);

    //write_image("black.png", LINE_IMAGE_TO_PARAMS_GRAY(black));

    line_follow();
    line_green();
    line_silver();
}
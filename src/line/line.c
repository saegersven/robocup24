#include "line.h"

#include "line_private.h"

#include "follow.h"
#include "green.h"
#include "red.h"
#include "obstacle.h"
#include "silver.h"
#include "unstuck.h"
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

    robot_servo(SERVO_CAM, CAM_POS_DOWN, false, false);
    robot_servo(SERVO_ARM, ARM_POS_UP, false, false);
    robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);

#ifdef DISPLAY_ENABLE
    display_set_mode(MODE_LINE_FOLLOW);
    display_set_image(IMAGE_FRAME, frame);
    display_set_image(IMAGE_BLACK, black);
    display_set_image(IMAGE_GREEN, red);
#endif
}

void line_stop() {
    robot_stop();
    silver_destroy();
    camera_stop_capture();
}

void line_correct_image() {
    for(int i = 0; i < LINE_FRAME_WIDTH * LINE_FRAME_HEIGHT * 3; i++) {
        if(frame[i] > line_correction[i]) {
            frame[i] -= line_correction[i];
        } else {
            frame[i] = 0;
        }
    }
}

void line_black_threshold() {
    num_black_pixels = 0;
    for(int i = 0; i < LINE_FRAME_HEIGHT; i++) {
        for(int j = 0; j < LINE_FRAME_WIDTH; j++) {
            int idx = i * LINE_FRAME_WIDTH + j;
            int s = 0;
            for(int k = 0; k < 3; k++) {
                if(frame[i] > line_correction[i]) {
                    s += frame[3*idx+k] - line_correction[3*idx+k];
                }
            }
            if(s < BLACK_MAX_SUM) {
                black[idx] = 0xFF;
                ++num_black_pixels;
            } else {
                black[idx] = 0;
            }
        }
    }
}

int line() {
    camera_grab_frame(frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);

    // Thresholding in here as some images are required by multiple functions
    line_black_threshold();

    num_green_pixels = 0;
    image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(green), LINE_IMAGE_TO_PARAMS(frame), &num_green_pixels, is_green);

    //write_image("black.png", LINE_IMAGE_TO_PARAMS_GRAY(black));
    int ret = 0;

#ifndef LINE_CAPTURE_MODE
    line_unstuck();
    line_follow();
    line_green(0);
    line_obstacle();
    line_red();

    ret = line_silver();
#else
    char filename[64];
    printf("%lld\n", milliseconds());
    sprintf(filename, "/home/pi/capture/%lld.png", milliseconds());
    write_image(filename, LINE_IMAGE_TO_PARAMS(frame));
    delay(84);
#endif

    memcpy(last_frame, frame, LINE_FRAME_WIDTH * LINE_FRAME_HEIGHT * 3);

    return ret;
}
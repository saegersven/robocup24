#pragma once

#include "line_private.h"

#include "../vision.h"

void line_unstuck() {
    float diff = average_difference(frame, last_frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 3);
    //printf("No diff counter: %d\n", no_difference_counter);

    if(diff < 6.2f) {
        ++no_difference_counter;
        if(no_difference_counter >= 200) {
            if(num_black_pixels > 200) {
                printf("No difference, ruckling\n");
                robot_drive(100, 100, 150);
                delay(30);
                if (rand() % 2 == 0) {
                    robot_drive(100, 50, 0);
                    delay(100);
                } else {
                    robot_drive(50, 100, 0);
                    delay(100);
                }
                robot_drive(-20, -20, 200);
                robot_drive(100, 100, 0);

                no_difference_time_stamp = milliseconds();
                no_difference_counter = 100;
            } else {
                printf("Few pixels, no ruckling\n");
                no_difference_counter = 0;
            }
        }
    } else {
        // Slowly decrease counter by 30 at a time
        no_difference_counter = no_difference_counter >= 30 ? no_difference_counter - 30 : 0;
    }
}
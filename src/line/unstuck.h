#pragma once

#include "line_private.h"

#include "../vision.h"

void line_unstuck() {
    float diff = average_difference(frame, last_frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 3);
    printf("No diff counter: %d\n", no_difference_counter);

    if(diff < 4.2f) {
        ++no_difference_counter;
        if(no_difference_counter >= 200) {
            if(num_black_pixels > 200) {
                printf("No difference, ruckling\n");
                
                robot_drive(100, 100, 160);
                delay(30);
                robot_drive(-50, -50, 120);
                robot_drive(-33, -33, 100);

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
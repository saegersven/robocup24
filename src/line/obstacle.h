#pragma once

#include "line_private.h"

#define LINE_RED_MIN_PERCENTAGE 0.27f

static int obstacle_counter = 0;

void line_black_threshold();

#define OBSTACLE_ROI_XMIN 0
#define OBSTACLE_ROI_XMAX 50
#define OBSTACLE_ROI_YMIN 0
#define OBSTACLE_ROI_YMAX 48

int line_obstacle_drive_time_or_line(int time_ms) {
    long long start_t = milliseconds();
    while(milliseconds() - start_t < time_ms) {
    //while(1) {
        robot_drive(100, 32, 0);

        camera_grab_frame(frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);
        line_black_threshold();

        int num = 0;
        for(int y = OBSTACLE_ROI_YMIN; y < OBSTACLE_ROI_YMAX; y++) {
            for(int x = OBSTACLE_ROI_XMIN; x < OBSTACLE_ROI_XMAX; x++) {
                int idx = y * LINE_FRAME_WIDTH + x;
                if(black[idx] != 0) num++;
            }
        }

        float percentage = (float)num / (float)(OBSTACLE_ROI_XMAX - OBSTACLE_ROI_XMIN) / (float)(OBSTACLE_ROI_YMAX - OBSTACLE_ROI_YMIN);
        display_set_number(NUMBER_PERCENTAGE, percentage);

        if(percentage > 0.2f) {
            robot_stop();
            return 0;
        }
    }
    robot_stop();
    return 1;
}

void line_obstacle_navigate() {
    display_set_mode(MODE_LINE_OBSTACLE);

    robot_drive(-80, -80, 100);
    robot_turn(DTOR(-90.0f));
    delay(500);
    robot_drive(60, 60, 200);
    delay(20);

    line_obstacle_drive_time_or_line(6000);

    delay(50);
    robot_turn(DTOR(-45.0f));
    robot_drive(80, 80, 200);
    robot_turn(DTOR(-30.0f));
    robot_drive(-80, -80, 200);
}

void line_obstacle() {
    if(obstacle_counter % 4 == 0 && obstacle_enabled) {
        int dist = robot_sensor(DIST_FRONT);

        if(dist < 160) {
            for(int i = 0; i < 3; i++) {
                dist = robot_sensor(DIST_FRONT);

                printf("%d\n", dist);
                if(dist > 160) break;

                if(i == 2) {
                    line_obstacle_navigate();
                    return;
                }
            }   
        }
    }
    ++obstacle_counter;
}
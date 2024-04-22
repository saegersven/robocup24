#pragma once

#include "line_private.h"

#define LINE_RED_MIN_PERCENTAGE 0.27f

static int obstacle_counter = 0;

void line_black_threshold();

#define OBSTACLE_ROI_YMIN 0
#define OBSTACLE_ROI_YMAX 48
#define DESIRED_OBSTACLE_DIST 90

int line_obstacle_drive_time_or_line(int time_ms, int left) {
    int OBSTACLE_ROI_XMIN = 0;
    int OBSTACLE_ROI_XMAX = 50;

    if(!left) {
        OBSTACLE_ROI_XMIN = 30;
        OBSTACLE_ROI_XMAX = 79;
    }

    long long start_t = milliseconds();
    robot_drive(left ? 100 : 34, left ? 34 : 100, 0);
    delay(400);
    while(milliseconds() - start_t < time_ms) {
        robot_drive(left ? 100 : 34, left ? 34 : 100, 0);

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
    int left = milliseconds() % 2 == 0;

    display_set_mode(MODE_LINE_OBSTACLE);
    robot_stop();

    int dist_delta = 0;
    long long start_time = milliseconds;

    for(int i = 0; i < 2; i++) {
        int dist = robot_sensor(DIST_FRONT);
        robot_turn(DTOR(left ? 10.0f : -10.0f));
        delay(300);
        int dist_right = robot_sensor(DIST_FRONT);
        robot_turn(DTOR(left ? -20.0f : 20.0f));
        delay(300);
        int dist_left = robot_sensor(DIST_FRONT);

        delay(300);
        if(dist_right < dist && dist_right < dist_left) {
            robot_turn(DTOR(left ? 20.0f : -20.0f));
        } else if(dist < dist_right && dist < dist_left) {
            robot_turn(DTOR(left ? 10.0f : -10.0f));
        }
    }

    for(int i = 0; i < 3; i++) {
        delay(50);
        dist_delta = robot_sensor(DIST_FRONT) - DESIRED_OBSTACLE_DIST;
        if(dist_delta > 100) {
            return;
        }
        printf("Delta: %d\n", dist_delta);
        robot_drive(50, 50, dist_delta * 4);
        delay(50);
    }

    robot_turn(left ? -R90 : R90);
    line_obstacle_drive_time_or_line(10000, left);

    robot_drive(-80, -80, 150);
    robot_turn(DTOR(left ? -45.0f : 45.0f));
    robot_drive(80, 80, 450);
    robot_turn(DTOR(left ? -25.0f : 25.0f));
    robot_drive(-80, -80, 300);
    display_set_mode(MODE_LINE_FOLLOW);
}

void line_obstacle() {
    if(obstacle_counter % 4 == 0 && obstacle_enabled) {
        int dist = robot_sensor(DIST_FRONT);

        if(dist < 160) {
            for(int i = 0; i < 3; i++) {
                dist = robot_sensor(DIST_FRONT);

                //printf("%d\n", dist);
                if(dist > 160) break;

                if(i == 1) robot_stop();

                if(i == 2) {
                    line_obstacle_navigate();
                    return;
                }
            }   
        }
    }
    ++obstacle_counter;
}
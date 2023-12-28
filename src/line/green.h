#pragma once

#include "line_private.h"

#define RESULT_LEFT     0x01
#define RESULT_RIGHT    0x02
#define RESULT_DEAD_END 0x03    // RIGHT | LEFT

const char* RESULT_STR[4] = {"", "LEFT", "RIGHT", "DEAD-END"};

#define LINE_MIN_NUM_GREEN_PIXELS 40
#define LINE_MIN_NUM_GROUP_PIXELS 40

#define MAX_NUM_GROUPS 8

#define CUT_WIDTH 30
#define CUT_HEIGHT 30

#define GROUP_Y_MIN 15
#define GROUP_Y_MAX 36

struct Group {
    uint32_t num_pixels;
    float center_x, center_y;
};

// Very basic and slow contour finding algorithm, but its mine
void line_add_to_group_center(int x_pos, int y_pos, struct Group *group) {
    int x_limit = LINE_FRAME_WIDTH - 1;
    int y_limit = LINE_FRAME_HEIGHT - 1;

    // Iterate over eight neighbouring pixels
    for(int y = -1; y <= 1; y++) {
        for(int x = -1; x <= 1; x++) {
            int y_idx = y_pos + y;
            int x_idx = x_pos + x;

            if(y == 0 && x == 0) continue; // Don't need to check center pixel

            int idx = y_idx * LINE_FRAME_WIDTH + x_idx;
            if(green[idx] == 0xFF) {
                green[idx] = 0x7F;
                group->center_x += (float)x_idx;
                group->center_y += (float)y_idx;
                group->num_pixels++;

                if(x_idx > 0 && x_idx < x_limit && y_idx > 0 && y_idx < y_limit)
                    line_add_to_group_center(x_idx, y_idx, group);
            }
        }
    }
}

void line_find_groups(struct Group *groups, uint32_t *num_groups) {
    for(int y = 0; y < LINE_FRAME_HEIGHT; y++) {
        for(int x = 0; x < LINE_FRAME_WIDTH; x++) {
            int idx = y * LINE_FRAME_WIDTH + x;
            if(green[idx] == 0xFF) {
                struct Group group;
                group.num_pixels = 0;
                group.center_x = 0.0f;
                group.center_y = 0.0f;
                line_add_to_group_center(x, y, &group);

                if(group.num_pixels > LINE_MIN_NUM_GROUP_PIXELS) {
                    // add_to_group_center is a recursive method that cannot know if it is
                    // the last to add to the group pixels, so we have to normalize
                    group.center_x /= group.num_pixels;
                    group.center_y /= group.num_pixels;
                    groups[*num_groups] = group;
                    *num_groups += 1;
                }
            }
        }
    }
}

uint8_t line_green_direction(float *global_average_x, float *global_average_y) {
    uint8_t result = 0;

    struct Group groups[MAX_NUM_GROUPS];
    uint32_t num_groups = 0;

    line_find_groups(groups, &num_groups);
    if(num_groups == 0) return 0;

    // Check if all groups y-coordinates are in a certain range to prevent premature evaluation
    // of a dead-end or late evaluation of green points behind a line, when the lower line is
    // already out of the frame
    for(int i = 0; i < num_groups; i++) {
        if(groups[i].center_y < GROUP_Y_MIN) return 0;
        if(groups[i].center_y > GROUP_Y_MAX) return 0;
    }

    *global_average_x = 0.0f;
    *global_average_y = 0.0f;
    uint32_t num_green_points = 0;

    for(int i = 0; i < num_groups; i++) {
        int x_start = groups[i].center_x - CUT_WIDTH / 2;
        int x_end = groups[i].center_x + CUT_WIDTH / 2;
        if(x_start < 0) x_start = 0;
        if(x_end > LINE_FRAME_WIDTH) x_end = LINE_FRAME_WIDTH;

        int y_start = groups[i].center_y - CUT_HEIGHT / 2;
        int y_end = groups[i].center_y + CUT_HEIGHT / 2;
        if(y_start < 0) y_start = 0;
        if(y_end > LINE_FRAME_HEIGHT) y_end = LINE_FRAME_HEIGHT;

        float average_x = 0.0f;
        float average_y = 0.0f;

        uint32_t num_pixels = 0;

        for(int y = y_start; y < y_end; y++) {
            for(int x = x_start; x < x_end; x++) {
                int idx = y * LINE_FRAME_WIDTH + x;

                if(black[idx] && !green[idx]) {
                    average_x += (float)x;
                    average_y += (float)y;
                    ++num_pixels;
                }
            }
        } 
        average_x /= num_pixels;
        average_y /= num_pixels;

        //printf("(%f, %f), (%f, %f)\n", average_x, average_y, groups[i].center_x, groups[i].center_y);

        if(average_y < groups[i].center_y) {
            // Green point below the line
            *global_average_x += average_x;
            *global_average_y += average_y;
            ++num_green_points;

            result |= average_x < groups[i].center_x ? RESULT_RIGHT : RESULT_LEFT;
        }
    }

    *global_average_x /= num_green_points;
    *global_average_y /= num_green_points;

    return result;
}

void line_green() {
    if(num_green_pixels > LINE_MIN_NUM_GREEN_PIXELS) {
        /*write_image("green.png", LINE_IMAGE_TO_PARAMS_GRAY(green));
        write_image("black.png", LINE_IMAGE_TO_PARAMS_GRAY(black));
        printf("Num green pixels: %d\n", num_green_pixels);*/

        float global_average_x, global_average_y;
        uint8_t green_result = line_green_direction(&global_average_x, &global_average_y);

        printf("GREEN RESULT: %d\n", green_result);

        if(green_result == 0) return;

        // TODO: disable obstacle

        robot_stop();
        delay(50); // TODO: delay needed?

        // Approach
        float dx = global_average_x - LINE_CENTER_X;
        float dy = global_average_y - LINE_CENTER_Y;
        float angle = atan2f(dy, dx) + PI / 2.0f;
        float distance = sqrtf(dx*dx + dy*dy);

        robot_turn(angle);
        delay(50);
        robot_drive(80, 80, DISTANCE_FACTOR * (distance - 50));

        robot_drive(60, 60, 200);

        if(green_result == RESULT_DEAD_END) {
            robot_turn(R180);
            robot_drive(80, 80, 100);
        } else if(green_result == RESULT_LEFT) {
            robot_turn(DTOR(-65.0f));
            delay(30);
        } else if(green_result == RESULT_RIGHT) {
            robot_turn(DTOR(65.0f));
            delay(30);
        }

        robot_drive(80, 80, 20);

        camera_grab_frame(frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);
        image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(black), LINE_IMAGE_TO_PARAMS(frame), &num_black_pixels, is_black);

        if(num_black_pixels < 200) {
            printf("Searching left and right\n");

            uint32_t num_black_pixels_left = 0;
            uint32_t num_black_pixels_right = 0;

            robot_turn(DTOR(40.0f));
            camera_grab_frame(frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);
            image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(black), LINE_IMAGE_TO_PARAMS(frame), &num_black_pixels_right, is_black);

            robot_turn(DTOR(-80.0f));
            camera_grab_frame(frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);
            image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(black), LINE_IMAGE_TO_PARAMS(frame), &num_black_pixels_left, is_black);

            printf("LEFT %d | %d RIGHT\n", num_black_pixels_left, num_black_pixels_right);

            if(num_black_pixels_right > num_black_pixels_left) {
                robot_turn(DTOR(85.0f));
            }
        }

        // TODO: Re-enable obstacle
    }
}
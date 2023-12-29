#pragma once

#include "line_private.h"

#include "../display/display.h"

#define LINE_FRAME_BOTTOM_CUT 7

#define LINE_FOLLOW_K_P 100.0f
#define LINE_FOLLOW_K_D 12.0f
#define LINE_FOLLOW_D_DT_MAX 2.5f
#define LINE_FOLLOW_BASE_SPEED 80

#define LINE_CENTER_X 40
#define LINE_CENTER_Y 48

#define LINE_MIN_NUM_ANGLES 80

float line_difference_weight(float x) {
    x = x / PI * 2.0f;
    float w = 0.25f + 0.75f * expf(-16.0f * x*x);
    return w > 0.0f ? (w < 1.0f ? w : 1.0f) : 0.0f;
}

float line_distance_weight(float x) {
    float e = (3.25f * x - 2.0f);
    float f = expf(-e*e) - 0.1f;
    return f > 0.0f ? f : 0.0f;
}

float line_center_weight(float x) {
    return expf(-x*x) / 2.0f + 0.5f;
}

static float distance_weight_map[LINE_FRAME_WIDTH * LINE_FRAME_HEIGHT];
static float pixel_angles_map[LINE_FRAME_WIDTH * LINE_FRAME_HEIGHT];

void line_create_maps() {
    for(int y = 0; y < LINE_FRAME_HEIGHT; y++) {
        for(int x = 0; x < LINE_FRAME_WIDTH; x++) {
            int idx = y * LINE_FRAME_WIDTH + x;

            float dx = x - LINE_CENTER_X;
            float dy = y - LINE_CENTER_Y;
            float dist = sqrtf(dx*dx + dy*dy);
            float angle = atan2f(dy, dx) + PI / 2.0f;

            if(y > LINE_FRAME_HEIGHT - LINE_FRAME_BOTTOM_CUT) {
                distance_weight_map[idx] = 0.0f;
            } else {
                distance_weight_map[idx] = line_distance_weight(dist / LINE_FRAME_HEIGHT);
            }
            distance_weight_map[idx] *= line_center_weight(angle);
            pixel_angles_map[idx] = angle;    
        }
    }
}

float line_get_line_angle(float last_line_angle) {
    float weighted_line_angle = 0.0f;
    float total_weight = 0.0f;

    uint32_t num_angles = 0;

    for(int y = 0; y < LINE_FRAME_HEIGHT; y++) {
        for(int x = 0; x < LINE_FRAME_WIDTH; x++) {
            int idx = y * LINE_FRAME_WIDTH + x;

            if(black[idx] != 0) {
                float distance_weight = distance_weight_map[idx];
                if(distance_weight > 0.0f) {
                    ++num_angles;

                    float angle = pixel_angles_map[idx];
                    float angle_difference_weight = line_difference_weight(angle - last_line_angle);
                    
                    float weight = angle_difference_weight * distance_weight;
                    weighted_line_angle += weight * angle;
                    total_weight += weight;
                }
            }
        }
    }

    //printf("Num angles: %d\n", num_angles);

    if(num_angles < LINE_MIN_NUM_ANGLES) return 0.0f;
    if(total_weight == 0.0f) return 0.0f;

    weighted_line_angle /= total_weight;

    return weighted_line_angle;
}

float line_get_gap_angle() {
    float x_avg = 0.0f;
    float y_avg = 0.0f;
    int num = 0;

    for(int i = 0; i < LINE_FRAME_HEIGHT; i++) {
        for(int j = 0; j < LINE_FRAME_WIDTH; j++) {
            int idx = i * LINE_FRAME_WIDTH + j;

            if(black[idx] != 0) {
                x_avg += j;
                y_avg += i;
                num++;
            }
        }
    }
    x_avg /= num;
    y_avg /= num;

    float total_angle = 0.0f;
    for(int i = 0; i < LINE_FRAME_HEIGHT; i++) {
        for(int j = 0; j < LINE_FRAME_WIDTH; j++) {
            int idx = i * LINE_FRAME_WIDTH + j;
            if(black[idx] != 0) {
                float dx = j - x_avg;
                float dy = i - y_avg;

                if(dy == 0.0f) continue;

                float angle = atanf(-dx / dy);

                total_angle += angle;
            }
        }
    }

    total_angle /= num;

    return total_angle;
}

static float last_line_angle = 0.0f;
static long long last_follow_time = 0; // can be left as-is on restart

void line_follow() {
    // TODO: Implement wiggle

    // GAPS
    /*if(num_black_pixels > 120) gap_enabled = 1;
    if(gap_enabled && num_black_pixels < 80) {
        printf("GAP");
        robot_stop();
        delay(2000);
        robot_drive(-70, -70, 250);

        // Capture new image
        camera_grab_frame(frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);
        num_black_pixels = 0;
        image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(black), LINE_IMAGE_TO_PARAMS(frame), &num_black_pixels, is_black);

        float gap_angle = line_get_gap_angle();

        robot_turn(gap_angle);

        // Disable gap until enough pixels are found again
        gap_enabled = 0;

        return;
    }*/

    // NORMAL LINE FOLLOWING

    float line_angle = line_get_line_angle(last_line_angle);

    long long time_now = microseconds();
    float dt = (time_now - last_follow_time) / 1e6f;
    float d_dt_line_angle = (line_angle - last_line_angle) / dt;

    d_dt_line_angle = clampf(d_dt_line_angle, -LINE_FOLLOW_D_DT_MAX, LINE_FOLLOW_D_DT_MAX);

    float u = LINE_FOLLOW_K_P * line_angle + LINE_FOLLOW_K_D * d_dt_line_angle;

    float u_left = u;
    if(u_left < 0) u_left *= 2.5;
    float u_right = -u;
    if(u_right < 0) u_right *= 2.5;

    int8_t m_left = clamp(LINE_FOLLOW_BASE_SPEED + u_left, -100, 100);
    int8_t m_right = clamp(LINE_FOLLOW_BASE_SPEED + u_right, -100, 100);

    robot_drive(m_left, m_right, 0);

    last_line_angle = line_angle;
    last_follow_time = time_now;

#ifdef DISPLAY_ENABLE
    display_set_number(NUMBER_ANGLE, line_angle);
    display_set_number(NUMBER_ANGLE_D, d_dt_line_angle);
    display_set_number(NUMBER_SPEED_L, (float)m_left);
    display_set_number(NUMBER_SPEED_R, (float)m_right);
    display_set_number(NUMBER_FPS, 1.0f / dt);
#endif
}
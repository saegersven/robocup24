#pragma once

#include "line_private.h"

#include "../display/display.h"

#define LINE_FOLLOW_K_P 57.0f
#define LINE_FOLLOW_K_D 6.0f
#define LINE_FOLLOW_D_DT_MAX 7.0f
#define LINE_FOLLOW_BASE_SPEED 80

#define LINE_CENTER_X 40
#define LINE_CENTER_Y 48

#define LINE_MIN_NUM_ANGLES 50

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

            distance_weight_map[idx] = line_distance_weight(dist / LINE_FRAME_HEIGHT);
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

static float last_line_angle = 0.0f;
static long long last_follow_time = 0; // can be left as-is on restart

void line_follow() {
    float line_angle = line_get_line_angle(last_line_angle);

    long long time_now = microseconds();
    float dt = (time_now - last_follow_time) / 1e6f;
    float d_dt_line_angle = (line_angle - last_line_angle) / dt;

    d_dt_line_angle = clampf(d_dt_line_angle, -LINE_FOLLOW_D_DT_MAX, LINE_FOLLOW_D_DT_MAX);

    float u = LINE_FOLLOW_K_P * line_angle + LINE_FOLLOW_K_D * d_dt_line_angle;

    int8_t m_left = clamp(LINE_FOLLOW_BASE_SPEED + u, -100, 100);
    int8_t m_right = clamp(LINE_FOLLOW_BASE_SPEED - u, -100, 100);

    //robot_drive(m_left, m_right, 0);

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
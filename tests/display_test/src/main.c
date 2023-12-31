#include <stdio.h>
#include <math.h>
#include <sys/time.h>

#include "display/display.h"

long long millis() {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec)*1000 + (tv.tv_usec/1000));
}

uint8_t frame[LINE_FRAME_WIDTH * LINE_FRAME_HEIGHT * 3];
uint8_t gray[LINE_FRAME_WIDTH * LINE_FRAME_HEIGHT];
uint8_t rescue_frame[RESCUE_FRAME_HEIGHT * RESCUE_FRAME_WIDTH * 3];

void update_images(float t) {
    for(int i = 0; i < LINE_FRAME_HEIGHT; i++) {
        for(int j = 0; j < LINE_FRAME_WIDTH; j++) {
            int idx = i * LINE_FRAME_WIDTH + j;

            for(int k = 0; k < 3; k++) {
                frame[3*idx + k] = (sinf(t + (float)i / 3) / 2 + 0.5f) * 255;
            }
            gray[idx] = (sinf(t + (float)i / 3) / 2 + 0.5f) * 255;
        }
    }

    for(int i = 0; i < RESCUE_FRAME_HEIGHT ; i++) {
        for(int j = 0; j < RESCUE_FRAME_WIDTH; j++) {
            int idx = i * RESCUE_FRAME_WIDTH + j;

            int d = (sinf(t + (float)i / 3) / 2 + 0.5f) * 255.0f;
            for(int k = 0; k < 3; k++) {
                rescue_frame[3 * idx + k] = d;
            }
        }
    }
}

int main() {
    display_create(0);

    display_set_mode(MODE_RESCUE);
    display_set_image(IMAGE_FRAME, frame);
    display_set_image(IMAGE_BLACK, gray);
    display_set_image(IMAGE_GREEN, gray);
    display_set_image(IMAGE_RESCUE_FRAME, rescue_frame);

    display_set_number(NUMBER_RESCUE_NUM_VICTIMS, 2);

    long long start_time = millis();

    while(1) {
        if(display_loop()) break;

        float t = (float)(millis() - start_time) / 500;

        display_set_number(NUMBER_FPS, t * 2);
        display_set_number(NUMBER_ANGLE, sinf(t));
        display_set_number(NUMBER_SILVER_CONFIDENCE, sinf(t / 10));

        update_images(t);

        display_set_number(NUMBER_RESCUE_IS_DEAD, (int)(sinf(t / 10) + 1.5f));
    }

    display_destroy();
}
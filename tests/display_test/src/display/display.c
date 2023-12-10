#include "display.h"

#include <stdio.h>

#include "sdl_interface.h"

//#include "../line/line.h"

static int mode;

static float numbers[NUM_NUMBERS];
static uint8_t *images[NUM_IMAGES];

void draw_mode_idle() {
    draw_text("IDLE", 220, 110, 1.0f, 1.0f, 1.0f);
    draw_text("PRESS BUTTON", 180, 160, 0.6f, 0.6f, 0.6f);
}

// Linearly maps val from [a, b] to [c, d]
float map(float val, float a, float b, float c, float d) {
    return c + (val - a) / (b - a) * (d - c);
}

void draw_mode_follow() {
    char buf[32];

    draw_text("FOLLOW", 30, 24, 1.0f, 1.0f, 1.0f);

    sprintf(buf, "FPS  %.1f", numbers[NUMBER_FPS]);
    float color_fps = map(numbers[NUMBER_FPS], 60.0f, 80.0f, 0.0f, 1.0f);
    if(color_fps < 0.0f) color_fps = 0.0f;
    if(color_fps > 1.0f) color_fps = 1.0f;
    draw_text(buf, 140, 24, 1.0f - color_fps, color_fps, 0.0f);

    sprintf(buf, "L  %.0f", numbers[NUMBER_SPEED_L]);
    draw_text(buf, 260, 24, 0.0f, 0.8f, 1.0f);
    sprintf(buf, "R  %.0f", numbers[NUMBER_SPEED_R]);
    draw_text(buf, 330, 24, 0.0f, 0.8f, 1.0f);

    sprintf(buf, "ANG  %.0f", (numbers[NUMBER_ANGLE] * 57));
    float color_angle = map(fabsf(numbers[NUMBER_ANGLE]), 0.0f, 0.6f, 0.0f, 1.0f);
    draw_text(buf, 30, 50, color_angle, 1.0f - color_angle, 0.0f);
    sprintf(buf, "D ANG  %.0f", (numbers[NUMBER_ANGLE_D] * 57));
    draw_text(buf, 140, 50, 0.0f, 0.8f, 1.0f);

    sprintf(buf, "SLV  %.1f", (numbers[NUMBER_SILVER_CONFIDENCE] * 100));
    draw_text(buf, 260, 50, 0.0f, 0.8f, 1.0f);

    draw_image(images[IMAGE_FRAME], LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 3, 30, 90, 300, 180, 0, 1.0f, 1.0f, 1.0f);
    draw_text("FRAME", 130, 272, 1.0f, 1.0f, 1.0f);
    
    draw_line(180.0f, 270.0f, 180.0f + 40.0f * sinf(numbers[NUMBER_ANGLE_D] / 7.0f), 270.0f - 40.0f * cosf(numbers[NUMBER_ANGLE_D] / 7.0f), 2.5f, 0.0f, 0.1f, 0.9f);
    draw_line(180.0f, 270.0f, 180.0f + 70.0f * sinf(numbers[NUMBER_ANGLE]), 270.0f - 70.0f * cosf(numbers[NUMBER_ANGLE]), 2.5f, 0.0f, 1.0f, 0.0f);

    draw_image(images[IMAGE_BLACK], LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 1, 331, 90, 130, 78, 0, 1.0f, 1.0f, 1.0f);
    draw_text("THR BLK", 350, 168, 1.0f, 1.0f, 1.0f);
    draw_image(images[IMAGE_GREEN], LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 1, 331, 190, 130, 78, 0, 1.0f, 1.0f, 1.0f);
    draw_text("THR GRN", 350, 270, 1.0f, 1.0f, 1.0f);
}

void draw_mode_obstacle() {
    char buf[32];

    draw_text("OBSTACLE", 30, 24, 1.0f, 1.0f, 1.0f);

    sprintf(buf, "BLACK PERCENT  %.0f", (numbers[NUMBER_PERCENTAGE] * 100.0f));
    draw_text(buf, 240, 24, 0.0f, 0.8f, 1.0f);

    draw_image(images[IMAGE_BLACK], LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 1, 8, 70, 340, 204, 0, 1.0f, 1.0f, 1.0f);
    draw_image(images[IMAGE_FRAME], LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 3, 350, 202, 120, 72, 0, 1.0f, 1.0f, 1.0f);
}

void draw_mode_rescue() {
    draw_text("EVAC ZONE", 30, 24, 1.0f, 1.0f, 1.0f);

    char buf[32];
    switch((int)numbers[NUMBER_RESCUE_OBJECTIVE]) {
        case RESCUE_OBJECTIVE_CORNER:
            sprintf(buf, "OBJ  CORNER");
            break;
        case RESCUE_OBJECTIVE_VICTIM:
            sprintf(buf, "OBJ  VICTIM");
            break;
        case RESCUE_OBJECTIVE_EXIT:
            sprintf(buf, "OBJ  EXIT");
            break;
    }
    draw_text(buf, 220, 24, 1.0f, 1.0f, 1.0f);

    sprintf(buf, "X  %.1f", numbers[NUMBER_RESCUE_POS_X]);
    draw_text(buf, 360, 60, 0.0f, 0.8f, 1.0f);
    sprintf(buf, "Y  %.1f", numbers[NUMBER_RESCUE_POS_Y]);
    draw_text(buf, 360, 90, 0.0f, 0.8f, 1.0f);

    int is_dead = numbers[NUMBER_RESCUE_IS_DEAD];
    if(is_dead == 0.0f) {
        sprintf(buf, "");
    } else if(is_dead == 1.0f) {
        sprintf(buf, "LIVING");
    } else if(is_dead == 2.0f) {
        sprintf(buf, "DEAD");
    }
    draw_text(buf, 360, 120, 0.0f, 0.8f, 1.0f);

    sprintf(buf, "NUM  %.0f", numbers[NUMBER_RESCUE_NUM_VICTIMS]);
    draw_text(buf, 360, 170, 0.0f, 0.8f, 1.0f);

    if(numbers[NUMBER_RESCUE_NUM_VICTIMS] >= 2) {
        draw_text("DEAD EN", 360, 200, 1.0f, 0.8f, 0.2f);
    }

    draw_image(images[IMAGE_RESCUE_FRAME], RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3, 20, 60, 320, 240, 0, 1.0f, 1.0f, 1.0f);
}

int display_loop() {
    sdl_clear_buffer();
    
    switch(mode) {
    case MODE_IDLE:
        draw_mode_idle();
        break;
    case MODE_LINE_FOLLOW:
        draw_mode_follow();
        break;
    case MODE_LINE_OBSTACLE:
        draw_mode_obstacle();
        break;
    case MODE_RESCUE:
        draw_mode_rescue();
        break;
    }

    sdl_render();
    return sdl_event_loop();
}

void display_create(int fullscreen) {
    memset(numbers, 0, NUM_NUMBERS * sizeof(float));
    memset(images, 0, NUM_IMAGES * sizeof(uint8_t*));
    mode = MODE_IDLE;

    read_font();
    sdl_create_window(fullscreen);
}

void display_set_mode(int new_mode) {
    mode = new_mode;
}

void display_set_number(int number_id, float value) {
    if(number_id < 0 || number_id >= NUM_NUMBERS) {
        fprintf(stderr, "display_set_number: Invalid number id %d\n", number_id);
        return;
    }

    numbers[number_id] = value;
}

void display_set_image(int image_id, uint8_t *ptr) {
    if(image_id < 0 || image_id >= NUM_IMAGES) {
        fprintf(stderr, "display_set_image: Invalid image id %d\n", image_id);
        return;
    }

    images[image_id] = ptr;
}

void display_destroy() {
    sdl_destroy_window();
}

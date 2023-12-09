#include "display.h"

#include <stdio.h>

//#include "../line/line.h"

static int mode;

static float numbers[NUM_NUMBERS];
static uint8_t *images[NUM_IMAGES];

void draw_mode_idle() {
    draw_text("IDLE", 220, 110);
    draw_text("PRESS BUTTON", 180, 160);
}

void draw_mode_follow() {
    char buf[32];

    draw_text("FOLLOW", 30, 24);

    sprintf(buf, "FPS  %.1f", numbers[NUMBER_FPS]);
    draw_text(buf, 160, 24);

    sprintf(buf, "L  %.0f", numbers[NUMBER_SPEED_L]);
    draw_text(buf, 350, 24);
    sprintf(buf, "R  %.0f", numbers[NUMBER_SPEED_R]);
    draw_text(buf, 350, 50);

    sprintf(buf, "ANG  %.0f", RTOD(numbers[NUMBER_ANGLE]));
    draw_text(buf, 30, 50);
    
    sprintf(buf, "D ANG  %.0f", RTOD(numbers[NUMBER_ANGLE_D]));
    draw_text(buf, 160, 50);

    //draw_image(images[IMAGE_FRAME], LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 3, 30, 80, 300, 180);
    draw_text("FRAME", 130, 272);

    //draw_image(images[IMAGE_BLACK], LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 1, 331, 80, 130, 78);
    draw_text("THR BLK", 350, 163);
    //draw_image(images[IMAGE_GREEN], LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT, 1, 331, 180, 130, 78);
    draw_text("THR GRN", 350, 270);
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
        //display_mode_obstacle();
        break;
    }

    sdl_render();
    return sdl_event_loop();
}

void display_create() {
    memset(numbers, 0, NUM_NUMBERS * sizeof(float));
    memset(images, 0, NUM_IMAGES * sizeof(uint8_t*));
    mode = MODE_IDLE;

    read_font();
    sdl_create_window();
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
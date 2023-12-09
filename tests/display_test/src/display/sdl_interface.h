#pragma once

#include <string.h>
#include <math.h>

#include "../libs/SDL2-2.28.5/include/SDL.h"

#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 320

static uint8_t buffer[WINDOW_WIDTH * WINDOW_HEIGHT * 3];

static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static int pitch;

void sdl_create_window(int fullscreen) {
    texture = NULL;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);
    SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    SDL_SetWindowTitle(window, "Debug");

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);
}

void sdl_destroy_window() {
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

void sdl_render() {
    void* pixels;

    SDL_LockTexture(texture, NULL, &pixels, &pitch);

    for(int x = 0; x < WINDOW_WIDTH; x++) {
        for(int y = 0; y < WINDOW_HEIGHT; y++) {
            uint8_t *base = ((uint8_t*)pixels) + (4 * (y * WINDOW_WIDTH + x));
            int buffer_idx = y * WINDOW_WIDTH + x;
            for(int k = 0; k < 3; k++) {
                base[k] = buffer[3 * buffer_idx + k];
            }
            base[3] = 255;
        }
    }

    SDL_UnlockTexture(texture);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

int sdl_event_loop() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == 'q') {
                    return 1;
                }
                break;
            case SDL_QUIT:
                return 1;
        }
    }
}

void sdl_clear_buffer() {
    memset(buffer, 0, WINDOW_WIDTH * WINDOW_HEIGHT * 3);
}

void draw_image(uint8_t *image, int image_w, int image_h, int image_c, int x_pos, int y_pos, int w, int h, int is_text, float b, float g, float r) {
    float color_factors[3] = {b, g, r};
    for(int x = x_pos; x < x_pos + w; x++) {
        for(int y = y_pos; y < y_pos + h; y++) {
            int buf_idx = y * WINDOW_WIDTH + x;
            float factor_x = (float)image_w / w;
            float factor_y = (float)image_h / h;
            int img_idx = factor_y * (y - y_pos) * image_w + factor_x * (x - x_pos);

            if(image_c == 3) {
                for(int k = 0; k < 3; k++) {
                    if(image[3*img_idx + k] == 0 && is_text) continue;
                    buffer[3*buf_idx + k] = image[3*img_idx + k] * color_factors[k];
                }
            } else {
                for(int k = 0; k < 3; k++) {
                    if(image[img_idx] == 0 && is_text) continue;
                    buffer[3*buf_idx + k] = image[img_idx] * color_factors[k];
                }
            }
        }
    }
}

#define MAX_GLYPH_WIDTH 18
#define MAX_GLYPH_HEIGHT 21
#define NUM_GLYPHS 45

#define FNT_FILE_PATH "../runtime_data/font.bin"
#define IMG_FILE_PATH "../runtime_data/font_0.bin"

typedef struct Glyph {
    uint8_t pixels[MAX_GLYPH_WIDTH * MAX_GLYPH_HEIGHT];
    uint8_t width, height;
    uint8_t x_advance;
} Glyph;

struct Glyph_Data {
    uint8_t id, x, y, width, height, xadvance;
};

static Glyph* unicode_table[91];

static Glyph glyphs[NUM_GLYPHS];

void read_font() {
    FILE* fnt_file = fopen(FNT_FILE_PATH, "r");
    FILE* img_file = fopen(IMG_FILE_PATH, "r");

    uint32_t img_width, img_height;
    fread(&img_width, sizeof(uint32_t), 1, img_file);
    fread(&img_height, sizeof(uint32_t), 1, img_file);

    uint8_t *img = (uint8_t*)malloc(img_width * img_height * 3);
    fread(img, sizeof(uint8_t), img_width * img_height * 3, img_file);

    uint8_t num_glyphs;
    fread(&num_glyphs, 1, 1, fnt_file);

    for(int i = 0; i < num_glyphs; i++) {
        struct Glyph_Data d;
        fread(&d, sizeof(struct Glyph_Data), 1, fnt_file);


        unicode_table[d.id] = &glyphs[i];

        glyphs[i].width = d.width;
        glyphs[i].height = d.height;
        glyphs[i].x_advance = d.xadvance;

        int img_start_idx = 3 * (d.y * d.width + d.x);
        for(int x = d.x; x < (d.x + d.width); x++) {
            for(int y = d.y; y < (d.y + d.height); y++) {
                int img_idx = y * img_width + x;
                int glyph_idx = (y - d.y) * MAX_GLYPH_WIDTH + (x - d.x);

                glyphs[i].pixels[glyph_idx] = img[3 * img_idx];
            }
        }
    }

    free(img);
}

void draw_text(const char *text, int x, int y, float b, float g, float r) {
    if(b > 1.0f) b = 1.0f;
    if(g > 1.0f) g = 1.0f;
    if(r > 1.0f) r = 1.0f;
    if(b < 0.0f) b = 0.0f;
    if(g < 0.0f) g = 0.0f;
    if(r < 0.0f) r = 0.0f;
    for(int i = 0; i < strlen(text); i++) {
        Glyph *gl = unicode_table[text[i]];
        draw_image(gl->pixels, MAX_GLYPH_WIDTH, MAX_GLYPH_HEIGHT, 1, x, y, MAX_GLYPH_WIDTH, MAX_GLYPH_HEIGHT, 1, r, g, b);
        x += gl->x_advance;
    }
}

void intensify_pixels(float x, float y, float b, float g, float r) {
    int buf_idx = y * WINDOW_WIDTH + x;
    buffer[3 * buf_idx] = b * 255;
    buffer[3 * buf_idx + 1] = g * 255;
    buffer[3 * buf_idx + 2] = r * 255;
}

void draw_line(float x1, float y1, float x2, float y2, float b, float g, float r) {
    int steep = 0;
    if(fabsf(x1 - x2) < fabsf(y1 - y2)) {
        float tmp = x1;
        x1 = y1;
        y1 = tmp;
        tmp = x2;
        x2 = y2;
        y2 = tmp;
        steep = 1;
    }

    if(x1 > x2) {
        float tmp = x1;
        x1 = x2;
        x2 = tmp;
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }

    float dx = x2 - x1;
    float dy = y2 - y1;
    float d_err = fabsf(dy)*2;
    float err = 0;
    float y = y1;

    for(int x = x1; x <= x2; x += 1.0f) {
        if(steep) {
            intensify_pixels(y, x, b, g, r);
        } else {
            intensify_pixels(x, y, b, g, r);
        }
        err += d_err;
        if(err > dx) {
            y += (y2 > y1 ? 1 : -1);
            err -= dx*2;
        }
    }
}
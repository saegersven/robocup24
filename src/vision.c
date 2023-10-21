/*
* Image data containers, image manipulation and image display using SDL.
*/

#include "vision.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "../libs/SDL2-2.28.4/include/SDL.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb_image_write.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

void alloc_image(Image* image) {
    image->data = malloc(image->width*image->height*image->channels);
}

void clear_image(Image* image) {
    memset(image->data, 0, image->width*image->height*image->channels);
}

void copy_image(Image src, Image *dest) {
    if(src.width != dest->width || src.height != dest->height || src.channels != dest->channels) {
        dest->width = src.width;
        dest->height = src.height;
        dest->channels = src.channels;

        if(dest->data != NULL) {
            dest->data = realloc(dest->data, dest->width * dest->height * dest->channels);
            if(dest->data == NULL) {
                fprintf(stderr, "Error while resizing image for copy\n");
            }
        }
    }

    if(dest->data == NULL) {
        alloc_image(&dest);
    }

    memcpy(dest->data, src.data, src.width * src.height * src.channels);
}

void free_image(Image image) {
    free(image.data);
}

// Appears to be very slow
void write_image(const char* path, Image image) {
    stbi_write_png(path, image.width, image.height, image.channels, image.data, image.width*image.channels);
}

void to_grayscale(Image src, Image *dest) {
    for(int i = 0; i < src.width; i++) {
        for(int j = 0; j < src.height; j++) {
            dest->data[j * dest->width + i] = 0;

            for(int k = 0; k < src.channels; k++) {
                dest->data[j * dest->width + i] += src.data[src.channels * (j * src.width + i) + k] / src.channels;
            }
        }
    }
}

void resize_image(Image src, Image *dest, int scale) {
    if(scale == -1 || scale == 0 || scale == 1) {
        dest->data = NULL;
        copy_image(src, dest);
        return;
    }

    int shrinking = scale < 0;

    if(shrinking) {
        scale = -scale;
        dest->width = src.width / scale;
        dest->height = src.height / scale;
    } else {
        dest->width = src.width * scale;
        dest->height = src.height * scale;
    }
    dest->channels = src.channels;

    alloc_image(dest);

    if(shrinking) {
        for(int y = 0; y < dest->height; y++) {
        for(int x = 0; x < dest->width; x++) {
        for(int k = 0; k < src.channels; k++) {
            int avg = 0;
            for(int i = 0; i < scale; i++) {
                for(int j = 0; j < scale; j++) {
                    avg += src.data[src.channels * ((y * scale + i) * src.width + (x * scale + j)) + k];
                }
            }
            avg /= scale * scale;
            dest->data[dest->channels * (y * dest->width + x) + k] = avg;
        }
        }
        }
    } else {
        for(int y = 0; y < src.height; y++) {
        for(int x = 0; x < src.width; x++) {
        for(int k = 0; k < src.channels; k++) {
            for(int i = 0; i < scale; i++) {
                for(int j = 0; j < scale; j++) {
                    dest->data[dest->channels * ((y * scale + i) * dest->width + (x * scale + j)) + k] =
                        src.data[src.channels * (y * src.width + x) + k];
                }
            }
        }
        }
        }
    }
}

void crop_image(Image src, Image *dest, int x, int y, int w, int h) {
    if(x + w > src.width || y + h > src.height) {
        fprintf(stderr, "Crop: Invalid arguments\n");
    }

    dest->channels = src.channels;
    dest->width = w;
    dest->height = h;
    alloc_image(dest);

    for(int i = 0; i < h; i++) {
        for(int j = 0; j < w; j++) {
            for(int k = 0; k < dest->channels; k++) {
                dest->data[dest->channels * (i * w + j) + k] = src.data[src.channels * ((i + y) * src.width + (x + j)) + k];
            }
        }
    }
}

// Black magic line drawing function
void draw_line(Image img, int x0, int y0, int x1, int y1, int b, int g, int r) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    int d_long = abs(dx);
    int d_short = abs(dy);

    int offset_long = dx > 0 ? 1 : -1;
    int offset_short = dy > 0 ? img.width : -img.width;

    if(d_long < d_short) {
        int tmp = d_long;
        d_long = d_short;
        d_short = tmp;

        tmp = offset_long;
        offset_long = offset_short;
        offset_short = tmp;
    }

    int error = 2 * d_short - d_long;
    int index = y0 * img.width + x0;
    const int offset[2] = {offset_long, offset_long + offset_short};
    const int abs_d[2] = {2*d_short, 2*(d_short - d_long)};
    for(int i = 0; i <= d_long; ++i) {
        if(index <= img.width * img.height - 2) {
            img.data[img.channels * index] = b;
            img.data[img.channels * index + 1] = g;
            img.data[img.channels * index + 2] = r;
        }
        const int error_too_big = error >= 0;
        index += offset[error_too_big];
        error += abs_d[error_too_big];
    }
}

void set_pixel(Image img, int x, int y, int r, int g, int b) {
    if(x < 0 || y < 0 || x >= img.width || y >= img.height) return;
    img.data[img.channels * (y * img.width + x) + 0] = r;
    img.data[img.channels * (y * img.width + x) + 1] = g;
    img.data[img.channels * (y * img.width + x) + 2] = b;
}

static SDL_Renderer* renderer;
static SDL_Window* window;
static SDL_Texture* texture;
static pthread_t window_thread_id;
static int pitch;

void create_and_show(Image img) {
    create_window(img.width, img.height);
    show_image(img);
}

static int window_running;
static int set_window_width;
static int set_window_height;
static int window_width;
static int window_height;
static pthread_mutex_t image_lock = PTHREAD_MUTEX_INITIALIZER;
static Image window_img;

void create_window() {
    window_running = 1;
    set_window_width = -1;
    set_window_height = -1;
    window_width = 640;
    window_height = 480;
    window_img.width = 640;
    window_img.height = 480;
    window_img.channels = 3;
    alloc_image(&window_img);

    pthread_create(&window_thread_id, NULL, window_loop, NULL);
}

void* window_loop(void* arg) {
    texture = NULL;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
    SDL_CreateWindowAndRenderer(640, 480, 0, &window, &renderer);
    SDL_SetWindowTitle(window, "Test");

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, window_width, window_height);

    while(window_running) {
        void* pixels;

        if(set_window_width != window_width || set_window_height != window_height) {
            printf("Window size (%dx%d) != image size (%dx%d)\n", window_width, window_height, set_window_width, set_window_height);

            SDL_SetWindowSize(window, set_window_width, set_window_height);

            /* Textures cannot be resized */
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, set_window_width, set_window_height);

            window_width = set_window_width;
            window_height = set_window_height;
        }

        SDL_LockTexture(texture, NULL, &pixels, &pitch);

        for(int x = 0; x < window_width; x++) {
            for(int y = 0; y < window_height; y++) {
                uint8_t* base = ((uint8_t*) pixels) + (4 * (y * window_width + x));
                pthread_mutex_lock(&image_lock);
                base[0] = window_img.data[3 * (y * window_width + x)];
                base[1] = window_img.data[3 * (y * window_width + x) + 1];
                base[2] = window_img.data[3 * (y * window_width + x) + 2];
                pthread_mutex_unlock(&image_lock);
                base[3] = 255;
            }
        }

        SDL_UnlockTexture(texture);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Event event;
        if(SDL_PollEvent(&event) && event.type == SDL_QUIT) {
            break;
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

void show_image(Image img) {
    pthread_mutex_lock(&image_lock);
    set_window_width = img.width;
    set_window_height = img.height;

    //printf("Window size: %dx%d\n", set_window_width, set_window_height);

    copy_image(img, &window_img);
    pthread_mutex_unlock(&image_lock);
}

void destroy_window() {
    window_running = 0;
    pthread_join(window_thread_id, NULL);
}
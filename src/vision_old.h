#pragma once

#include <stdint.h>

#define MIN_GROUP_PIXELS 70

// 8-bit image
typedef struct Image {
    uint32_t width, height, channels;

    uint8_t* data;
} Image;

// Maybe use static image structures?
// Group of pixels
typedef struct Group {
    uint32_t num_pixels;
    float center_x, center_y;
}

void alloc_image(Image *image);
void clear_image(Image *image);
void copy_image(Image src, Image *dest);
void free_image(Image image);

void write_image(const char *path, Image image);

void resize_image(Image src, Image *dest, int scale);
void crop_image(Image src, Image *dest, int x, int y, int w, int h);

void draw_line(Image img, int x0, int y0, int x1, int y1, int r, int g, int b);
void set_pixel(Image img, int x, int y, int r, int g, int b);

void find_groups(Image img, Group *groups);

void create_window();
void *window_loop(void *arg);
void show_image(Image img);
void destroy_window();

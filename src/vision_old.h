/****************************************************************************************************************
 *                                                                                                              *
 * VISION                                                                                                       *
 *                                                                                                              *
 * Provides the data structure for images, functions for memory allocation and de-allocation, for image         *
 * manipulation such as resizing and cropping and draw functions. Also provides functions for displaying        *
 * images in a simple window using SDL2.                                                                        *
 *                                                                                                              *
 * alloc_image(image)                     - Allocates memory for image data                                     *
 * clear_image(image)                     - Writes zeroes into image data                                       *
 * copy_image(src, dest)                  - Copies an image into the provided image                             *
 * free_image(image)                      - Frees memory of image data                                          *
 *                                                                                                              *
 * write_image(path, image)               - Write image to disk as a PNG file                                   *
 *                                                                                                              *
 * resize_image(src, dest, scale)         - Resize image (in powers of two). Negative scale will scale image    *
 *                                          down                                                                *
 * crop_image(src, dest, x, y, w, h)      - Crops an image (copies cropped part into dest)                      *
 *                                                                                                              *
 * draw_line(img, x0,y0, x1,y1, r,g,b)    - Draws a one-pixel wide line into the image provided                 *
 *                                                                                                              *
 * set_pixel(img, x, y, r, g, b)          - Sets the pixel at point x, y to colors r, g, b                      *
 *                                          (only for three-channel images)                                     *
 *                                                                                                              *
 * find_groups(image, groups)                     - Extracts groups of pixels from black and white image                *
 *                                                                                                              *
 *                                                                                                              *
 * create_window()                        - Creates a window to display an image in. Only one window at a time! *
 * show_image(img)                        - Shows image in the created window                                   *
 * destroy_window()                       - Destroys created window                                             *
 *                                                                                                              *
 ****************************************************************************************************************/

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

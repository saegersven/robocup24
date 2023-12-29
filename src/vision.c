#include "vision.h"

#include <stdio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb_image_write.h"

void copy_image(S_IMAGE(src), S_IMAGE(dest)) {
    if(src_w != dest_w || src_h != dest_h || src_c != dest_c) {
        fprintf(stderr, "copy_image: mismatched sizes");
        return;
    }

    memcpy(dest_d, src_d, src_w * src_h * src_c);
}

void read_raw_image(const char* path, uint8_t *img) {
    FILE* f = fopen(path, "r");

    uint32_t img_width, img_height;
    fread(&img_width, sizeof(uint32_t), 1, f);
    fread(&img_height, sizeof(uint32_t), 1, f);
    fread(img, sizeof(uint8_t), img_width * img_height * 3, f);

    fclose(f);
}

void write_image(const char* path, S_IMAGE(img)) {
    stbi_write_png(path, img_w, img_h, img_c, img_d, img_w * img_c);
}

void resize_image(S_IMAGE(src), S_IMAGE(dest), int new_width, int new_height) {
    if(dest_w != new_width || dest_h != new_height || src_c != dest_c) {
        fprintf(stderr, "resize_image: mismatched sizes");
        return;
    }

    for(int i = 0; i < new_height; i++) {
        for(int j = 0; j < new_width; j++) {
            int dest_idx = src_c * (i * new_width + j);
            int src_i = i * (float)src_h / new_height;
            int src_j = j * (float)src_w / new_width;
            int src_idx = src_c * (src_i * src_w + src_j);

            for(int k = 0; k < src_c; k++) {
                dest_d[dest_idx + k] = src_d[src_idx + k];
            }
        }
    }
}
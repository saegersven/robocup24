#pragma once

#include <stdint.h>

#include "../libs/tensorflow/lite/c/c/c_api.h"

#include "../display/display.h"

#define VICTIMS_MODEL_PATH "/home/pi/robocup24/runtime_data/victims_v3.tflite"

#define INPUT_WIDTH 320
#define INPUT_HEIGHT 240
#define INPUT_CHANNELS 3

#define MODEL_INPUT_WIDTH 96
#define MODEL_INPUT_HEIGHT 96
#define MODEL_INPUT_CHANNELS 1

#define MODEL_OUTPUT_WIDTH 12
#define MODEL_OUTPUT_HEIGHT 12
#define MODEL_OUTPUT_CHANNELS 3
#define CHANNEL_LIVING 2
#define CHANNEL_DEAD 1

#define NUM_DETECTIONS 3

struct Victim {
    float x, y, d;
    int dead;
};

static TfLiteModel *victims_model;
static TfLiteInterpreterOptions *victims_options;
static TfLiteInterpreter *victims_interpreter;

void victims_init() {
    victims_model = TfLiteModelCreateFromFile(VICTIMS_MODEL_PATH);
    victims_options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(victims_options, 4);

    victims_interpreter = TfLiteInterpreterCreate(victims_model, victims_options);

    if(TfLiteInterpreterAllocateTensors(victims_interpreter) != kTfLiteOk) {
        fprintf(stderr, "victims_init: Could not allocate tensors\n");
        return;
    }
}

void victims_destroy() {
    TfLiteInterpreterDelete(victims_interpreter);
    TfLiteInterpreterOptionsDelete(victims_options);
    TfLiteModelDelete(victims_model);

    victims_interpreter = NULL;
    victims_options = NULL;
    victims_model = NULL;
}

#define DETECTION_THRESHOLD 160

struct Group {
    uint32_t num_pixels;
    float center_x, center_y;
};

// Very basic and slow contour finding algorithm, but its mine
void victims_add_to_group_center(uint8_t *heatmap, int x_pos, int y_pos, struct Group *group, int width, int height) {
    int x_limit = width - 1;
    int y_limit = height - 1;

    // Iterate over eight neighbouring pixels
    for(int y = -1; y <= 1; y++) {
        for(int x = -1; x <= 1; x++) {
            int y_idx = y_pos + y;
            int x_idx = x_pos + x;

            if(y == 0 && x == 0) continue; // Don't need to check center pixel

            int idx = y_idx * LINE_FRAME_WIDTH + x_idx;
            if(heatmap[idx] == 0xFF) {
                heatmap[idx] = 0x7F;
                group->center_x += (float)x_idx;
                group->center_y += (float)y_idx;
                group->num_pixels++;

                if(x_idx > 0 && x_idx < x_limit && y_idx > 0 && y_idx < y_limit)
                    victims_add_to_group_center(heatmap, x_idx, y_idx, group, width, height);
            }
        }
    }
}

void victims_find_groups(uint8_t *heatmap, struct Group *groups, uint32_t *num_groups, int width, int height) {
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            int idx = y * width + x;
            if(heatmap[idx] == 0xFF) {
                struct Group group;
                group.num_pixels = 1;
                group.center_x = x;
                group.center_y = y;
                heatmap[idx] = 0x7F;
                victims_add_to_group_center(heatmap, x, y, &group, width, height);

                if(group.num_pixels > 0) {
                    // add_to_group_center is a recursive method that cannot know if it is
                    // the last to add to the group pixels, so we have to normalize
                    group.center_x /= group.num_pixels;
                    group.center_y /= group.num_pixels;
                    groups[*num_groups] = group;
                    *num_groups += 1;
                }
            }
        }
    }
}

int victims_detect(uint8_t *image, struct Victim *victims) {
    char filename[64];
    sprintf(filename, "/home/pi/capture/victim/%lld.png", milliseconds());
    write_image(filename, image, INPUT_WIDTH, INPUT_HEIGHT, 3);

    TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor(victims_interpreter, 0);

    int8_t input_image_norm[MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS];

    for(int i = 0; i < MODEL_INPUT_HEIGHT; i++) {
        for(int j = 0; j < MODEL_INPUT_WIDTH; j++) {
            int src_i = (float)i * (float)INPUT_HEIGHT / MODEL_INPUT_HEIGHT;
            int src_j = (float)j * (float)INPUT_WIDTH / MODEL_INPUT_WIDTH;
            int src_idx = src_i * INPUT_WIDTH + src_j;

            int idx = i * MODEL_INPUT_WIDTH + j;

            /*for(int k = 0; k < MODEL_INPUT_CHANNELS; k++) {
                input_image_norm[MODEL_INPUT_CHANNELS * idx + k] = (float)image[INPUT_CHANNELS * src_idx + k] / 127.5f - 1.0f;
            }*/
            float sum = 0.0f;
            for(int k = 0; k < INPUT_CHANNELS; k++) {
                sum += (float)image[INPUT_CHANNELS * src_idx + k] - 127.5f;
            }
            sum /= INPUT_CHANNELS;
            if(sum > 128.0f) sum = 128.0f;
            if(sum < -127.0f) sum = -127.0f;
            input_image_norm[idx] = (int8_t)sum;
            //printf("%f\n", sum);
        }
    }

    if(TfLiteTensorCopyFromBuffer(input_tensor, input_image_norm, MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS * sizeof(int8_t)) != kTfLiteOk) {
        fprintf(stderr, "victims_detect: Could not copy input data\n");
        return 0;
    }

    long long start_time = milliseconds();
    TfLiteInterpreterInvoke(victims_interpreter);
    printf("NN took: %lld ms\n", milliseconds() - start_time);


    int8_t heatmap[MODEL_OUTPUT_WIDTH * MODEL_OUTPUT_HEIGHT * MODEL_OUTPUT_CHANNELS];
    const TfLiteTensor *output_tensor = TfLiteInterpreterGetOutputTensor(victims_interpreter, 0);
    TfLiteTensorCopyToBuffer(output_tensor, heatmap, MODEL_OUTPUT_WIDTH * MODEL_OUTPUT_HEIGHT * MODEL_OUTPUT_CHANNELS * sizeof(int8_t));

    uint8_t debug_heatmap[MODEL_OUTPUT_WIDTH * MODEL_OUTPUT_HEIGHT * MODEL_OUTPUT_CHANNELS];
    for(int i = 0; i < MODEL_OUTPUT_WIDTH * MODEL_OUTPUT_HEIGHT * MODEL_OUTPUT_CHANNELS; i++) {
        debug_heatmap[i] = (uint8_t)((float)heatmap[i] + 127);
        //printf("%d\n", debug_heatmap[i]);
    }

    //write_image("/home/pi/robocup24/heatmap.png", debug_heatmap, 12, 12, 3);

    uint8_t heatmap_living[MODEL_OUTPUT_WIDTH * MODEL_OUTPUT_HEIGHT];
    uint8_t heatmap_dead[MODEL_OUTPUT_WIDTH * MODEL_OUTPUT_HEIGHT];
    
    for(int i = 0; i < MODEL_OUTPUT_HEIGHT; i++) {
        for(int j = 0; j < MODEL_OUTPUT_WIDTH; j++) {
            for(int k = 0; k < MODEL_OUTPUT_CHANNELS; k++) {
                int idx = MODEL_OUTPUT_WIDTH * i + j;
                int src_idx = MODEL_OUTPUT_CHANNELS * idx + k;
                
                
                /*printf("(%d, %d, %d)\n", i, j, k);
                printf("Val: %d\n", heatmap[src_idx]);*/

                int val = heatmap[src_idx] + 127;

                if(k == CHANNEL_LIVING) {
                    heatmap_living[idx] = val > DETECTION_THRESHOLD ? 0xFF : 0;
                } else if(k == CHANNEL_DEAD) {
                    heatmap_dead[idx] = val > DETECTION_THRESHOLD ? 0xFF : 0;
                }
            }
        }
    }

    //write_image("/home/pi/robocup24/heatmap_living.png", heatmap_living, 12, 12, 1);

    struct Group groups_living[32];
    uint32_t num_groups_living = 0;
    struct Group groups_dead[32];
    uint32_t num_groups_dead = 0;
    victims_find_groups(heatmap_living, groups_living, &num_groups_living, MODEL_OUTPUT_WIDTH, MODEL_OUTPUT_HEIGHT);
    victims_find_groups(heatmap_dead, groups_dead, &num_groups_dead, MODEL_OUTPUT_WIDTH, MODEL_OUTPUT_HEIGHT);

    printf("Living: %d, Dead: %d\n", num_groups_living, num_groups_dead);

    int num_victims = 0;

    for(int i = 0; i < num_groups_living; i++) {
        struct Victim victim;
        victim.x = (float)groups_living[i].center_x / MODEL_OUTPUT_WIDTH;
        victim.y = (float)groups_living[i].center_y / MODEL_OUTPUT_HEIGHT;
        victim.dead = false;

        victims[num_victims] = victim;

        num_victims++;
    }

    for(int i = 0; i < num_groups_dead; i++) {
        struct Victim victim;
        victim.x = (float)groups_dead[i].center_x / MODEL_OUTPUT_WIDTH;
        victim.y = (float)groups_dead[i].center_y / MODEL_OUTPUT_HEIGHT;
        victim.dead = true;

        victims[num_victims] = victim;

        num_victims++;
    }

    /*
    int num_victims = 0;
    for(int i = 0; i < NUM_DETECTIONS; i++) {
        if(confidence[i] > DETECTION_THRESHOLD) {
            float ymin = boxes[i * 4 + 0];
            float xmin = boxes[i * 4 + 1];
            float ymax = boxes[i * 4 + 2];
            float xmax = boxes[i * 4 + 3];

            struct Victim v;
            v.x = (xmin + xmax) / 2.0f;
            v.y = (ymin + ymax) / 2.0f;
            v.d = (xmax - xmin + ymax - ymin) / 2.0f;
            v.dead = classes[i] > DEAD_THRESHOLD;

            victims[num_victims] = v;
            ++num_victims;
        }
    }

    if(num_victims > 3) {
        printf("victims_detect: More than 3 victims (%d)\n", num_victims);
    }*/

    return num_victims;
}

// Returns index of victim in supplied array, or -1
int victims_choose(struct Victim *victims, int num_victims, int enable_dead) {
    // We want to choose the victim that is closest (highest y-coordinate)
    // We also want to prefer living victims, even if they are further away than
    // the dead one.

    if(num_victims == 0) return -1;

    int idx_living = -1;
    int idx_dead = -1;
    float highest_y_dead = 0.0f;
    float highest_y_living = 0.0f;

    for(int i = 0; i < num_victims; i++) {
        if(victims[i].dead) {
            if(victims[i].y > highest_y_dead) {
                idx_dead = i;
                highest_y_dead = victims[i].y;
            }
        } else {
            if(victims[i].y > highest_y_living) {
                idx_living = i;
                highest_y_living = victims[i].y;
            }
        }
    }

    if(idx_living != -1) return idx_living;
    if(enable_dead && idx_dead != -1) return idx_dead;
    
    return -1;
}

// Returns 1 if victim was found, 0 otherwise
int victims_find(uint8_t *image, int enable_dead, struct Victim* victim) {
    struct Victim victims[NUM_DETECTIONS];
    int num_victims = victims_detect(image, victims);

    printf("Num victims: %d\n", num_victims);

    int victim_idx = victims_choose(victims, num_victims, enable_dead);
    if(victim_idx == -1) return 0;

    *victim = victims[victim_idx];
    return 1;
}
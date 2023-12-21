#pragma once

#include <stdint.h>

#include "../libs/tensorflow/lite/c/c_api.h"

#define VICTIMS_MODEL_PATH "/home/pi/robocup24/runtime_data/victims.tflite"

#define INPUT_WIDTH 240
#define INPUT_HEIGHT 320
#define INPUT_CHANNELS 3
#define MODEL_INPUT_WIDTH 320
#define MODEL_INPUT_HEIGHT 320
#define MODEL_INPUT_CHANNELS 3

#define NUM_DETECTIONS 10

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
    TfLiteInterpreterOptionsSetNumThreads(victims_options, 1);

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
}

#define DETECTION_THRESHOLD 0.5f
#define DEAD_THRESHOLD      0.5f

int victims_detect(uint8_t *image, Victim *victims) {
    TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor(victims_interpreter, 0);

    float input_image_norm[MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS];

    for(int i = 0; i < MODEL_INPUT_HEIGHT; i++) {
        for(int j = 0; j < MODEL_INPUT_WIDTH; j++) {
            int src_i = i * (float)INPUT_HEIGHT / MODEL_INPUT_HEIGHT;
            int src_j = j * (float)INPUT_WIDTH / MODEL_INPUT_WIDTH;
            int src_idx = src_i * INPUT_WIDTH + src_j;

            int idx = i * MODEL_INPUT_WIDTH + j;

            for(int k = 0; k < MODEL_INPUT_CHANNELS; k++) {
                input_image_norm[MODEL_INPUT_CHANNELS * idx + k] = (float)image[INPUT_CHANNELS * src_idx + k] / 255.0f;
            }
        }
    }

    if(TfLiteTensorCopyFromBuffer(input_tensor, input_image_norm, MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS * sizeof(float)) != kTfLiteOk) {
        fprintf(stderr, "victims_detect: Could not copy input data\n");
        return 0;
    }

    TfLiteInterpreterInvoke(victims_interpreter);

    float confidence[NUM_DETECTIONS];
    const TfLiteTensor *output_tensor0 = TfLiteInterpreterGetOutputTensor(victims_interpreter, 0);
    TfLiteTensorCopyToBuffer(output_tensor0, confidence, NUM_DETECTIONS * sizeof(float));

    float boxes[NUM_DETECTIONS * 4];
    const TfLiteTensor *output_tensor1 = TfLiteInterpreterGetOutputTensor(victims_interpreter, 1);
    TfLiteTensorCopyToBuffer(output_tensor1, boxes, NUM_DETECTIONS * 4 * sizeof(float));

    float classes[NUM_DETECTIONS];
    const TfLiteTensor *output_tensor3 = TfLiteInterpreterGetOutputTensor(victims_interpreter, 3);
    TfLiteTensorCopyToBuffer(output_tensor3, classes, NUM_DETECTIONS * sizeof(float));

    int num_victims = 0;
    for(int i = 0; i < NUM_DETECTIONS; i++) {
        if(confidence[i] > DETECTION_THRESHOLD) {
            float ymin = boxes[i * 4 + 0];
            float xmin = boxes[i * 4 + 1];
            float ymax = boxes[i * 4 + 2];
            float xmax = boxes[i * 4 + 3];

            Victim v;
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
    }

    return num_victims;
}

// Returns index of victim in supplied array, or -1
int victims_choose(Victim *victims, int num_victims, int enable_dead) {
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
int victims_find(uint8_t *image, int enable_dead, Victim* victim) {
    Victim *victims[NUM_DETECTIONS];
    int num_victims = victims_detect(image, victims);

    int victim_idx = victims_choose(victims, num_victims, enable_dead);
    if(victim_idx == -1) return 0;

    *victim = victims[victim_idx];
    return 1;
}
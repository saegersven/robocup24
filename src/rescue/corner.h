#pragma once

#include <stdint.h>

#include "../libs/tensorflow/lite/c/c/c_api.h"

#include "../utils.h"
#include "../thresholding.h"

#define CORNER_MODEL_PATH "/home/pi/robocup24/runtime_data/corner.tflite"

#define CORNER_MODEL_CORNER_INPUT_WIDTH 160
#define CORNER_MODEL_CORNER_INPUT_HEIGHT 120
#define CORNER_MODEL_CORNER_INPUT_CHANNELS 3

#define CORNER_MODEL_OUTPUT_WIDTH 32
#define CORNER_MODEL_OUTPUT_HEIGHT 12
#define CORNER_MODEL_OUTPUT_CHANNELS 2

#define CORNER_INPUT_WIDTH 320
#define CORNER_INPUT_HEIGHT 240
#define CORNER_INPUT_CHANNELS 3

static TfLiteModel *corner_model = NULL;
static TfLiteInterpreterOptions *corner_options = NULL;
static TfLiteInterpreter *corner_interpreter = NULL;

void corner_init() {
    corner_model = TfLiteModelCreateFromFile(CORNER_MODEL_PATH);
    corner_options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(corner_options, 1);

    corner_interpreter = TfLiteInterpreterCreate(corner_model, corner_options);

    if(TfLiteInterpreterAllocateTensors(corner_interpreter) != kTfLiteOk) {
        fprintf(stderr, "corner_init: Could not allocate tensors\n");
        return;
    }
}

void corner_destroy() {
    TfLiteInterpreterDelete(corner_interpreter);
    TfLiteInterpreterOptionsDelete(corner_options);
    TfLiteModelDelete(corner_model);

    corner_interpreter = NULL;
    corner_options = NULL;
    corner_model = NULL;
}

int corner_detect(uint8_t *input, float *x, int green) {
    TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor(corner_interpreter, 0);

    float input_image[CORNER_MODEL_CORNER_INPUT_WIDTH * CORNER_MODEL_CORNER_INPUT_HEIGHT * CORNER_MODEL_CORNER_INPUT_CHANNELS];
    for(int i = 0; i < CORNER_MODEL_CORNER_INPUT_HEIGHT; i++) {
        for(int j = 0; j < CORNER_MODEL_CORNER_INPUT_WIDTH; j++) {
            int src_i = (float)i * (float)CORNER_INPUT_HEIGHT / CORNER_MODEL_CORNER_INPUT_HEIGHT;
            int src_j = (float)j * (float)CORNER_INPUT_WIDTH / CORNER_MODEL_CORNER_INPUT_WIDTH;
            int src_idx = src_i * CORNER_INPUT_WIDTH + src_j;

            int idx = i * CORNER_MODEL_CORNER_INPUT_WIDTH + j;

            for(int k = 0; k < 3; k++) {
                input_image[3*idx + k] = (float)input[3*src_idx + k];
            }
        }
    }

    if(TfLiteTensorCopyFromBuffer(input_tensor, input_image, CORNER_MODEL_CORNER_INPUT_WIDTH * CORNER_MODEL_CORNER_INPUT_HEIGHT * CORNER_MODEL_CORNER_INPUT_CHANNELS * sizeof(float)) != kTfLiteOk) {
        fprintf(stderr, "corner_invoke: Could not copy input data\n");
        return -1;
    }

    TfLiteInterpreterInvoke(corner_interpreter);

    float output[CORNER_MODEL_OUTPUT_WIDTH * CORNER_MODEL_OUTPUT_HEIGHT * CORNER_MODEL_OUTPUT_CHANNELS];

    const TfLiteTensor *output_tensor = TfLiteInterpreterGetOutputTensor(corner_interpreter, 0);
    TfLiteTensorCopyToBuffer(output_tensor, output, CORNER_MODEL_OUTPUT_WIDTH * CORNER_MODEL_OUTPUT_HEIGHT * CORNER_MODEL_OUTPUT_CHANNELS * sizeof(float));

    float output_blurred[CORNER_MODEL_OUTPUT_WIDTH * CORNER_MODEL_OUTPUT_HEIGHT];
    memcpy(output_blurred, output, CORNER_MODEL_OUTPUT_WIDTH * CORNER_MODEL_OUTPUT_HEIGHT * CORNER_MODEL_OUTPUT_CHANNELS * sizeof(float));
    //box_blur(output, CORNER_MODEL_OUTPUT_WIDTH, CORNER_MODEL_OUTPUT_HEIGHT, CORNER_MODEL_OUTPUT_CHANNELS, output_blurred, 3, 1); // TODO: adjust parameters

    uint8_t output_blurred_byte[CORNER_MODEL_OUTPUT_WIDTH * CORNER_MODEL_OUTPUT_HEIGHT];
    for(int i = 0; i < CORNER_MODEL_OUTPUT_HEIGHT * CORNER_MODEL_OUTPUT_WIDTH * CORNER_MODEL_OUTPUT_CHANNELS; i++) {
        output_blurred_byte[i] = output_blurred[i];
    }
    write_image("corner_nn_output.png", output_blurred_byte, CORNER_MODEL_OUTPUT_WIDTH, CORNER_MODEL_OUTPUT_HEIGHT, CORNER_MODEL_OUTPUT_CHANNELS);

    int num_pixels = 0;
    for(int i = 4; i < CORNER_MODEL_OUTPUT_HEIGHT; i++) {
        for(int j = 0; j < CORNER_MODEL_OUTPUT_WIDTH; j++) {
            int idx = CORNER_MODEL_OUTPUT_CHANNELS * (i * CORNER_MODEL_OUTPUT_WIDTH + j) + green;
            if(output_blurred[idx] > 0.25f) {
                num_pixels++;
                *x += j;
            }
        }
    }
    printf("Corner Num pixels: %d\n", num_pixels);
    if(num_pixels > 7) {
        *x /= num_pixels;
        *x /= CORNER_MODEL_OUTPUT_WIDTH;
        *x -= 0.5f;
        return 1;
    }
    
    *x = 0.0f;
    return 0;
}

static uint8_t corner_thresh[CORNER_INPUT_WIDTH * CORNER_INPUT_HEIGHT];

#define CORNER_Y_SKIP 80

int corner_detect_classic(uint8_t *input, float *x, int green) {
    char filename[64];
    sprintf(filename, "/home/pi/capture/corner/%lld.png", milliseconds());
    write_image(filename, input, CORNER_INPUT_WIDTH, CORNER_INPUT_HEIGHT, 3);

    uint32_t num_pixels = 0;

    image_threshold(corner_thresh, CORNER_INPUT_WIDTH, CORNER_INPUT_HEIGHT, 1, input, CORNER_INPUT_WIDTH, CORNER_INPUT_HEIGHT, CORNER_INPUT_CHANNELS, &num_pixels, green ? is_green : is_red);

    num_pixels = 0;
    for(int i = CORNER_Y_SKIP; i < CORNER_INPUT_HEIGHT; i++) {
        for(int j = 0; j < CORNER_INPUT_WIDTH; j++) {
            if(corner_thresh[i * CORNER_INPUT_WIDTH + j]) {
                *x += j;
                num_pixels++;
            }
        }
    }
    if(num_pixels < 0.01f * CORNER_INPUT_WIDTH * CORNER_INPUT_HEIGHT) return 0;

    *x /= num_pixels * CORNER_INPUT_WIDTH;
    *x -= 0.5f;
    return 1;
}
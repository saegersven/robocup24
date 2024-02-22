#pragma once

#include <stdint.h>

#include "../libs/tensorflow/lite/c/c/c_api.h"

#include "../utils.h"
#include "../thresholding.h"

#define CORNER_MODEL_PATH "/home/pi/robocup24/runtime_data/corner.tflite"

#define MODEL_INPUT_WIDTH 160
#define MODEL_INPUT_HEIGHT 120
#define MODEL_INPUT_CHANNELS 3

#define MODEL_OUTPUT_WIDTH 32
#define MODEL_OUTPUT_HEIGHT 12
#define MODEL_OUTPUT_CHANNELS 2

#define INPUT_WIDTH 320
#define INPUT_HEIGHT 240
#define INPUT_CHANNELS 3

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
}

int corner_detect(uint8_t *input, float *x, int green) {
    TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor(corner_interpreter, 0);

    float input_image[MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS];
    for(int i = 0; i < MODEL_INPUT_HEIGHT; i++) {
        for(int j = 0; j < MODEL_INPUT_WIDTH; j++) {
            int src_i = (float)i * (float)INPUT_HEIGHT / MODEL_INPUT_HEIGHT;
            int src_j = (float)j * (float)INPUT_WIDTH / MODEL_INPUT_WIDTH;
            int src_idx = src_i * INPUT_WIDTH + src_j;

            int idx = i * MODEL_INPUT_WIDTH + j;

            for(int k = 0; k < 3; k++) {
                input_image[3*idx + k] = (float)input[3*src_idx + k];
            }
        }
    }

    if(TfLiteTensorCopyFromBuffer(input_tensor, input_image, MODEL_INPUT_WIDTH * MODEL_INPUT_HEIGHT * MODEL_INPUT_CHANNELS * sizeof(float)) != kTfLiteOk) {
        fprintf(stderr, "corner_invoke: Could not copy input data\n");
        return;
    }

    TfLiteInterpreterInvoke(corner_interpreter);

    float output[MODEL_OUTPUT_WIDTH * MODEL_OUTPUT_HEIGHT * MODEL_OUTPUT_CHANNELS];

    const TfLiteTensor *output_tensor = TfLiteInterpreterGetOutputTensor(corner_interpreter, 0);
    TfLiteTensorCopyToBuffer(output_tensor, output, sizeof(output));

    float output_blurred[MODEL_OUTPUT_WIDTH * MODEL_OUTPUT_HEIGHT];
    //box_blur(output, MODEL_OUTPUT_WIDTH, MODEL_OUTPUT_HEIGHT, MODEL_OUTPUT_CHANNELS, output_blurred, 5, 2); // TODO: adjust parameters

    int num_pixels = 0;
    for(int i = 0; i < MODEL_OUTPUT_HEIGHT; i++) {
        for(int j = 0; j < MODEL_OUTPUT_WIDTH; j++) {
            int idx = MODEL_OUTPUT_CHANNELS * (i * MODEL_OUTPUT_WIDTH + j) + green;
            if(output_blurred[idx] > 0.5f) {
                num_pixels++;
                *x += j;
            }
        }
    }
    if(num_pixels > 10) {
        *x /= num_pixels;
        *x /= MODEL_OUTPUT_WIDTH;
        *x += 0.5f;
        return 1;
    }
    
    *x = 0.0f;
    return 0;
}

int corner_detect_classic(uint8_t *input, float *x, int green) {
    char filename[64];
    sprintf(filename, "/home/pi/capture/corner/%lld.png", milliseconds());
    write_image(filename, input, INPUT_WIDTH, INPUT_HEIGHT, 3);

    uint8_t thresh[INPUT_WIDTH * INPUT_HEIGHT];

    uint32_t num_pixels = 0;

    image_threshold(thresh, INPUT_WIDTH, INPUT_HEIGHT, 1, input, INPUT_WIDTH, INPUT_HEIGHT, INPUT_CHANNELS, &num_pixels, green ? is_green : is_red);

    if(num_pixels < 0.01f * INPUT_WIDTH * INPUT_HEIGHT) return 0;

    for(int i = 0; i < INPUT_HEIGHT; i++) {
        for(int j = 0; j < INPUT_WIDTH; j++) {
            if(thresh[i * INPUT_WIDTH + j]) {
                *x += j;
            }
        }
    }
    *x /= num_pixels * INPUT_WIDTH;
    *x -= 0.5f;
    return 1;
}
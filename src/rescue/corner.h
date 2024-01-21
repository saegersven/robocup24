#pragma once

#include <stdint.h>

#include "../libs/tensorflow/lite/c/c/c_api.h"

#include "../utils.h"

#define CORNER_MODEL_PATH "/home/pi/robocup24/runtime_data/corner.tflite"

#define MODEL_INPUT_WIDTH 160
#define MODEL_INPUT_HEIGHT 120
#define MODEL_INPUT_CHANNELS 3

#define MODEL_OUTPUT_WIDTH 32
#define MODEL_OUTPUT_HEIGHT 12
#define MODEL_OUTPUT_CHANNELS 2

#define INPUT_WIDTH 320
#define INPUT_HEIGHT 240

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

// Macro for indexing image arrays
#define IDX(dx, dy) (MODEL_OUTPUT_CHANNELS * ((i + (dy)) * MODEL_OUTPUT_WIDTH + j + (dx)) + k)

float corner_detect(uint8_t *input, int green) {
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

    const TfLiteTensor *output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
    TfLiteTensorCopyToBuffer(output_tensor, output, sizeof(output));

    float output_blurred[MODEL_OUTPUT_WIDTH * MODEL_OUTPUT_HEIGHT];
    memset(output_blurred, 0, sizeof(output_blurred));
    for(int i = 1; i < MODEL_OUTPUT_HEIGHT - 1; i++) {
        for(int j = 1; j < MODEL_OUTPUT_WIDTH - 1; j++) {
            int k = green;
            output_blurred[i * MODEL_OUTPUT_WIDTH + j] = (output[IDX(-1, -1)] + output[IDX(-1, 1)] + output[IDX(1, 1)] + output[IDX(1, -1)]) / 16.0f + 
                                        (output[IDX(-1, 0)] + output[IDX(0, -1)] + output[IDX(1, 0)] + output[IDX(0, 1)]) / 8.0f +
                                        output[IDX(0, 0)] / 4.0f;
        }
    }

    int num_pixels = 0;
    for(int i = 0; i < MODEL_OUTPUT_HEIGHT; i++) {
        for(int j = 0; j < MODEL_OUTPUT_WIDTH; j++) {
            int idx = i * MODEL_OUTPUT_WIDTH + j;
            if(output_blurred[idx] > 0.5f) {
                num_pixels++;
                x += j;
            }
        }
    }
    if(num_pixels > 10) {
        x /= num_pixels;
        x /= MODEL_OUTPUT_WIDTH;
        return 1;
    }
    x = 0.0f;
    return 0;
}
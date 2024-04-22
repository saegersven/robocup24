#pragma once

#include <stdint.h>
#include <pthread.h>
#include <time.h>

#include "../libs/tensorflow/lite/c/c/c_api.h"

#include "../utils.h"

#define SILVER_MODEL_PATH "/home/pi/robocup24/runtime_data/silver.tflite"

#define WIDTH 80
#define HEIGHT 48

static TfLiteModel *model = NULL;
static TfLiteInterpreterOptions *options = NULL;
static TfLiteInterpreter *interpreter = NULL;

static pthread_mutex_t silver_output_lock = PTHREAD_MUTEX_INITIALIZER;

void rescue_silver_init() {
    model = TfLiteModelCreateFromFile(SILVER_MODEL_PATH);
    options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options, 1);

    interpreter = TfLiteInterpreterCreate(model, options);

    if(TfLiteInterpreterAllocateTensors(interpreter) != kTfLiteOk) {
        fprintf(stderr, "silver_init: Could not allocate tensors\n");
        return;
    }
}

void rescue_silver_destroy() {
    TfLiteInterpreterDelete(interpreter);
    interpreter = NULL;
    TfLiteInterpreterOptionsDelete(options);
    options = NULL;
    TfLiteModelDelete(model);
    model = NULL;
}

float rescue_silver_detect_flipped(uint8_t *frame, uint8_t input_channels) {
    return 0.0f;
    TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor(interpreter, 0);

    uint8_t input_image_norm[WIDTH * HEIGHT];
    uint8_t input_image_debug[WIDTH * HEIGHT * 3];

    memset(input_image_norm, 0, sizeof(input_image_norm));

    for(int i = 0; i < HEIGHT; i++) {
        for(int j = 0; j < WIDTH; j++) {
            int idx = i * WIDTH + j;
            int idx_flipped = i * WIDTH + j;

            input_image_norm[idx] = 0.0f;
            for(int k = 0; k < input_channels; k++) {
                input_image_norm[idx] += (float)frame[input_channels*idx_flipped + k];
                input_image_debug[3*idx + k] = frame[input_channels*idx_flipped + k];
            }
            input_image_norm[idx] /= 3.0f;
        }
    }
    
    write_image("/home/pi/Desktop/silver.png", LINE_IMAGE_TO_PARAMS(input_image_debug));

    if(TfLiteTensorCopyFromBuffer(input_tensor, input_image_norm, WIDTH * HEIGHT * sizeof(float)) != kTfLiteOk) {
        fprintf(stderr, "silver_detect: Could not copy input tensor\n");
        return 0.0f;
    }

    if(TfLiteInterpreterInvoke(interpreter) != kTfLiteOk) {
        fprintf(stderr, "silver_detect: Could not invoke\n");
        return 0.0f;
    }

    const TfLiteTensor *output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
    float silver_outputs[2];
    TfLiteTensorCopyToBuffer(output_tensor, silver_outputs, 2 * sizeof(float));

    int res = silver_outputs[0] > silver_outputs[1];

    /*if(res) {
        char path[64];
        sprintf(path, "/home/pi/silver/%lld.png", milliseconds());
        write_image(path, LINE_IMAGE_TO_PARAMS(frame_copy));
    }*/

    return silver_outputs[0];
}
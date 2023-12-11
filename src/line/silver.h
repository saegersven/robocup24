#pragma once

#include "line_private.h"

#include <stdint.h>
#include <pthread.h>

#include "../libs/tensorflow/lite/c/c_api.h"

#include "utils.h"

#define SILVER_MODEL_PATH "/home/pi/robocup24/runtime_data/silver.tflite"

#define WIDTH 80
#define HEIGHT 48

static TfLiteModel *model;
static TfLiteInterpreterOptions *options;
static TfLiteInterpreter *interpreter;

static float silver_outputs[2];
static int stop_signal;

static pthread_t silver_thread_id;

void silver_loop() {
    pthread_detach(pthread_self());

    while(!stop_signal) {
        TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor(interpreter, 0);

        // Input to model is grayscale
        float input_image_norm[WIDTH * HEIGHT];
        for(int i = 0; i < HEIGHT; i++) {
            for(int j = 0; j < WIDTH; j++) {
                int idx = i * WIDTH + j;

                for(int k = 0; k < 3; k++) {
                    input_image_norm[idx] += (float)frame[idx];
                }
                input_image_norm[idx] /= 3.0f;
            }
        }

        if(TfLiteTensorCopyFromBuffer(input_tensor, input_image_norm, WIDTH * HEIGHT * sizeof(float)) != kTfLiteOk) {
            fprintf(stderr, "silver_detect: Could not copy input tensor\n");
            return;
        }

        TfLiteInterpreterInvoke(interpreter);

        const TfLiteTensor *output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
        TfLiteTensorCopyToBuffer(output_tensor, silver_outputs, 2 * sizeof(float));
    }
}

void silver_init() {
    model = TfLiteModelCreateFromFile(SILVER_MODEL_PATH);
    options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options, 1);

    interpreter = TfLiteInterpreterCreate(model, options);

    if(TfLiteInterpreterAllocateTensors(interpreter) != kTfLiteOk) {
        fprintf(stderr, "silver_init: Could not allocate tensors\n");
        return;
    }

    stop_signal = 0;
    pthread_create(&silver_thread_id, NULL, silver_loop);
}

void silver_destroy() {
    stop_signal = 1;
    pthread_join(silver_thread_id, NULL);

    TfLiteInterpreterDelete(interpreter);
    TfLiteInterpreterOptionsDelete(options);
    TfLiteModelDelete(model);
}

int silver_get_current_prediction() {
    return silver_outputs[0] > silver_outputs[1];
}
#pragma once

#include "line_private.h"

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

static float silver_outputs[2];
static pthread_mutex_t silver_output_lock = PTHREAD_MUTEX_INITIALIZER;
static int stop_signal;

static pthread_t silver_thread_id;

#define SILVER_PAUSE_NUM_FRAMES 20

static int silver_pause_counter = 0;

void *silver_loop(void* args) {
    pthread_detach(pthread_self());

    model = TfLiteModelCreateFromFile(SILVER_MODEL_PATH);
    options = TfLiteInterpreterOptionsCreate();
    TfLiteInterpreterOptionsSetNumThreads(options, 1);

    interpreter = TfLiteInterpreterCreate(model, options);

    if(TfLiteInterpreterAllocateTensors(interpreter) != kTfLiteOk) {
        fprintf(stderr, "silver_init: Could not allocate tensors\n");
        return;
    }
    //printf("Created silver\n");

    clock_t start_t = clock();

    while(1) {
        if(stop_signal) break;

        TfLiteTensor *input_tensor = TfLiteInterpreterGetInputTensor(interpreter, 0);

        uint8_t frame_copy[LINE_FRAME_WIDTH * LINE_FRAME_HEIGHT * 3];
        memcpy(frame_copy, frame, sizeof(frame));

        // Input to model is grayscale
        float input_image_norm[WIDTH * HEIGHT];
        memset(input_image_norm, 0, sizeof(input_image_norm));
        for(int i = 0; i < HEIGHT; i++) {
            for(int j = 0; j < WIDTH; j++) {
                int idx = i * WIDTH + j;
                input_image_norm[idx] = 0.0f;
                for(int k = 0; k < 3; k++) {
                    input_image_norm[idx] += (float)frame_copy[3*idx + k];
                }
                input_image_norm[idx] /= 3.0f;
            }
        }

        if(TfLiteTensorCopyFromBuffer(input_tensor, input_image_norm, WIDTH * HEIGHT * sizeof(float)) != kTfLiteOk) {
            fprintf(stderr, "silver_detect: Could not copy input tensor\n");
            return NULL;
        }

        if(TfLiteInterpreterInvoke(interpreter) != kTfLiteOk) {
            fprintf(stderr, "silver_detect: Could not invoke\n");
            return NULL;
        }
        //printf("Invoke\n");

        const TfLiteTensor *output_tensor = TfLiteInterpreterGetOutputTensor(interpreter, 0);
        //pthread_mutex_lock(&silver_output_lock);
        TfLiteTensorCopyToBuffer(output_tensor, silver_outputs, 2 * sizeof(float));
        //pthread_mutex_unlock(&silver_output_lock);
    
        if(silver_outputs[0] > 0.5f) {
            char path[64];
            sprintf(path, "/home/pi/silver/%lld.png", milliseconds());
            write_image(path, LINE_IMAGE_TO_PARAMS(frame_copy));
        }

        clock_t now = clock();
        //printf("%f\n", ((double)now - (double)start_t)/CLOCKS_PER_SEC*1000);
        start_t = now;
    }

    //printf("Deleting Silver\n");
    TfLiteInterpreterDelete(interpreter);
    interpreter = NULL;
    TfLiteInterpreterOptionsDelete(options);
    options = NULL;
    TfLiteModelDelete(model);
    model = NULL;

    return NULL;
}

void silver_init() {
    stop_signal = 0;
    memset(silver_outputs, 0, sizeof(silver_outputs));
    pthread_create(&silver_thread_id, NULL, silver_loop, NULL);
}

void silver_destroy() {
    stop_signal = 1;
    delay(50);
    pthread_cancel(silver_thread_id);
    delay(20);
    //printf("Join: %d\n", pthread_join(silver_thread_id, NULL));
}

int line_silver() {
    //printf("Counter: %d\n", silver_pause_counter);
    if(silver_pause_counter > 0) {
        silver_pause_counter--;
        return 0;
    }

    if(num_green_pixels > 50) return 0;

    //pthread_mutex_lock(&silver_output_lock);
    int res = silver_outputs[0] > silver_outputs[1];
    //printf("Silver_outputs[0]: %f \n", silver_outputs[0]);
    //printf("%f\n", silver_outputs[0]);

#ifdef DISPLAY_ENABLE
    //display_set_number(NUMBER_SILVER_CONFIDENCE, silver_outputs[0]);
#endif
    //pthread_mutex_unlock(&silver_output_lock);

    if(res) {
        printf("NN detects entrance!\n");
        robot_stop();

        int dist = robot_distance_avg(DIST_FRONT, 20, 0.2f);

        printf("Silver check distance: %d\n", dist);
        if (dist > 2000 && dist > 500) {
            char path[64];
            sprintf(path, "/home/pi/silver_false_positives/%lld.png", milliseconds());
            write_image(path, LINE_IMAGE_TO_PARAMS(frame));

            // Disable silver for a few frames
            silver_pause_counter = SILVER_PAUSE_NUM_FRAMES;
            return 0;
        }
        robot_servo(SERVO_CAM, CAM_POS_DOWN2, false, false);
        delay(300);
        camera_grab_frame(frame, LINE_FRAME_WIDTH, LINE_FRAME_HEIGHT);

        robot_servo(SERVO_CAM, CAM_POS_DOWN, false, false);
        delay(300);

        // Thresholding in here as some images are required by multiple functions
        num_black_pixels = 0;
        image_threshold(LINE_IMAGE_TO_PARAMS_GRAY(black), LINE_IMAGE_TO_PARAMS(frame), &num_black_pixels, is_black);

        // Count pixels at the bottom to avoid far away shadows
        num_black_pixels = 0;
        for(int i = 15; i < LINE_FRAME_HEIGHT; i++) {
            for(int j = 0; j < LINE_FRAME_WIDTH; j++) {
                if(black[i * LINE_FRAME_WIDTH + j]) {
                    num_black_pixels++;
                }
            }
        }

        float black_percentage = (float)num_black_pixels/LINE_FRAME_WIDTH/(LINE_FRAME_HEIGHT - 15);
        printf("%f\n", black_percentage);
        if(black_percentage > 0.07f) {
            printf("Too many black pixels for evac zone.\n");
            
            // Disable silver for a few frames
            silver_pause_counter = SILVER_PAUSE_NUM_FRAMES;
            
            char path[64];
            sprintf(path, "/home/pi/silver_false_positives/%lld.png", milliseconds());
            write_image(path, LINE_IMAGE_TO_PARAMS(frame));
            return 0;
        } else {
            printf("Detected entrance!\n");
            return 1;
        }
    }
    return 0;
}
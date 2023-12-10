#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#include "camera.h"
#include "robot.h"
#include "vision.h"
#include "utils.h"
#include "line/line.h"
#include "rescue/rescue.h"
#include "display/display.h"

#define STATE_LINE 0
#define STATE_RESCUE 1

#define DISABLE_BUTTON_START

static int state;
static pthread_t main_thread_id;

int main_loop() {
    pthread_detach(pthread_self());

    printf("MAIN THREAD START\n");

    // Do all the setup here
    state = STATE_LINE;
    line_start();

    while(1) {
        if(state == STATE_LINE) {
            line();

            if(line_found_silver) {
                state = STATE_RESCUE;
                line_stop();
            }
        } else {
            rescue();
        }
    }
}

// Reset
void stop() {
    printf("MAIN THREAD CANCEL\n");
    pthread_cancel(&main_thread_id);
    pthread_join(main_thread_id, NULL);
    robot_stop();
    delay(100);
    robot_stop(); // Just to be sure
    delay(50);
    robot_stop();
    printf("STOP\n");

#ifdef DISPLAY_ENABLE
    display_set_mode(MODE_IDLE);
#endif
}

void quit() {
    stop();
    display_destroy();

    exit(0);
}

void sig_int_handler(int sig) {
    if(sig == SIGINT) {
        printf("SIGINT\n");
        quit();
    }
}

void button_loop(int button_value) {
    while((robot_button() ? 1 : 0) == (button_value ? 1 : 0)) {
#ifdef DISPLAY_ENABLE
        if(display_loop()) quit();
#endif
    }
}

int main() {
    if(signal(SIGINT, sig_int_handler) == SIG_ERR) {
        fprintf(stderr, "Can't catch SIGINT\n");
    }

    robot_init();

#ifdef DISPLAY_ENABLE
    display_create();
#endif

    while(1) {
#ifndef DISABLE_BUTTON_START
        button_loop(0);
        button_loop(1);
#endif

        pthread_create(&main_thread_id, NULL, main_loop, NULL);

        button_loop(0);

        stop();

        button_loop(1);
    }
}
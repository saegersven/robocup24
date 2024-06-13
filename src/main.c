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

//#define DISABLE_BUTTON_START
//#define RESCUE_START


static int state;
static pthread_t main_thread_id;

void *main_loop(void *arg) {
    pthread_detach(pthread_self());

    printf("MAIN THREAD START\n");

    // Do all the setup here
#ifndef RESCUE_START
    state = STATE_LINE;
    line_start();
#else
    state = STATE_RESCUE;
#endif

    while(1) {
        if(state == STATE_LINE) {
            int ret = line();
            if(ret == 1) {
                printf("Found silver\n");
                state = STATE_RESCUE;
                line_stop();
                delay(200);
            }
        } else {
            rescue();

            // Rescue finished
            state = STATE_LINE;
            line_start();
        }
    }
}

// Reset
void stop() {
    printf("MAIN THREAD CANCEL\n");
    pthread_cancel(main_thread_id);
    pthread_join(main_thread_id, NULL);

    robot_stop();

    // Cleanup a bit (hopefully works)
    if(state == STATE_LINE) {
        line_stop();
    } else if(state == STATE_RESCUE) {
        rescue_cleanup();
    }
    //robot_serial_close();
    //robot_serial_init();

    printf("STOP\n");

#ifdef DISPLAY_ENABLE
    display_set_mode(MODE_IDLE);
#endif
}

void quit() {
    stop();
    display_destroy();

    robot_led(0);

    exit(0);
}

void sig_int_handler(int sig) {
    if(sig == SIGINT) {
        printf("SIGINT\n");
        quit();
    }
}

void button_loop(int button_value, int idle) {
    while((robot_button() ? 1 : 0) == (button_value ? 1 : 0)) {
#ifdef DISPLAY_ENABLE
        if(display_loop()) quit();
        if(idle) {
            display_set_number(NUMBER_BAT_VOLTAGE, robot_sensor(BAT_VOLTAGE));
            delay(20);
        }
#endif
    }
}

int main() {
    if(signal(SIGINT, sig_int_handler) == SIG_ERR) {
        fprintf(stderr, "Can't catch SIGINT\n");
    }

    robot_init();

#ifdef DISPLAY_ENABLE
    display_create(0);
#endif

    while(1) {
#ifndef DISABLE_BUTTON_START
        robot_led(1);
        button_loop(0, 1);
        robot_led(0);
        button_loop(1, 1);
#endif

        pthread_create(&main_thread_id, NULL, main_loop, NULL);

        button_loop(0, 0);

        exit(1);

        stop();

        button_loop(1, 0);
    }
}
#include <stdio.h>
#include <pthread.h>

#include "camera.h"
#include "robot.h"
#include "vision.h"
#include "utils.h"
#include "line.h"
#include "rescue.h"

#define STATE_LINE 0
#define STATE_RESCUE 1

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

int main() {
    robot_init();

    while(1) {
        while(!robot_button());
        while(robot_button());

        pthread_create(&main_thread_id, NULL, main_loop, NULL);

        while(!robot_button());

        // Reset, definitely leaks memory, but has worked last year so whatever
        printf("MAIN THREAD CANCEL\n");
        pthread_cancel(&main_thread_id);
        delay(100);

        // Try to clean up the mess
        robot_stop();

        while(robot_button());
    }
}
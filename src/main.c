#include <stdio.h>

#include "camera.h"
#include "robot.h"
#include "vision.h"
#include "utils.h"

int main() {
    printf("Hello, world!");

    // Test camera
    camera_start_capture(80, 48);
    create_window();

    Image frame;
    while(1) {
        frame = camera_grab_frame();

        show_image(frame);
    }

    destroy_window();
}
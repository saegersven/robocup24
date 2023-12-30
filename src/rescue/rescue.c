#include "rescue.h"

#include <stdio.h>

#include "../vision.h"
#include "../camera.h"
//#include "../thresholding.h"
#include "../utils.h"
#include "../robot.h"

#include "victims.h"

static DECLARE_S_IMAGE(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3);

void rescue_collect_victim() {
	// DO THE COLLECTING THING
	robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);
    robot_servo(SERVO_ARM, ARM_POS_HALF_DOWN, false, false);
    delay(800);
    robot_drive(70, 70, 0);
    delay(100);
    robot_servo(SERVO_ARM, ARM_POS_DOWN, true, true);
    delay(200);
    robot_stop();
    robot_servo(SERVO_ARM, ARM_POS_DOWN, false, true);
    robot_drive(-70, -70, 200);
    robot_drive(70, 70, 200);
    robot_servo(SERVO_STRING, STRING_POS_CLOSED, false, false);
    robot_servo(SERVO_ARM, ARM_POS_UP, false, false);
    robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);
    delay(500);
}

// Returns 0 if no victims was found, 1 if victim is alive and 2 if victim is dead
int rescue_collect(int find_dead) {
	display_set_number(NUMBER_RESCUE_OBJECTIVE, RESCUE_OBJECTIVE_VICTIM);

	struct Victim victim;
	
	float APPROACH_K_P = 0.1f;
	int APPROACH_BASE_SPEED = 40;

	float CAM_K_P = 1.5f;
	int CAM_COLLECT_POS = CAM_POS_DOWN - 15;
	int cam_angle = CAM_POS_UP;

	while(1) {
		robot_stop();
		delay(300);
		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
		delay(100);

		if(victims_find(frame, find_dead, &victim)) {
			display_set_number(NUMBER_RESCUE_POS_X, victim.x);
			display_set_number(NUMBER_RESCUE_POS_Y, victim.y);
			display_set_number(NUMBER_RESCUE_IS_DEAD, victim.dead);

			float angle_horizontal = (victim.x - 0.5f) * CAM_HORIZONTAL_FOV;
			printf("Turning %f\n °\n", RTOD(angle_horizontal));
			robot_turn(angle_horizontal * 0.2f); // TEMP FACTOR

			float angle_vertical = (victim.y - 0.5f) * CAM_VERTICAL_FOV;
			float u_cam = angle_vertical * CAM_K_P;
			cam_angle += RTOD(u_cam);
			if(cam_angle < CAM_POS_UP) cam_angle = CAM_POS_UP;
			if(cam_angle > CAM_COLLECT_POS) cam_angle = CAM_COLLECT_POS;
			robot_servo(SERVO_CAM, cam_angle, false, false);
			delay(300);

			float alpha = (float)CAM_POS_DOWN - (float)cam_angle;
			float dist = sinf(DTOR(alpha) - angle_vertical);
			printf("%f\n", dist);

			if(dist < 0.2f) {
				robot_drive(-60, -60, 130);
			} else if(dist > 0.4f) {
				robot_drive(80, 80, 200);
			} else {
				robot_stop();
				delay(100);
				robot_turn(angle_horizontal + DTOR(22.0f));

				robot_drive(-60, -60, 150);
				rescue_collect_victim();

				return victim.dead ? 2 : 1;
			}

			/*if(cam_angle >= CAM_COLLECT_POS && angle_vertical > DTOR(20.0f)) {
				robot_stop();
				delay(100);
				robot_turn(angle_horizontal - DTOR(20.0f));
				// Collect victim
				rescue_collect_victim();

				return victim.dead ? 2 : 1;
			}*/
			//robot_drive(80, 80, 250);
			delay(100);
		} else {
			display_set_number(NUMBER_RESCUE_POS_X, 0.0f);
			display_set_number(NUMBER_RESCUE_POS_Y, 0.0f);

			robot_turn(DTOR(30.0f));
		}
	}
}

void rescue_drop_victim() {
	delay(200);
    robot_servo(SERVO_STRING, STRING_POS_OPEN, true, true);
    delay(500);
    robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);
    robot_servo(SERVO_ARM, ARM_POS_HALF_DOWN, true, true);
    delay(1000);
    robot_servo(SERVO_STRING, STRING_POS_CLOSED, true, true);
    delay(400);
    robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);
    delay(400);
    robot_servo(SERVO_ARM, ARM_POS_UP, true, true);
    delay(250);
    robot_servo(SERVO_ARM, ARM_POS_HALF_DOWN, true, true);
    delay(250);
    robot_servo(SERVO_ARM, ARM_POS_UP, true, true);
    delay(250);
    robot_servo(SERVO_ARM, ARM_POS_HALF_DOWN, true, true);
    delay(250);
    robot_servo(SERVO_ARM, ARM_POS_UP, false, false);
    delay(600);
}

void rescue_deliver(int is_dead) {
	display_set_number(NUMBER_RESCUE_OBJECTIVE, RESCUE_OBJECTIVE_CORNER);
}

void rescue() {
	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);

	robot_servo(SERVO_CAM, CAM_POS_UP, false, false);
	robot_servo(SERVO_ARM, ARM_POS_UP, false, false);
	robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);

	display_set_mode(MODE_RESCUE);
	display_set_image(IMAGE_RESCUE_FRAME, frame);

	victims_init();

	int num_victims = 0;

	while(num_victims < 3) {
		display_set_number(NUMBER_RESCUE_NUM_VICTIMS, num_victims);

		int ret = rescue_collect(num_victims < 2);
		rescue_deliver(ret == 2);
		
		num_victims++;
	}

	rescue_cleanup();
}

void rescue_cleanup() {
	victims_destroy();
	camera_stop_capture();
	delay(100);
}
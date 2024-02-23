#include "rescue.h"

#include <stdio.h>
#include <math.h>

#include "../vision.h"
#include "../camera.h"
//#include "../thresholding.h"
#include "../utils.h"
#include "../robot.h"

#include "victims.h"
#include "corner.h"

static DECLARE_S_IMAGE(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3);

void rescue_find_center() {
	float x = robot_distance_avg(DIST_FRONT, 10, 4) - 500;
	float y = (robot_distance_avg(DIST_RIGHT_FRONT, 10, 4) + robot_distance_avg(DIST_RIGHT_REAR, 10, 4)) / 2.0f - 500;

	//printf("%f, %f\n", x, y);

	float angle = atan2f(y, x);
	robot_turn(angle);

	float mag = sqrtf(x*x + y*y);
	robot_drive(100, 100, (int)mag);
}

void rescue_collect_victim() {
	// DO THE COLLECTING THING
	robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);
    robot_servo(SERVO_ARM, ARM_POS_HALF_DOWN, false, false);
    delay(800);
    printf("Driving forward\n");
    robot_drive(70, 70, 0);
    delay(200);
    robot_servo(SERVO_ARM, ARM_POS_DOWN, true, true);
    delay(100);
    robot_stop();
    delay(300);
    robot_servo(SERVO_ARM, ARM_POS_DOWN, true, true);
    robot_drive(-70, -70, 200);
    robot_drive(70, 70, 200);
    delay(200);
    robot_servo(SERVO_STRING, STRING_POS_CLOSED, false, false);
    delay(450);
    robot_drive(-70, -70, 600);
    robot_servo(SERVO_ARM, ARM_POS_UP, false, false);
    delay(400);
}

// Returns 0 if no victim was found, 1 if victim is alive and 2 if victim is dead
int rescue_collect(int find_dead) {
	display_set_number(NUMBER_RESCUE_OBJECTIVE, RESCUE_OBJECTIVE_VICTIM);

	struct Victim victim;

	int cam_angle = CAM_POS_UP;
	robot_servo(SERVO_CAM, cam_angle, false, false);

	int final_approach = 0;

	int turn_counter = 0;
	while(1) {
		robot_stop();
		camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
		camera_stop_capture();
		printf("Frame grabbed\n");
		int ret = victims_find(frame, find_dead, &victim);
		printf("%d\n", ret);
		delay(10);

		if(ret) {
			display_set_number(NUMBER_RESCUE_POS_X, victim.x);
			display_set_number(NUMBER_RESCUE_POS_Y, victim.y);
			display_set_number(NUMBER_RESCUE_IS_DEAD, victim.dead + 1);

			float angle_horizontal = (victim.x - 0.5f) * CAM_HORIZONTAL_FOV;
			robot_turn(angle_horizontal);

			printf("Victim at (%f, %f)\n", victim.x, victim.y);

			if(final_approach) {
				int duration = (0.5f - victim.y) * 500;
				robot_drive(50, 50, duration);

				if(victim.x > 0.45f && victim.x < 0.55f
					&& victim.y > 0.42f && victim.y < 0.58f) {
					robot_turn(DTOR(30.0f));
					delay(50);
					robot_drive(-60, -60, 180);
					delay(50);
					rescue_collect_victim();
					return victim.dead + 1;
				}
			} else {
				float angle_vertical = (victim.y - 0.5f) * CAM_VERTICAL_FOV;
				printf("Angle vertical: %f\n", RTOD(angle_vertical));

				float alpha = DTOR(cam_angle - CAM_POS_HORIZONTAL) + angle_vertical;
				if(alpha < 0.1f) alpha = 0.1f;
				float dist = CAM_HEIGHT_MM / alpha;
				int duration = dist * 0.6f;
				printf("Duration: %d\n", duration);
				robot_drive(80, 80, duration);


				float beta = atanf(CAM_HEIGHT_MM / dist);
				if(dist < 400.0f) beta = 0.0f;
				printf("Beta: %f\n", beta);
				cam_angle += RTOD(angle_vertical + beta);

				printf("New cam angle: %d\n", cam_angle);

				robot_servo(SERVO_CAM, cam_angle, false, false);

				if(cam_angle > 100 && dist < 105.0f) {
					printf("Final approach\n");
					cam_angle = 120;
					robot_servo(SERVO_CAM, cam_angle, false, false);
					final_approach = 1;
				}
			}
		} else {
			display_set_number(NUMBER_RESCUE_POS_X, 0.0f);
			display_set_number(NUMBER_RESCUE_POS_Y, 0.0f);
			display_set_number(NUMBER_RESCUE_IS_DEAD, 0.0f);

			cam_angle = CAM_POS_UP;
			robot_servo(SERVO_CAM, cam_angle, false, false);
			final_approach = 0;

			robot_turn(DTOR(30.0f));
			turn_counter++;
			delay(250);
			if(turn_counter == 12) {
				rescue_find_center();
				rescue_find_center();
				turn_counter = 0;
			}
		}
	}
}

int rescue_collect_test(int find_dead) {
	display_set_number(NUMBER_RESCUE_OBJECTIVE, RESCUE_OBJECTIVE_VICTIM);

	struct Victim victim;

	int cam_angle = CAM_POS_UP;
	robot_servo(SERVO_CAM, cam_angle, false, false);

	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);

	while(1) {
		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);

		int ret = victims_find(frame, find_dead, &victim);
		
		if(ret) {
			display_set_number(NUMBER_RESCUE_POS_X, victim.x);
			display_set_number(NUMBER_RESCUE_POS_Y, victim.y);
			display_set_number(NUMBER_RESCUE_IS_DEAD, victim.dead + 1);

			float angle_horizontal = (victim.x - 0.5f) * CAM_HORIZONTAL_FOV;
			
			float u = 20 * angle_horizontal;
			robot_drive(30 + u, 30 - u, 0);

			float angle_vertical = (victim.y - 0.5f) * CAM_VERTICAL_FOV;
			float v = 10 * angle_vertical;
			cam_angle += v;
			robot_servo(SERVO_CAM, cam_angle, false, false);
		} else {
			display_set_number(NUMBER_RESCUE_POS_X, 0.0f);
			display_set_number(NUMBER_RESCUE_POS_Y, 0.0f);
			display_set_number(NUMBER_RESCUE_IS_DEAD, 0.0f);

			cam_angle = CAM_POS_UP;
			robot_servo(SERVO_CAM, cam_angle, false, false);

			robot_turn(DTOR(30.0f));
			delay(250);
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

	robot_servo(SERVO_CAM, CAM_POS_UP, false, false);

	int SLOW_TURN_SPEED = 35;
	int FAST_TURN_SPEED = 40;

	int turn_speed = FAST_TURN_SPEED;

	long total_time = 8000;
	long max_time = 10000;

	float x_corner = 0.0f;
	float last_x_corner = 0.0f;
	
	delay(300);
	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
	for(int i = 0; i < 5; i++) camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
	camera_stop_capture();
	delay(200);
	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);

	int turn_counter = 0;
	while(1) {
		robot_turn(DTOR(20.0f));
		delay(250);

		//camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
		//camera_stop_capture();

		delay(50);

		float x = 0.0f;
		if(corner_detect(frame, &x, !is_dead)) {
			printf("Corner pos: %f\n", x);

			robot_stop();
			delay(50);

			robot_turn(x * DTOR(65.0f) + DTOR(1.0f));

			int alignment_successful = 1;
			// Align for final approach
			for(int i = 0; i < 6; i++) {
				camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
				delay(50);

				if(!corner_detect(frame, &x, !is_dead)) {
					if(i < 3) {
						printf("Lost corner");
						robot_turn(-DTOR(20.0f));
						alignment_successful = 0;
					}
					x = 0.0f;
					break;
				}

				robot_turn(x * DTOR(65.0f) / (i + 1) + DTOR(1.0f));
				robot_drive(45, 45, 700);
				delay(80);
			}

			if(!alignment_successful) continue;

			// Robot is aligned with corner, approach

			/*
			robot_drive(40, 40, 1600);
			robot_drive(-80, -80, 200);
			robot_turn(DTOR(25.0f));
			robot_drive(40, 40, 650);
			*/
			robot_drive(-100, 0, 250);
			robot_drive(0, -100, 350);
			robot_drive(50, 50, 850);

			// Drop victim
			rescue_drop_victim();

			// Go back to center
			robot_drive(-100, -100, 600);
			robot_turn(DTOR(45.0f)); // face away from corner to avoid detecting rescued victims (just to be safe)

			camera_stop_capture();
			return;
		}
		last_x_corner = x_corner;

		turn_counter++;

		if(turn_counter == 18) {
			rescue_find_center();
			rescue_find_center();

			turn_counter = 0;
		}
	}
}

void rescue() {
	display_set_mode(MODE_RESCUE);
	display_set_image(IMAGE_RESCUE_FRAME, frame);
	display_set_image(IMAGE_RESCUE_THRESHOLD, corner_thresh);

	robot_drive(127, 127, 800);

	int dist = robot_distance_avg(DIST_RIGHT_FRONT, 10, 2);
	printf("Dist right front: %d\n", dist);
	if(dist < 400) {
		robot_turn(DTOR(130.0f));
		robot_drive(-100, -100, 800);
		robot_turn(DTOR(100.0f));
	} else {
		robot_turn(DTOR(-130.0f));
		robot_drive(-100, -100, 800);
		robot_turn(DTOR(-100.0f));
	}

	//camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);

	robot_servo(SERVO_CAM, CAM_POS_UP, false, false);
	robot_servo(SERVO_ARM, ARM_POS_UP, false, false);
	robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);

	victims_init();
	corner_init();

	display_set_number(NUMBER_RESCUE_OBJECTIVE, RESCUE_OBJECTIVE_VICTIM);

	int num_victims = 0;

	while(num_victims < 8) {
		display_set_number(NUMBER_RESCUE_NUM_VICTIMS, num_victims);

		int ret = rescue_collect(num_victims >= 2);

		if(ret) {
			rescue_deliver(ret == 2);
		} else {
			rescue_find_center();
			rescue_find_center();
		}
		
		num_victims++;
	} 

	rescue_cleanup();
}

void rescue_cleanup() {
	victims_destroy();
	corner_destroy();
	camera_stop_capture();
	delay(100);
}
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

void rescue_collect_victim() {
	// DO THE COLLECTING THING
	robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);
    robot_servo(SERVO_ARM, ARM_POS_HALF_DOWN, false, false);
    delay(800);
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
    delay(500);
}

// Returns 0 if no victim was found, 1 if victim is alive and 2 if victim is dead
int rescue_collect(int find_dead) {
	display_set_number(NUMBER_RESCUE_OBJECTIVE, RESCUE_OBJECTIVE_VICTIM);

	struct Victim victim;

	int cam_angle = CAM_POS_UP;
	robot_servo(SERVO_CAM, cam_angle, false, false);

	int final_approach = 0;

	while(1) {
		robot_stop();
		
		// Restart camera for exposure adjustment
		camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
		camera_stop_capture();

		if(victims_find(frame, find_dead, &victim)) {
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
					robot_drive(-60, -60, 200);
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
				if(dist < 600.0f) beta = 0.0f;
				printf("Beta: %f\n", beta);
				cam_angle += RTOD(angle_vertical + beta);

				printf("New cam angle: %d\n", cam_angle);

				robot_servo(SERVO_CAM, cam_angle, false, false);

				if(cam_angle > 105 && dist < 110.0f) {
					printf("Final approach\n");

					// Fixed cam angle from here on
					cam_angle = 120;
					robot_servo(SERVO_CAM, cam_angle, false, false);
					final_approach = 1;
				}
			}
		} else {
			cam_angle = CAM_POS_UP;
			robot_servo(SERVO_CAM, cam_angle, false, false);

			display_set_number(NUMBER_RESCUE_POS_X, 0.0f);
			display_set_number(NUMBER_RESCUE_POS_Y, 0.0f);
			display_set_number(NUMBER_RESCUE_IS_DEAD, 0.0f);

			robot_turn(DTOR(30.0f));
			delay(200);
		}
	}
}

void rescue_drop_victim() {
	delay(200);
    robot_servo(SERVO_STRING, STRING_POS_OPEN, true, true);
    delay(500);
    robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);
    robot_servo(SERVO_ARM, ARM_POS_HALF_DOWN, true, true);
    delay(800);
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
    delay(300);
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
	
	delay(50);
	// For exposure adjustment, does not always work
	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
	for(int i = 0; i < 10; i++) camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
	camera_stop_capture();
	delay(100);
	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
	delay(50);
	while(1) {
		robot_turn(DTOR(25.0f));
		delay(100);

		//camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
		//camera_stop_capture();

		delay(50);

		float x = 0.0f;
		if(corner_detect_classic(frame, &x, !is_dead)) {
			printf("Corner pos: %f\n", x);

			if(fabsf(x) < 0.2f || x <= 0.0f && last_x_corner > 0.0f) {
				robot_stop();
				delay(50);

				robot_turn(-DTOR(10.0f));

				int alignment_successful = 1;
				// Align for final approach
				for(int i = 0; i < 6; i++) {
					camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
					delay(50);

					if(!corner_detect_classic(frame, &x, !is_dead)) {
						printf("Lost corner");
						robot_turn(-DTOR(20.0f));
						alignment_successful = 0;
						break;
					}

					robot_turn(x * DTOR(65.0f) / (i + 1) + DTOR(2.0f));
					robot_drive(45, 45, 700);
					delay(80);
				}

				if(!alignment_successful) continue;

				// Robot is aligned with corner, approach

				robot_drive(40, 40, 1600);
				robot_drive(-80, -80, 200);
				robot_turn(DTOR(25.0f));
				robot_drive(40, 40, 650);

				// Drop victim
				rescue_drop_victim();

				// Go back to center
				robot_drive(-100, -100, 600);
				robot_turn(DTOR(45.0f)); // face away from corner to avoid detecting rescued victims (just to be safe)

				camera_stop_capture();
				return;
			}
		}
		last_x_corner = x_corner;
	}
}

void rescue_reposition() {
	int dist_front = robot_distance_avg(DIST_FRONT, 10, 4);
	delay(30);
	int dist_right_front = robot_distance_avg(DIST_RIGHT_FRONT, 10, 4);
	delay(30);
	int dist_right_rear = robot_distance_avg(DIST_RIGHT_REAR, 10, 4);

	float x = (dist_right_front + dist_right_rear) / 2.0f - 500;
	float y = dist_front - 500;

	float angle = atan2f(x, y);
	float mag = sqrtf(x*x + y*y);

	robot_turn(angle);
	robot_drive(100, 100, mag);
}

void rescue() {
	display_set_mode(MODE_RESCUE);
	display_set_image(IMAGE_RESCUE_FRAME, frame);

	/*robot_drive(100, 100, 600);

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
	}*/

	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);

	robot_servo(SERVO_CAM, CAM_POS_UP, false, false);
	robot_servo(SERVO_ARM, ARM_POS_UP, false, false);
	robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);

	victims_init();
	corner_init();

	int num_victims = 0;

	while(num_victims < 3) {
		display_set_number(NUMBER_RESCUE_NUM_VICTIMS, num_victims);

		int ret = rescue_collect(num_victims >= 2);

		if(ret) {
			rescue_deliver(ret == 2);
		} else {
			// Re-position
			// TODO
			rescue_reposition();
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
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
    delay(150);
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
    robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);
    delay(500);
}

// Returns 0 if no victim was found, 1 if victim is alive and 2 if victim is dead
int rescue_collect(int find_dead) {
	display_set_number(NUMBER_RESCUE_OBJECTIVE, RESCUE_OBJECTIVE_VICTIM);

	struct Victim victim;
	
	float APPROACH_K_P = 0.1f;
	int APPROACH_BASE_SPEED = 40;

	float CAM_K_P = 1.5f;
	int CAM_COLLECT_POS = CAM_POS_DOWN - 15;
	int cam_angle = CAM_POS_UP;

	int dist_mode = 1;

	while(1) {
		robot_stop();
		delay(300);
		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
		delay(100);

		if(victims_find(frame, find_dead, &victim)) {
			display_set_number(NUMBER_RESCUE_POS_X, victim.x);
			display_set_number(NUMBER_RESCUE_POS_Y, victim.y);
			display_set_number(NUMBER_RESCUE_IS_DEAD, victim.dead + 1);

			float angle_horizontal = (victim.x - 0.5f) * CAM_HORIZONTAL_FOV;
			robot_turn(angle_horizontal); // TEMP FACTOR

			if(dist_mode) {
				float angle_vertical = (victim.y - 0.5f) * CAM_VERTICAL_FOV;
				float u_cam = angle_vertical * CAM_K_P;
				cam_angle += RTOD(u_cam);
				if(cam_angle < CAM_POS_UP) cam_angle = CAM_POS_UP;
				if(cam_angle > CAM_COLLECT_POS) cam_angle = CAM_COLLECT_POS;
				robot_servo(SERVO_CAM, cam_angle, false, false);
				delay(100);

				float alpha = (float)CAM_POS_DOWN + 5 - (float)cam_angle;
				float dist = tanf(DTOR(alpha) - angle_vertical);
				if(DTOR(alpha) - angle_vertical > DTOR(90.0f)) {
					dist = 30.0f;
				}

				printf("%f\n", alpha);

				printf("Dist: %f\n", dist);
				int dur = clamp((dist - 0.4f) * 300, -150, 700);
				printf("Driving: %d\n", dur);

				robot_drive(60, 60, dur);
				cam_angle += 5;
				robot_servo(SERVO_CAM, cam_angle, false, false);
				delay(50);

				if(dist > 0.3f && dist < 0.5f) {
					dist_mode = 0;
					robot_servo(SERVO_CAM, CAM_POS_DOWN3, false, false);
					delay(200);
				}
			} else {
				if(victim.y > 0.5f && victim.y < 0.6f) {
					robot_stop();
					delay(100);
					robot_turn(angle_horizontal + DTOR(22.0f));

					//robot_drive(-60, -60, 150);
					rescue_collect_victim();

					return victim.dead ? 2 : 1;
				} else {
					int dur = (0.55f - victim.y) * 500;
					if(dur < 0) dur = clamp(dur, -150, -50);
					if(dur > 0) dur = clamp(dur, 50, 150);
					robot_drive(50, 50, dur);
				}
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
			display_set_number(NUMBER_RESCUE_IS_DEAD, 0.0f);

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

	robot_servo(SERVO_CAM, CAM_POS_UP, false, false);

	int SLOW_TURN_SPEED = 35;
	int FAST_TURN_SPEED = 50;

	int turn_speed = FAST_TURN_SPEED;

	long total_time = 8000;
	long max_time = 10000;

	float x_corner = 0.0f;
	float last_x_corner = 0.0f;
	
	while(1) {
		robot_drive(turn_speed, -turn_speed, 0);

		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);

		float x = 0.0f;
		if(corner_detect(frame, &x, !is_dead)) {
			printf("Corner pos: %f\n", x);

			if(fabsf(x) < 0.2f || x <= 0.0f && last_x_corner > 0.0f) {
				robot_stop();
				delay(50);

				robot_turn(-DTOR(20.0f));

				int alignment_successful = 1;
				// Align for final approach
				for(int i = 0; i < 3; i++) {
					camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_CAPTURE_HEIGHT);

					if(!corner_detect(frame, &x, !is_dead)) {
						printf("Lost corner");
						robot_turn(-DTOR(20.0f));
						alignment_successful = 0;
						break;
					}

					robot_turn(x * DTOR(65.0f) / (i + 1));
				}

				if(!alignment_successful) continue;

				// Robot is aligned with corner, approach
				robot_drive(100, 100, 1200);
				robot_drive(40, 40, 2600);

				// Drop victim
				rescue_drop_victim();

				// Go back to center
				robot_drive(-100, -100, 1500);
				robot_turn(DTOR(45.0f)); // face away from corner to avoid detecting rescued victims (just to be safe)

				return;
			}
		}
		last_x_corner = x_corner;
	}
}

void rescue() {
	if(robot_distance_avg(DIST_RIGHT_FRONT, 10, 2) < 400) {
		robot_turn(DTOR(130.0f));
		robot_drive(-100, -100, 800);
		robot_turn(DTOR(100.0f));
	} else {
		robot_turn(DTOR(-130.0f));
		robot_drive(-100, -100, 800);
		robot_turn(DTOR(-100.0f));
	}

	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);

	robot_servo(SERVO_CAM, CAM_POS_UP, false, false);
	robot_servo(SERVO_ARM, ARM_POS_UP, false, false);
	robot_servo(SERVO_STRING, STRING_POS_OPEN, false, false);

	display_set_mode(MODE_RESCUE);
	display_set_image(IMAGE_RESCUE_FRAME, frame);

	victims_init();
	corner_init();

	int num_victims = 0;

	while(num_victims < 3) {
		display_set_number(NUMBER_RESCUE_NUM_VICTIMS, num_victims);

		int ret = rescue_collect(num_victims > 2);

		if(ret) {
			rescue_deliver(ret == 2);
		} else {
			// Re-position
			// TODO
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
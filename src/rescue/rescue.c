#include "rescue.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#include "../vision.h"
#include "../camera.h"
#include "../thresholding.h"
#include "../utils.h"
#include "../robot.h"

#include "victims.h"
#include "corner.h"

static DECLARE_S_IMAGE(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3);

// accurate flag uses rescue_reposition for improved accuracy
void rescue_find_center(bool accurate) {
	const int MAX_TIME = 2500;

	for(int i = 0; i < 2; i++) {
		long long start_time = milliseconds();

		while(milliseconds() - start_time < MAX_TIME) {
			int dist = robot_sensor(DIST_FRONT);
			delay(30);
			if(dist > 800 && dist < 1100) {
				if(i == 1) {
					robot_drive(127, 35, 0);
				} else {
					robot_drive(35, 127, 0);
				}
			} else if (dist < 500) {
				if(i == 1) {
					robot_drive(-35, -127, 0);
				} else {
					robot_drive(-127, -35, 0);
				}
			} else {
				if(i == 1) {
					robot_drive(60, -60, 0);
				} else {
					robot_drive(-60, 60, 0);
				}
			}
		}
	}
	robot_stop();

	if (accurate) rescue_reposition();
}

void rescue_reposition() {
	int dist_front = robot_distance_avg(DIST_FRONT, 10, 3);
	int dist_right_front = robot_distance_avg(DIST_RIGHT_FRONT, 10, 3);
	int dist_right_rear = robot_distance_avg(DIST_RIGHT_REAR, 10, 3);

	// exit somewhere, turn a bit and try again
	if ((dist_front > 1000) | (dist_right_front > 1000) | (dist_right_rear > 1000)) {
		robot_drive(100, -100, 73);
		rescue_reposition();
		return;
	}

	float x = (dist_right_front + dist_right_rear) / 2.0f - 500;
	float y = dist_front - 500;

	float angle = atan2f(x, y);
	float mag = sqrtf(x*x + y*y);

	robot_turn(angle);
	robot_drive(100, 100, mag * 2);
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
	int none_found_counter = 0;

	float classification_accumulator = 0.0f;
	int num_classifications = 0;
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
			num_classifications++;
			classification_accumulator += victim.dead;
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
					return (int)roundf(classification_accumulator / num_classifications) + 1;
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

				if(cam_angle > 110 && dist < 105.0f) {
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

			num_classifications = 0;
			classification_accumulator = 0.0f;

			cam_angle = CAM_POS_UP;
			robot_servo(SERVO_CAM, cam_angle, false, false);
			final_approach = 0;

			robot_turn(DTOR(30.0f));
			turn_counter++;
			delay(250);
			if(turn_counter == 12) {
				if(none_found_counter == 1) {
					find_dead = 1;
				}
				rescue_find_center(0);
				none_found_counter++;
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

				robot_turn(x * DTOR(65.0f) + DTOR(1.0f));
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
			robot_drive(50, 50, 950);

			// Drop victim
			rescue_drop_victim();

			// Go back to center
			robot_drive(-100, -100, 600);
			robot_turn(DTOR(45.0f)); // face away from corner to avoid detecting rescued victims (just to be safe)

			camera_stop_capture();
			return;
		}

		turn_counter++;

		if(turn_counter == 18) {
			rescue_find_center(0);

			turn_counter = 0;
		}
	}
}

// Counts red/green pixels
bool rescue_is_corner() {
	robot_servo(SERVO_CAM, 0.4 * (CAM_POS_UP + CAM_POS_DOWN), true, false);
	delay(300);

	const int NUM_PIXELS_THRESHOLD = 420;

	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
	camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
	camera_stop_capture();

	int green_pixels = image_count_pixels(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3, is_green);
	int red_pixels = image_count_pixels(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3, is_red);

	printf("Num green pixels: %d \t Red pixels: %d \n", green_pixels, red_pixels);

	return green_pixels > NUM_PIXELS_THRESHOLD || red_pixels > NUM_PIXELS_THRESHOLD;
}

// Lets hope num black pixels is enough
bool rescue_is_exit() {
	const int NUM_PIXELS_THRESHOLD = 35000;

	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
	
	int max_num_pixels = 0;

	robot_drive(60, 60, 0);
	long long start_time = milliseconds();
	while(milliseconds() - start_time < 600) {
		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);



		int num_pixels = image_count_pixels(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3, is_black);
		if(num_pixels > max_num_pixels) max_num_pixels = num_pixels;
	}
	robot_stop();
	camera_stop_capture();

	printf("Max num pixels: %d\n", max_num_pixels);
	delay(1000);

	return max_num_pixels > NUM_PIXELS_THRESHOLD;
}

// returns number of black pixels in current frame
int pixels_black() {
	robot_servo(SERVO_CAM, CAM_POS_DOWN - 10, true, false);
	delay(300);

	camera_start_capture(RESCUE_CAPTURE_WIDTH, RESCUE_CAPTURE_HEIGHT);
	camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);
	camera_stop_capture();
	printf("Black pixels: %d \n", image_count_pixels(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3, is_black));
    return image_count_pixels(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3, is_black);
}

void rescue_find_exit() {
	robot_drive(-100, -100, 200);
	robot_turn(DTOR(-90.0f));
	robot_drive(50, 50, 0);
	while (robot_sensor(DIST_FRONT) > 200);
	robot_turn(DTOR(135.0f));
	robot_drive(-100, -100, 350);
	robot_turn(DTOR(-180.0f));
	robot_drive(-100, -100, 200);

	bool already_checked_for_corner = false;
	long long side_exit_cooldown = 0; // after side exit check, don't check again for some time
	while (1) {
		int front_dist = robot_sensor(DIST_FRONT);
		int side_dist = robot_sensor(DIST_RIGHT_FRONT);

		// --- START OF WALLFOLLOWER LOGIC ---
		const int SPEED = 50;
		const int TARGET_DIST_SIDE = 60;   // Target distance for the side sensor
		const float KP = 0.3f;
		const float KD = 2.0f;

		// PD controller for side distance
		int error = TARGET_DIST_SIDE - side_dist;
		static int prev_error = 0;
		int derivative = error - prev_error;
		prev_error = error;
		int speed_adjustment = KP * error + KD * derivative;

		int left_speed = SPEED - speed_adjustment;
		int right_speed = SPEED + speed_adjustment;

		left_speed = (left_speed > 100) ? 100 : ((left_speed < -100) ? -100 : left_speed);
		right_speed = (right_speed > 100) ? 100 : ((right_speed < -100) ? -100 : right_speed);

		// Only drive with calculates speeds when there is no exit on side
		if (side_dist < 150) robot_drive(left_speed, right_speed, 0);
		else robot_drive(50, 50, 0);
		// --- END OF WALLFOLLOWER LOGIC ---

		// There are 4 different cases:

		// 1) 35cm in front of wall, check for corner
		// 2) 10cm in front of wall, turn 90°
		// 3) potential exit front
		// 4) potential exit side

		// 1. case
		if (!already_checked_for_corner &&
			front_dist < 340
			&& robot_stop()
			&& robot_distance_avg(DIST_FRONT, 5, 1) < 360) {

			printf("Case 1\n");
			if (rescue_is_corner()) {
				robot_turn(DTOR(135.0f));
				robot_drive(-100, -100, 300);
				robot_turn(DTOR(-180.0f));
				robot_drive(50, 50, 0);
				while (robot_sensor(DIST_FRONT) > 200);
				robot_turn(DTOR(135.0f));
				robot_drive(-100, -100, 400);
				robot_turn(DTOR(-180.0f));
				robot_drive(-100, -100, 200);
			} else {
				already_checked_for_corner = true;
			}
		}

		// 2. case
		else if (front_dist < 90
			&& robot_stop()
			&& robot_distance_avg(DIST_FRONT, 5, 1) < 100) {

			printf("Case 2\n");
			robot_turn(DTOR(90.0f));
			robot_drive(-100, -100, 200);
			robot_turn(DTOR(-180.0f));
			robot_drive(-50, -50, 500);
			already_checked_for_corner = false;
		}
		
		// 3. case
		else if (side_dist > 200
			&& robot_stop() 
			&& robot_distance_avg(DIST_FRONT, 5, 1) > 1000
			&& robot_distance_avg(DIST_RIGHT_FRONT, 5, 1) > 180 
			&& pixels_black() > 2000) {

			printf("Case 3\n");
			robot_drive(-100, -100, 50);
			printf("Potential front exit\n");

			// TODO: check if this is really exit or just silver
			if (!rescue_is_exit()) {
				// no exit, so turn 90° and continue
				robot_drive(-100, -100, 350);
				robot_turn(DTOR(90.0f));
				robot_drive(-100, -100, 250);
				robot_turn(DTOR(-180.0f));
				robot_drive(-100, -100, 250);
			} else return;
		}

		// 4. case
		else if (milliseconds() - side_exit_cooldown > 3000 
			&& side_dist > 200 
			&& robot_stop() 
			&& robot_distance_avg(DIST_RIGHT_FRONT, 5, 1) > 180 
			&& pixels_black() < 2000) {

			printf("Case 4\n");
			robot_drive(100, 100, 200);
			robot_turn(DTOR(90.0f));
			printf("Potential side exit\n");

			if (!rescue_is_exit()) {
				// no exit, skip
				robot_drive(-80, -80, 600);
				robot_turn(DTOR(-90.0f));
				side_exit_cooldown = milliseconds();
			} else return;
		}		
	}

}

void rescue() {
	display_set_mode(MODE_RESCUE);
	display_set_image(IMAGE_RESCUE_FRAME, frame);
	display_set_image(IMAGE_RESCUE_THRESHOLD, corner_thresh);

	// TEST
	rescue_find_exit();
	return;
	// TEST END

	robot_drive(100, 100, 400);

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
	
	robot_drive(100, 100, 800);


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
			if(ret == 2) {
				break;
			}
		} else {
			rescue_find_center(0);
			robot_drive(-70, -70, 400);
		}
		
		num_victims++;
	}
	rescue_find_center(0);
	rescue_find_exit();

	rescue_cleanup();
}

void rescue_cleanup() {
	victims_destroy();
	corner_destroy();
	camera_stop_capture();
	delay(100);
}
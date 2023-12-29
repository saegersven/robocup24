#include "rescue.h"

#include "../vision.h"
#include "../camera.h"
#include "../thresholding.h"
#include "../utils.h"
#include "../robot.h"

#include "victims.h"

static DECLARE_S_IMAGE(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT, 3);

void rescue_collect_victim() {
	// DO THE COLLECTING THING
	printf("Picking up victim\n");
	delay(2000);
}

// Returns 0 if no victims was found, 1 if victim is alive and 2 if victim is dead
int rescue_collect(int find_dead) {
	display_set_number(NUMBER_RESCUE_OBJECTIVE, RESCUE_OBJECTIVE_VICTIM);

	Victim victim;
	
	float APPROACH_K_P = 0.1f;
	int APPROACH_BASE_SPEED = 40;

	float CAM_K_P = 1.0f;
	int CAM_COLLECT_POS = CAM_POS_DOWN - 15;
	int cam_angle = CAM_POS_UP;

	while(1) {
		camera_grab_frame(frame, RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);

		if(victims_find(frame, find_dead, &victim)) {
			display_set_number(NUMBER_RESCUE_POS_X, victim.x);
			display_set_number(NUMBER_RESCUE_POS_Y, victim.y);
			display_set_number(NUMBER_RESCUE_IS_DEAD, victim.dead);

			float angle_horizontal = (victim.x / RESCUE_FRAME_WIDTH - 0.5f) * CAM_HORIZONTAL_FOV;
			float u_approach = angle_horizontal * APPROACH_K_P;
			int8_t left_speed = clamp(APPROACH_BASE_SPEED + u_approach, -100, 100);
			int8_t right_speed = clamp(APPROACH_BASE_SPEED - u_approach, -100, 100);
			robot_drive(left_speed, right_speed, 0);

			float angle_vertical = (victim.y / RESCUE_FRAME_HEIGHT - 0.5f) * CAM_VERTICAL_FOV;
			float u_cam = angle_vertical * CAM_K_P;
			cam_angle += u_cam;
			if(cam_angle < CAM_POS_UP) cam_angle = CAM_POS_UP;
			if(cam_angle > CAM_COLLECT_POS) cam_angle = CAM_COLLECT_POS;
			robot_servo(SERVO_CAM, cam_angle, false);

			if(cam_angle >= CAM_COLLECT_POS && angle_vertical > DTOR(20.0f)) {
				robot_stop();
				delay(100);
				robot_turn(angle_horizontal - DTOR(20.0f));
				// Collect victim
				rescue_collect_victim();

				return victim.dead ? 2 : 1;
			}
		} else {
			display_set_number(NUMBER_RESCUE_POS_X, 0.0f);
			display_set_number(NUMBER_RESCUE_POS_Y, 0.0f);

			robot_turn(DTOR(30.0f));
		}
	}
}

void rescue_deliver(int is_dead) {
	display_set_number(NUMBER_RESCUE_OBJECTIVE, RESCUE_OBJECTIVE_CORNER);
}

void rescue() {
	camera_start_capture(RESCUE_FRAME_WIDTH, RESCUE_FRAME_HEIGHT);

	display_set_mode(MODE_RESCUE);

	victims_init();

	int num_victims = 0;

	while(num_victims < 3) {
		display_set_number(NUMBER_RESCUE_NUM_VICTIMS, num_victims);

		int ret = rescue_collect(num_victims < 2);
		rescue_deliver(ret == 2);
		
		num_victims++;
	}

	victims_destroy();

	camera_stop_capture();
}
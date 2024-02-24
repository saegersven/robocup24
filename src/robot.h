/****************************************************************************************************************
 *                                                                                                              *
 * ROBOT                                                                                                        *
 *                                                                                                              *
 * Controls robot hardware. Provides methods for driving actuators and for reading sensors. Communicates        *
 * with Arduino over SPI (device 0.0). Also reads out GPIO (for start/stop button).                             *
 *                                                                                                              *
 * robot_init()                           - Initialize robot (GPIO and SPI communication)                       *
 * robot_spi_init()                       - Initializes SPI                                                     *
 *                                                                                                              *
 * robot_drive(left, right, duration)     - Sends drive command to Arduino with motor speeds and sends          *
 *                                          stop command after 'duration' milliseconds.                         *
 * robot_stop()                           - Send drive command to Arduino with speed 0.                         *
 * robot_turn(angle)                      - Send turn command to Arduino and wait for turning to finish.        *
 *                                                                                                              *
 * robot_servo(angle, stall)              - Send servo command to Arduino. Setting stall to true lets servo     *
 *                                          stay attached after movement. Setting stall to false detaches       *
 *                                          servo after movement. Angle of zero means no movement.              *
 * robot_button()                         - Returns true if start/stop button is pressed.                       *
 *                                                                                                              *
 * robot_sensor(sensor_id)                - Sends sensor command to Arduino and returns response.               *
 * robot_distance_avg(sensor_id, n, p)    - Reads sensor n times and returns the average of the readings        *
 *                                          in the middle (1 - 2*p)*100 %.                                      *
 *                                                                                                              *
 ****************************************************************************************************************/

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ARDUINO_BOOT_UP_DELAY 2000

// GPIO Pins
#define PIN_BTN 22 // TODO

#define SERVO_CAM 0
#define SERVO_ARM 1
#define SERVO_STRING 2

// Sensor IDs
#define DIST_FRONT      (2)
#define DIST_RIGHT_FRONT (3)
#define DIST_RIGHT_REAR  (4)
#define IMU_PITCH       (10)
#define IMU_YAW         (11) // Yaw/Heading
#define BAT_VOLTAGE     (12)

// Command IDs
#define CMD_MOTOR   (0x01)
#define CMD_SERVO   (0x03)
#define CMD_SENSOR  (0x04)
#define CMD_TURN    (0x05)
#define CMD_LED     (0x06)

#define CAM_HEIGHT_MM 140

#define CAM_POS_UP 50
#define CAM_POS_HORIZONTAL 36
#define CAM_POS_DOWN2 90
#define CAM_POS_DOWN3 110
#define CAM_POS_DOWN 135
#define ARM_POS_DOWN 252
#define ARM_POS_HALF_DOWN 135
#define ARM_POS_UP 40
#define STRING_POS_CLOSED 130
#define STRING_POS_OPEN 30

#define DISTANCE_FACTOR 0.0f // TODO

#define CAM_HORIZONTAL_FOV DTOR(62.0f)
#define CAM_VERTICAL_FOV DTOR(49.0f)

// Initialization
void robot_init();
void robot_serial_init();
void robot_serial_close();

void robot_serial_write_command(uint8_t command, uint8_t *data, uint8_t len);
int robot_serial_read(uint8_t *data, uint8_t len);

// Movement
void robot_drive(int8_t left, int8_t right, int32_t duration);
void robot_stop();
void robot_turn(float angle);
void robot_led(bool state);
void robot_servo(uint8_t servo_id, uint8_t angle, bool stall, bool nodelay);

// Sensing
bool robot_button();

int16_t robot_sensor(uint8_t sensor_id);

int robot_distance_avg(uint8_t sensor_id, int num_measurements, int remove_percentage);
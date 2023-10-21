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

#define ARDUINO_BOOT_UP_DELAY 200

// GPIO Pins
#define PIN_BTN 0 // TODO

// Sensor IDs
#define DIST_FRONT      (2)
#define DIST_LEFT_FRONT (3)
#define DIST_LEFT_REAR  (4)
#define DIST_REAR_LEFT  (5)
#define DIST_REAR_RIGHT (6)
#define IMU_PITCH       (10)
#define IMU_YAW         (11) // Yaw/Heading

// Command IDs
#define CMD_MOTOR   (0x01)
#define CMD_SERVO   (0x03)
#define CMD_SENSOR  (0x04)
#define CMD_TURN    (0x05)

// Initialization
void robot_init();
void robot_spi_init();

// Movement
void robot_drive(int8_t left, int8_t right, int32_t duration);
void robot_stop();
void robot_turn(float angle);

void robot_servo(uint8_t angle, bool stall);

// Sensing
bool robot_button();

uint16_t robot_sensor(uint8_t sensor_id);

int robot_distance_avg(uint8_t sensor_id, uint8_t num_measurements, float remove_percentage);
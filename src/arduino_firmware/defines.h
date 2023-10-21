#pragma once

#define PI 3.141592653f

// MOTORS
// A and B are pins to control direction
// EN are PWM control pins
#define M_LEFT_A         0
#define M_LEFT_B         0
#define M_LEFT_FRONT_EN  0
#define M_LEFT_REAR_EN   0

#define M_RIGHT_A        0
#define M_RIGHT_B        0
#define M_RIGHT_FRONT_EN 0
#define M_RIGHT_REAR_EN  0

#define PIN_BTN             0
#define PIN_LED             13
#define PIN_BATTERY_VOLTAGE 0

#define REAR_WHEEL_FACTOR 1.1f

#define NUM_SERVOS 3
int servo_pins[NUM_SERVOS] = {0, 0, 0};

#define NUM_DIST_SENSORS 6
// TODO: Dist sensor definitions

#define SENSOR_PITCH 10
#define SENSOR_HEADING 11

#define SENSOR_DIST_START 2
#define SENSOR_DIST_END 9

// PROTOCOL
#define CMD_MOTOR   (0x01)
#define CMD_SERVO   (0x03)
#define CMD_SENSOR  (0x04)
#define CMD_TURN    (0x05)

#define MIN_BATTERY_VOLTAGE 6.5f

// Message lengths, indexed with command ID. Length including command and data
// Command ID between transfers is zero -> large message length for ID zero
const int message_lengths[6] = {0xFF, 3, 4, 2, 3};

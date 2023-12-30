#pragma once

#define PI 3.141592653f

// MOTORS
// A and B are pins to control direction
// EN are PWM control pins
#define M_LEFT_A         4
#define M_LEFT_B         7
#define M_LEFT_EN        6

#define M_RIGHT_A        8
#define M_RIGHT_B        9
#define M_RIGHT_EN       5

#define MOTOR_CORRECTION_FACTOR 0.9

#define PIN_BTN             0
#define PIN_BATTERY_VOLTAGE A7

#define REAR_WHEEL_FACTOR 1.1f

#define NUM_SERVOS 3
int servo_pins[NUM_SERVOS] = {3, A0, 2};

Servo servos[NUM_SERVOS];

#define NUM_DIST_SENSORS 1
VL53L0X dist_sensors[NUM_DIST_SENSORS];

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);
static float heading, pitch;

#define SENSOR_PITCH 10
#define SENSOR_HEADING 11

#define SENSOR_DIST_START 2
#define SENSOR_DIST_END 9

// PROTOCOL
#define CMD_MOTOR   (0x01)
#define CMD_SERVO   (0x03)
#define CMD_SENSOR  (0x04)
#define CMD_TURN    (0x05)

#define CMD_LED     (0x06)


#define BYTE_TURN_DONE (0x01)
#define MIN_TIME_PER_DEG 4
#define MAX_TIME_PER_DEG 10
#define TURN_TOLERANCE 11


#define CMD_LEN_MOTOR 3
#define CMD_LEN_SERVO 5
#define CMD_LEN_SENSOR 2
#define CMD_LEN_TURN 3
#define CMD_LEN_LED 2

#define MIN_BATTERY_VOLTAGE 11.2f // 4x 3V - 0.8V should trigger alert (0.8V is due to voltage drop accross diodes for reverse voltage protection)

// Message lengths, indexed with command ID. Length including command and data
// Command ID between transfers is zero -> large message length for ID zero
const int message_lengths[7] = {0xFF, CMD_LEN_MOTOR, 0xFF, CMD_LEN_SERVO, CMD_LEN_SENSOR, CMD_LEN_TURN, CMD_LEN_LED};

float start_up_bat_voltage = 0;

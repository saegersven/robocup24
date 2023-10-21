#include <VL53L0X.h>
#include <Servo.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>

#include "defines.h"

Servo servos[NUM_SERVOS];

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);
float pitch;
float heading;

VL53L0X dist_sensors[NUM_DIST_SENSORS];

bool led_on = false;

void init_robot() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(M_LEFT_A, OUTPUT);
  pinMode(M_LEFT_B, OUTPUT);
  pinMode(M_LEFT_FRONT_EN, OUTPUT);
  pinMode(M_LEFT_REAR_EN, OUTPUT);

  pinMode(M_RIGHT_A, OUTPUT);
  pinMode(M_RIGHT_B, OUTPUT);
  pinMode(M_RIGHT_FRONT_EN, OUTPUT);
  pinMode(M_RIGHT_REAR_EN, OUTPUT);

  /*for(int i = 0; i < NUM_VL53L0X; ++i) {
    
  }*/

  // Initialize BNO055
  if(!bno.begin()) error(2);

  // TODO: Initialize distance sensors

  if(get_battery_voltage() < MIN_BATTERY_VOLTAGE) error(1);
}

void m(int8_t left, int8_t right) {
  digitalWrite(M_LEFT_A, left < 0);
  digitalWrite(M_LEFT_B, left > 0);

  digitalWrite(M_RIGHT_A, right < 0);
  digitalWrite(M_RIGHT_B, right > 0);

  if(left == -128) left = -127;

  if(right == -128) right = -127;

  uint8_t left_front_duty_cycle = abs(left) * 2;
  uint8_t left_rear_duty_cycle = left_front_duty_cycle * REAR_WHEEL_FACTOR;
  if (left_rear_duty_cycle > 255) left_rear_duty_cycle = 255;

  if(left == 0) {
    left_front_duty_cycle = 255;
    left_rear_duty_cycle = 255;
  }
  

  uint8_t right_front_duty_cycle = abs(right) * 2;
  uint8_t right_rear_duty_cycle = right_front_duty_cycle * REAR_WHEEL_FACTOR;
  if (right_rear_duty_cycle > 255) right_rear_duty_cycle = 255;

  analogWrite(M_LEFT_FRONT_EN, left_front_duty_cycle);
  analogWrite(M_LEFT_REAR_EN, left_rear_duty_cycle);

  analogWrite(M_RIGHT_FRONT_EN, right_front_duty_cycle);
  analogWrite(M_RIGHT_REAR_EN, right_rear_duty_cycle);
}

void servo(uint8_t id, uint8_t angle, bool stall) {
  if(angle == 0) {
    // Toggle attachmeant of servo
    if(servos[id].attached() && !stall) servos[id].detach();
    else if(stall) servos[id].attach(servo_pins[id]);
    return;
  }

  if(!servos[id].attached()) servos[id].attach(servo_pins[id]);

  servos[id].write(angle);

  if(!stall) servos[id].detach();
}

void update_orientation() {
  sensors_event_t orientation_data;
  bno.getEvent(&orientation_data, Adafruit_BNO055::VECTOR_EULER);
  heading = orientation_data.orientation.x / 180.0f * PI;
  pitch = orientation_data.orientation.z / 180.0f * PI;
}

float get_heading() {
  return heading;
}

float get_pitch() {
  return pitch;
}

int16_t distance(int sensor_id) {
  int16_t dist = dist_sensors[sensor_id].readRangeSingleMillimeters();
  
  if(dist < 0) dist = 0;
  if(dist > 2000) dist = 2000;
  
  return dist;
}

float get_battery_voltage() {
  return ((5.0f / 1023.0f) * analogRead(PIN_BATTERY_VOLTAGE)) * 2.0f;
}

void toggle_led() {
  digitalWrite(PIN_LED, (led_on = !led_on));
}

// Emergency function that is only being called when something is really wrong
// Errors:
// 1 - Battery voltage is critically low
// 2 - Gyro sensor could not be initialized
// 3 - One of the distance sensors could not be initialized
void error(int error_code) {
  while(1) {
    Serial.print("--- ERROR:");
    Serial.print(error_code);
    Serial.println(" ---");
    toggle_led();
    delay(42);
    toggle_led();
    delay(42);
  }
}

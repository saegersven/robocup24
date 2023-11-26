/*
* This file contains only SPI setup, message transfer and parsing.
* All robot functionality is in functions.ino.
*/

#include <Servo.h>
#include <EEPROM.h>

#include "defines.h"

#define SERIAL_BUF_SIZE 32

uint8_t message[SERIAL_BUF_SIZE];
int message_pos = 0;

Servo servo;  // create servo object to control a servo

// TODO: add function description
void parse_message() {
  if (message[0] == CMD_MOTOR) {
    //digitalWrite(13, HIGH);
    m(*((int8_t*)&message[1]), *((int8_t*)&message[2]), 0);
  } else if (message[0] == CMD_SERVO) {
    servo2(message[1], message[2], message[3]);
  } else if (message[0] == CMD_SENSOR) {
    int16_t value = 0;

    if (message[1] == SENSOR_PITCH) {
      update_orientation();
      value = (int16_t)(get_pitch() / 1000.0f);
    } else if (message[1] == SENSOR_HEADING) {
      update_orientation();
      value = (int16_t)(get_heading() / 1000.0f);
    } else if (message[1] >= SENSOR_DIST_START && message[1] <= SENSOR_DIST_END) {
      // One of the distance sensors
      int sensor_id = message[1] - SENSOR_DIST_START;
      if (sensor_id < NUM_DIST_SENSORS) {
        // TODO: Read out distance sensor
      }
    }

    if (value == 0) value = 1; // Value 0 signals no data yet
  } else if (message[0] == CMD_TURN) {
  
  } else if (message[0] == CMD_LED) {
    digitalWrite(13, message[1]);
  }
}

void setup() {
  Serial.begin(115200);
  init_robot();
}

void loop() {
  while(Serial.available() > 0) {
    message[message_pos] = Serial.read();
    message_pos++;

    if(message_pos == SERIAL_BUF_SIZE) {
      message_pos = 0;
    }

    if(message_pos == message_lengths[message[0]]) {
      parse_message();
      message_pos = 0;
    }
  }
}

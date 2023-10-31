/*
* This file contains only SPI setup, message transfer and parsing.
* All robot functionality is in functions.ino.
*/

#include <SPI.h>

#include "defines.h"

#define SPI_BUF_SIZE 32

int spi_pos = 0;
uint8_t buf[SPI_BUF_SIZE];
float start_up_bat_voltage = 0.0f;

// Must be filled with zeros or contain valid data
uint8_t return_data[SPI_BUF_SIZE];


// TODO: add function description
void parse_message() {
  if (spi_pos == message_lengths[buf[0]]) {
    if (buf[0] == CMD_MOTOR) {
      m(*((int8_t*)&buf[1]), *((int8_t*)&buf[2]));
    } else if (buf[0] == CMD_SERVO) {
      servo(buf[1], buf[2], buf[3]);
    } else if (buf[0] == CMD_SENSOR) {
      int16_t value = 0;

      if (buf[1] == SENSOR_PITCH) {
        update_orientation();
        value = (int16_t)(get_pitch() / 1000.0f);
      } else if (buf[1] == SENSOR_HEADING) {
        update_orientation();
        value = (int16_t)(get_heading() / 1000.0f);
      } else if (buf[1] >= SENSOR_DIST_START && buf[1] <= SENSOR_DIST_END) {
        // One of the distance sensors
        int sensor_id = buf[1] - SENSOR_DIST_START;
        if (sensor_id < NUM_DIST_SENSORS) {
          // TODO: Read out distance sensor
        }
      }

      if (value == 0) value = 1; // Value 0 signals no data yet

      memcpy(return_data, value, sizeof(int16_t));
    } else if (buf[0] == CMD_TURN) { }

    // Reset
    buf[0] = 0;
    spi_pos = 0;
  }
}

void setup() {
  init_robot();

  // init SPI stuff
  pinMode(MISO, OUTPUT);
  pinMode(MOSI, INPUT);
  pinMode(SCK, INPUT);
  pinMode(SS, INPUT);

  // SPCR is SPI control register. Bits (left is 7):
  // 7 - SPIE: Enables interrupt when 1
  // 6 - SPE: Enables SPI when 1
  // 5 - DORD: LSB first when 1
  // 4 - MSTR: Peripheral mode when 0
  // 3 - CPOL: Clock Idle Low when 0
  // 2 - CPHA: Sample data on rising edge when 0
  // 1,0 - SPI speed, 00 is fastest (4MHz)
  SPCR = 0b11100000;

  // Initialize with valid command ID
  buf[0] = 0;
  // Write zeros to return data so Pi doesn't receive garbage
  memset(return_data, 0, SPI_BUF_SIZE);
}

// SPI interrupt routine
ISR(SPI_STC_vect) {
  buf[spi_pos] = SPDR;
  SPDR = return_data[spi_pos];

  return_data[spi_pos] = 0;

  ++spi_pos;
}

void loop() {
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(4000);
}

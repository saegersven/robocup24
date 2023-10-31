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

void setup() {
  init_robot();
}

// SPI interrupt routine
ISR(SPI_STC_vect) {
  buf[spi_pos] = SPDR;
  SPDR = return_data[spi_pos];

  return_data[spi_pos] = 0;

  ++spi_pos;
}

void loop() {
  m(128, 128, 1000);
}

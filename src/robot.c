#include "robot.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <wiringPi.h>

#include "utils.h"

// SPI definitions
#define SPI_MAX_RETRIES 2

static int spi_fd;
static int spi_mode            = SPI_MODE_0;
static int spi_bits_per_word   = 8;
static int spi_speed           = 500'000;
static int spi_lsb_first       = 0; // MSB first

void robot_init() {
    wiringPiSetupGpio();
    pinMode(PIN_BTN, INPUT);

    delay(ARDUINO_BOOT_UP_DELAY);

    robot_spi_init();
}

void robot_spi_init() {
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if(spi_fd < 0) {
        fprintf(stderr, "Could not open SPI device\n");
        exit(EXIT_FAILURE);
    }

    // Set mode
    if(ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode) < 0) {
        fprintf(stderr, "Could not set SPI mode\n");
    }

    // Set MSB first
    if(ioctl(spi_fd, SPI_IOC_WR_LSB_FIRST, &lsb_first) < 0) {
        fprintf(stderr, "Could not set MSB first\n");
    }

    // Set and read back bits per word
    if(ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits_per_word) < 0) {
        fprintf(stderr, "Could not set SPI bits per word\n");
    }

    if(ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits_per_word) < 0) {
        fprintf(stderr, "Could not read SPI bits per word\n");
    }

    // Set and read back speed
    if(ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) {
        fprintf(stderr, "Could not set SPI speed\n");
    }

    if(ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed) < 0) {
        fprintf(stderr, "Could not read SPI speed\n");
    }
}

// Computes parity for one byte
uint8_t parity_byte(uint8_t b) {
    b ^= b >> 4;
    b &= 0xF;
    return (0x6996 >> b) & 1;
}

void robot_spi_transfer(uint8_t command, uint8_t *data, size_t data_len, uint8_t* ret, size_t return_len) {
    uint8_t buf[32];

    buf[0] = command;
    memcpy(buf + 1, data, data_len);

    int num_transfers = ret == NULL ? 1 : 2;

    struct spi_ioc_transfer transfer[2];
    CLEAR(transfer);
    transfer[0].tx_buf = (unsigned long)buf;
    transfer[1].rx_buf = (unsigned long)ret;
    transfer[0].len    = data_len + 1;
    transfer[1].len    = return_len; 
    for(int i = 0; i < 2; i++) {
        transfer[i].delay_usecs    = 0;
        transfer[i].bits_per_word  = spi_bits_per_word;
        transfer[i].cs_change      = 1;
    }

    if(ioctl(spi_fd, SPI_IOC_MESSAGE(num_transfers), transfer) < 0) {
        fprintf(stderr, "SPI transfer failed\n");
    }

    if(ret != NULL) memcpy(ret, buf + 1 + data_len, return_len);
}

void robot_drive(int8_t left, int8_t right, int32_t duration) {
    if(duration < 0) {
        left = -left;
        right = -right;
        duration = -duration;
    }

    uint8_t data[2] = {*(uint8_t*)(&left), *(uint8_t*)(&right)};

    robot_spi_transfer(CMD_MOTOR, data, 2, NULL, 0);

    if(duration > 0) {
        delay(duration);

        robot_stop();
    }
}

void robot_stop() {
    robot_drive(0, 0, 0);
}

void robot_turn(float angle) {
    // Conver to milliradians
    int16_t angle_mrad = (int16_t)(angle / 1000.0f);

    uint8_t finished = 0;
    while(!finished) { // TODO: Add timeout
        robot_spi_transfer(CMD_TURN, &angle_mrad, sizeof(angle_mrad), &finished, sizeof(finished));
        // TODO: Add delay here
    }
}

void robot_servo(uint8_t angle, bool stall) {
    uint8_t data[2] = {angle, stall};

    robot_spi_transfer(CMD_SERVO, data, 2, NULL, 0);
}

bool robot_button() {
    for(int i = 0; i < 4; i++) {
        if(!digitalRead(PIN_BTN)) {
            return false;
        }
    }
    return true;
}

int16_t robot_sensor(uint8_t sensor_id) {
    int16_t value;

    // TODO: Sensor readings take a long time, so maybe re-send until proper values are received
    robot_spi_transfer(CMD_SENSOR, &sensor_id, sizeof(sensor_id), &value, sizeof(value));

    return value;
}

int robot_distance_avg(uint8_t sensor_id, uint8_t num_measurements, float remove_percentage) {
    float arr[256];

    for(int i = 0; i < num_measurements; i++) {
        float dist = (float)robot_sensor(sensor_id);
        arr[i] = dist;
        delay(15);
    }

    qsort(arr, num_measurements, sizeof(*arr), compare_float);

    int kth_percent = (num_measurements * remove_percentage);
    float sum = 0;

    for(int i = 0; i < num_measurements; i++) {
        if(i > kth_percent && i < (num_measurements - kth_percent)) {
            sum += arr[i];
        }
    }

    float avg = sum / (num_measurements - 2 * kth_percent);

    return (int)avg;
}
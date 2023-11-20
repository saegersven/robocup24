#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>

// Command IDs
#define CMD_MOTOR   (0x01)
#define CMD_SERVO   (0x03)
#define CMD_SENSOR  (0x04)
#define CMD_TURN    (0x05)

static int spi_fd;
static int spi_mode          = SPI_MODE_0;
static int spi_bits_per_word = 8;
static int spi_speed         = 500000;
static int spi_lsb_first     = 0;

void delay(unsigned int milliseconds) {
    usleep(milliseconds * 1000);
}

void print_hex_buffer(uint8_t *buf, size_t len) {
    for (int i = 0; i < len; i++)
    {
        printf("%02X ", buf[i]);
    }
    printf("\n");
}

void spi_init() {
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if(spi_fd < 0) {
        fprintf(stderr, "Could not open SPI device\n");
        exit(EXIT_FAILURE);
    }

    if(ioctl(spi_fd, SPI_IOC_WR_MODE, &spi_mode) < 0) {
        fprintf(stderr, "Could not set SPI mode\n");
    }

    if(ioctl(spi_fd, SPI_IOC_WR_LSB_FIRST, &spi_lsb_first) < 0) {
        fprintf(stderr, "Could not set MSB first\n");
    }

    if(ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &spi_bits_per_word) < 0) {
        fprintf(stderr, "Could not set SPI bits per word\n");
    }

    if(ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &spi_bits_per_word) < 0) {
        fprintf(stderr, "Could not read SPI bits per word\n");
    }

    if(ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed) < 0) {
        fprintf(stderr, "Could not write SPI speed\n");
    }

    if(ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &spi_speed) < 0) {
        fprintf(stderr, "Could not read SPI speed\n");
    }
}

void robot_spi_transfer(uint8_t command, uint8_t *data, size_t data_len, uint8_t *return_data, size_t return_len) {
    uint8_t buf[32];

    buf[0] = command;
    memcpy(buf + 1, data, data_len);

    int num_transfers = return_data == NULL ? 1 : 2;

    struct spi_ioc_transfer transfer[2];
    memset(transfer, 0, sizeof(transfer));
    transfer[0].tx_buf = (unsigned long)buf;
    
    
    // uint8_t buf_rx[32];
    // transfer[0].rx_buf = (unsigned long)buf_rx;
    // print_hex_buffer(rx_buf, data_len + 1);
    

    transfer[1].rx_buf = (unsigned long)return_data;
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
    
    printf("Sent command %d\n", command);

    if(return_data != NULL) memcpy(return_data, buf + 1 + data_len, return_len);
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

        robot_drive(0, 0, 0);
    }
}

void robot_stop() {
    robot_drive(0, 0, 0);
}

int16_t robot_sensor(uint8_t sensor_id) {
    int16_t value;

    while(!value) {
        robot_spi_transfer(CMD_SENSOR, &sensor_id, 1, (uint8_t*)&value, 2);
    }

    return value;
}

#define CMD_LED 0x06

int main() {
    printf("SPI Test\n");

    spi_init();
    delay(1000);

    while(1) {
        robot_drive(127, 127, 2000);
        delay(1000);
        robot_drive(-127, -127, 2000);
        delay(1000);
    }
}
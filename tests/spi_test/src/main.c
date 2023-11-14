#include <stdio.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>

static int spi_fd;
static int spi_mode          = SPI_MODE_0;
static int spi_bits_per_word = 8;
static int spi_speed         = 500'000;
static int spi_lsb_first     = 0;

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

    if(ioctl(spi_fd, SPI_IOC_WR_LSB_FIRST, &lsb_first) < 0) {
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

    int num_transfers = ret == NULL ? 1 : 2;

    struct spi_ioc_transfer transfer[2];
    memset(transfer, 0, sizeof(transfer));
    transfer[0].tx_buf = (unsigned long)buf;
    
    
    // uint8_t buf_rx[32];
    // transfer[0].rx_buf = (unsigned long)buf_rx;
    // print_hex_buffer(rx_buf, data_len + 1);
    

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

        robot_drive(0, 0, 0);
    }
}

void robot_stop() {
    robot_drive(0, 0, 0);
}

int16_t robot_sensor(uint8_t sensor_id) {
    int16_t value;

    while(!value) {
        robot_spi_transfer(CMD_SENSOR, &sensor_id, 1, &value, 2);
    }

    return value;
}

int main() {
    printf("SPI Test\n");

    
}
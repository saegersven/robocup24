#include "robot.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <wiringPi.h>

#include "utils.h"

static int serial_fd;
static speed_t baud_rate = B115200;

void robot_init() {
    wiringPiSetupGpio();
    pinMode(PIN_BTN, INPUT);

    delay(ARDUINO_BOOT_UP_DELAY);

    robot_serial_init();
}

void robot_serial_init() {
    // Set latency timer to 2 ms
    FILE* latency_file = fopen("/sys/bus/usb-serial/drivers/ftdi_sio/ttyUSB0/latency_timer", 'w');
    fwrite(latency_file, 1, '2');

    int status;
    termios options;

    if((serial_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY)) < 0) {
        fprintf(stderr, "Could not open Serial: %s\n", strerror(errno));
    }

    fcntl(serial_fd, F_SETFL, O_RDWR);

    tcgetattr(serial_fd, &options);

    cfmakeraw(&options);
    cfsetispeed(&options, baud_rate);
    cfsetospeed(&options, baud_rate);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    options.c_cc[VMIN] = 1; // block
    options.c_cc[VTIME] = 2; // 0.2 seconds

    tcsetattr(serial_fd, TCSANOW, &options);
    ioctl(serial_fd, TIOCMGET, &status);

    status |= TIOCM_DTR;
    status |= TIOCM_RTS;

    ioctl(serial_fd, TIOCMSET, &status);

    delay(10);
}

void robot_serial_write_command(uint8_t command, uint8_t *data, uint8_t len) {
    uint8_t buf[32];

    buf[0] = command;
    memcpy(buf + 1, data, len);

    if(write(serial_fd, buf, len + 1) < 0) {
        fprintf(stderr, "Write error: %s\n", strerror(errno));
    }
}

int robot_serial_read(uint8_t *data, uint8_t len) {
    int n = 0;
    if((n = read(serial_fd, data, len)) < 0) {
        fprintf(stderr, "Read error: %s\n", strerror(errno));
    }
    return n;
}

void robot_drive(int8_t left, int8_t right, int32_t duration) {
    if(duration < 0) {
        left = -left;
        right = -right;
        duration = -duration;
    }

    uint8_t data[2] = {*(uint8_t*)(&left), *(uint8_t*)(&right)};

    robot_serial_write_command(CMD_MOTOR, data, 2);

    if(duration > 0) {
        delay(duration);
        robot_stop();
    }
}

void robot_stop() {
    robot_drive(0, 0, 0);
}

void robot_turn(float angle) {
    int16_t angle_mrad = (int16_t)(angle / 1000.0f);

    uint8_t finished = 0;
    // TODO
}

void robot_servo(uint8_t angle, bool stall) {
    uint8_t data[2] = {angle, stall};

    robot_serial_write_command(CMD_SERVO, data, 2);
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
    int16_t value = 0;

    robot_serial_write_command(CMD_SENSOR, sensor_id, 1);
    
    int res = 0;
    while((res = robot_serial_read(&value, 2)) < 0);

    return value;
}

int robot_distance_avg(uint8_t sensor_id, uint8_t num_measurements, float remove_percentage) {
    // TODO
}
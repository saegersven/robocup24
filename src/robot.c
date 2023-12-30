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
#include <errno.h>
#include <string.h>

#include <wiringPi.h>

#include "utils.h"

static int serial_fd;
static speed_t baud_rate = B115200;

void robot_init() {
    wiringPiSetupGpio();
    pinMode(PIN_BTN, INPUT);

    robot_serial_init();

    delay(ARDUINO_BOOT_UP_DELAY);
}

void robot_serial_init() {
    // Set latency timer to 2 ms
    FILE* latency_file = fopen("/sys/bus/usb-serial/drivers/ftdi_sio/ttyUSB0/latency_timer", "w");

    if(latency_file == NULL) {
        fprintf(stderr, "Failed to open latency timer file\n");
    }

    char two[] = "2";
    fwrite(two, 1, 1, latency_file);

    int status;
    struct termios options;

    if((serial_fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) < 0) {
        fprintf(stderr, "Could not open Serial: %s\n", strerror(errno));
    }

    delay(10);

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

    options.c_cc[VMIN] = 0; // block
    options.c_cc[VTIME] = 5; // 0.5 seconds

    tcsetattr(serial_fd, TCSANOW, &options);
    ioctl(serial_fd, TIOCMGET, &status);

    status |= TIOCM_DTR;
    status |= TIOCM_RTS;

    ioctl(serial_fd, TIOCMSET, &status);

    delay(10);
}

void robot_serial_close() {
    close(serial_fd);
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
    tcflush(serial_fd, TCIFLUSH);

    int n = read(serial_fd, data, len);
    if(n < 0) {
        fprintf(stderr, "Read error: %s\n", strerror(errno));
    }
    return n;
}

void robot_drive(int8_t left, int8_t right, int32_t duration) {
    left = clamp(left, -100, 100);
    right = clamp(right, -100, 100);
    
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

    if(angle > 0) {
        robot_drive(60, -60, angle * 800.0f);
    } else {
        robot_drive(-60, 60, -angle * 800.0f);
    }
    robot_stop();
}

void robot_servo(uint8_t servo_id, uint8_t angle, bool stall, bool nodelay) {
    uint8_t data[4] = {servo_id, angle, (uint8_t)stall, (uint8_t)nodelay};

    robot_serial_write_command(CMD_SERVO, data, 4);
}

void robot_led(bool state) {
    robot_serial_write_command(CMD_LED, &state, 1);
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

    robot_serial_write_command(CMD_SENSOR, &sensor_id, 1);
    
    robot_serial_read(&value, 2);

    return value;
}

int robot_distance_avg(uint8_t sensor_id, int num_measurements, int n_remove) {
    // Take multiple measurements and take average of all but the top and bottom n_remove measurements
    float arr[num_measurements];

    for(int i = 0; i < num_measurements; i++) {
        arr[i] = (float)robot_sensor(sensor_id);
        delay(35);
    }

    //qsort(arr, num_measurements, sizeof(float), compare_float);

    float sum = 0.0f;
    for(int i = n_remove; i < num_measurements - n_remove; i++) {
        sum += arr[i];
    }

    return sum / num_measurements;
}
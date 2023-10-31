void init_robot() {
  Serial.begin(115200);
  Serial.println(" _____ _ _   _____ _ _     ");
  Serial.println("| __  |_| |_|   __| |_|___ ");
  Serial.println("| __ -| |  _|   __| | | . |");
  Serial.println("|_____|_|_| |__|  |_|_|  _|");
  Serial.println("                      |_|  ");
  Serial.println("");
  Serial.println("--- DEBUGGING WINDOW ---");

  
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

  start_up_bat_voltage = get_battery_voltage(); // store current bat voltage on startup so it does not have to be read out each loop iteration for m() function
}

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

// function to drive robot
// int8_t left     : left motor speed from -127 to 128
// int8_t right    : right motor speed from -127 to 128
// int16_t duration: duration for how long the motors should turn
void m(int8_t left, int8_t right, int16_t duration) {
  // full stop:
  if (left == 0 && right == 0) {
    digitalWrite(M_LEFT_A, LOW);
    digitalWrite(M_LEFT_B, LOW);
    digitalWrite(M_RIGHT_A, LOW);
    digitalWrite(M_RIGHT_B, LOW);
    analogWrite(M_LEFT_EN, 0);
    analogWrite(M_RIGHT_EN, 0);
    delay(duration);
    return;
  }
  
  // set spinning direction of both motors:
  if (left < 0) {
    digitalWrite(M_LEFT_A, LOW);
    digitalWrite(M_LEFT_B, HIGH);
  } else {
    digitalWrite(M_LEFT_A, HIGH);
    digitalWrite(M_LEFT_B, LOW);
  }
  if (right < 0) {
    digitalWrite(M_RIGHT_A, LOW);
    digitalWrite(M_RIGHT_B, HIGH);
  } else {
    digitalWrite(M_RIGHT_A, HIGH);
    digitalWrite(M_RIGHT_B, LOW);
  }

  // set pwm signal (aka speed) for both motors:
  // since bat voltage can be as high as 16.8V and we have 12V motors, we need to adjust the duty cicle based on bat voltage
  float left_pwm  =  (float)map(left, -127, 128, 0, 255);
  float right_pwm = (float)map(right, -127, 128, 0, 255);

  Serial.print("Left: ");
  Serial.print(left_pwm);
  Serial.print("  Right: ");
  Serial.println(right_pwm);

  delay(duration);
}

// wrapper for main m() method
void m(int8_t left, int8_t right) {
  m(left, right, 0);
}

void servo(uint8_t id, uint8_t angle, bool stall) {
  /*
  if(angle == 0) {
    // Toggle attachmeant of servo
    if(servos[id].attached() && !stall) servos[id].detach();
    else if(stall) servos[id].attach(servo_pins[id]);
    return;
  }

  if(!servos[id].attached()) servos[id].attach(servo_pins[id]);

  servos[id].write(angle);

  if(!stall) servos[id].detach();
  */
}

void update_orientation() {
  /*
  sensors_event_t orientation_data;
  bno.getEvent(&orientation_data, Adafruit_BNO055::VECTOR_EULER);
  heading = orientation_data.orientation.x / 180.0f * PI;
  pitch = orientation_data.orientation.z / 180.0f * PI;
  */
}

float get_heading() {
  float heading = -1.0f;
  return heading;
}

float get_pitch() {
  float pitch = -1.0f;
  return pitch;
}

int16_t distance(int sensor_id) {
  int16_t dist = 0; //int16_t dist = dist_sensors[sensor_id].readRangeSingleMillimeters();
  /*
  if(dist < 0) dist = 0;
  if(dist > 2000) dist = 2000;
  */
  return dist;
}

// returns current battery voltage in V (!!! actual battery voltage is around 0.8V higher due to voltage drop accross diodes)
float get_battery_voltage() {
  return analogRead(PIN_BATTERY_VOLTAGE) * (16.8 / 968.1672);
}

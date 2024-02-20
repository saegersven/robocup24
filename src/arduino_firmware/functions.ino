void init_robot() {
  /*
    Serial.println(" _____ _ _   _____ _ _     ");
    Serial.println("| __  |_| |_|   __| |_|___ ");
    Serial.println("| __ -| |  _|   __| | | . |");
    Serial.println("|_____|_|_| |__|  |_|_|  _|");
    Serial.println("                      |_|  ");
    Serial.println("");
    Serial.println("--- DEBUGGING WINDOW ---");*/
  // store current bat voltage on startup so it does not have to be read out each loop iteration for m() function

  start_up_bat_voltage = get_battery_voltage();

  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);

  // battery is not switched on, don't start moving
  if (start_up_bat_voltage < 5.0) {
    pinMode(13, OUTPUT);
    m(0, 0, 0);
    while (start_up_bat_voltage < 5.0) {
      //Serial.println("Waiting for power...");
      digitalWrite(13, HIGH);
      delay(500);
      digitalWrite(13, LOW);
      delay(500);
      start_up_bat_voltage = get_battery_voltage();
    }
  }
  pinMode(13, OUTPUT);

  Wire.begin();

  Serial.println("Before dist init");
  init_dist_sensors();

  Serial.println("Before BNO init");

  if (!bno.begin()) {
    panic();
  }

  Serial.println("Init done");
  /*
    Serial.print("Start up battery voltage: ");
    Serial.print(start_up_bat_voltage);
    Serial.println("V");
    Serial.println("Starting robot...");*/
}

// function to drive robot
// int8_t left     : left motor speed from -127 to 128
// int8_t right    : right motor speed from -127 to 128
// int16_t duration: duration for how long the motors should turn
void m(int8_t left, int8_t right, int16_t duration) {
  if (left > 100) left = 100;
  if (right > 100) right = 100;
  if (left < -100) left = -100;
  if (right < -100) right = -100;

  // full stop:
  if (left == 0 && right == 0) {
    digitalWrite(M_LEFT_A, LOW);
    digitalWrite(M_LEFT_B, LOW);
    digitalWrite(M_RIGHT_A, LOW);
    digitalWrite(M_RIGHT_B, LOW);
    analogWrite(M_LEFT_EN, 255);
    analogWrite(M_RIGHT_EN, 255);
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
  uint8_t left_pwm  = (uint8_t)((float)abs(left) * 255.0f / 100.0f);
  uint8_t right_pwm = (uint8_t)((float)abs(right) * 255.0f / 100.0f);

  // since bat voltage can be as high as 16.8V and we have 12V motors, we need to adjust the duty cycle based on bat voltage
  float normalize_factor = (11.0f / start_up_bat_voltage); // 11V instead of 12V because voltages are still a little too high
  if (normalize_factor > 1.0f) normalize_factor = 1.0f;
  left_pwm *= normalize_factor;
  right_pwm *= normalize_factor;

  /*
    Serial.print("Left: ");
    Serial.print(left_pwm);
    Serial.print("  Right: ");
    Serial.println(right_pwm);*/
  analogWrite(M_LEFT_EN, left_pwm);
  analogWrite(M_RIGHT_EN, right_pwm);
  if (duration > 0) {
    delay(duration);
    m(0, 0, 0);
  }
}

// wrapper for main m() method
void m(int8_t left, int8_t right) {
  m(left, right, 0);
}

void turn(int16_t angle_mrad) {
  float angle_rad = (float)angle_mrad / 1000.0f;
  float angle_deg = angle_rad * 180.0f / 3.141592f;

  if (abs(angle_deg) <= 30) {
    int duration = powf(abs(angle_rad), 0.8f) * 340.0f;
    if (angle_deg < 0) {
      m(-60, 60, duration);
    } else {
      m(60, -60, duration);
    }

    Serial.write(BYTE_TURN_DONE);
    return;
  }

  if (angle_deg > 30) angle_deg -= 3;
  if (angle_deg < -30) angle_deg += 3;
  if (angle_deg == 0) return;

  int min_duration = MIN_TIME_PER_DEG * abs(angle_deg);
  int max_duration = MAX_TIME_PER_DEG * abs(angle_deg);

  update_orientation();
  float current_heading = get_heading() * 180.0f / PI;
  float final_heading = current_heading + angle_deg;

  if (final_heading > 360.0f) final_heading -= 360.0f;
  if (final_heading < 0.0f) final_heading += 360.0f;

  if (angle_deg < 0) {
    m(-60, 60, 0);
  } else {
    m(60, -60, 0);
  }

  long long start_time = millis();

  while (millis() - start_time < min_duration);
  while (millis() - start_time < max_duration) {
    update_orientation();
    float heading = get_heading() * 180.0f / PI;

    if (abs(heading - final_heading) < TURN_TOLERANCE) break;
  }

  m(0, 0, 50);

  Serial.write(BYTE_TURN_DONE);
}

void servo(uint8_t id, uint8_t angle, bool stall, bool nodelay) {
  if (angle == 0) {
    // Toggle attachmeant of servo
    if (servos[id].attached() && !stall) servos[id].detach();
    else if (stall) servos[id].attach(servo_pins[id]);
    return;
  }

  if (!servos[id].attached()) servos[id].attach(servo_pins[id]);

  int prev_angle = servos[id].read();
  servos[id].write(angle);

  if (!nodelay) delay(abs(angle - prev_angle) * 8);

  if (!stall) servos[id].detach();
}

void update_orientation() {
  sensors_event_t orientation_data;
  bno.getEvent(&orientation_data, Adafruit_BNO055::VECTOR_EULER);
  heading = orientation_data.orientation.x / 180.0f * PI;
  pitch = orientation_data.orientation.z / 180.0f * PI;
}

float get_heading() {
  //float heading = -1.0f;
  return heading;
}

float get_pitch() {
  //float pitch = -1.0f;
  return pitch;
}

// inits all our VL53L0X distance sensors
// shift register is used for multiplexing
// logic inside function:
// 1. power off all sensors
// 2. power on 1. sensor
// 3. change I2C address of 1. sensor
// 4. power on 1. and 2. sensor
// 4. change I2C address of 2. sensor
// ...

void init_dist_sensors() {
  pinMode(PIN_SHIFT_REGISTER_LATCH, OUTPUT);
  pinMode(PIN_SHIFT_REGISTER_CLOCK, OUTPUT);
  pinMode(PIN_SHIFT_REGISTER_DATA, OUTPUT);

  bool init_successful = true;
  int d = 10; // is increased after init fail

  do {
    //Serial.println(init_successful);
    // power off all sensors
    digitalWrite(PIN_SHIFT_REGISTER_LATCH, LOW);
    shiftOut(PIN_SHIFT_REGISTER_DATA, PIN_SHIFT_REGISTER_CLOCK, MSBFIRST, B00000000);
    digitalWrite(PIN_SHIFT_REGISTER_LATCH, HIGH);
    delay(d);

    for (int i = 0; i < NUM_DIST_SENSORS; ++i) {
      digitalWrite(PIN_SHIFT_REGISTER_LATCH, LOW);
      shiftOut(PIN_SHIFT_REGISTER_DATA, PIN_SHIFT_REGISTER_CLOCK, MSBFIRST, dist_sensors_bitmasks[i]);
      digitalWrite(PIN_SHIFT_REGISTER_LATCH, HIGH);
      delay(d);
      if (!dist_sensors[i].init()) {
        Serial.println(i);

        init_successful = false;
      } else {
        dist_sensors[i].setAddress(dist_sensors_addresses[i]);
        dist_sensors[i].setMeasurementTimingBudget(20000);
        dist_sensors[i].setTimeout(100);
      }
      delay(d);
    }
    d = d + 20;
  } while (!init_successful && d < 100);

  if(!init_successful) {
    Serial.println("Panic!");
  }
}

void write_to_shift_register(byte data) {
  digitalWrite(PIN_SHIFT_REGISTER_LATCH, LOW);
  shiftOut(PIN_SHIFT_REGISTER_DATA, PIN_SHIFT_REGISTER_CLOCK, MSBFIRST, data);
  digitalWrite(PIN_SHIFT_REGISTER_LATCH, HIGH);
}

int16_t distance(int sensor_id) {
  int16_t dist = dist_sensors[sensor_id].readRangeSingleMillimeters();
  if (dist_sensors[sensor_id].timeoutOccurred()) return 10000;

  if (dist < 0) dist = 0;
  if (dist > 2000) dist = 2000;

  return dist;
}

// returns current battery voltage in V (!!! actual battery voltage is around 0.8V higher due to voltage drop accross diodes)
float get_battery_voltage() {
  return analogRead(PIN_BATTERY_VOLTAGE) * (15.74 / 880);
}

void panic() {
  while (true) {
    digitalWrite(13, HIGH);
    delay(200);
    digitalWrite(13, LOW);
    delay(200);
  }
}

void init_robot() {
  /*
  Serial.println(" _____ _ _   _____ _ _     ");
  Serial.println("| __  |_| |_|   __| |_|___ ");
  Serial.println("| __ -| |  _|   __| | | . |");
  Serial.println("|_____|_|_| |__|  |_|_|  _|");
  Serial.println("                      |_|  ");
  Serial.println("");
  Serial.println("--- DEBUGGING WINDOW ---");*/

  start_up_bat_voltage = get_battery_voltage(); // store current bat voltage on startup so it does not have to be read out each loop iteration for m() function
  
  // battery is not switched on, don't start moving
  if (start_up_bat_voltage < 5.0) {
    pinMode(13, OUTPUT);
    m(0, 0, 0);
    while (start_up_bat_voltage < 5.0) {
      Serial.println("Waiting for power...");
      digitalWrite(13, HIGH);
      delay(500);
      digitalWrite(13, LOW);
      delay(500);
      start_up_bat_voltage = get_battery_voltage();
    }
  }
  pinMode(13, OUTPUT);
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
  if (left == -128) left = -127;
  if (right == -128) right = -127;
  uint8_t left_pwm  = abs(left) * 2 * MOTOR_CORRECTION_FACTOR;
  uint8_t right_pwm = abs(right) * 2 * MOTOR_CORRECTION_FACTOR; 
  
  // since bat voltage can be as high as 16.8V and we have 12V motors, we need to adjust the duty cycle based on bat voltage
  float normalize_factor = (12.0f / start_up_bat_voltage);
  if (normalize_factor > 1.0f) normalize_factor = 1.0f;
  left_pwm *= normalize_factor * 0.8f;
  right_pwm *= normalize_factor * 0.8f;
  
  /*
  Serial.print("Left: ");
  Serial.print(left_pwm);
  Serial.print("  Right: ");
  Serial.println(right_pwm);*/
  analogWrite(M_LEFT_EN, left_pwm);
  analogWrite(M_RIGHT_EN, right_pwm);
  delay(duration);
}

// wrapper for main m() method
void m(int8_t left, int8_t right) {
  m(left, right, 0);
}

void servo2(uint8_t id, uint8_t angle, bool stall) {
  Serial.print("Moving servo: ");
  Serial.print(id);
  Serial.print(" angle: ");
  Serial.print(angle);
  Serial.print(" stall?: ");
  Serial.println(stall);
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
  return analogRead(PIN_BATTERY_VOLTAGE) * (15.74 / 880);
}

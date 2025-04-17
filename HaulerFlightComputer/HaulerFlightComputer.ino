// LIBRARIES
#include <Wire.h>
#include <Servo.h>
#include <RF24.h>

// CONSTANTS

const float AILERON_ANGLE_MIN = -50;
const float AILERON_ANGLE_MAX = 50;

const float FLAP_ANGLE_MAX = 50;
const float FLAP_ANGLE_MIN = -50;

enum SurfaceID {
  AILERON_LEFT,
  AILERON_RIGHT,
  FLAP_LEFT,
  FLAP_RIGHT,
  _num
};

struct ControlSurface {
  Servo servo;
  int pin;
  float zero;
  float min;
  float max;
  int dir;  // +1 or -1 to correct for rotational symmetry
  void init() {
    servo.attach(pin);
    move(0);
    delay(1000);
    test();
  }

  void test() {
    for (float i = min; i <= max; i += 1.0) {
      move(i);
      delay(20);
    }

    for (float i = max; i >= min; i -= 1.0) {
      move(i);
      delay(20);
    }

    for (float i = min; i <= 0; i += 1.0) {
      move(i);
      delay(20);
    }
    move(0);
  }

  void move(float angle) {  //Moves to specified angle, accounting for zero-level
    float target = constrain(angle, min, max) * dir;
    Serial.println(target);
    servo.write(zero + target);
  }
};

// VARIABLES
ControlSurface surfaces[_num] = {
  { Servo(), 2, 90, AILERON_ANGLE_MIN, AILERON_ANGLE_MAX, 1 },  // AILERON_LEFT
  //{Servo(), 3, 90, AILERON_ANGLE_MIN, AILERON_ANGLE_MAX, 1 },  // AILERON_RIGHT
  { Servo(), 4, 80, FLAP_ANGLE_MIN, FLAP_ANGLE_MAX, -1 },  // FLAP_LEFT
  //{Servo(), 5, 90, FLAP_ANGLE_MIN, FLAP_ANGLE_MAX, 1 }         // FLAP_RIGHT
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  InitializeSystems();
  Serial.println("Initialized!");
}

void loop() {
}

void ProcessInputs() {
}

void InitializeSystems() {
  for (ControlSurface& s : surfaces) {
    s.init();
  }
}
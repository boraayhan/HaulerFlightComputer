// LIBRARIES
#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include <stdint.h>
#include "RF24.h"

#define RADIO_PIN_CE 7
#define RADIO_PIN_CSN 8

// CONSTANTS
const float AILERON_ANGLE_MIN = -40;
const float AILERON_ANGLE_MAX = 40;

const float FLAP_ANGLE_MAX = 0;
const float FLAP_ANGLE_MIN = -40;

uint8_t address[][6] = { "1Node", "2Node" };

enum ControlSurfaces {
  AILERON_LEFT = 0,
  AILERON_RIGHT,
  FLAP_LEFT,
  FLAP_RIGHT,
  _num
};

struct Payload {
  int32_t id;
  float p1;
  float p2;
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

// VARIABLES AND OBJECTS
RF24 radio(RADIO_PIN_CE, RADIO_PIN_CSN);

ControlSurface surfaces[_num] = {
  { Servo(), 2, 90, AILERON_ANGLE_MIN, AILERON_ANGLE_MAX, -1 },  // AILERON_LEFT
  { Servo(), 3, 90, AILERON_ANGLE_MIN, AILERON_ANGLE_MAX, 1 },   // AILERON_RIGHT
  { Servo(), 4, 80, FLAP_ANGLE_MIN, FLAP_ANGLE_MAX, -1 },        // FLAP_LEFT
  { Servo(), 5, 90, FLAP_ANGLE_MIN, FLAP_ANGLE_MAX, 1 }          // FLAP_RIGHT
};

Servo propeller;  // This is NOT a servo lmao

float flap = 0;
Payload payload;

// FUNCTIONS
void setup() {
  Serial.begin(115200);
  InitializeSystems();
  Serial.println("Initialized!");
}

void loop() {
  ReceiveRadio();
}
void InitializeSystems() {
  SetupRadio();
  for (ControlSurface& s : surfaces) {
    s.init();
  }
}

void SetupRadio() {  // Initializes radio
  if (!radio.begin()) {
    Serial.println(F("Error: Radio hardware failure!"));
    while (1) {}
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(Payload));
  radio.openReadingPipe(1, address[0]);
  radio.startListening();
}

void ReceiveRadio() {  // Receives radio payload {id, p1, p2}, processes accordingly
  uint8_t pipe;
  if (radio.available(&pipe)) {
    Payload payload;
    radio.read(&payload, sizeof(Payload));  // Receive the payload
    float p1 = payload.p1;
    float p2 = payload.p2;
    switch (payload.id) {
      case 0:  //Joystick input
        MoveSurfacesWithJoystick(p1, p2);
        break;
      case 2:  // delta flap
        flap += p1 * abs(FLAP_ANGLE_MIN) / 3;
        flap = constrain(flap, FLAP_ANGLE_MIN, FLAP_ANGLE_MAX);
        MoveFlaps();
        break;
      case 3: // Throttle
        float speed = map(p1, 0, 1, 0, 180)
        SetThrottle(speed);
        break;
      case 4:  // test surfaces
        for (ControlSurface& s : surfaces) {
          s.test();
          Serial.println("Testing surface");
          delay(500);
        }
        break;
    }
  }
}

void MoveSurfacesWithJoystick(float jX, float jY) {  // Translates payload data to aileron and elevator motion
  float pAileron = jX * (AILERON_ANGLE_MAX - AILERON_ANGLE_MIN);
  Serial.println(pAileron);
  surfaces[AILERON_LEFT].move(pAileron);
  surfaces[AILERON_RIGHT].move(pAileron);
}

void MoveFlaps() {  // Updates flap level
  surfaces[FLAP_LEFT].move(flap);
  surfaces[FLAP_RIGHT].move(flap);
}

void SetThrottle(float speed) {
  propeller.write(speed);
}
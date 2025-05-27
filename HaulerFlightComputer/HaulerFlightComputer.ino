// LIBRARIES
#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include <stdint.h>
#include "RF24.h"

#define RADIO_PIN_CE 7
#define RADIO_PIN_CSN 8

#define PROPELLER_PIN 49

// CONSTANTS
const float FLAPERON_RATIO_CONSTANT = 0;  // 0 for flaperon mode off, 0.3 recommended

const float AILERON_POS_MIN = -40;
const float AILERON_POS_MAX = 40;

const float FLAP_POS_MAX = 0;
const float FLAP_POS_MIN = -40;

const float ELEVATOR_POS_MIN = -40;
const float ELEVATOR_POS_MAX = 40;

float flap = 0;

uint8_t address[][6] = { "1Node", "2Node" };

enum ControlSurfaces {
  AILERON_LEFT = 0,
  AILERON_RIGHT,
  FLAP_LEFT,
  FLAP_RIGHT,
  ELEVATOR,
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

  void move(float angle) {                                                               //Moves to specified angle, accounting for zero-level
    float target = constrain((angle + flap * FLAPERON_RATIO_CONSTANT), min, max) * dir;  //TODO: Verify
    //Serial.println(target);
    servo.write(zero + target);
  }
};

// VARIABLES AND OBJECTS
RF24 radio(RADIO_PIN_CE, RADIO_PIN_CSN);
Payload payload;
Servo propeller;  // This is NOT a servo lmao
bool active = false;

ControlSurface surfaces[_num] = {
  { Servo(), 2, 90, AILERON_POS_MIN, AILERON_POS_MAX, -1 },   // AILERON_LEFT
  { Servo(), 3, 90, AILERON_POS_MIN, AILERON_POS_MAX, 1 },    // AILERON_RIGHT
  { Servo(), 4, 80, FLAP_POS_MIN, FLAP_POS_MAX, -1 },         // FLAP_LEFT
  { Servo(), 5, 90, FLAP_POS_MIN, FLAP_POS_MAX, 1 },          // FLAP_RIGHT
  { Servo(), 6, 90, ELEVATOR_POS_MIN, ELEVATOR_POS_MAX, 1 },  // ELEVATOR_LEFT
};

// FUNCTIONS
void setup() {
  Serial.begin(115200);
  InitializeSystems();
  Serial.println("Initialized!");
}

void loop() {
  ReceiveRadio();
  if (!active) {
    SetThrottle(0);
  }
}
void InitializeSystems() {
  SetupRadio();
  propeller.attach(PROPELLER_PIN);
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
    radio.read(&payload, sizeof(Payload));  // Receive the payload
    float p1 = payload.p1;
    float p2 = payload.p2;
    if (payload.id == 0) {  // Joystick input
      MoveSurfacesWithJoystick(payload.p1, payload.p2);
    }

    if (payload.id == 2) {  // delta flap
      flap += payload.p1 * abs(FLAP_POS_MIN) / 3;
      flap = constrain(flap, FLAP_POS_MIN, FLAP_POS_MAX);
      MoveFlaps();
    }

    if (payload.id == 3) {  // Throttle
      float speed = payload.p1 * 180;
      SetThrottle(speed);
      Serial.println(speed);
    }

    if (payload.id == 4) {  // test surfaces
      for (ControlSurface& s : surfaces) {
        s.test();
        Serial.println("Testing surface");
        delay(500);
      }
    }

    if (payload.id == 5) {  // Enable/Disable engine
      active = !active;
      Serial.println("active");
    }
  }
}

void MoveSurfacesWithJoystick(float jX, float jY) {  // Translates payload data to aileron and elevator motion
  float pAileron = jX * (AILERON_POS_MAX - AILERON_POS_MIN);
  float pElevator = jY * (ELEVATOR_POS_MAX - ELEVATOR_POS_MIN);

  surfaces[AILERON_LEFT].move(pAileron);
  surfaces[AILERON_RIGHT].move(pAileron);
  surfaces[ELEVATOR].move(pElevator);
}

void MoveFlaps() {  // Updates flap level
  surfaces[FLAP_LEFT].move(flap);
  surfaces[FLAP_RIGHT].move(flap);
}

void SetThrottle(float speed) {
  if (active) {
    propeller.write(speed);
  } else {
    propeller.write(0);
  }
}
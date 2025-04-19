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

enum SurfaceID {W
  AILERON_LEFT,
  AILERON_RIGHT,
  FLAP_LEFT,
  FLAP_RIGHT,
  _num
};

uint8_t address[][6] = { "1Node", "2Node" };


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
    servo.write(zero + target);
  }
};

// VARIABLES AND OBJECTS

RF24 radio(RADIO_PIN_CE, RADIO_PIN_CSN);

ControlSurface surfaces[_num] = {
  { Servo(), 2, 90, AILERON_ANGLE_MIN, AILERON_ANGLE_MAX, 1 },  // AILERON_LEFT
  //{Servo(), 3, 90, AILERON_ANGLE_MIN, AILERON_ANGLE_MAX, 1 },  // AILERON_RIGHT
  { Servo(), 4, 80, FLAP_ANGLE_MIN, FLAP_ANGLE_MAX, -1 },  // FLAP_LEFT
  //{Servo(), 5, 90, FLAP_ANGLE_MIN, FLAP_ANGLE_MAX, 1 }         // FLAP_RIGHT
};

float flap = 0;

Payload payload;

// FUNCTIONS
void setup() {
  // put your setup code here, to run once:
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

void SetupRadio() {
  if (!radio.begin()) {
    Serial.println(F("Error: Radio hardware failure!"));
    while (1) {}
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(Payload));
  radio.openReadingPipe(1, address[0]);
  radio.startListening();
}

void ReceiveRadio() {
  uint8_t pipe;
  if (radio.available(&pipe)) {
    Payload payload;
    radio.read(&payload, sizeof(Payload));  // Receive the payload
    float p1 = payload.p1;
    float p2 = payload.p2;
    switch (payload.id) {
      case 0:  //Joystick input
      processJoystick(p1, p2);
        break;
      case 1:  // set flap
        flap = p1;
        surfaces[FLAP_LEFT].move(flap);
        //surfaces[FLAP_RIGHT].move(flap);
        break;
      case 2:  // delta flap
        //flaps.move(flap + p1);
        flap += p1;
        surfaces[FLAP_LEFT].move(flap);
        //surfaces[FLAP_RIGHT].move(flap);
        break;
      case 3:  // thrust set
               // engine1.write(payload.p1);
               // engine2.write(payload.p2);
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

void processJoystick(float rX, float rY) {
  surfaces[AILERON_LEFT].move(rX * (AILERON_ANGLE_MAX - AILERON_ANGLE_MIN));
  //Serial.println(rX*(AILERON_ANGLE_MAX - AILERON_ANGLE_MIN));
}
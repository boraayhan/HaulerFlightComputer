// LIBRARIES
#include <Wire.h>
#include <Servo.h>
#include <SPI.h>
#include <stdint.h>
#include <RF24.h>
#include <VL53L0X.h>

#define RADIO_PIN_CE 7
#define RADIO_PIN_CSN 8

#define PROPELLER_PIN 10

#define GEAR_DOWN 0
#define GEAR_UP 1

// CONSTANTS
const float FLAPERON_RATIO_CONSTANT = 0.3;  // 0 for flaperon mode off, 0.3 recommended

const float AILERON_POS_MIN = -70;
const float AILERON_POS_MAX = 70;

const float ELEVATOR_POS_MIN = -45;
const float ELEVATOR_POS_MAX = 45;

const float AUTOPILOT_DISENGAGE_THRESHOLD = 0.3;  // 0 to 1, how "hard" input is needed to disengage AP. > 0.5 is dangerous.

/*
const float KP_ROLL = 1; // FIXME: Tune!!!
const float KI_ROLL = 1; // FIXME: Tune!!!
const float KD_ROLL = 1; // FIXME: Tune!!!
*/

float flap = 0;
float throttle = 0;

uint8_t address[][6] = { "1Node", "2Node" };

enum ControlSurfaces {
  AILERON_LEFT = 0,
  AILERON_RIGHT,
  ELEVATOR_LEFT,
  ELEVATOR_RIGHT,
  num_surface
};

enum AutopilotDirections {
  roll = 0,
  pitch,
  yaw,
};

struct Payload {
  int32_t id;
  float p1;
  float p2;
};

struct LandingGear {

  Servo servo;
  int PIN_SIGNAL;
  float ANGLE_RETRACTED;
  float ANGLE_DEPLOYED;

  void enable() {
    servo.attach(PIN_SIGNAL);
    deploy();
  }

  void disable() {
    servo.detach();
  }

  void deploy() {
    servo.write(ANGLE_DEPLOYED);
  }

  void retract() {
    servo.write(ANGLE_RETRACTED);
  }
};

struct ControlSurface {
  Servo servo;
  int PIN_SIGNAL;
  float zero;
  float trimAngle;
  float min;
  float max;
  int dir;  // +1 or -1 to correct for rotational symmetry
  void init() {
    servo.attach(PIN_SIGNAL);
    move(0);
  }

  void test() {
    for (float i = 0; i < max; i++) {
      move(i);
      delay(5);
    }

    for (float i = max; i > min; i--) {
      move(i);
      delay(5);
    }

    for (float i = min; i <= 0; i += 1.0) {
      move(i);
      delay(5);
    }
    move(0);
  }

  void move(float angle) {                                                                           // Moves to specified angle, accounting for zero-level
    float target = constrain((angle + trimAngle + flap * FLAPERON_RATIO_CONSTANT), min, max) * dir;  // TODO: Verify
    // Serial.println(target);
    servo.write(zero + target);
  }

  void trim(float angle) {
    trimAngle += dir * angle;
  }
};

// VARIABLES AND OBJECTS
RF24 radio(RADIO_PIN_CE, RADIO_PIN_CSN);
Payload payload;
Servo propeller;  // This is NOT a servo lmao

bool autopilot[3] = { false, false, false };

LandingGear gear[3] = {
  // Not used.
};

ControlSurface surfaces[num_surface] = {
  { Servo(), 3, 90, 0, AILERON_POS_MIN, AILERON_POS_MAX, -1 },     // AILERON_LEFT
  { Servo(), 4, 90, 0, AILERON_POS_MIN, AILERON_POS_MAX, -1 },     // AILERON_RIGHT
  { Servo(), 5, 90, 0, ELEVATOR_POS_MIN, ELEVATOR_POS_MAX, 1 },  // ELEVATOR_LEFT
  { Servo(), 6, 70, 0, ELEVATOR_POS_MIN, ELEVATOR_POS_MAX, -1 },   // ELEVATOR_RIGHT
};

// FUNCTIONS
void setup() {
  Serial.begin(115200);
  InitializeSystems();
  delay(1000);
}

void loop() {
  ReceiveRadio();
  SetThrottle();
  /*if (autopilot[roll])
  { // Maintains zero bank angle
    RollAutopilot();
  }
  if (autopilot[pitch])
  {
  }*/
}

void InitializeSystems() {
  propeller.attach(PROPELLER_PIN);
  propeller.writeMicroseconds(1000);
  SetupRadio();
  for (ControlSurface &s : surfaces) {
    s.init();
  }
  Serial.println(F("Initialized!"));
}

void SetupRadio() {  // Initializes radio
  if (!radio.begin()) {
    Serial.println(F("Error: Radio hardware failure!"));
    while (1) {
    }
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(Payload));
  radio.openReadingPipe(1, address[0]);
  radio.startListening();
  delay(1000);  // Allow for radio to stabilize
}

void ReceiveRadio() {  // Receives radio payload {id, p1, p2}, processes accordingly
  uint8_t pipe;
  if (radio.available(&pipe)) {
    radio.read(&payload, sizeof(Payload));  // Receive the payload
    Serial.print(payload.id);
    int id = payload.id;
    float p1 = payload.p1;
    float p2 = payload.p2;

    if (id == 0) {  // Joystick input
      MoveSurfaces(p1, p2);
      if (abs(p1) > AUTOPILOT_DISENGAGE_THRESHOLD || abs(p2) > AUTOPILOT_DISENGAGE_THRESHOLD) {  // Disengage autopilot
        DisengageAutopilot();
      }
    }

    if (id == 1) {  // Trim
      if (p1 == 0)  // Aileron trim
      {
        surfaces[AILERON_LEFT].trim(p2);
        surfaces[AILERON_RIGHT].trim(p2);

        surfaces[AILERON_LEFT].move(0);
        surfaces[AILERON_RIGHT].move(0);
      }
      if (p1 == 1)  // Elevator trim
      {
        surfaces[ELEVATOR_LEFT].trim(p2);
        surfaces[ELEVATOR_RIGHT].trim(p2);

        surfaces[ELEVATOR_LEFT].move(0);
        surfaces[ELEVATOR_RIGHT].move(0);
      }
    }

    if (id == 3) {  // Throttle
      if (p1 < 0.07) {
        throttle = 0;
      } else {
        throttle = p1;
      }
    }

    if (id == 4) {  // Resets trim on all surfaces
      for (ControlSurface &s : surfaces) {
        s.trimAngle = 0;
        s.move(0);
      }
    }
  }
}

void MoveSurfaces(float jR, float jP) {                           // Translates a value (-1.00 to 1.00) to aileron and elevator motion
  float pAileron = jR * (AILERON_POS_MAX - AILERON_POS_MIN) / 2;  // TODO: Verify whether div by 2 is correct
  float pElevator = jP * (ELEVATOR_POS_MAX - ELEVATOR_POS_MIN) / 2;

  surfaces[AILERON_LEFT].move(pAileron);
  surfaces[AILERON_RIGHT].move(pAileron);
  surfaces[ELEVATOR_LEFT].move(pElevator);
  surfaces[ELEVATOR_RIGHT].move(pElevator);
}

void SetThrottle() {
  float speed = constrain(1000 + 1000 * throttle, 1000, 2000);
  propeller.writeMicroseconds(speed);
}

void TestSurfaces() {
  for (ControlSurface &s : surfaces) {
    s.test();
    Serial.println(F("Testing surface"));
    delay(500);
  }
  Serial.println(F("Testing complete!"));
}

void DisengageAutopilot() {
  autopilot[roll] = false;
  autopilot[pitch] = false;
  autopilot[yaw] = false;
}

void SetLandingGear(int state) {
}
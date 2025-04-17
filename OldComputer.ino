//Esp
#include <Servo.h>

#define MAX_AILERON_ANGLE 25  //Maximum flaperon angle. Higher values are less stable but allow for more freedom of motion. Does NOT include flaps

#define VRX_PIN A0
#define VRY_PIN A1
#define F1_PIN 2
#define F2_PIN 3

double jX = 0.001;
double jY = 0;
int flap = 0;

Servo f1;  //Flaperon
Servo f2;
Servo elev;  //Elevon

float f1_a0 = 88;  //Calibrated zero-level for flaperons, initially manual but might add method to calibrate it midair using PID
float f2_a0 = 111;
void setup() {
  Serial.begin(9600);
  f1.attach(F1_PIN);
  f2.attach(F2_PIN);
}

void loop() {
  //TestFlaperons(30);
  UpdateControls();
}

void TestFlaperons(float ang) {
  f1.write(f1_a0 - ang);
  delay(700);
  f1.write(f1_a0 + ang);
  delay(700);
}

void UpdateControls() {
  if (abs(analogRead(VRX_PIN) - 505) > 15) {
    jX = constrain(analogRead(VRX_PIN) - 505, -400, 400)/400.00 * MAX_AILERON_ANGLE;
    f1.write(f1_a0 + jX - flap);
    f2.write(f2_a0 + jX + flap);
  } else {
    if (jX != 0) {
      f1.write(f1_a0 - flap);
      f2.write(f2_a0 + flap);
    }
    jX = 0;
  }
  jY = constrain(analogRead(VRY_PIN) - 500, -400, 400);
  Serial.println(jX);
  //Serial.println(jY);
}

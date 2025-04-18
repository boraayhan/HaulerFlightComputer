#include <SPI.h>
#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8

const float FLAP_ANGLE_MAX = 0;
const float FLAP_ANGLE_MIN = -40;

RF24 radio(CE_PIN, CSN_PIN);

uint8_t address[][6] = { "1Node", "2Node" };

bool flapDown = false;

struct Payload {
  int id;
  float p1;
  float p2;
};

// Array of recognized command strings
const char* commands[] = { "C0", "C1", "", "FLAP", "TEST" };

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  SetupRadio();
}

void loop() {
  EvaluateInput();
}

void SetupRadio() {
  if (!radio.begin()) {
    Serial.println(F("Error: Radio hardware failure!"));
    while (1) {}
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(Payload));
  radio.openWritingPipe(address[0][6]);
  radio.openReadingPipe(1, address[1][6]);
  radio.stopListening();
}

bool TransmitPayload(int id, float p1, float p2) {
  Payload p = { id, p1, p2 };
  bool report = radio.write(&p, sizeof(p));  // transmit & save the report
  if (report) {
    //Succesful transmission
  } else {
    Serial.println(F("Transmission failed or timed out"));
    return false;
  }
  delay(20);
}

bool GetJoystickFromSerial() {
  while (Serial.available() > 2) {
    float inp = Serial.read();
    if (inp == 'J') { //Joystick Input for Aileron
      float jX = Serial.read();
      float jY = Serial.read();
      return TransmitPayload(0, jX, jY);;
    }
    if (inp == 'F') { //Flap Delta
      jY = Serial.read();
      jX = Serial.read();
      return true;
    }
    if (inp == 'R') {
      jY = Serial.read();
      jX = Serial.read();
      return true;
    }
    if (inp == 'R') {
      jY = Serial.read();
      jX = Serial.read();
      return true;
    }
    return false;
  }
}
int GetCommandFromSerial() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();

    for (int i = 0; i < (sizeof(commands) / sizeof(commands[0])); i++) {
      if (input == commands[i]) {
        return i;
      }
    }
    Serial.println("Unknown command: " + input);
  }
  return -1;
}

void EvaluateInput() {
  int c = GetCommandFromSerial();
  if (c != -1) {
    Serial.println("Valid");
    switch (c) {
      case 1:
      if(flapDown)
        TransmitPayload(3, FLAP_ANGLE_MIN , 0);
      else
        TransmitPayload(3, FLAP_ANGLE_MIN , 0);
      flapDown = !flapDown;
      case 4:
        TransmitPayload(4, 0, 0);
        Serial.println("Testing Surfaces...");
        break;
    }
  }
}
#include <SPI.h>
#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);

uint8_t address[][6] = { "1Node", "2Node" };

struct Payload {
  int id;
  float p1;
  float p2;
};

// Array of recognized command strings
const char* commands[] = { "C0", "C1", "C2", "C3", "TEST",};

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
  Payload p = {id, p1, p2}; 
  bool report = radio.write(&p, sizeof(p));  // transmit & save the report
  if (report) {
    //Succesful transmission
  } else {
    Serial.println(F("Transmission failed or timed out"));
    return false;
  }
  delay(20);
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
    switch(c) {
      case 4:
        TransmitPayload(4, 0, 0);
        Serial.println("Testing...");
      break;
    }
  }
}
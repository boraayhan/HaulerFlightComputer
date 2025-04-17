#include <SPI.h>
#include "printf.h"
#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8

RF24 radio(CE_PIN, CSN_PIN);

float payload[3] = {0, 0, 0};

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  SetupRadio();
}

void loop() {
  TransmitPayload();
}

void SetupRadio() {
  if (!radio.begin()) {
    Serial.println(F("Error: Radio hardware failure!"));
    while (1) {}
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(payload));
  radio.openWritingPipe("1Node");
  radio.openReadingPipe(1, "2Node");
  radio.stopListening();
}

bool TransmitPayload() {
  payload[2] = millis() / 1000;
  bool report = radio.write(payload, sizeof(payload));  // transmit & save the report
  if (report) {
    //Succesful transmission
  } else {
    Serial.println(F("Transmission failed or timed out"));
    return false;
  }
  delay(20);
}
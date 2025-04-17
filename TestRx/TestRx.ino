#include <SPI.h>
#include "printf.h"
#include "RF24.h"

#define CE_PIN 7
#define CSN_PIN 8
RF24 radio(CE_PIN, CSN_PIN);

float payload[] = { 0, 0, 0, 0 };

void setup() {
  Serial.begin(115200);
  SetupRadio();
}

void loop() {
  ReceiveRadio();
}

void SetupRadio() {
  if (!radio.begin()) {
    Serial.println(F("Error: Radio hardware failure!"));
    while (1) {}
  }
  radio.setPALevel(RF24_PA_LOW);
  radio.setPayloadSize(sizeof(payload));  //FIXME: set proper size for array
  radio.openWritingPipe("2Node");
  radio.openReadingPipe(1, "1Node");
  radio.startListening();
}

void ReceiveRadio() {
  uint8_t pipe;
  if (radio.available(&pipe)) {
    uint8_t bytes = radio.getPayloadSize();
    radio.read(payload, bytes);  // Payload is stored into the payload variable
    switch (payload[0]) {
      case 0:  //Joystick input
        //rX = payload[1];
        //rY = payload[2];
        //processJoystick[]
        break;
      case 1:  // set flap
               //flaps.move(payload[1]);
        break;
      case 2:  // delta flap
               //flaps.move(flap + payload[1]);
        break;
      case 3:  // rudder
               // engine 1 thrust -= payload[1];
               // engine 2 thrust += payload[1];
        break;
        //
    }
  }
}
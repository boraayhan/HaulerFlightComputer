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
  radio.setPayloadSize(sizeof(Payload));  //FIXME: set proper size for array
  radio.openWritingPipe(address[1][6]);
  radio.openReadingPipe(1, address[0][6]);
  radio.startListening();
}

void ReceiveRadio() {
  uint8_t pipe;
  if (radio.available(&pipe)) {
    uint8_t bytes = radio.getPayloadSize();
    Payload payload;
    radio.read(&payload, bytes);  // Payload is stored into the payload variable
    switch (payload.id) {
      case 0:  //Joystick input
        //rX = payload.p1;
        //rY = payload.p2;
        //processJoystick(rX, rY);
        break;
      case 1:  // set flap
               //flaps.move(payload.p1);
        break;
      case 2:  // delta flap
               //flaps.move(flap + payload.p1);
        break;
      case 3:  // thrust set
               // engine1.write(payload.p1);
               // engine2.write(payload.p2);
        break;
      case 4:  // rudder
               // engine 1 thrust -= payload.p1;
               // engine 2 thrust += payload.p1;
        break;
        //
    }
  }
}
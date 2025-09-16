#include <SPI.h>
#include "RF24.h"
#include <stdint.h>


#define RADIO_CE_PIN 7
#define RADIO_CSN_PIN 8

struct Payload {
  int32_t id;
  float p1;
  float p2;
};

RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);
uint8_t addresses[][6] = { "1Node", "2Node" };
Payload receivedData;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  if (!radio.begin()) {
    Serial.println(F("RF24 initialization failed!"));
    while (1)
      ;
  }

  radio.setPayloadSize(sizeof(Payload));
  radio.setPALevel(RF24_PA_HIGH);
  radio.openWritingPipe(addresses[0]);
  radio.stopListening();
  Serial.println(F("Transmitter Initialized!"));
}

void loop() {
  if (Serial.available() >= sizeof(Payload)) {
    Serial.readBytes((char*)&receivedData, sizeof(Payload));
    if (!radio.write(&receivedData, sizeof(Payload))) {
      Serial.println(F("RF24 transmission fail"));
    }
  }
}
#include <SPI.h>
#include "RF24.h"
#include <stdint.h>

// ------------------------
// NRF24L01 Pin Definitions
#define RADIO_CE_PIN 7
#define RADIO_CSN_PIN 8

// ------------------------
// Payload Struct - must match Python and RX
struct Payload {
  int32_t id;  // 4 bytes
  float p1;    // 4 bytes
  float p2;    // 4 bytes
};

// ------------------------
// RF24 Setup
RF24 radio(RADIO_CE_PIN, RADIO_CSN_PIN);
uint8_t addresses[][6] = { "1Node", "2Node" };  // Must match RX

// ------------------------
// Global Variables
Payload receivedData;

void setup() {
  Serial.begin(115200);             // Match Python baud
  while (!Serial) {}                // Wait for USB

  if (!radio.begin()) {
    Serial.println(F("RF24 initialization failed!"));
    while (1);
  }

  radio.setPayloadSize(sizeof(Payload));  // 12 bytes
  radio.setPALevel(RF24_PA_LOW);          // Safe power level
  radio.openWritingPipe(addresses[0]);    // TX pipe
  radio.stopListening();                  // We're transmitting

  Serial.println(F("TX Arduino ready."));
}

void loop() {
  if (Serial.available() >= sizeof(Payload)) {
    Serial.readBytes((char*)&receivedData, sizeof(Payload));

    // Print received from Python
    Serial.print(F("From Serial → ID: "));
    Serial.print(receivedData.id);
    Serial.print(F(" | P1: "));
    Serial.print(receivedData.p1, 6);
    Serial.print(F(" | P2: "));
    Serial.println(receivedData.p2, 6);

    // Send over RF
    bool success = radio.write(&receivedData, sizeof(Payload));

    if (success) {
      Serial.println(F("Payload sent via RF24."));
    } else {
      Serial.println(F("⚠️ RF24 transmission failed."));
    }

    delay(20);  // Short delay to prevent radio spamming
  }
}




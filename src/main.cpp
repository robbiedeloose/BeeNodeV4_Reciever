#include <Arduino.h>

///////////////////////////////////// RADIO ////////////////////////////////////
#include <RF24.h>
#include <RF24Network.h>
#include <SPI.h>
#define radioPin1 7 // RF24
#define radioPin2 8 // RF24
RF24 radio(radioPin1, radioPin2); // start RF24 communication layer
RF24Network network(radio);       // start RF24 network layer
const uint16_t thisNode = 00;     // Coordinator address

struct Payload_t {
  uint8_t id[4];
  int16_t temp;
  uint16_t bat;
  uint16_t weight;
  uint16_t humidity;
};

////////////////////////// FUNCTION DECLARATIONS ///////////////////////////////
// radio functions
void initRFRadio(uint8_t channel, uint16_t nodeAddress);
void checkForNetworkData();

/////////////// SETUP //////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println(F("BeeNode v4.0.1 - nRF24 tester"));
  initRFRadio(90, thisNode); // start nRF24l radio

  Serial.println("init complete");
}

/////////////// LOOP ///////////////////////////////////////////////////////////
void loop() { checkForNetworkData(); }

//// Getting data //////////////////////////////////////////////////////////////
void checkForNetworkData() {
  network.update();
  RF24NetworkHeader header;
  Payload_t payload;

  while (network.available()) { // Any data on the network ready to read
    network.read(header, &payload, sizeof(payload));
    Serial.print(" Node ID: ");
    for (byte b : payload.id)
      Serial.print(b, HEX);
    Serial.println();
    Serial.print(" Node ID: ");
    Serial.println(payload.temp);
  }
}

//////////// init nrf radio ////////////////////////////////////////////////////
void initRFRadio(uint8_t channel, uint16_t nodeAddress) { // clean
  Serial.print("Channel: ");
  Serial.print(channel);
  Serial.print(", Node:  ");
  Serial.println(nodeAddress);

  SPI.begin();
  radio.begin();
  // radio.setPALevel(HIGH);
  network.begin(channel, nodeAddress);
}

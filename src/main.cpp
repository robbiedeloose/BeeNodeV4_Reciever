#include <Arduino.h>
#include <Wire.h>
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

// Globalstruct array to collect data before Sending
#define BUFFERSIZE 6
Payload_t plBuffer[BUFFERSIZE];

////////////////////////// FUNCTION DECLARATIONS ///////////////////////////////
void initRFRadio(uint8_t channel, uint16_t nodeAddress);
void checkForNetworkData();
void requestEvent();
void wake();

uint8_t isAwake = 0;

/////////////// SETUP //////////////////////////////////////////////////////////
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();   
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("BeeNode v4.0.1 - nRF24 tester"));
  initRFRadio(90, thisNode); // start nRF24l radio
  pinMode(3,INPUT); 
  Wire.begin(8);  // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event
      attachInterrupt(digitalPinToInterrupt(3), wake, FALLING);
  Serial.println("init complete");
  delay(1000);
}

/////////////// LOOP ///////////////////////////////////////////////////////////
void loop() { 
  if (isAwake == 1) {
    Serial.println("awake");
    delay(1000);
  }
  else {
    Serial.println("sleep");
    checkForNetworkData();
    delay(250);
    network.sleepNode(1, 0); // 8 cycles of 4 seconds
  }
}

//// Getting data //////////////////////////////////////////////////////////////
void checkForNetworkData() {
  network.update();
  RF24NetworkHeader header;
  Payload_t payload;
  while (network.available()) { // Any data on the network ready to read
    network.read(header, &payload, sizeof(payload));
 
    // fill buffer
    fillBufferArray(&payload);

    Serial.println();
    Serial.print(" - temp: ");
    Serial.println(payload.temp);
    Serial.print(" - hum: ");
    Serial.println(payload.humidity);
    Serial.print(" - bat: ");
    Serial.println(payload.bat);
    Serial.print(" - weight: ");
    Serial.println(payload.weight);
  }
}

void fillBufferArray(Payload_t *payloadAddress) {
  uint8_t bufferLocation = 0;
  // get next free buffer location
  for (int i = 0; i < BUFFERSIZE; i++) {
    if (plBuffer[bufferLocation].bat != 0)
      bufferLocation++;
  }
  Serial.print(F(" Array position "));
  Serial.println(bufferLocation); // print the buffer location that is used
  for (int i = 0; i < 4; i++) {
    plBuffer[bufferLocation].id[i] = payloadAddress->id[i];
  }
  plBuffer[bufferLocation].temp = payloadAddress->temp;
  plBuffer[bufferLocation].humidity = payloadAddress->humidity;
  plBuffer[bufferLocation].bat = payloadAddress->bat;
  plBuffer[bufferLocation].weight = payloadAddress->weight;
}

//////////// init nrf radio ////////////////////////////////////////////////////
void initRFRadio(uint8_t channel, uint16_t nodeAddress) { // clean
  Serial.print("Channel: ");
  Serial.print(channel);
  Serial.print(", Node:  ");
  Serial.println(nodeAddress);

  SPI.begin();
  radio.begin();
  radio.setPALevel(HIGH);
  network.begin(channel, nodeAddress);
  network.setup_watchdog(8);
}

void requestEvent() {
  isAwake = 0;
  Serial.println("trigger");
  char buf[120] = "";
  sprintf(buf, "%i,%u,%u,%02X%02X%02X%02X", plBuffer[0].temp,plBuffer[0].humidity, plBuffer[0].bat, plBuffer[0].id[0], plBuffer[0].id[1], plBuffer[0].id[2], plBuffer[0].id[3]);
  Serial.println(buf);
  Wire.write(buf); // respond with message of 6 bytes as expected by master
  Serial.println("set interrupt");
  attachInterrupt(digitalPinToInterrupt(3), wake, LOW);
}

void wake() {
  detachInterrupt(1); 
  isAwake = 1;
}
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
void fillBufferArray(Payload_t *payloadAddress);
void clearBufferArray();
void displayBufferArray();
void requestEvent();
void wake();

uint8_t isAwake = 0;
uint8_t readNumber = 0;

/////////////// SETUP //////////////////////////////////////////////////////////
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Wire.begin();
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("BeeNode v4.0.1 - nRF24 reciever"));
  initRFRadio(90, thisNode); // start nRF24l radio
  pinMode(3,INPUT);
  Wire.begin(8); // join i2c bus with address #8
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
    network.sleepNode(1, digitalPinToInterrupt(2)); // x cycles of watchdog time (see below), interupt pin
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
    Serial.print(" Node: ");
    for (int i = 0; i < 4; i++) {
      Serial.print(payload.id[i]);
    }
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
  Serial.print(F("Array position "));
  Serial.println(bufferLocation); // print the buffer location that is used
  for (int i = 0; i < 4; i++) {
    plBuffer[bufferLocation].id[i] = payloadAddress->id[i];
  }
  plBuffer[bufferLocation].temp = payloadAddress->temp;
  plBuffer[bufferLocation].humidity = payloadAddress->humidity;
  plBuffer[bufferLocation].bat = payloadAddress->bat;
  plBuffer[bufferLocation].weight = payloadAddress->weight;
}

void clearBufferArray(){
  Serial.println("Clear buffer array");
  for (int i = 0; i < BUFFERSIZE; i++) {
    plBuffer[i].bat = 0;
  }
}

void displayBufferArray() {
  for (int bufferLocation = 0; bufferLocation < BUFFERSIZE; bufferLocation++) {
    Serial.print(" Node: ");
    for (int i = 0; i < 4; i++) {
      Serial.print(plBuffer[bufferLocation].id[i]);
    }
    Serial.println();
    Serial.print(" - temp: ");
    Serial.println(plBuffer[bufferLocation].temp);
    Serial.print(" - hum: ");
    Serial.println(plBuffer[bufferLocation].humidity);
    Serial.print(" - bat: ");
    Serial.println(plBuffer[bufferLocation].bat);
    Serial.print(" - weight: ");
    Serial.println(plBuffer[bufferLocation].weight);
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
  radio.setPALevel(HIGH);
  network.begin(channel, nodeAddress);
  network.setup_watchdog(8);
}

void requestEvent() {
  Serial.print("read ");
  Serial.println(readNumber);
  char buf[120] = "";
  sprintf(buf, "%02X%02X%02X%02X,%04i,%03u,%03u,%06u", plBuffer[readNumber].id[0], plBuffer[readNumber].id[1], plBuffer[readNumber].id[2], plBuffer[readNumber].id[3], plBuffer[readNumber].temp, plBuffer[readNumber].humidity, plBuffer[readNumber].bat, plBuffer[readNumber].weight);
  Serial.println(buf);
  Wire.write(buf); // respond with message of 6 bytes as expected by master
  readNumber ++;
  if (readNumber > 5) { //which means all payloads are sent
    isAwake = 0;
    readNumber = 0;
    clearBufferArray();
    Serial.println("set interrupt");
    attachInterrupt(digitalPinToInterrupt(3), wake, LOW);
  }
}

void wake() {
  detachInterrupt(1); 
  isAwake = 1;
}

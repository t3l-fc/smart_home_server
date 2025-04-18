#include <Arduino.h>
#include "Display.h"

// Pin definitions
const int vinylePin = 27;
const int ananasPin = 15;
const int dinoPin = 32;
const int cactusPin = 14;
const int allPlugsPin = 33;

// Current states
bool cactusState = false;
bool ananasState = false;
bool dinoState = false;
bool vinyleState = false;
bool allPlugsState = false;

Display alpha4 = Display();

void initStates() {
  cactusState = !digitalRead(cactusPin);
  ananasState = !digitalRead(ananasPin);
  dinoState = !digitalRead(dinoPin);
  vinyleState = !digitalRead(vinylePin);
  allPlugsState = !digitalRead(allPlugsPin);
}

void setup() {
  Serial.begin(115200);
  delay(4000);
  
  Serial.println("Display setup");
  alpha4.setup();
  
  // Initialize switch pins as inputs with pull-up resistors
  pinMode(cactusPin, INPUT_PULLUP);
  pinMode(ananasPin, INPUT_PULLUP);
  pinMode(dinoPin, INPUT_PULLUP);
  pinMode(vinylePin, INPUT_PULLUP);
  pinMode(allPlugsPin, INPUT_PULLUP);
  
  initStates();

  Serial.println("Display setup done");
  alpha4.display("CACA");
  
  // Show initial states
  delay(1000);
  //alpha4.showStates(cactusState, ananasState, dinoState, vinyleState);
}

void loop() {
  Serial.print("Cactus: ");
  Serial.print(cactusState);
  Serial.print(", Ananas: ");
  Serial.print(ananasState);
  Serial.print(", Dino: ");
  Serial.print(dinoState);
  Serial.print(", Vinyle: ");
  Serial.print(vinyleState);
  Serial.print(", All Plugs: ");
  Serial.println(allPlugsState);

  if(digitalRead(cactusPin) == LOW && cactusState == false) {
    cactusState = true;
    alpha4.display("CA_+");
  }
  if(digitalRead(cactusPin) == HIGH && cactusState == true) {
    cactusState = false;
    alpha4.display("CA_-");
  }

  if(digitalRead(ananasPin) == LOW && ananasState == false) {
    ananasState = true;
    alpha4.display("AN_+");
  }
  if(digitalRead(ananasPin) == HIGH && ananasState == true) {
    ananasState = false;
    alpha4.display("AN_-");
  }

  if(digitalRead(dinoPin) == LOW && dinoState == false) {
    dinoState = true;
    alpha4.display("DI_+");
  }
  if(digitalRead(dinoPin) == HIGH && dinoState == true) {
    dinoState = false;
    alpha4.display("DI_-");
  }

  if(digitalRead(vinylePin) == LOW && vinyleState == false) {
    vinyleState = true;
    alpha4.display("VI_+");
  }
  if(digitalRead(vinylePin) == HIGH && vinyleState == true) {
    vinyleState = false;
    alpha4.display("VI_-");
  }

  if(digitalRead(allPlugsPin) == LOW && allPlugsState == false) {
    allPlugsState = true;
    alpha4.display("ALL_+");
  }
  if(digitalRead(allPlugsPin) == HIGH && allPlugsState == true) {
    allPlugsState = false;
    alpha4.display("ALL_-");
  }
}
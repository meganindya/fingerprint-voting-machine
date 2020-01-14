#include <Wire.h>

// I2C bus addresses
//------------------
#define AM 0x10 // Acknowledgement Unit
#define VM 0x11 // Voting Machine


// pin numbers
//------------
int pinInp[] = { 8, 6, 4, 2 };
int pinOut[] = { 9, 7, 5, 3 };
int pinBuz = 16;
int pinAct = 13;

// status variables
//-----------------
int pinsNo = -1;  // candidate count
int btnNo  = -1;  // candidate ID

bool devSet = false;  // is device set
bool active = false;  // is device active





// initial setup function
void setup() {
  // bus setup
  Wire.begin(VM);
  Wire.onReceive(receiveEvent);

  // hardware serial setup
  Serial.begin(9600);
}


// communication functions
//------------------------
// Voting Machine sends candidate ID voted for
void sendData(int address, byte data) {
  Wire.beginTransmission(address);
  Wire.write(data);
  Wire.endTransmission();
}


// Voting Machine receives action signals from Acknowledgement Unit
void receiveEvent(int x) {
  byte code, data;

  while (Wire.available()) {
    code = Wire.read();
    data  = Wire.read();
  }


  // reset device on code 0
  if (code == 0) {
    Serial.println("Reset");
    btnNo  = -1;
    active = false;
    digitalWrite(pinAct, LOW);
  }
  // setup device on code 1 if not set already
  if (code == 1 && !devSet) {
    pinsNo = data;
    setupVM();
    Serial.println("Device Set");
  }
  // activate device on code 2 if not activated already
  if (code == 2 && !active) {
    active = true;
    digitalWrite(pinAct, HIGH);
    Serial.println("Activated");
  }
}


// hardware setup functions
//-------------------------
void setupVM() {
  for (int i = 0; i < pinsNo; i++) {
    pinMode(pinInp[i],  INPUT);
    pinMode(pinOut[i], OUTPUT);
  }

  pinMode(pinBuz, OUTPUT);  // buzzer
  pinMode(pinAct, OUTPUT);  // activated LED

  devSet = true;
}
//================================================================================






// main loop function
void loop() {
  if (!devSet) return;

  if (!active) return;

  btnNo = readPins();
  if (btnNo != -1) {
    Serial.print("Voted for ");
    Serial.println(btnNo + 1);
    Serial.println("Deactivated");
    active = false;
    digitalWrite(pinAct, LOW);

    sendData(AM, btnNo);  // send candidate ID to Acknowledgement Unit
    
    digitalWrite(pinOut[btnNo], HIGH);
    digitalWrite(pinBuz, HIGH);
    delay(800);
    digitalWrite(pinOut[btnNo],  LOW);
    digitalWrite(pinBuz,  LOW);
  }
}


// reads input buttons
int readPins() {
  for (int i = 0; i < pinsNo; i++)
    if (digitalRead(pinInp[i]))
      return i;
  
  return -1;
}

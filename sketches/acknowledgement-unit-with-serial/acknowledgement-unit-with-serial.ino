#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>

// I2C bus addresses
//------------------
#define AM 0x10 // Acknowledgement Unit
#define VM 0x11 // Voting Machine

#define M1 0x50 // Fingerprint EEPROM
#define M2 0x51 // Information EEPROM


// hardware library objects
//-------------------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

SoftwareSerial sofSer(8, 9);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&sofSer);


// pin numbers
//-------------
int pinVer =  5; int pinAct = 4; int pinRes =  3; int pinDis = 2; // buttons
int pinVRY = 15; int pinVRN = 6; int pinVCY = 14; int pinVCN = 7; // LEDs

int pinInp[] = { pinVer, pinAct, pinRes, pinDis };
int pinOut[] = { pinVRY, pinVRN, pinVCY, pinVCN };


// status variables
//-----------------
// lcd variables
String prev, curr;

int cNo;  // candidate count
int cID;  // candidate ID
int vID;  // voter ID

bool finish, verify, fFound, active, marked, stall;
bool hwErr = false;





// initial setup function
void setup() {
  // bus setup
  Wire.begin(AM);
  Wire.onReceive(receiveEvent);

  // lcd setup
  lcd.begin();
  lcd.backlight();

  // hardware serial setup
  Serial.begin(9600);

  resetDev(); // reset states

  setupAM();  // hardware setup Acknowledgement Unit
  setupFM();  // hardware setup Fingerprint Machine

  delay(100);
  cNo = readEEPROM(M2, 1023);
  sendData(VM, 1, cNo); // setup Voting Machine for cNo candidates

  if (!hwErr)
    Serial.println("Device Set");
}


// reset function
void resetDev() {
  // pull low on all status pins
  for (int i = 0; i < 4; i++)
    digitalWrite(pinOut[i], LOW);

  // reset status variables
  prev = "Ready";
  curr = "Ready";

  cID = vID = -1;

  finish = true;
  verify = fFound = active = marked = stall = false;

  // write lcd
  lcd.clear();
  lcd.print(curr);
  Serial.println("Reset");
}


// communication functions
//------------------------
void sendData(int address, byte code, byte data) {
  Wire.beginTransmission(address);
  Wire.write(code);
  Wire.write(data);
  Wire.endTransmission();
}

// Voting Machine sends candidate number voted for
void receiveEvent(int howMany) {
  while (Wire.available()) {
    cID = Wire.read();
    Serial.print("Candidate: ");
    Serial.println(cID + 1);
    Serial.println("Finished");
  }

  textSet("Vote Complete");

  marked = true;
  stall  = true;
}
//-----------------------


// hardware setup functions
//-------------------------
void setupAM() {
  for (int i = 0; i < 4; i++) {
    pinMode(pinInp[i],  INPUT);
    pinMode(pinOut[i], OUTPUT);
  }
}

void setupFM() {
  finger.begin(57600);
  delay(5);
  if (!finger.verifyPassword()) {
    Serial.println("Sensor Not Found");
    textSet("Sensor Not Found");
    printLCD();
    hwErr = true;
  }
}
//-------------------------
//================================================================================






// main loop function
void loop() {
  if (hwErr)  return;
  
  while (Wire.available());
  if (marked) writeData();

  while (Wire.available());
  printLCD();

  // reset button pressed
  if (digitalRead(pinRes)) {
    sendData(VM, 0, 0); // send reset signal to Voting Machine
    delay(200);

    resetDev();
  }

  if (stall)  return;   // no action performed if stalling

  // if no transaction running
  if (finish) {
    // verify button pressed
    if (digitalRead(pinVer)) {
      textSet("Verifying");
      printLCD();
      
      verifyID();
    }

    // display button pressed
    if (digitalRead(pinDis)) {
      lcd.clear();
      lcd.print("C1: C2: C3: C4: ");
      lcd.setCursor(0, 1);
      
      for (int i = 0; i < cNo; i++) {
        Serial.print("Candidate "); Serial.print(i + 1); Serial.print(": ");
        byte temp = readEEPROM(M2, 1024 + i);
        Serial.println(temp);

        if      (temp < 10)               lcd.print("00");
        else if (temp > 9 && temp < 100)  lcd.print("0");
        lcd.print(temp); lcd.print(" ");
      }

      stall = true;
    }
  }
  // if transaction running
  else {
    if (verify && !active) {
      // activate button pressed
      if (digitalRead(pinAct)) {
        active = true;
        sendData(VM, 2, 0); // send activation signal to Voting Machine
        Serial.println("Waiting");
        textSet("Waiting");
      }
    }
  }
}


// lcd print function
void printLCD() {
  if (prev == curr) return;

  lcd.clear();
  lcd.print(curr);
  textSet(curr);
  delay(50);
}

// lcd text set function
void textSet(String str) {
  prev = curr;
  curr = str;
}



// fingerprint functions
//----------------------
void verifyID() {
  int cnt= 0;
  while (!fFound && cnt < 25) {
    vID = getFingerprintID() - 1;
    delay(50);
    cnt += 1;
  }
  
  finish = false;

  // if fingerprint not found
  if (!fFound) {
    verify = false;
    stall  = true;
    digitalWrite(pinVRN, HIGH);
    
    Serial.println("Not Verified");
    textSet("Not Verified");

    return;
  }
  // if fingerprint found
  else {
    digitalWrite(pinVRY, HIGH);
    
    // if voter has already voted
    if (readEEPROM(M2, vID) == 1) {
      verify = false;
      stall  = true;
      digitalWrite(pinVCN, HIGH);

      Serial.println("Already Voted");
      textSet("Already Voted");
    }
    // if voter hasn't already voted
    else {
      verify = true;
      digitalWrite(pinVCY, HIGH);

      Serial.println("Verified");
      textSet("Verified");
    }
  }
}

// fingerprint hardware function
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;
  
  // match found
  fFound = true;
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  
  return finger.fingerID;
}
//----------------------


// EEPROM activity functions
//--------------------------
// write to EEPROM function
void writeEEPROM(int deviceaddress, unsigned int eeaddress, byte data) {
  while (Wire.available());
  
  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();

  delay(5);
}

// read from EEPROM function
byte readEEPROM(int deviceaddress, unsigned int eeaddress) {
  byte rdata = 0xFF;

  Wire.beginTransmission(deviceaddress);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();

  Wire.requestFrom(deviceaddress, 1);

  if (Wire.available()) rdata = Wire.read();
  
  return rdata;
}

void writeData() {
  // increment candidate vote
  unsigned int address = 1024 + cID;     // address starts at 1024
  byte temp = readEEPROM(M2, address);
  writeEEPROM(M2, address, temp + 1);
  Serial.print("Prev: "); Serial.print(temp);
  Serial.print(" Curr: "); Serial.println((byte) (temp + 1));

  // mark voter as voted
  writeEEPROM(M2, vID, 1);
  Serial.print("Voter "); Serial.print(vID + 1); Serial.println(" marked");

  marked = false;
}
//--------------------------

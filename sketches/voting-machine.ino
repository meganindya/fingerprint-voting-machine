bool pinStatus[4] = { false };

int inpPins[] = { 7, 6,  5,  4 };
int outPins[] = { 8, 9, 10, 11 };

bool active		= false;
bool selected	= false;
int selectedPin = 9;
int activePins	= 0;



void setup() {
  Serial.begin(9600);
  
  pinMode(14, OUTPUT);
  
  while (!Serial.available());
  activePins = Serial.read() - (int) '0';
  
  for (int i = 0; i < activePins; i++)
    pinStatus[i] = true;
  //Serial.print(activePins);
  //Serial.println(" set");
  
  for (int i = 0; i < activePins; i++) {
    pinMode(inpPins[i], INPUT);
    pinMode(outPins[i], OUTPUT);
  }
}


void loop() {
  // if not active
  if (!active) {
    digitalWrite(14, LOW);
    
    while (!Serial.available());
    if (Serial.read() == '0') {
      active = true;
      //Serial.println("Activated");
    }
  }
  
  // if active
  else {
    digitalWrite(14, HIGH);
    
    for (int i = 0; i < activePins; i++) {
      if (digitalRead(inpPins[i])) {
        selected	= true;
        selectedPin	= i;
        break;
      }
    }
    
    if (selected) {
      //Serial.print("Candidate ");
      //Serial.println(selectedPin + 1);
      Serial.write(selectedPin + '0');
      digitalWrite(outPins[selectedPin], HIGH);
      active = false;
      //Serial.println("Deactivated");
      
      delay(100);
      
      digitalWrite(outPins[selectedPin], LOW);
      selected		= false;
      selectedPin	= -1;
    }
  }
}

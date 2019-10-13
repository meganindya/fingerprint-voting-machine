void activateVU();
void deactivateVU();
int  listenVU();
void activateVM();
void resetStatus();



bool activeVU	= false;
int  voterID	= 99;
bool voterPass	= false;
bool voted[16]	= { false };
bool answered	= false;
int  votes[2]	= { 0 };



void setup() {
  Serial.begin(9600);
  
  pinMode( 2, INPUT);
  pinMode( 3, INPUT);
  pinMode( 4, INPUT);
  pinMode( 5, INPUT);
  pinMode( 6, INPUT);
  pinMode( 7, INPUT);
  pinMode( 8, INPUT);
  pinMode(14, INPUT);
  
  pinMode( 9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
}



void loop() {
  if (digitalRead(14))
  	activateVU();
  
  if (activeVU)
  	voterID = listenVU();
  
  if	  (voterID == -1)
    digitalWrite(13, HIGH);
  else if (voterID == 99);
  else {
    voterPass = true;
    if (voted[voterID]) {
      voterPass = false;
      digitalWrite(11, HIGH);
    }
    else
      digitalWrite(10, HIGH);
  }
  
  
  if (voterPass) {
    if (digitalRead(8))
      activateVM();
    
    if (!answered) {
      if (Serial.available()) {
        int v = Serial.read() - (int) '0';
        //Serial.println(v);
        votes[v]++;
        answered = true;
        
        resetStatus();
        Serial.println("");
        Serial.print(votes[0]);
        Serial.print(" ");
        Serial.println(votes[1]);
  	  }
    }
  }
}

void activateVU() {
  resetStatus();
  
  activeVU = true;
  digitalWrite( 9, HIGH);
}

void deactivateVU() {
  activeVU = false;
  voterID = 99;
  digitalWrite( 9, LOW);
}

int listenVU() {
  if (digitalRead(2)) {
    deactivateVU();
    
    return -1;
  }
  
  if (digitalRead(3)) {
    deactivateVU();
    
    int v = 0;
    v += 1 * digitalRead(4);
    v += 2 * digitalRead(5);
    v += 4 * digitalRead(6);
    v += 8 * digitalRead(7);
    return v;
  }
  
  return 99;
}

void activateVM() {
  answered = false;
  
  Serial.write('2');
  delay(100);
  Serial.write('0');
}

void resetStatus() {
  Serial.println("Reset");
  voterPass = false;
  
  digitalWrite(10, LOW);
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
}
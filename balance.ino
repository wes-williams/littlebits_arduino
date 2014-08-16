/*
* Visualize the balance of signal strength received by the cloudbit
* to communicate the balance or imbalance of things. i.e. work vs life
*  - pie chart (with motor)
*  - farris wheel (with motor)
*  - weighted scale (with servo)
*  - seesaw (with servo)
* 
* The 2 things are distinguished by the length of their signal.
*  thing 1 <= 3 seconds (Use 1 seconds)
*  thing 2 > 3 seconds ( Use 5 seconds)
*
* Circuit is intended to be:
*
*             -> button   -> wire ->         -> led
*  p3 -> fork -> cloudbit -> wire -> arduino -> bargraph 
*             -> button   -> wire ->         -> wire     -> motor
*
*/
const boolean DEBUG=true;

// pins used (all of them)
const int calibrateIn = 0;    // button for calibration
const int cloudIn = A0;       // cloudbit for receiving signals
const int overrideIn = A1;    // button for manually adjusting motor
const int calibrateOut = 1;   // led for indicating calibration in progress
const int bargraphOut = 5;    // bargraph for visualizing the received signal
const int motorOut = 9;       // motor to move the phyiscal world

// current readings from cloudbit
int val = 0;      // the current reading
int lastVal = 0;  // the previous reading
int highVal = 0;  // the highest value while reading greater than 0
int valTime = 0;  // the length of time reading greater than 0

// balance the readings
float balance1 = 0; // short readings (use 1 sec)
float balance2 = 0; // long readings (use 5 sec)
float rotation=90; // start balanced (3 o'clock)
const float degreesInCircle = 360.0; 

// calibration settings
int cloudOnVal = 0;

void setup() {
  // log activity?
  if(DEBUG) {
    Serial.begin(9600);  
  }
  
  // input
  pinMode( calibrateIn, INPUT );
  pinMode( cloudIn, INPUT );
  pinMode( overrideIn, INPUT );
  
  // output
  pinMode( calibrateOut, OUTPUT );
  pinMode( bargraphOut, OUTPUT );
  pinMode( motorOut, OUTPUT );
  
  // takes a little while to start the cloudbit
  delay(3000); // may not be long enough
  // detect the cloubits baseline reading
  calibrate();
}

void loop() {
  // what is the cloud sending?
  val = analogRead(cloudIn);
  // make the highest value during calibration be 0
  val = map(val, cloudOnVal, 1023, 0, 255);
  // keep between 0 and 255 
  val = constrain(val, 0, 255); 
  
  // has the value changed since last time?
  if(val != lastVal) {
    change();
    lastVal = val; // track the last value
  }

  // track length of signal
  if(val != 0) {
    valTime += 1; // track the time in seconds
    delay(1000); // pause for one second 
  }
  else { // allow for manual adjustments 
  
     // need to rebalance?
     // correct the position of the motor
     if(digitalRead(overrideIn) == HIGH) {
       override();
     }
     
     // cloudbit have a lot of noise?
     // recalibrate it
     if(digitalRead(calibrateIn) == HIGH) {
       calibrate();
     }
  }
}

// cloudbit doesn't output zero at rest, so discover what zero is
void calibrate() {
  digitalWrite(calibrateOut, HIGH);
  cloudOnVal=0;
  unsigned long stopTime = millis() + 5000;
  while (millis() < stopTime) {  
    val = analogRead(cloudIn);
    if(val>cloudOnVal) {
      cloudOnVal = val; 
    }
  } 
  digitalWrite(calibrateOut, LOW);
}

// make the necessary changes in output
void change() {
  if(DEBUG) {
    Serial.print("CHANGE=");
    Serial.println(val);
  }
  // is it still on?
  if(val>0) {  
      // display the highest value during each read period
    if(val > highVal) {
      analogWrite( bargraphOut, val);
      highVal = val; // track the highest value
    }
    return;
  }    
  
  // who sent the signal?
  // use time to identify the sender
  if(valTime<=3) { // 3 secs or less = thing 1
    balance1 += highVal;
  }
  else { // more than 3 secs = thing 2
    balance2 += highVal;
  }
  
  adjust();
}

void adjust() { 
  // what is the balance of these things?
  int balance = balance1-balance2;
  
  if(DEBUG) {
    Serial.print("Received ");
    Serial.print(highVal);
    Serial.print(" for ");
    Serial.print(valTime);
    Serial.println(" secs");
    
    Serial.print("Balance=");
    Serial.println(balance);
  }
  
  // 90 degrees is even/flat/balanced
  // find 90 degrees from current rotation position  
  // NOTE: the motor is one-directional (clockwise)   
  int rotateBy = 0;
  if(rotation>90) { // might be close to a full rotation
    rotateBy = (degreesInCircle-rotation)+90;
  }
  else { // shouldn't need to move much
    rotateBy = 90-rotation;
  }
  
  // find here we should be
  float rotateTo=0;
  // are the things within 5% of each other?
  if(abs(balance1-balance2) < (abs(balance1+balance2)*.05)) {
    rotateTo=90; // treat them as balanced
  }
  else { // treat this like a weighted scale
     // which is heavier?
     if(balance<0) { // thing2
      rotateTo=135;
     }
     else { // thing1 
       rotateTo=45;
     }
  }
  
  // adjust from where we are to where we should be
  if(rotateTo<90) {
    rotateBy += degreesInCircle;
    rotateBy -= rotateTo;  
  }
  else {
    rotateBy += (rotateTo-90);
  }
  
  // don't make more than one rotation
  rotateBy = abs(rotateBy);
  if(rotateBy>=degreesInCircle) {
    rotateBy = rotateBy - degreesInCircle;
  }
   
  // a full rotation takes ~2 seconds
  int motorTime = 2200*(rotateBy/degreesInCircle);
  
  // exit if no rotation required
  if(motorTime==0) {
    if(DEBUG) {
      Serial.println("No rotation required");
    }
  }
  else { 
    if(DEBUG) {
      Serial.print("Rotate from ");
      Serial.print(rotation);
      Serial.print(" to ");
      Serial.print(rotateTo);
      Serial.print(" by ");
      Serial.print(rotateBy);
      Serial.print(" in ");
      Serial.print(motorTime);
      Serial.println(" secs");
    }      
   
    // rotate the motor
    digitalWrite(motorOut,HIGH);
    delay(motorTime);
    digitalWrite(motorOut,LOW); 
  } 
  
  // reset current reading 
  analogWrite( bargraphOut, 0); 
  rotation = rotateTo;
  valTime=0;
  highVal=0; 
}

// power motor and reset values
void override() {
   digitalWrite(motorOut,HIGH);
   delay(100);
   digitalWrite(motorOut,LOW);
   // rebalance
   rotation = 90;
   valTime=0;
   highVal=0;
   balance1=0;
   balance2=0;
}


// VERSION 0
// KDS 2022-10-05 simple tuner
// plan is to
// read an oscilator, estimate the frequency, 
// send 1 volt to it
// reestimate freq and display tuning performance on some leds
// iterate against 0/1v outputs


//// warning these pinouts are different on PCB vs the stripboard
//int LAYOUT_VERSION = 0; // 0 is the protoboard veroboard device
int LAYOUT_VERSION = 1; // 1 is the v101 PCB
// pin assignment variables:
// willbe set in setup() at runtime 
int pinL1 ; // lowest LED
int pinL2 ; // led for low tune
int pinL3 ; // led for good tune
int pinL4 ; // led for high tune
int pinL5 ; // led for highest tuning
int pinLL ; // led for too log frequency
int pinLE ; // led for read errors
int CVOUT ; // output CV digital high/low
int OSC_IN ; // reading signal



bool DEBUG=0;

// nine states is eight thresholds
float Threshold0 = 1.25f; // plus the error condition for inversion upon misreads
float Threshold1 = 2.0f - 0.25f;
float Threshold2 = 2.0f - 0.10f;
float Threshold3 = 2.0f - 0.05f;
float Threshold4 = 2.0f - 0.0125f;
float Threshold5 = 2.0f + 0.0125f;
float Threshold6 = 2.0f + 0.05f;
float Threshold7 = 2.0f + 0.10f;
float Threshold8 = 2.0f + 0.25f;


// these globals tally zero crossings and hold them
int ACCUMULATOR = 0;
int LOWREAD = 100;
int HIGHREAD = 200;

// these are adjusted over time as we watch the signal in
int minLevel = 1000;
int maxLevel = 0;
int lThresh = 250;
int hThresh = 750;
bool isLow = true; // flipflop memory


bool lowmode = true; // output cv tone 0.5V
long lastTime = -99999; // start in the past so we get immediate response
int timeGap = 5000;


int Regress(int input){
  float newValue;
  newValue = (float)(input) * 0.98 + 10;//(float)(500) * 0.02;
  return(newValue);
}

void SignalWatcher(){
  int newValue = analogRead(OSC_IN);
  // of a 1024 pin??
  
  // revamp range
  maxLevel = Regress(maxLevel);
  minLevel = Regress(minLevel);
  if(newValue > maxLevel){
    maxLevel = newValue + 1;
  }
  if(newValue < minLevel){
    minLevel = newValue - 1;
  }
  // adjust the hysteresis line to the new range.
  lThresh = 0.40 * (maxLevel - minLevel) + minLevel;
  hThresh = 0.60 * (maxLevel - minLevel) + minLevel;
  // count a flipflop
  if(isLow && newValue > hThresh){
    isLow = false;
    ACCUMULATOR ++;
  }else if(!isLow && newValue < lThresh){
    isLow = true;
    ACCUMULATOR ++; // count both crossings
  }
}

void allOff(){
    digitalWrite(pinL1,LOW);
    digitalWrite(pinL2,LOW);
    digitalWrite(pinL3,LOW);
    digitalWrite(pinL4,LOW);
    digitalWrite(pinL5,LOW);
    digitalWrite(pinLE,LOW);
    digitalWrite(pinLL,LOW);
}

void Display(){
  float octave = ((float)HIGHREAD) / ((float)LOWREAD);
  allOff();
  if(octave < Threshold0){ // mark for Errors
    digitalWrite(pinLE,HIGH); 
  }else if(octave < Threshold1){
    digitalWrite(pinL1,HIGH);
  }else if(octave < Threshold2){
    digitalWrite(pinL1,HIGH);
    digitalWrite(pinL2,HIGH);
  }else if(octave < Threshold3){
    digitalWrite(pinL2,HIGH);
  }else if(octave < Threshold4){
    digitalWrite(pinL2,HIGH);
    digitalWrite(pinL3,HIGH);
  }else if(octave < Threshold5){
    digitalWrite(pinL3,HIGH);
  }else if(octave < Threshold6){
    digitalWrite(pinL3,HIGH);
    digitalWrite(pinL4,HIGH);
  }else if(octave < Threshold7){
    digitalWrite(pinL4,HIGH);
  }else if(octave < Threshold8){
    digitalWrite(pinL4,HIGH);
    digitalWrite(pinL5,HIGH);
  }else{
    digitalWrite(pinL5,HIGH);
  }
  if(LOWREAD < 111){ // mark for inaccuracy
    digitalWrite(pinLL,HIGH);
  }
}


void GoHighMode(){
  // just finished a low mode, estimate tuning
  //analogWrite(CVOUT, 153); //   3/5 OF 255 
  digitalWrite(CVOUT, 1);
  //LOWREAD = (LOWREAD + ACCUMULATOR)/2;
  LOWREAD = ACCUMULATOR;
  ACCUMULATOR = 0;
  
  lowmode=false;
}


void GoLowMode(){
  // just finished a high mode, estimate tuning
  //analogWrite(CVOUT, 51); //   1/5 OF 255 
  digitalWrite(CVOUT, 0);
  //HIGHREAD = (HIGHREAD + ACCUMULATOR)/2; 
  HIGHREAD = ACCUMULATOR; 
  ACCUMULATOR = 0;
  
  lowmode=true;
}

#define LEDTESTTIME 125
void ledTest(){

  for(int i=0;i<4;i++){
    digitalWrite(pinL1,1);
    digitalWrite(pinL2,1);
    digitalWrite(pinL3,1);
    digitalWrite(pinL4,1);
    digitalWrite(pinL5,1);
    delay(LEDTESTTIME);
    digitalWrite(pinL1,0);
    digitalWrite(pinL2,0);
    digitalWrite(pinL3,0);
    digitalWrite(pinL4,0);
    digitalWrite(pinL5,0);
    delay(LEDTESTTIME);
  }
  
}

// put your setup code here, to run once:
void setup() {

  if(LAYOUT_VERSION==0){
    pinL1 = 11;
    pinL2 = 9;
    pinL3 = 7;
    pinL4 = 5;
    pinL5 = 3;
    pinLL = 2;
    pinLE = 4;
    CVOUT = 6;
    OSC_IN = A3;
  }
  if(LAYOUT_VERSION==1){
    pinL1 = 8;
    pinL2 = 7;
    pinL3 = 6;
    pinL4 = 5;
    pinL5 = 4;
    pinLL = 10;
    pinLE = 11;
    CVOUT = 2;
    OSC_IN = A3;
  }
  

   // outputs
  pinMode(pinL1, OUTPUT);
  pinMode(pinL2, OUTPUT);
  pinMode(pinL3, OUTPUT);
  pinMode(pinL4, OUTPUT);
  pinMode(pinL5, OUTPUT);

  // inputs 
  pinMode(OSC_IN, INPUT);

   if(DEBUG){
    Serial.begin(9600);
   }
 
   //initialize the PWMs
   pinMode(CVOUT, OUTPUT);
   //noTone(CVOUT); 
   //analogWrite(CVOUT, 51);
   digitalWrite(CVOUT, 0);
   
  // leave a pin held high so user can probe for the software's running
   pinMode(12, OUTPUT);
   digitalWrite(12,HIGH);
 
  ledTest();

}// done setup()


int iteration = 0;
void loop() {
  
  long newTime = millis();
  long elapsed = newTime - lastTime;
  if(elapsed > timeGap){
    lastTime = newTime;
    // flip modes
    if(lowmode){
      GoHighMode();
    }else{
      GoLowMode();
      if(iteration++ > 4)
        timeGap = 500;
    }
    Display();
  }
  if(elapsed > 2) { // skip the slew
    // look for zero-crossings
    SignalWatcher();
  }
}

//
//
// .c file for Arduino 1.8.10
// EuclidSeq V1.2
// aims to be a dual channel pattern sequencer. 
// it's got positive and negative outs, so 4 trigger channels
// KDS 2020-04-18 Version 1
// 2020-04-19 Version 1.1 is functional !.
//
// update V1.2 now checks if the knobs have moved
// so we dont have to recalculate the patterns so often.
// probably no user impact, but we might support faster cycling.

// update V1.3 going to try to add the second mode
// save states require a new library
#include <EEPROM.h>




// firstly, a pinout description
// digital writes:
#define GATE_OUT1A  2
#define GATE_OUT1B  3
#define GATE_OUT2A  4
#define GATE_OUT2B  5

// digital reads:
#define TRIGGER_IN  6
// analog reads:
#define LENGTH_IN1 A2
#define LENGTH_IN2 A3
#define FILL_IN1 A7
#define FILL_IN2 A6
#define ROTATE_IN1 A4
#define ROTATE_IN2 A5

/////////////////////////

// how long is a long pattern?
#define MAX_PATTERN 32
// and how much do we scale the knob?
float downconversion = 1024.0f  /  ((float)(MAX_PATTERN));

// enable or disable serial messaging:
#define DEBUG 0
// those debug serial prints cause some hangups
// because we have <600 bytes free ram.

// how many loops should we keep a trigger output online?
#define OUTPUT_DURATION_CYCLES 5
// might be nice to put a trimpot here someday.

int MODULE_MODE = 1; // default to Euclid mode.

bool gateState[4] = {0,0,0,0}; // remember what we set
int gatePin[4] = {GATE_OUT1A, GATE_OUT1B, GATE_OUT2A, GATE_OUT2B};

// these defaults are never used since setup() recalculates things.
int Length1 = 8;
int Length2 = 8;

float Fill1 = 0.50f;
float Fill2 = 0.50f;
float Rotate1 = 0.0f;
float Rotate2 = 0.0f;
// also keeping the plain int versions
int Fill1i = 512;
int Fill2i = 512;
int Rotate1i = 1;
int Rotate2i = 1;


// save the states we're gonna use:
bool PATTERN1[MAX_PATTERN];
bool PATTERN2[MAX_PATTERN];
bool PATTERN3[MAX_PATTERN];
bool PATTERN4[MAX_PATTERN];

// arbitrary constants
#define SAVE_BYTE 111
#define MODE_1_BYTE 8
#define MODE_2_BYTE 13

void readSaveState(){
  // pull back MODULE_MODE
  byte x;
  x = EEPROM.read(SAVE_BYTE); 
  if(x == MODE_1_BYTE){
    MODULE_MODE = 1;
  }
  if(x == MODE_2_BYTE){
    MODULE_MODE = 2;
  }
}

void blink_all()
{
  // deactivate all outputs for now.
  for(int i=0;i<4;i++){
    gateState[i]=0; digitalWrite(gatePin[i],LOW);
  }
  delay(10);
  for(int i=0;i<4;i++){
    digitalWrite(gatePin[i],HIGH);
  }
  delay(400);
  for(int i=0;i<4;i++){
    digitalWrite(gatePin[i],LOW);
  }
}

// this is a big one, switch from Euclid to Grid modes. 
void updateMode()
{
  if(Rotate1 > 0.9 && Rotate2 > 0.9){
    // just some animation to start.
    blink_all();
    for(int j=0;j<4;j++)
    for(int i=0;i<4;i++){
      delay(5);
      digitalWrite(gatePin[i],HIGH);
      delay(100);
      digitalWrite(gatePin[i],LOW);
    }
    // finally set mode and save it to persistent storage
    MODULE_MODE = 2; // Grids
    EEPROM.update(SAVE_BYTE, MODE_2_BYTE);
  }
  if(Rotate1 < 0.2 && Rotate2 < 0.2){
    blink_all();
    for(int j=0;j<4;j++)
    for(int i=3;i>=0;i--){ // zip other way.
      delay(5);
      digitalWrite(gatePin[i],HIGH);
      delay(100);
      digitalWrite(gatePin[i],LOW);
    }
    MODULE_MODE = 1; // Euclid Seq
    EEPROM.update(SAVE_BYTE, MODE_1_BYTE);
  }
}


void setup()                    
{
   pinMode(GATE_OUT1A, OUTPUT); // digitalWrite here plz
   pinMode(GATE_OUT1B, OUTPUT);
   pinMode(GATE_OUT2A, OUTPUT);
   pinMode(GATE_OUT2B, OUTPUT);
   
   digitalWrite(GATE_OUT1A,LOW); // be sure theyre down at boot.
   digitalWrite(GATE_OUT2A,LOW);
   digitalWrite(GATE_OUT1B,LOW);
   digitalWrite(GATE_OUT2B,LOW);
   
   pinMode(TRIGGER_IN, INPUT); // this guy has a transistor inverter

   pinMode(LENGTH_IN1, INPUT); // 0-5V from a knob
   pinMode(LENGTH_IN2, INPUT); 
   pinMode(FILL_IN1, INPUT); // 0-5V from a knob and a CV Jack
   pinMode(FILL_IN2, INPUT);
   pinMode(ROTATE_IN1, INPUT); // 0-5V from a knob
   pinMode(ROTATE_IN2, INPUT);

   if(DEBUG){
    Serial.begin(9600); delay(5);//milliseconds
    Serial.print("Hello! EuclidSeq V1.3\n");
   }

   for(int i=0;i<MAX_PATTERN;i++){
    PATTERN1[i] = 0; // all flat, no notes
    PATTERN2[i] = 0; // will update in a second tho
    PATTERN3[i] = 0; // will update in a second tho
    PATTERN4[i] = 0; // will update in a second tho
   }
   readSaveState(); // update MODULE_MODE;
   updateKnobs(); // initializer
}


void updatePatterns(){
  if(MODULE_MODE==1){
    updatePatterns_Euclid();
  }
  if(MODULE_MODE==2){
    updatePatterns_Grids();
  }
}

void updatePatterns_Euclid()
{ 
  // flatten everyone to start.
  int n = max(Length1, Length2);
  for(int i=0;i<n;i++){
    PATTERN1[i] = 0;
    PATTERN2[i] = 0;
  }
  // Euclidean Sequences follow Bresenham's Line algorithm
  // but Bresenham's is faster whereas I use floats 
  float stepSize; int nHits; int startStep; int stepHit; float ctr;
  
  // fill the bool array. takes some math.
  nHits = (int)( Fill1 * (float)Length1 );
  startStep = (int)( Rotate1 * (float)Length1 );
  stepSize = (float)nHits / (float)(Length1);
  if(DEBUG){ 
    Serial.print("nHits="); Serial.print(nHits);
    Serial.print("  startStep="); Serial.print(startStep);
    Serial.print("  stepSize=");Serial.print(stepSize);
    Serial.print("\n");
  }
  ctr = 0.50f;
  for(int i=0;i < Length1;i++){
    ctr += stepSize;
    if(ctr > 1.0f){
      ctr -= 1.0f;
      stepHit = i+startStep;
      if(stepHit >= Length1) stepHit -= Length1;
      PATTERN1[stepHit] = 1;
    }
  }

  // repeat for channel 2
  // fill the bool array. takes some math.
  nHits = (int)( Fill2 * (float)Length2 );
  startStep = (int)( Rotate2 * (float)Length2 );
  stepSize = (float)nHits / (float)(Length2);
  ctr = 0.50f;
  for(int i=0;i < Length2;i++){
    ctr += stepSize;
    if(ctr > 1.0f){
      ctr -= 1.0f;
      stepHit = i+startStep;
      if(stepHit >= Length2) stepHit -= Length2;
      PATTERN2[stepHit] = 1;
    }
  }
  
}



byte PDB1[288] = {150,236,183,13,29,22,109,220,190,196,26,164,250,138,156,129,172,5,149,175,49,245,20,160,11,19,237,126,241,49,120,101,163,3,209,5,200,118,156,0,16,189,129,22,84,11,53,209,73,88,92,58,31,155,193,177,56,138,18,91,243,172,93,51,45,180,127,18,204,142,233,59,108,138,128,117,198,228,11,27,215,8,131,9,173,116,174,65,240,77,179,200,24,118,105,251,134,69,214,151,219,57,70,185,169,24,147,12,120,204,41,114,189,176,21,163,206,9,179,6,66,52,22,233,80,38,146,39,197,161,5,183,6,233,16,207,169,174,109,175,231,103,133,53,191,242,123,242,162,188,33,239,74,176,4,80,77,155,117,163,15,193,73,190,173,215,49,164,39,122,69,181,118,238,194,174,15,218,204,124,25,19,91,198,109,165,247,35,64,14,125,135,23,189,220,229,29,138,243,247,64,71,81,35,188,251,23,100,157,245,71,44,247,28,250,131,32,171,98,100,252,129,172,148,92,227,182,37,77,166,234,117,48,136,80,169,80,236,219,107,146,242,168,81,130,129,20,185,82,85,91,62,72,99,28,133,63,238,93,29,146,38,113,5,96,58,102,212,222,102,226,50,191,187,149,110,50,245,133,9,211,179,7,170,216,209,108,246};
byte PDB2[288] = {28,136,73,192,198,37,180,74,10,230,15,226,102,157,168,85,26,80,152,198,46,12,96,173,245,108,222,150,112,58,190,77,25,50,208,129,141,229,6,237,129,120,23,119,172,20,213,179,130,79,150,237,21,221,7,166,127,31,84,131,50,7,240,61,234,211,186,8,48,58,26,62,203,213,100,242,88,179,202,164,204,136,111,39,46,181,113,14,113,158,141,156,104,123,192,243,243,201,237,147,196,184,221,156,117,200,160,179,213,187,243,159,21,124,184,77,241,7,10,64,102,249,36,212,210,200,163,208,103,238,94,150,197,200,245,226,179,131,59,157,145,189,66,15,232,80,213,8,15,73,41,10,130,196,71,29,65,184,59,35,225,16,106,11,135,247,76,155,180,179,184,19,63,185,92,215,90,119,135,213,125,148,40,58,132,227,141,23,21,182,58,45,110,229,203,70,187,94,25,211,185,91,161,100,205,118,49,57,160,28,180,235,150,206,176,137,213,175,56,159,107,0,71,36,61,100,100,148,37,34,91,232,147,185,180,219,97,107,89,179,66,80,170,142,84,118,183,150,192,34,209,92,74,45,27,135,214,67,66,179,155,192,218,96,23,152,158,0,253,28,55,242,77,239,190,228,212,238,87,254,238,100,117,51,131,252,186,254};
byte PDB3[288] = {83,8,106,105,125,131,235,119,31,94,182,33,249,184,88,44,84,47,79,151,142,97,49,17,5,224,211,141,116,250,241,234,25,107,133,19,70,41,137,227,197,141,51,136,121,166,174,125,228,1,160,101,132,67,66,72,91,35,193,222,39,108,178,78,190,160,210,31,81,48,224,195,111,88,243,174,212,58,171,119,22,205,27,104,241,158,229,81,26,109,4,24,133,167,34,198,67,218,138,80,31,178,188,65,70,33,229,191,4,254,254,56,182,130,8,166,84,229,62,249,6,156,139,114,168,145,66,154,31,130,169,67,105,126,14,39,247,138,151,30,111,8,204,75,115,240,98,187,114,59,48,233,29,212,32,0,172,185,212,192,61,151,99,98,243,249,213,5,51,178,166,101,180,68,46,104,47,102,64,222,98,120,145,161,156,188,138,33,82,145,219,199,235,233,116,99,175,11,118,40,155,37,182,22,3,36,117,199,242,184,15,220,239,54,229,129,209,150,194,35,50,106,253,77,162,25,83,30,225,216,212,249,178,165,75,73,87,58,15,111,176,7,237,196,85,68,144,85,71,45,187,6,15,106,254,27,51,95,93,239,0,100,78,17,116,178,220,139,2,168,60,189,144,42,73,154,233,61,44,190,71,81,165,234,106,205,195,202};
byte PDB4[288] = {225,113,196,141,69,102,159,216,198,160,3,35,136,181,209,155,170,144,119,183,123,200,29,42,208,181,13,245,160,12,228,178,252,98,28,143,11,196,8,46,107,94,112,83,175,97,112,142,58,78,78,67,156,136,21,64,201,239,157,111,231,78,146,44,39,186,87,189,63,97,200,69,110,83,153,247,60,22,145,8,108,92,17,132,102,15,15,253,234,158,163,70,240,26,183,38,110,240,16,163,96,40,87,143,139,132,157,251,8,172,202,212,31,34,182,113,168,181,75,50,27,151,106,39,70,161,111,39,56,38,173,12,136,236,19,173,8,174,220,110,129,128,241,197,227,207,23,90,109,68,126,171,91,213,108,69,144,212,65,182,124,65,169,47,150,192,237,16,105,81,131,153,162,62,201,180,41,186,125,249,208,183,191,133,159,206,98,124,239,39,66,208,47,201,30,206,179,135,214,244,192,166,46,134,94,213,180,118,62,62,151,125,178,175,92,241,242,58,185,171,123,125,170,56,91,67,52,150,1,154,165,185,145,53,101,231,144,200,132,170,41,19,72,40,91,126,110,237,236,119,219,194,137,191,148,158,101,181,139,104,108,188,120,153,227,16,107,146,41,85,174,72,199,73,185,222,62,130,163,254,10,103,147,186,158,245,44,188};


void updatePatterns_Grids()
{ 
  // based on pattern_database
  // 
  // will keep Length and Fill knobs, but RotateNi is now startPos.
  byte bFill1 = (byte)( Fill1i / 4); // 0-255
  byte bFill2 = (byte)( Fill2i / 4); // 0-255
  int start1 = Rotate1i / 2; // 0-255
  int start2 = Rotate2i / 2; // 0-255
  
  for(int i=0;i < Length1;i++){
   // do something with PATTERN1 and PATTERN2
   PATTERN1[i] = (bFill1 > PDB1[i+start1]);
   PATTERN2[i] = (bFill1 > PDB2[i+start1]);
  }
  for(int i=0;i < Length2;i++){
   // do something with PATTERN3 and PATTERN4
   PATTERN3[i] = (bFill2 > PDB3[i+start2]);
   PATTERN4[i] = (bFill2 > PDB4[i+start2]);
  }
  if(DEBUG){ 
    Serial.print("bFill1="); Serial.print(bFill1);
    Serial.print("  start1="); Serial.print(start1);
    Serial.print("\n");
  }
}




void updateKnobs()
{ 
  // read all six analogs and interpret results
  int x; float y;
  bool UPDATED = 0;// default to not seeing any change.

  x = Length1; // save prev value so we can see if it's changed.
  y=(float)analogRead(LENGTH_IN1)/1024.0f; // 0-100%.
  Length1 = (int)(  y*(float)MAX_PATTERN );
  Length1 = (Length1 < 2)?2:Length1; // and round off the corners
  Length1 = (Length1 > (MAX_PATTERN-2))?MAX_PATTERN:Length1;
  if(Length1 != x) UPDATED=1;
  
  x=Length2;
  y=(float)analogRead(LENGTH_IN2)/1024.0f;
  Length2 = (int)(  y*(float)MAX_PATTERN );
  Length2 = (Length2 < 2)?2:Length2;
  Length2 = (Length2 > (MAX_PATTERN-2))?MAX_PATTERN:Length2;
  if(Length2 != x) UPDATED=1;

  x = Rotate1i;
  Rotate1i = analogRead(ROTATE_IN1);
  if( Rotate1i != x){
    UPDATED=1;
    Rotate1 = (float)Rotate1i / 1024.0f; // 0-100%
  }
  
  x = Rotate2i;
  Rotate2i = analogRead(ROTATE_IN2);
  if(Rotate2i != x){
    Rotate2 = (float)Rotate2i / 1024.0f;
    UPDATED=1;
  }
 
  // Fill should be 100% at halfway due to the physical scaling
  // allows for +12V at CV jack, while letting the +5V be top.
  x = Fill1i;
  Fill1i = analogRead(FILL_IN1);
  if(Fill1i != x){
    Fill1 = (float)Fill1i / 700.0f;
    Fill1 = (Fill1 > 1.0f)?1.0f:Fill1; // capped.
    UPDATED=1;
  }
 
  x = Fill2i;
  Fill2i = analogRead(FILL_IN2);
  if(Fill2i != x){
    Fill2 = (float)Fill2i / 700.0f;
    Fill2 = (Fill2 > 1.0f)?1.0f:Fill2;
    UPDATED=1;  
  }
  
  if(UPDATED) updatePatterns();
  
}

// gonna use a countdown to keep the trigger high for a few milliseconds.
int triggertime[4] = {0,0,0,0};
 
void updateOuts()
{
   for(int i=0;i<4;i++){
    if(triggertime[i] > 0){
      triggertime[i]--; // decrement.
      if(gateState[i]){
        // up and should be up. no issues.
      }else{
        // down and should be up:
        digitalWrite(gatePin[i], HIGH);
        gateState[i] = 1;
      }
    }else{ // trigger time is not >0
      if(gateState[i]){
        // up and should be down. set down
        digitalWrite(gatePin[i], LOW);
        gateState[i] = 0;
      }//else{
        // down and should be down.
      //}
    }
   }
   
}


// what step are we on right now?
int Step1= Length1;
int Step2= Length2;

void stepClockTrigger_Euclid(){
  if(PATTERN1[Step1]){ // got a hit!
    triggertime[0] = OUTPUT_DURATION_CYCLES;
  }else{ // no hit, is a hit for the 'not' output
    triggertime[1] = OUTPUT_DURATION_CYCLES;
  }
  if(PATTERN2[Step2]){ // got a hit!
    triggertime[2] = OUTPUT_DURATION_CYCLES;
  }else{  // no hit, is a hit for the 'not' output
    triggertime[3] = OUTPUT_DURATION_CYCLES;
  }
}

void stepClockTrigger_Grids(){
  // four 'independent' channels:
  if(PATTERN1[Step1]){ // got a hit!
    triggertime[0] = OUTPUT_DURATION_CYCLES;
  }
  if(PATTERN2[Step1]){ // got a hit!
    triggertime[1] = OUTPUT_DURATION_CYCLES;
  }
  if(PATTERN3[Step2]){ // got a hit!
    triggertime[2] = OUTPUT_DURATION_CYCLES;
  }
  if(PATTERN4[Step2]){ // got a hit!
    triggertime[3] = OUTPUT_DURATION_CYCLES;
  }
}

void stepClockTrigger()
{ // got a clock signal, let's perform
  Step1++; Step2++;
  if(Step1 >= Length1) Step1=0;
  if(Step2 >= Length2) Step2=0;
  if(MODULE_MODE == 1){
    stepClockTrigger_Euclid();
  }else if(MODULE_MODE == 2){
    stepClockTrigger_Grids();
  }
}


// what was the last state of the trigger input??
bool CLOCKSEEN=0;
int clock_ticker=0;

void loop()                    
{
   // reads with inversion due to tre transistor follower
   bool trigger = !digitalRead(TRIGGER_IN);
   if(trigger != CLOCKSEEN){ // then it's changed since last loop.
    CLOCKSEEN = trigger; // fix that.
    if(trigger){
      stepClockTrigger();
    }else{ // downbeat. now we have time to do some math:
      clock_ticker = 0;
      updateKnobs();
      if(DEBUG) DEBUG_PRINTS();
    }
   }else{  // same as we saw last time
    if(trigger){ // still high?
      clock_ticker++;
      // five seconds in sets of 3ms is sixteen hundred
      if(clock_ticker > 1660){      // Easteregg mode activate.
        clock_ticker = 0;
        if(Fill1 < 0.15 && Fill2 < 0.15) // fill knobs must be 100% left
           updateMode();
      }
    }
   }
   updateOuts();  // turn on and off the outputs:
   delay(3); // milliseconds   
}


void DEBUG_PRINTS()
{
  Serial.print("L1= ");Serial.print(Length1); Serial.print("\t");
  Serial.print("F1= ");Serial.print(Fill1);Serial.print("\t");
  Serial.print("R1= ");Serial.print(Rotate1);Serial.print("\n");
  Serial.print("Pat1= { ");
  for(int i=0 ; i < Length1;i++){
    // WARNING this part can take a while if Length1 is too high.
    // little CPU gets funny if we overload it here. 
    Serial.print(PATTERN1[i]); Serial.print(" , ");
  }Serial.print(" }\n");
  
}

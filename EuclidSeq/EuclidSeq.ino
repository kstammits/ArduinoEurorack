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



// four channels of 4 bits make up a 16 bit array
unsigned int PDB0[288] = {63033,19433,47616,62903,52382,46042,32628,65321,12452,25215,11982,15206,8298,9007,6174,50279,12468,6811,54463,29103,11867,34800,60579,54921,59155,43024,27023,50315,6263,19161,64049,35632,36112,15182,55789,20095,938,1329,57934,18538,9266,32285,13803,27224,12035,42794,20822,56827,37712,57295,8905,40131,2340,43707,24330,24804,37776,14607,49365,13841,53008,14294,41104,43521,58251,2447,17688,15344,373,50017,63467,53481,54086,49693,3824,52680,33660,34515,39571,55805,19270,59589,35311,27463,3967,11559,17474,39358,45744,55799,17631,28234,37788,27218,18965,34456,28075,48559,7009,9814,12902,64979,21884,6144,11006,21560,16878,21591,4091,53248,6524,45682,12789,61043,31158,52473,31347,57767,5780,21441,58757,64232,64677,13594,20838,21141,27143,58097,23643,15607,7287,3312,14145,3597,7761,42990,58828,60915,7971,17052,12525,6073,27815,36017,9890,12663,45308,14731,11606,53020,7993,32666,54417,42744,43746,2373,20230,15010,48443,40731,59444,25969,32246,30901,20421,56442,20141,33293,52334,33395,36436,12378,31111,14810,7166,12169,39676,36531,53467,61348,30932,32144,65375,62587,5902,48946,62420,2106,51889,408,27341,46421,15412,39809,21901,51164,49887,51875,38630,51779,9493,60139,39345,48613,43946,2311,8547,19540,14281,27225,28719,50426,48123,51752,59740,30739,3494,58708,39369,23285,51575,61688,20448,3753,37014,8258,63640,34303,32733,30977,31666,35081,62727,4726,14804,698,32030,28663,17391,15753,55924,23219,18196,39658,34224,36911,42111,58730,45067,24192,62597,18395,22211,9011,14785,42738,49363,221,58063,1411,52445,8288,48079,62898,556,44997,57415,10076,43783,6406,61602,19196,58189,59874,5953,51008,42358,52912,3281,25259,59382,42603,49720,11605,10233,5852,15180,26458};


void updatePatterns_Grids()
{ 
  // based on pattern_database
  // 
  // will keep Length and Fill knobs, but RotateNi is now startPos.
  byte bFill1 = (byte)( Fill1i >> 6); // 0-16 is 0-1024/(2^6)
  byte bFill2 = (byte)( Fill2i >> 6); 
  int start1 = Rotate1i >> 2; // 0-255 is 0-1024/4
  int start2 = Rotate2i >> 2;
  byte PDBH,PDBL;
  for(int i=0;i < Length1;i++){
   // do something with PATTERN1 and PATTERN2
   PDBL = lowByte( PDB0[i+start1]) ;
   PATTERN1[i] = (bFill1 > (PDBL & B00001111) );
   PATTERN2[i] = (bFill1 > ( (PDBL & B11110000) >> 4 ));
  }
  for(int i=0;i < Length2;i++){
   // do something with PATTERN3 and PATTERN4
   PDBH = highByte( PDB0[i+start2]) ;
   PATTERN3[i] = (bFill2 > (PDBL & B00001111) );
   PATTERN4[i] = (bFill2 > ( (PDBL & B11110000) >> 4 ));
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

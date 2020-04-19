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

// how many loops should we keep a trigger output online?
int OUTPUT_DURATION_CYCLES = 5;
// might be nice to put a trimpot here someday.

// save the states we're gonna use:
bool PATTERN1[MAX_PATTERN];
bool PATTERN2[MAX_PATTERN];

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
    Serial.begin(9600);
    Serial.print("Hello! EuclidSeq V1.2\n");
   }

   for(int i=0;i<MAX_PATTERN;i++){
    PATTERN1[i] = 0; // all flat, no notes
    PATTERN2[i] = 0; // will update in a second tho
   }
   
   updateKnobs(); // initializer
}

// these defaults are never used since setup() recalculates things.
int Length1 = 8;
int Length2 = 8;
float Fill1 = 0.50f;
float Fill2 = 0.50f;
float Rotate1 = 0.0f;
float Rotate2 = 0.0f;

void updatePatterns()
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


void updateKnobs()
{ // read all six analogs and interpret results
  // Lengths I want to scroll from 0-32 steps
  // and this analogRead sees 0-1023 on the Nano.
  // so divide by 32
  int x = Length1; // save prev value so we can see if it's changed.
  bool UPDATED = 0;// default to not seeing any change.
  Length1 = (int)(((float)analogRead(LENGTH_IN1))/downconversion);
  Length1 = (Length1 < 2)?2:Length1; // and round off the corners
  Length1 = (Length1 > (MAX_PATTERN-2))?MAX_PATTERN:Length1;
  if(Length1 != x) UPDATED=1;
  x=Length2;
  Length2 = (int)(((float)analogRead(LENGTH_IN2))/downconversion);
  Length2 = (Length2 < 2)?2:Length2;
  Length2 = (Length2 > (MAX_PATTERN-2))?MAX_PATTERN:Length2;
  if(Length2 != x) UPDATED=1;

  float y = Rotate1;
  Rotate1 = (float)(analogRead(ROTATE_IN1)) / 1024.0f; // 0-100%
  if(abs(Rotate1 - y) > 0.05) UPDATED=1;
  y=Rotate2;
  Rotate2 = (float)(analogRead(ROTATE_IN2)) / 1024.0f;
  if(abs(Rotate2 - y) > 0.05) UPDATED=1;

  // Fill should be 100% at halfway due to the physical scaling
  // allows for +12V at CV jack, while letting the +5V be top.
  y=Fill1;
  Fill1 = (float)(analogRead(FILL_IN1)) / 931.0f;
  Fill1 = (Fill1 > 1.0f)?1.0f:Fill1; // capped.
  if(abs(Fill1 - y) > 0.05) UPDATED=1;
  Fill2 = (float)(analogRead(FILL_IN2)) / 931.0f;
  Fill2 = (Fill2 > 1.0f)?1.0f:Fill2;
  if(abs(Fill2 - y) > 0.05) UPDATED=1;

  if(UPDATED) updatePatterns();
  
}

// gonna use a countdown to keep the trigger high for a few milliseconds.
int triggertime[4] = {0,0,0,0};
bool gateState[4] = {0,0,0,0}; // remember what we set
int gatePin[4] = {GATE_OUT1A, GATE_OUT1B, GATE_OUT2A, GATE_OUT2B};
 
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

void stepClockTrigger()
{ // got a clock signal, let's perform
  Step1++; Step2++;
  if(Step1 >= Length1) Step1=0;
  if(Step2 >= Length2) Step2=0;
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


// what was the last state of the trigger input??
bool CLOCKSEEN=0;


void loop()                    
{
   // reads with inversion due to tre transistor follower
   bool trigger = !digitalRead(TRIGGER_IN);
   if(trigger != CLOCKSEEN){ // then it's changed since last loop.
    CLOCKSEEN = trigger; // fix that.
    if(trigger){
      stepClockTrigger();
    }else{ // downbeat. now we have time to do some math:
      updateKnobs();
      if(DEBUG) DEBUG_PRINTS();
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

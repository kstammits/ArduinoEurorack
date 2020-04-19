//
//
// .c file
// Arduino 1.8.10
// EuclidSeq V0.1
// aims to be a dual channel pattern sequencer. 
// KDS 2020-04-18 Version 0
// 

// firstly, a pinout description
// digital writes:
#define GATE_OUT1A  3
#define GATE_OUT1B  4
#define GATE_OUT2A  5
#define GATE_OUT2B  6

// digital reads:
#define TRIGGER_IN  4
// analog reads:
#define LENGTH_IN1 8
#define LENGTH_IN2 9
#define FILL_IN1 8
#define FILL_IN2 9
#define ROTATE_IN1 8
#define ROTATE_IN2 9

// how long is a long pattern?
#define MAX_PATTERN 32
float downconversion = 1024.0f  /  ((float)(MAX_PATTERN));

// enable or disable serial messaging:
#define DEBUG 0

// how many loops should we keep a trigger output online?
int OUTPUT_DURATION_CYCLES = 5;
// might be nice to put a trimpot here someday.



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
    Serial.print("Hello! EuclidSeq V0.1\n");
   }

   for(int i=0;i<MAX_PATTERN;i++){
    PATTERN1[i] = 0; // all flat, no notes
    PATTERN2[i] = 0; // will update in a second tho
   }
   
   updateKnobs();
   updatePatterns();
}


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
  
  float stepSize; int nHits; int startStep; int stepHit;
  
  // fill the bool array. takes some math.
  nHits = (int)( Fill1 * (float)Length1 );
  startStep = (int)( Rotate1 * (float)Length1 );
  for(int i=1;i <= nHits;i++){
    stepHit = Length1/i + startStep;
    if(stepHit>=Length1) stepHit -= Length1;
    PATTERN1[stepHit] = 1;
  }

  // repeat for channel 2
  nHits = (int)( Fill2 * (float)Length2 );
  startStep = (int)( Rotate2 * (float)Length2 );  
  for(int i=1;i <= nHits;i++){
    stepHit = Length2/i + startStep;
    if(stepHit>=Length2) stepHit -= Length2;
    PATTERN2[stepHit] = 1;
  }

}


void updateKnobs()
{ // read all six analogs and interpret results
  // Lengths I want to scroll from 0-32 steps
  // and this analogRead sees 0-1023 on the Nano.
  // so divide by 32
  Length1 = (int)(((float)analogRead(LENGTH_IN1))/downconversion);
  Length1 = (Length1 < 2)?2:Length1; // and round off the corners
  Length1 = (Length1 > (MAX_PATTERN-2))?MAX_PATTERN:Length1;

  Length2 = (int)(((float)analogRead(LENGTH_IN2))/downconversion);
  Length2 = (Length2 < 2)?2:Length2;
  Length2 = (Length2 > (MAX_PATTERN-2))?MAX_PATTERN:Length2;

  Rotate1 = (float)(analogRead(ROTATE_IN1)) / 1024.0f; // 0-100%
  Rotate2 = (float)(analogRead(ROTATE_IN2)) / 1024.0f;

  // Fill should be 100% at halfway due to the physical scaling
  // allows for +12V at CV jack, while letting the +5V be top.
  Fill1 = (float)(analogRead(FILL_IN1)) / 512.0f;
  Fill2 = (float)(analogRead(FILL_IN2)) / 512.0f;
  Fill1 = (Fill1 > 1.0f)?1.0f:Fill1; // capped.
  Fill2 = (Fill2 > 1.0f)?1.0f:Fill2;
  
}

// gonna usea countdown to keep the trigger high for a few milliseconds.

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
   
   if(trigger != CLOCKSEEN){
    // then it's changed since last loop.
    CLOCKSEEN = trigger; // fix that.
    if(trigger){
      stepClockTrigger();
      if(DEBUG) DEBUG_PRINTS();
    }else{
      // downbeat. now we have time to do some math:
      updateKnobs();
      updatePatterns();
    }
   }
   // turn on and off the outputs:
   updateOuts();

   delay(3); // milliseconds???   
}


void DEBUG_PRINTS()
{
  Serial.print("L1= ");Serial.print(Length1); Serial.print("\n");
  Serial.print("F1= ");Serial.print(Fill1);Serial.print("\n");
  Serial.print("R1= ");Serial.print(Rotate1);Serial.print("\n");
  Serial.print("Pat1= { ");
  for(int i=0 ; i < MAX_PATTERN;i++){
    Serial.print(PATTERN1[i]); Serial.print(" , ");
  }Serial.print(" }\n");
  
}

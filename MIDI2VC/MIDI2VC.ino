
#include <MIDI.h>
#include <AltSoftSerial.h>
//#include <SoftwareSerial.h>


// 2020-03-23 proof of concept
// working midi in - cv out
// made for Arduino Nano
// March 24th 2020, code cleanup so we can debug fine performance later.
// DEPLOYED to PROD 2020-03-24
// bugfixes and smoothening 2020-03-28
// 2020-03-29 implemented Jack sensing to smarten kbsplit and roundRobin out.
// took some new wires and all open pins...





//D0 and D1 are taken by HardwareSerial
// and MIDI-IN uses HardwareSerial.

#define GATE_OUT1  2
#define GATE_OUT2  10
#define GATE_OUT3  3

#define PITCH_OUT1  11
#define PITCH_OUT2  5
#define PITCH_OUT3  6

#define GATE_SENSE1  4
#define GATE_SENSE2  7
#define GATE_SENSE3  A6


// A0 = Switch for MIDI_OUT Mode
// A1 = Switch for MIDI_IN Mode

int GATE_IN_DEF[] = {12,13,A5};
int PITCH_IN_DEF[] = {A2,A3,A4};

// D4 appears maybe unused?
// D7 appears maybe unused?
// D8 used by AltSoftSerial as transmit
// D9 used by AltSoftSerial as receive
// A6 appears maybe unused?
// A7 set it up as a trimmer for output scaling.
float TUNING_TRIMMER;

// YES, three spare pins could read the jack state on GATE OUTS.



#define DEBUG 0
// debug means no MIDI IN, instead a USB/PC Serial


 // Created and binds the MIDI interface to the default hardware Serial port
 // MIDI_CREATE_DEFAULT_INSTANCE();
 MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI_in); // pins 0/1

 //AltSoftSerial demands Timer1, so we gotta move our PWM outs to Timer0/2.
 //Occupies Timer1: the only 16 bit Timer on Nano.
 // SoftwareSerial gtSerial(9,8); // transmit 8 , recv 9
 AltSoftSerial gtSerial; // transmit 9, recv 8.
 MIDI_CREATE_INSTANCE(AltSoftSerial, gtSerial, MIDI_out);
 //MIDI_CREATE_INSTANCE(SoftwareSerial, gtSerial, MIDI_out);



void TIMERCONFIG(){
   cli(); // stop interrupts

   // and Pin D5/D6 is on Timer0 
   TCCR0A = 0;
   TCCR0B = 0;   
   TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
   TCCR0B = _BV(CS00);    //the Fast PWM

   OCR0A = 20; // D6 PWM duty = 20/255
   OCR0B = 20; // D5 PWM duty = 20/255

// Pin D3/11 is on Timer2 
   TCCR2A = 0;
   TCCR2B = 0; 
   TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
   TCCR2B = _BV(CS20);    //the Fast PWM

   OCR2A = 20; // set initial PWM duty cycle on pin 11
   OCR2B = 20; // set initial PWM duty cycle on pin 3

   sei(); // allow interrupts
}

int unison_keyCount =0;
int unison_pitchStack[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int kbSplit_keyCount[3] ={0,0,0};
int kbSplit_pitchStack[48]; // init during setuo()
int rr_pitches[3] ={0,0,0};
int rr_nextSlot=1;

void setup()
 {
    for(int i=0;i<48;i++){// set/clear
      kbSplit_pitchStack[i]=0;
    }
  
    pinMode(8,INPUT_PULLUP); // 
    pinMode(9,OUTPUT); // prep for serial???
    
    pinMode(A0, INPUT); // a switch for midi_out mode.
    pinMode(A1, INPUT); // a switch for midi_in mode.
    pinMode(A7, INPUT); // a potentiometer for tuning 
    
    for(int j=0;j<3;j++)// gates in top 3 jacks
      pinMode(GATE_IN_DEF[j], INPUT);// left middle right

    for(int j=0;j<3;j++)// pitch in top 3 jacks
      pinMode(PITCH_IN_DEF[j], INPUT_PULLUP); // left middle right

    pinMode(PITCH_OUT1, OUTPUT);
    pinMode(PITCH_OUT2, OUTPUT); // pitch out bottom 3 jacks
    pinMode(PITCH_OUT3, OUTPUT);
    //initialize the PWMs
    noTone(PITCH_OUT1); noTone(PITCH_OUT2);noTone(PITCH_OUT3);
    
    // then speed up Timer0/2
    TIMERCONFIG(); // and enable PWMs

    pinMode(GATE_OUT1, OUTPUT);
    pinMode(GATE_OUT2, OUTPUT); // gates out bottom 3 jacks
    pinMode(GATE_OUT3, OUTPUT);  // left middle right
    digitalWrite(GATE_OUT1,LOW);
    digitalWrite(GATE_OUT2,LOW);
    digitalWrite(GATE_OUT3,LOW);

    pinMode(GATE_SENSE1, INPUT);// the normalled gate_out
    pinMode(GATE_SENSE2, INPUT); // with a pulldown
    pinMode(GATE_SENSE3, INPUT);  // to sense if jack plug.
    
    handleSwitches(); //initialize their position and the ADC

    //top half is cvgate in and MIDI out.
    MIDI_out.begin(1);  // 
  
  if(!DEBUG){
      //bottom half is Midi_input and cvgate out
      MIDI_in.setHandleNoteOn(handleNoteOn);  // Put only the name of the function
      MIDI_in.setHandleNoteOff(handleNoteOff);
  
      //MIDI_in.begin(MIDI_CHANNEL_OMNI);  // Listen to all incoming messages
      MIDI_in.begin(1);  // Listen to all incoming messages
  }else{
      Serial.begin(9600); Serial.print("Hello.\n");
  }

 
}

//remember the last position of the switches.
char MODE_MIDIOUT = 'x';
char MODE_MIDIIN = 'x';

int NUM_JACKS_OUT=0; // hardwire before we get polling.
bool JACKS_PLUGGED[3] = {0,0,0};

int KBSPLITO1 = 68-12; // in units of PWM 'myPitch' 
int KBSPLITO2 = 97-12; // found by trial and error since I cant get midi in and serial out simultaneously in this device
int KBSPLITLINE[2] = {KBSPLITO1,KBSPLITO2};

int MIDINote2PWM(byte pitch){
  float pitch_fine = (float)(pitch) - 24.0f;
  int x = (int)(pitch_fine * TUNING_TRIMMER);
  if(x<3)  x=3;
  if(x>252)  x=252;
  return(x);
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
    // Do whatever you want when a note is pressed.
    if(channel == 1){
      int myPitch = MIDINote2PWM(pitch);
      
      if(MODE_MIDIIN=='a'){
        // that's unison.
        // this might need a stack/list to pop during keyoff.
        unison_keyCount++;
        if(unison_keyCount >14)
          unison_keyCount=14; // weird behavior is okay if all keys are held
 
            unison_pitchStack[unison_keyCount-1] = myPitch;
            OCR0A = myPitch; // 0a is pin D6 is pitch3 (right)
            OCR2A = myPitch; // 2a ia pin D11 is Pitch1 (left)
            OCR0B = myPitch; // 0b is pin D5 is Pitch2 (middle)
           // OCR2B = myPitch; // 2b is pin 3 not used for pitch.  
            digitalWrite(GATE_OUT1,HIGH);
            digitalWrite(GATE_OUT2,HIGH);
            digitalWrite(GATE_OUT3,HIGH);
      }else if(MODE_MIDIIN=='b'){
        // skooch upwards?
        myPitch = myPitch + 12;
        if(myPitch > 250) myPitch = 250;
        // kbSplit mode.
        // then noteOn is just for one of the three things.
        if(myPitch < KBSPLITLINE[0]){
          // low register.
          OCR2A = myPitch; // 2a ia pin D11 is Pitch1 (left)
          kbSplit_pitchStack[kbSplit_keyCount[0]] = myPitch;
          kbSplit_keyCount[0]++;
          if(kbSplit_keyCount[0] >14) kbSplit_keyCount[0] = 14;
          digitalWrite(GATE_OUT1,HIGH);
        }else if(myPitch < KBSPLITLINE[1]){
          OCR0B = myPitch - KBSPLITLINE[0]; // 0b is pin D5 is Pitch2 (middle)
          kbSplit_pitchStack[kbSplit_keyCount[1]+16] = myPitch;
          kbSplit_keyCount[1]++;
          if(kbSplit_keyCount[1] >14) kbSplit_keyCount[1] = 14;
          digitalWrite(GATE_OUT2,HIGH);
        }else{ // >170 is high register.
          OCR0A = myPitch - KBSPLITLINE[1]; // 0a is pin D6 is Pitch3 (right)
          kbSplit_pitchStack[kbSplit_keyCount[2]+32] = myPitch;
          kbSplit_keyCount[2]++;
          if(kbSplit_keyCount[2] >14) kbSplit_keyCount[2] = 14;
          digitalWrite(GATE_OUT3,HIGH);
        }
                  
      }else if(MODE_MIDIIN=='c'){
          // Round-Robin Style.

          if(rr_nextSlot==1){
            OCR2A = myPitch; // 2a ia pin D11 is Pitch1 (left)
            rr_pitches[0] = myPitch;
            digitalWrite(GATE_OUT1,HIGH);
            // now assign next slot 
            if(rr_pitches[1]==0 && JACKS_PLUGGED[1]){
              rr_nextSlot = 2;
            }else if(rr_pitches[2]==0 && JACKS_PLUGGED[2]){
              rr_nextSlot = 3;
            }//else{ // no free keys!
              // leave the 'next slot' here?
            //}
          }else if(rr_nextSlot==2){
            OCR0B = myPitch; // 0b is pin D5 is Pitch2 (middle)
            rr_pitches[1]= myPitch;
            digitalWrite(GATE_OUT2,HIGH);
              // now assign next slot 
              if(rr_pitches[2]==0 && JACKS_PLUGGED[2]){
                rr_nextSlot = 3;
              }else if(rr_pitches[0]==0 && JACKS_PLUGGED[0]){
                rr_nextSlot = 1;
              }//else{ // no free keys!
               // leave the 'next slot' here?
               //}
           }else { // it's pointing to slot3.
             OCR0A = myPitch; // 0a is pin D6 is Pitch3 (right)
             rr_pitches[2]= myPitch;
             digitalWrite(GATE_OUT3,HIGH);
              // now assign next slot 
              if(rr_pitches[0]==0 && JACKS_PLUGGED[0]){
                rr_nextSlot = 1;
              }else if(rr_pitches[1]==0 && JACKS_PLUGGED[1]){
                rr_nextSlot = 2;
              }//else{ // no free keys!
               // leave the 'next slot' here?
               //}
           }
                 
      }//endif MODE==c
    }//endif channel==1
}


void remove_kbSplitKeys(int myPitch){
// the PWM note is given

  if(myPitch < KBSPLITLINE[0]){
    // low register.
    for(int i=0;i<kbSplit_keyCount[0];i++){
      if(kbSplit_pitchStack[i] == myPitch){
        // remove and being left shifting the remainder.
        for(int j=i;j<kbSplit_keyCount[0];j++){
          kbSplit_pitchStack[j] = kbSplit_pitchStack[j+1];
        }
        kbSplit_keyCount[0]--;
        break;//out the for 'i' I hope.
      }
    }
    if(kbSplit_keyCount[0] < 1){
      kbSplit_keyCount[0] = 0;
      digitalWrite(GATE_OUT1,LOW);
    }else{ // another key held? set new pitch.
      OCR2A = kbSplit_pitchStack[kbSplit_keyCount[0]-1]; // 2a ia pin D11 is Pitch1 (left)   
    }
  }else if(myPitch < KBSPLITLINE[1]){
    for(int i=0;i<kbSplit_keyCount[1];i++){
      if(kbSplit_pitchStack[i+16] == myPitch){
        // remove and being left shifting the remainder.
        for(int j=i;j<kbSplit_keyCount[1];j++){
          kbSplit_pitchStack[j+16] = kbSplit_pitchStack[j+17];
        }
        kbSplit_keyCount[1]--;
        break;//out the for 'i' I hope.
      }
    }    
    if(kbSplit_keyCount[1] < 1){
      kbSplit_keyCount[1] = 0;
      digitalWrite(GATE_OUT2,LOW);
    }else{ // another key held? set new pitch.
      OCR0B = kbSplit_pitchStack[kbSplit_keyCount[1]+15] -KBSPLITLINE[0]; // 0b is pin D5 is Pitch2 (middle)  
    }
  }else{ // >170 is high register.
    for(int i=0;i<kbSplit_keyCount[2];i++){
      if(kbSplit_pitchStack[i+32] == myPitch){
        // remove and being left shifting the remainder.
        for(int j=i;j<kbSplit_keyCount[2];j++){
          kbSplit_pitchStack[j+32] = kbSplit_pitchStack[j+33];
        }
        kbSplit_keyCount[2]--;
        break;//out the for 'i' I hope.
      }
    }
    if(kbSplit_keyCount[2] < 1){
      kbSplit_keyCount[2] = 0;
      digitalWrite(GATE_OUT3,LOW);
    }else{ // another key held? set new pitch.
     OCR0A = kbSplit_pitchStack[kbSplit_keyCount[2]+31] - KBSPLITLINE[1]; // 0a is pin D6 is pitch3 (right)        
    }
  }
  
}

void remove_unisonKeys(int myPitch){

  for(int i=0;i<unison_keyCount;i++){
    if(unison_pitchStack[i] == myPitch){
      // remove and being left shifting the remainder.
      for(int j=i;j<unison_keyCount;j++){
        unison_pitchStack[j] = unison_pitchStack[j+1];
      }
      unison_keyCount--;
      break;//out the for 'i' I hope.
    }
  }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
    // Do something when the note is released.
    // Note that NoteOn messages with 0 velocity are interpreted as NoteOffs.
    if(channel == 1){
      int myPitch = MIDINote2PWM(pitch);
      
      if(MODE_MIDIIN=='a'){
        // that's unison.
        remove_unisonKeys(myPitch);
        if(unison_keyCount < 1){
          unison_keyCount=0;
          digitalWrite(GATE_OUT1,LOW);
          digitalWrite(GATE_OUT2,LOW);
          digitalWrite(GATE_OUT3,LOW);
        }else{ // some other key held.
          // set the pervious pitch
            OCR0A = unison_pitchStack[unison_keyCount-1]; // 0a is pin D6 is pitch3 (right)
            OCR2A = unison_pitchStack[unison_keyCount-1]; // 2a ia pin D11 is Pitch1 (left)
            OCR0B = unison_pitchStack[unison_keyCount-1]; // 0b is pin D5 is Pitch2 (middle)
        }

      }else if(MODE_MIDIIN=='b'){
        // kbSplit mode.
        // skooch upwards?
        myPitch = myPitch + 12;
        if(myPitch > 250) myPitch = 250;
        // then noteOff is just for one of the three things.
        remove_kbSplitKeys(myPitch);   
      }else if(MODE_MIDIIN=='c'){
          // round-robin
          // just let go the appropriate one.
          if(myPitch==rr_pitches[0]){
            rr_pitches[0]=0;
            digitalWrite(GATE_OUT1,LOW);
          }
          if(myPitch==rr_pitches[1]){
            digitalWrite(GATE_OUT2,LOW);
            rr_pitches[1]=0;
          }
          if(myPitch==rr_pitches[2]){
            rr_pitches[2]=0;
            digitalWrite(GATE_OUT3,LOW);
          }
      }
    }
 }


void handle_ReconfigureMIDIIn(){
 // when the midi_in switch changes
  if(DEBUG){
   Serial.print("MIDI_IN = ");Serial.print(MODE_MIDIIN);Serial.print("\n");
  }
  
  // poll the gate outs for jacks
  digitalWrite(GATE_OUT1, HIGH); delay(2);
  JACKS_PLUGGED[0] = !digitalRead(GATE_SENSE1);
  digitalWrite(GATE_OUT1, LOW);
  digitalWrite(GATE_OUT2, HIGH); delay(2);
  JACKS_PLUGGED[1] = !digitalRead(GATE_SENSE2);
  digitalWrite(GATE_OUT2, LOW);
  digitalWrite(GATE_OUT3, HIGH); delay(2);
  JACKS_PLUGGED[2] = !(analogRead(GATE_SENSE3) > 500);
  digitalWrite(GATE_OUT3, LOW);


 if(DEBUG){
   Serial.print("JP_L = ");Serial.print(JACKS_PLUGGED[0]);
   Serial.print("  JP_M = ");Serial.print(JACKS_PLUGGED[1]);
   Serial.print("  JP_R = ");Serial.print(JACKS_PLUGGED[2]);
   Serial.print("\n");
  }

  NUM_JACKS_OUT = JACKS_PLUGGED[0] + JACKS_PLUGGED[1] + JACKS_PLUGGED[2]; // because I dont know how to poll this yet. theyre alll on
 
  TUNING_TRIMMER = ((float)(analogRead(A7))) / 341.3f ; // (0 , +3)
  // reset counters:
  unison_keyCount = 0;
  rr_pitches[0]=0;  rr_pitches[1]=0;   rr_pitches[2]=0;

  if(JACKS_PLUGGED[0]){
    rr_nextSlot=1;
  }else if(JACKS_PLUGGED[1]){
    rr_nextSlot=2; 
  }else{
    // doesnt matter, somebody needs it!
    rr_nextSlot=3;
  }
  
  kbSplit_keyCount[0] = 0;
  kbSplit_keyCount[1] = 0;
  kbSplit_keyCount[2] = 0;
  if(NUM_JACKS_OUT < 2){ // one or zero
    KBSPLITLINE[0] = 999;
    KBSPLITLINE[1] = 999;
  }else if(NUM_JACKS_OUT==2){
    KBSPLITLINE[0] = KBSPLITO1-1; // IDK WHY, something is impacted
    KBSPLITLINE[1] = 999;
  }else{ // its three   
    KBSPLITLINE[0] = KBSPLITO1;
    KBSPLITLINE[1] = KBSPLITO2;
  }  
 
}

bool KnownGate[3] = {0,0,0};
int SentPitch[3] = {0,0,0};

void handle_ReconfigureMIDIOut(){
 // when the midi_out switch chanes
 // poll the gate ins for jacks.
 // i dont know that we can
if(DEBUG){
  Serial.print("MIDI_OUT=");Serial.print(MODE_MIDIOUT);Serial.print("\n");
}
 // shut down existing notes:
 for(int i=0;i<3;i++){
  if(KnownGate[i]){
    KnownGate[i]=0;
    if(SentPitch[i]>0){
      MIDI_out.sendNoteOff(SentPitch[i], 0, 1);
      SentPitch[i]=0;
    }
  }
 }
 
}


void handleSwitches()
{
  
    int switch_1 = analogRead(A0);
    int switch_2 = analogRead(A1);
      // due to the resistor network on the ON-OFF-ON switches:
      // left left is like 480 - 500
      //center is <10    and   right is 850-890
      
      if(switch_1 <50){
           // top pin, center
           if(MODE_MIDIOUT != 'b'){ // then it changed
            MODE_MIDIOUT = 'b';
            handle_ReconfigureMIDIOut();
           }
      }else if(switch_1 < 600){
        // top pin, left
         if(MODE_MIDIOUT != 'a'){ // then it changed
          MODE_MIDIOUT = 'a';
          handle_ReconfigureMIDIOut();
         }     
      }else{
         // top in, right
         if(MODE_MIDIOUT != 'c'){ // then it changed
          MODE_MIDIOUT = 'c';
          handle_ReconfigureMIDIOut();
         }
      }
      
      if(switch_2 <50){
        // bottom pin, center
         if(MODE_MIDIIN != 'b'){ // then it changed.
          MODE_MIDIIN = 'b';
          handle_ReconfigureMIDIIn();
         }
      }else if(switch_2 < 600){
        // bottom pin, left
         if(MODE_MIDIIN != 'a'){ // then it changed.
          MODE_MIDIIN = 'a';
          handle_ReconfigureMIDIIn();
         }
      }else{
       // bottom in, right
         if(MODE_MIDIIN != 'c'){// changed ... 
          MODE_MIDIIN = 'c';
          handle_ReconfigureMIDIIn();
         }
      }
}

int SENDVELO=50;
int SPLIT_BASELINE_OFFSET[3] = {0,24,36};

int grabNote(int PIN){
  int x = analogRead(PIN) + analogRead(PIN);
  // now ranges 0-2046
  // want the top note, to correspond to key 60.
  // and bottom to be note 12. 
  return(x/34 + 12);
}

void handleGateIns(){
    int p1;
    if(MODE_MIDIOUT=='a'){
      // chording
      // means we dont care about gate identity, loop em:
      for(int i=0;i<3;i++){
          p1 = digitalRead(GATE_IN_DEF[i]);
          if(p1 != KnownGate[i]){  // changed state on pin
            if(DEBUG){
            Serial.print("Gate in ");
            Serial.print(i);Serial.print(" ");Serial.print(p1);Serial.print("\n");
            }
            if(!p1){ // high
              for(int j=0;j<3;j++){ // fire all pitches!
                  SentPitch[j] = grabNote(PITCH_IN_DEF[j]);
                  if(SentPitch[j] < 71){ // with a pullup, nojack = max key = 72
                     MIDI_out.sendNoteOn(SentPitch[j], SENDVELO, 1);
                     if(DEBUG){
                        Serial.print("Sent Note ");Serial.print(j);
                        Serial.print(" because of jack ");Serial.print(i);
                        Serial.print(" with pitch ");Serial.print(SentPitch[j]);
                        Serial.print("\n");
                      }
                  } 
              }
            }else{//Low
              for(int j=0;j<3;j++)
                  MIDI_out.sendNoteOff(SentPitch[j], 0, 1);
            }
            KnownGate[i] = p1;
          }
      }
    }else if(MODE_MIDIOUT=='b'){
      // kb split, like indiv mode but with offsets
      for(int i=0;i<3;i++){
        p1 = digitalRead(GATE_IN_DEF[i]);
        if(p1 != KnownGate[i]){  // changed state on pin
            if(!p1){ // high
              SentPitch[i] = grabNote(PITCH_IN_DEF[i]) + SPLIT_BASELINE_OFFSET[i];
              if( SentPitch[i] >120)  SentPitch[i]=120;
              if( SentPitch[i] < 71){ // with a pullup, nojack = max key = 72
                 MIDI_out.sendNoteOn(SentPitch[i], SENDVELO, 1);
              }
            }else{//Low
              MIDI_out.sendNoteOff(SentPitch[i], 0, 1);
            }
          KnownGate[i] = p1;
        }
      }
    }else if(MODE_MIDIOUT=='c'){
      // individual.
      for(int i=0;i<3;i++){
          p1 = digitalRead(GATE_IN_DEF[i]);
          if(p1 != KnownGate[i]){  // changed state on pin
            if(!p1){ // high
              SentPitch[i] = grabNote(PITCH_IN_DEF[i]);
              if(SentPitch[i] < 71){ // with a pullup, nojack = max key = 72
                MIDI_out.sendNoteOn(SentPitch[i], SENDVELO, 1);
              }
            }else{//Low
              MIDI_out.sendNoteOff(SentPitch[i], 0, 1);
            }
            KnownGate[i] = p1;
          }
      }
    }  
}

 int switch_delay = 3000;
 void loop()
 {
    
    // dont want to read the switches too often.
    switch_delay--;
    if(switch_delay < 1){
     switch_delay = 7000;
     handleSwitches();
    }

    handleGateIns(); // and generate midi_out

    if(!DEBUG)
    MIDI_in.read(); // that's handle messages incoming
}

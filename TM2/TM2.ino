

// VERSION 0
// KDS 2019-12-07 a Turing Machine / Sequencer 2.
// The Tooring Machine. . ! :)

// these variables are holding 64 or 512 steps fo sequence
// but due to knob precision, it's scaled back to 32/256
// so half the stuff isnt used yet.

int pinP1 = A0;
int pinP2 = A2;
int pinL1 = A1;
int pinL2 = A3;

int pinO1 = 3;
int pinO2 = 5;
int pinC1 = 11;
int pinC2 = 12;

int pinS1 = 7;
int pinS2 = 8;

byte TMEM2[32];
byte TMEM1[32];


const int SEQMEM1[256] = { 33 , 36 , 29 , 31 , 27 , 21 , 30 , 34 , 45 , 38 , 24 , 41 , 31 , 36 , 36 , 30 , 52 , 40 , 33 , 30 , 31 , 29 , 38 , 35 , 26 , 26 , 10 , 29 , 44 , 25 , 40 , 34 , 33 , 31 , 33 , 53 , 14 , 29 , 35 , 35 , 40 , 23 , 34 , 26 , 35 , 36 , 18 , 25 , 26 , 23 , 46 , 60 , 43 , 8 , 22 , 54 , 42 , 12 , 28 , 31 , 21 , 58 , 48 , 6 , 26 , 57 , 27 , 7 , 33 , 43 , 47 , 47 , 18 , 28 , 29 , 12 , 43 , 35 , 32 , 37 , 17 , 46 , 26 , 35 , 55 , 32 , 30 , 12 , 13 , 42 , 48 , 28 , 33 , 23 , 44 , 51 , 18 , 13 , 31 , 29 , 34 , 39 , 47 , 40 , 20 , 43 , 34 , 6 , 33 , 43 , 29 , 53 , 35 , 14 , 44 , 47 , 10 , 25 , 46 , 30 , 14 , 23 , 48 , 38 , 23 , 33 , 26 , 17 , 48 , 52 , 35 , 32 , 34 , 44 , 21 , 31 , 31 , 9 , 33 , 32 , 21 , 28 , 36 , 59 , 42 , 24 , 38 , 32 , 35 , 37 , 17 , 6 , 37 , 63 , 44 , 14 , 19 , 42 , 52 , 22 , 24 , 39 , 13 , 27 , 48 , 20 , 39 , 31 , 37 , 62 , 8 , 7 , 34 , 26 , 51 , 50 , 22 , 26 , 25 , 24 , 47 , 46 , 37 , 33 , 9 , 38 , 37 , 31 , 50 , 23 , 35 , 29 , 33 , 39 , 17 , 24 , 26 , 30 , 45 , 29 , 17 , 44 , 26 , 31 , 38 , 28 , 55 , 31 , 23 , 46 , 43 , 28 , 1 , 20 , 48 , 41 , 43 , 27 , 32 , 48 , 31 , 26 , 23 , 42 , 33 , 17 , 29 , 26 , 42 , 26 , 21 , 38 , 39 , 58 , 31 , 28 , 34 , 5 , 13 , 49 , 36 , 45 , 53 , 7 , 25 , 36 , 26 , 35 , 37 , 44 , 22 , 21 , 30 , 35 , 51 , 27 };
const int SEQMEM2[256] = { 54 , 53 , 16 , 26 , 18 , 20 , 36 , 23 , 33 , 45 , 56 , 21 , 32 , 42 , 26 , 18 , 12 , 59 , 51 , 8 , 14 , 59 , 31 , 6 , 53 , 59 , 18 , 3 , 22 , 34 , 56 , 28 , 36 , 44 , 27 , 31 , 19 , 31 , 29 , 24 , 28 , 39 , 37 , 25 , 47 , 30 , 25 , 36 , 41 , 56 , 20 , 14 , 27 , 24 , 40 , 59 , 28 , 11 , 19 , 41 , 41 , 28 , 38 , 20 , 30 , 34 , 25 , 54 , 25 , 19 , 60 , 49 , 27 , 16 , 11 , 26 , 53 , 37 , 21 , 41 , 20 , 27 , 53 , 21 , 25 , 33 , 41 , 34 , 24 , 25 , 43 , 42 , 36 , 34 , 19 , 32 , 45 , 32 , 1 , 50 , 36 , 2 , 32 , 43 , 50 , 44 , 27 , 15 , 46 , 32 , 21 , 43 , 29 , 26 , 47 , 22 , 31 , 50 , 22 , 20 , 16 , 55 , 45 , 21 , 18 , 25 , 53 , 47 , 21 , 24 , 46 , 30 , 26 , 34 , 42 , 13 , 29 , 58 , 19 , 16 , 29 , 42 , 46 , 13 , 30 , 39 , 28 , 47 , 41 , 11 , 25 , 54 , 14 , 14 , 36 , 39 , 27 , 48 , 56 , 28 , 24 , 13 , 14 , 47 , 48 , 30 , 23 , 15 , 49 , 42 , 21 , 33 , 40 , 33 , 23 , 43 , 28 , 13 , 41 , 35 , 37 , 38 , 32 , 42 , 15 , 12 , 42 , 41 , 19 , 16 , 59 , 55 , 17 , 18 , 35 , 30 , 27 , 24 , 25 , 33 , 26 , 39 , 49 , 29 , 33 , 32 , 38 , 36 , 35 , 47 , 14 , 35 , 31 , 17 , 36 , 36 , 29 , 19 , 24 , 30 , 55 , 40 , 33 , 24 , 33 , 32 , 6 , 55 , 40 , 23 , 41 , 27 , 29 , 33 , 25 , 23 , 41 , 29 , 42 , 63 , 8 , 23 , 57 , 30 , 24 , 29 , 14 , 23 , 49 , 32 , 32 , 26 , 41 , 54 , 14 , 16 };


int SM1i=0;
int SM2i=0;

bool CLOCKSEEN1;
bool CLOCKSEEN2;

void setup() {
  // put your setup code here, to run once:
pinMode(A0, INPUT);
pinMode(A1, INPUT);
pinMode(A2, INPUT);
pinMode(A3, INPUT);

pinMode(pinO2, OUTPUT);
pinMode(pinO1, OUTPUT);
// clocks 
pinMode(pinC1, INPUT);
pinMode(pinC2, INPUT);

//switches
pinMode(pinS1, INPUT);
pinMode(pinS2, INPUT);

noTone(pinO2);noTone(pinO1);

//   Serial.begin(9600);
   randomSeed(analogRead(A5));

   // FILL the TM lists
   for(int i=0;i<32;i++){
     TMEM1[i] = random(1,63);
     TMEM2[i] = random(1,63);
   }
   
   // use a flip flop to decide is trigger has been handled already
   CLOCKSEEN1=false;
   CLOCKSEEN2=false;



 TCCR2A = 0;
  TCCR2B = 0;
 // TCCR2A = _BV(COM0A1) | _BV(WGM00) | _BV(WGM01) | _BV(WGM02);  // Clear OC1A (PIN 9) on Compare Match, set OC1A at Bottom; low WGM bits
 // TCCR2B = _BV(WGM13) | _BV(WGM12) | _BV(WGM11) | _BV(CS11);    // fast PWM mode 14, Prescaler=1

  TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS22);// | _BV(CS20);    // fast PWM mode 14
   OCR2A = 64;
   OCR2B = 10;
   // PD3 is OC2B 
   // and PD5 is OC0B
   TCCR0A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
   TCCR0B = _BV(WGM02) | _BV(CS01) | _BV(CS00);    // fast PWM mode 14 
   OCR0A = 64;
   OCR0B = 10;
}

void loop() {
    // put your main code here, to run repeatedly:

    bool CLOCK1_READ = digitalRead(pinC1);
    //in demo mode
  //  CLOCK1_READ = random(0,100) < random(0,10000);
    
    if(!CLOCKSEEN1 & CLOCK1_READ){
      // went up
      CLOCKSEEN1=true;
      SM1i++;
      int newValue = 1; // placeholder for analowWrite()
      int POTENTIOMETER_L1 = analogRead(pinL1);
      int PatLength = POTENTIOMETER_L1/32+1; // 0-32 steps
      if(SM1i >= PatLength){
         SM1i = 0;
      }
  //    Serial.print("Begin Sequence 1 Index ");
  //    Serial.println(SM1i);
      
       int POTENTIOMETER_P1 = analogRead(pinP1);  
      // POTENTIOMETER_P1 = 100;
       if(digitalRead(pinS1)){
         //# pattern mode
         int ii = (POTENTIOMETER_P1/4) + SM1i;
         if(ii >= 256) ii = ii - 256;
      
   //      Serial.print("Setting O1 SMi:\t");
   //      Serial.print(ii, DEC);
   //      Serial.print("\tVAL\t");
   //      Serial.print(SEQMEM1[ii], DEC);
         
      //   analogWrite(pinO1,SEQMEM1[ii]);
         newValue = SEQMEM1[ii];

       }else{
         // TURING MODE
  //    Serial.println("Turing mode");
         // range of random thresh is 2x potentiometer
         // so full clockwise = 50% flip
         if(POTENTIOMETER_P1 > random(0,2048)){
           TMEM1[SM1i] = random(1,63);
         }
   //      Serial.print("Setting O1 TM: ");
   //      Serial.println(TMEM1[SM1i]);
       //  analogWrite(pinO1,TMEM1[SM1i]);
         newValue = TMEM1[SM1i];
       }
 //      Serial.print("P1: ");
  //     Serial.print(POTENTIOMETER_P1);
  //     Serial.print("\t");
  //     Serial.print("L1: ");
  //     Serial.print(PatLength);
  //     Serial.print("\t");
  //     Serial.print("\n");
   // set the analog pin correctly here:
       OCR2B = newValue;
    }
    if(!CLOCK1_READ & CLOCKSEEN1){
     // coming down
     CLOCKSEEN1=false;
     // dont do much on this event.
    }

    bool CLOCK2_READ = digitalRead(pinC2);
    if(!CLOCKSEEN2 & CLOCK2_READ){
      // went up
      CLOCKSEEN2=true;
      SM2i++;
      int POTENTIOMETER_L2 = analogRead(pinL2);
      int newValue = 1; // placeholder for new timer ticker.
      int PatLength = POTENTIOMETER_L2/32+1; // 0-32 steps
      if(SM2i >= PatLength){
         SM2i=0;
      }
       int POTENTIOMETER_P2 = analogRead(pinP2);
       if(digitalRead(pinS2)){
         //# pattern mode
         int ii = (POTENTIOMETER_P2/4) + SM2i;
         if(ii >= 256) ii = ii - 256;
        // analogWrite(pinO2,SEQMEM2[ii]);
         newValue = SEQMEM2[ii];
       }else{
         // TURING MODE
         // range of random thresh is 2x potentiometer
         // so full clockwise = 50% flip
         if(POTENTIOMETER_P2 > random(0,2048)){
           TMEM2[SM2i] = random(1,63);
         }
   //      Serial.print("Setting O2 TM: ");
   //      Serial.println(TMEM2[SM2i]);
  //       analogWrite(pinO2,TMEM2[SM2i]);
           newValue = TMEM2[SM2i];
       }
   //    Serial.print("P2: ");
   //    Serial.print(POTENTIOMETER_P2);
   //    Serial.print("\t");
   //    Serial.print("L2: ");
   //    Serial.print(PatLength);
   //    Serial.print("\t");
   //    Serial.print("\n");
   // set the analogwrite here
    OCR0B = newValue;
    }
    if(!CLOCK2_READ & CLOCKSEEN2){
     // coming down
     CLOCKSEEN2=false;
     // dont do much on this event.
    }

   delay(50); // use shorter for PROD
   // the faster clock means these may not be ms anymore.
}

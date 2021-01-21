// the MCP4922 has two channels at 12 bits, so 0-4095
void printDAC(int channel, int value){
  if(channel==1)
  dac.outputA(value);
  if(channel==2)
  dac.outputB(value);
}

void ProcessSide1(){
       SM1i++; // increment pointer
      int newValue = 1; // output Value
      int POTENTIOMETER_L1 = analogRead(pinL1);
      int PatLength = POTENTIOMETER_L1/32+1; // 0-32 steps
      if(SM1i >= PatLength){
         SM1i = 0;
      }
      
      if(DEBUG){
         Serial.print("Begin Sequence 1 Index ");
         Serial.println(SM1i);
      }
      
       // 0-1023 on this board.
       int POTENTIOMETER_P1 = analogRead(pinP1);  
      
       if(digitalRead(pinS1)){
         //# pattern mode
         int ii = (POTENTIOMETER_P1/4) + SM1i;
         if(ii >= 256) ii = ii - 256;
         if(DEBUG){ 
           Serial.print("Setting O1 SMi:\t");
           Serial.print(ii, DEC);
           Serial.print("\tVAL\t");
           Serial.print(SEQMEM1[ii], DEC);
         }
         newValue = SEQMEM1[ii];
       }else{
         // TURING MODE
          if(DEBUG) {
            Serial.println("Turing mode");
          }
         // range of random thresh is 2x potentiometer
         // so full clockwise = 50% flip
         if(POTENTIOMETER_P1 > random(0,2048)){
           TMEM1[SM1i] = random(1,4094);
         }
         if(DEBUG){
           Serial.print("Setting O1 TM: ");
           Serial.println(TMEM1[SM1i]);
         }
         newValue = TMEM1[SM1i];
       }
       if(DEBUG){
          Serial.print("P1: ");
          Serial.print(POTENTIOMETER_P1);
          Serial.print("\t");
          Serial.print("L1: ");
          Serial.print(PatLength);
          Serial.print("\t");
          Serial.print("\n");
       }

       printDAC(1,newValue);
}

void ProcessSide2(){
      SM2i++;
      int POTENTIOMETER_L2 = analogRead(pinL2);
      int newValue = 1; // placeholder for new timer ticker.
      int PatLength = POTENTIOMETER_L2/32+1; // 0-32 steps
      if(SM2i >= PatLength){
         SM2i=0;
      }
      
      // 0-1023 on this board.
       int POTENTIOMETER_P2 = analogRead(pinP2);
       if(digitalRead(pinS2)){
         //# pattern mode
         int ii = (POTENTIOMETER_P2/4) + SM2i;
         if(ii >= 256) ii = ii - 256;
         newValue = SEQMEM2[ii];
       }else{
         // TURING MODE
         // range of random thresh is 2x potentiometer
         // so full clockwise = 50% flip
         if(POTENTIOMETER_P2 > random(0,2048)){
           TMEM2[SM2i] = random(1,4094);
         }
           newValue = TMEM2[SM2i];
       }
       printDAC(2,newValue);
}

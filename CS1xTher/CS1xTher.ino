#include <CapacitiveSensor.h>

int pinS1R = 2;
int pinS1P = A0;
int pinS1O = 3;


//int WAVETABLE1[256] = { 0 , 16 , 23 , 28 , 32 , 36 , 39 , 42 , 45 , 48 , 51 , 53 , 56 , 58 , 60 , 62 , 64 , 66 , 68 , 70 , 72 , 73 , 75 , 77 , 79 , 80 , 82 , 83 , 85 , 86 , 88 , 89 , 91 , 92 , 93 , 95 , 96 , 98 , 99 , 100 , 101 , 103 , 104 , 105 , 106 , 108 , 109 , 110 , 111 , 112 , 113 , 114 , 116 , 117 , 118 , 119 , 120 , 121 , 122 , 123 , 124 , 125 , 126 , 127 , 128 , 129 , 130 , 131 , 132 , 133 , 134 , 135 , 136 , 137 , 138 , 139 , 140 , 141 , 142 , 142 , 143 , 144 , 145 , 146 , 147 , 148 , 149 , 150 , 150 , 151 , 152 , 153 , 154 , 155 , 155 , 156 , 157 , 158 , 159 , 160 , 160 , 161 , 162 , 163 , 163 , 164 , 165 , 166 , 167 , 167 , 168 , 169 , 170 , 170 , 171 , 172 , 173 , 173 , 174 , 175 , 176 , 176 , 177 , 178 , 179 , 179 , 180 , 181 , 181 , 182 , 183 , 183 , 184 , 185 , 186 , 186 , 187 , 188 , 188 , 189 , 190 , 190 , 191 , 192 , 192 , 193 , 194 , 194 , 195 , 196 , 196 , 197 , 198 , 198 , 199 , 200 , 200 , 201 , 202 , 202 , 203 , 203 , 204 , 205 , 205 , 206 , 207 , 207 , 208 , 208 , 209 , 210 , 210 , 211 , 211 , 212 , 213 , 213 , 214 , 214 , 215 , 216 , 216 , 217 , 217 , 218 , 219 , 219 , 220 , 220 , 221 , 222 , 222 , 223 , 223 , 224 , 224 , 225 , 226 , 226 , 227 , 227 , 228 , 228 , 229 , 230 , 230 , 231 , 231 , 232 , 232 , 233 , 233 , 234 , 235 , 235 , 236 , 236 , 237 , 237 , 238 , 238 , 239 , 239 , 240 , 240 , 241 , 242 , 242 , 243 , 243 , 244 , 244 , 245 , 245 , 246 , 246 , 247 , 247 , 248 , 248 , 249 , 249 , 250 , 250 , 251 , 251 , 252 , 252 , 253 , 253 , 254 , 254 , 255 , 255 , 255 };
// cosine function to give a smooth output
//  (1-cos(x/256*3.14)) /2 
// is going to start at zero and sweep up to 1 at 256 steps later. 
int WAVETABLE2[256] = { 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 1 , 1 , 1 , 1 , 2 , 2 , 2 , 2 , 3 , 3 , 3 , 4 , 4 , 5 , 5 , 6 , 6 , 6 , 7 , 7 , 8 , 9 , 9 , 10 , 10 , 11 , 12 , 12 , 13 , 14 , 14 , 15 , 16 , 17 , 17 , 18 , 19 , 20 , 21 , 22 , 22 , 23 , 24 , 25 , 26 , 27 , 28 , 29 , 30 , 31 , 32 , 33 , 34 , 35 , 36 , 37 , 39 , 40 , 41 , 42 , 43 , 44 , 46 , 47 , 48 , 49 , 50 , 52 , 53 , 54 , 56 , 57 , 58 , 60 , 61 , 62 , 64 , 65 , 66 , 68 , 69 , 70 , 72 , 73 , 75 , 76 , 78 , 79 , 80 , 82 , 83 , 85 , 86 , 88 , 89 , 91 , 92 , 94 , 95 , 97 , 98 , 100 , 101 , 103 , 105 , 106 , 108 , 109 , 111 , 112 , 114 , 115 , 117 , 119 , 120 , 122 , 123 , 125 , 126 , 128 , 130 , 131 , 133 , 134 , 136 , 137 , 139 , 141 , 142 , 144 , 145 , 147 , 148 , 150 , 151 , 153 , 155 , 156 , 158 , 159 , 161 , 162 , 164 , 165 , 167 , 168 , 170 , 171 , 173 , 174 , 176 , 177 , 178 , 180 , 181 , 183 , 184 , 186 , 187 , 188 , 190 , 191 , 192 , 194 , 195 , 196 , 198 , 199 , 200 , 202 , 203 , 204 , 206 , 207 , 208 , 209 , 210 , 212 , 213 , 214 , 215 , 216 , 217 , 219 , 220 , 221 , 222 , 223 , 224 , 225 , 226 , 227 , 228 , 229 , 230 , 231 , 232 , 233 , 234 , 234 , 235 , 236 , 237 , 238 , 239 , 239 , 240 , 241 , 242 , 242 , 243 , 244 , 244 , 245 , 246 , 246 , 247 , 247 , 248 , 249 , 249 , 250 , 250 , 250 , 251 , 251 , 252 , 252 , 253 , 253 , 253 , 254 , 254 , 254 , 254 , 255 , 255 , 255 , 255 , 255 , 256 , 256 , 256 , 256 , 256 , 256 , 256 };


// enable or disable serial messaging:
#define DEBUG 0



CapacitiveSensor   cs_1 = CapacitiveSensor(pinS1R,pinS1P);   

void setup()                    
{

 //  cs_1.set_CS,gregief ,    jjj  rvv  b_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   pinMode(pinS1O, OUTPUT);
   
   if(DEBUG){
    Serial.begin(9600);
    Serial.print("Hello! Ver 1xTher_P_m1 here.\n");
   }
   noTone(pinS1O);
   
}

void loop()                    
{
    long start;
    if(DEBUG){
      start = millis();
    }//else{
    //  start aint used.
    //}

    
    int POTENTIOMETER = analogRead(A1) / 4 - 64;

    long xl =  cs_1.capacitiveSensor(20);// runs 3 to 32K
    if(xl < 0 ) {
     // had a timeout, set high plz
     xl = 3333;
    }

    int ANTENNA=2500;
    if(xl<2500){
      ANTENNA=xl;
    }
    int STEPI = (int)(  (float)(ANTENNA)/2500.0L * 256);
    STEPI = STEPI + POTENTIOMETER;
    if(STEPI > 255) STEPI=255;
    if(STEPI < 0) STEPI=0;
    
    int ANALOGOUTVAL;

    ANALOGOUTVAL =    WAVETABLE2[STEPI] ;

    if(ANALOGOUTVAL > 250){
       ANALOGOUTVAL = 250;
    }
       
    //if(ANALOGOUTVAL > 1) {
       analogWrite(pinS1O, ANALOGOUTVAL);
    //}else{
    //   noTone(pinS1O);
   // }


   if(DEBUG){
    Serial.print(POTENTIOMETER);                  // print sensor output 1
    Serial.print("\t");
    Serial.print(xl);             
    Serial.print("\t");
    Serial.print(STEPI);
    Serial.print("\t");
    Serial.print(ANALOGOUTVAL);
    Serial.print("\t");
    Serial.print(millis() - start);        // check on performance in milliseconds
    Serial.print("\n");                    // tab character for debug windown spacing
    delay(20);// arbitrary delay to limit data to serial port 
   }
 
   delay(10);   

}

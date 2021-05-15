
// V5 2021-05-15
// adding a pin-high output to D12 so users have a place to 
//   probe if the software is loaded properly!

// VERSION 0
// KDS 2019-12-07 a Turing Machine / Sequencer 2.
// The Tooring Machine. . ! :)

// these variables are holding 64 or 512 steps of sequence
// but due to knob precision, it's scaled back to 32/256


#include <SPI.h>
#include <DAC_MCP49xx.h>

// The Arduino pin used for the slave select / chip select
// to enable the MCP DAC!
#define SS_PIN 10

// Arduino Nano pinout::
//SPI:
//10 (SS) or Chip Select,
//11 (MOSI),
//12 (MISO), (we don't use this, it would be communication from DAC to Nano)
//13 (SCK).


//Set up the DAC
// we're using this library, you may need to install.
// https://github.com/exscape/electronics/tree/master/Arduino/Libraries/DAC_MCP49xx
//
DAC_MCP49xx dac(DAC_MCP49xx::MCP4922, SS_PIN);

/////////////////////////
// version TM3 on 2021-01-20
// looking to convert the clock/trig statemachine into an interrupt.
// refactored
// V 4 is now trying to change the statemachine into an interrupt machine.
// but that only works on pins 2&3 of the nano
// and now the sequences go from 1-4095

int pinP1 = A0;
int pinP2 = A2;
int pinL1 = A1;
int pinL2 = A3;

// this changed recently:::
int pinC1 = 2;
int pinC2 = 3;

// pin S1 and S2 were changed recently to suit an error in the PCB v1
int pinS1 = 8;
int pinS2 = 7;

int TMEM2[32];
int TMEM1[32];

// the MCP4922 has two channels at 12 bits, so 0-4095
const int SEQMEM1[256] = { 1500 , 1981 , 2228 , 3307 , 1659 , 2171 , 3332 , 1145 , 855 , 2099 , 3830 , 1600 , 2011 , 1707 , 1938 , 2987 , 528 , 680 , 2655 , 2607 , 3323 , 2371 , 1867 , 2413 , 37 , 1411 , 2357 , 1982 , 3296 , 2558 , 770 , 1316 , 3384 , 3363 , 1405 , 1488 , 3025 , 1892 , 630 , 2566 , 2220 , 476 , 1705 , 2203 , 3707 , 1964 , 1868 , 3985 , 716 , 80 , 3010 , 1900 , 1729 , 2476 , 1401 , 2813 , 2352 , 1021 , 1326 , 3176 , 3778 , 775 , 1750 , 3361 , 792 , 669 , 3336 , 1978 , 1119 , 3366 , 2114 , 2443 , 2200 , 875 , 2660 , 3260 , 1742 , 1556 , 824 , 1272 , 3540 , 2393 , 1251 , 3158 , 2400 , 1238 , 737 , 2666 , 3375 , 296 , 2249 , 2339 , 2431 , 3576 , 154 , 1356 , 3013 , 1717 , 3120 , 1664 , 1 , 1437 , 3402 , 3839 , 1455 , 2372 , 2079 , 1357 , 3274 , 2765 , 1380 , 1614 , 2625 , 403 , 1782 , 4094 , 779 , 173 , 2729 , 2092 , 2847 , 2858 , 585 , 2485 , 2490 , 433 , 1001 , 3321 , 2454 , 1724 , 2294 , 3074 , 2916 , 1979 , 572 , 488 , 3613 , 3193 , 1442 , 1849 , 2829 , 1595 , 2176 , 1261 , 1353 , 3260 , 2295 , 2032 , 1970 , 1862 , 1165 , 1122 , 1590 , 1827 , 2435 , 3703 , 3131 , 736 , 1041 , 3294 , 1394 , 1793 , 3967 , 809 , 712 , 3010 , 1873 , 2703 , 2791 , 329 , 2883 , 3826 , 585 , 2048 , 1546 , 547 , 3832 , 3131 , 123 , 1089 , 2311 , 2565 , 3843 , 1160 , 1357 , 3646 , 523 , 349 , 3758 , 3769 , 556 , 1653 , 2080 , 1158 , 2572 , 3176 , 2464 , 1820 , 1994 , 1103 , 2614 , 1400 , 1608 , 2239 , 723 , 2466 , 3803 , 2825 , 2075 , 1573 , 1227 , 1982 , 2399 , 2537 , 1464 , 1143 , 2780 , 3151 , 1626 , 1001 , 968 , 1627 , 2792 , 3790 , 2089 , 2004 , 2993 , 551 , 1237 , 3453 , 1095 , 717 , 3300 , 1976 , 1780 , 3114 , 2599 , 2356 , 914 , 1878 , 3181 , 1226 , 1239 , 3262 , 1783 , 1983 , 2968 , 1133 , 1018 , 1578 , 2459 , 2016 , 1036 , 2001 , 2907 , 2041 };
const int SEQMEM2[256] = { 2077 , 2400 , 2602 , 2350 , 2086 , 1723 , 1956 , 2421 , 2013 , 1524 , 1540 , 2142 , 3372 , 2180 , 1206 , 2669 , 2828 , 465 , 1648 , 2846 , 890 , 3364 , 3369 , 662 , 824 , 2052 , 3814 , 1797 , 1114 , 3773 , 2729 , 42 , 537 , 2172 , 3102 , 2974 , 2061 , 1565 , 1777 , 2723 , 3351 , 3072 , 956 , 1108 , 3033 , 1747 , 339 , 2917 , 3701 , 2286 , 417 , 2225 , 2621 , 652 , 1944 , 2368 , 2585 , 2042 , 2977 , 2567 , 1085 , 1380 , 2871 , 1906 , 1724 , 2373 , 1542 , 3683 , 1973 , 258 , 2288 , 3129 , 3498 , 1717 , 346 , 1892 , 3609 , 2645 , 1575 , 2861 , 735 , 315 , 2303 , 1837 , 3739 , 2525 , 1653 , 1708 , 1473 , 2242 , 2359 , 2911 , 1457 , 1806 , 3554 , 3342 , 1874 , 424 , 2369 , 3040 , 1938 , 2129 , 2073 , 1201 , 1085 , 3589 , 2847 , 1665 , 1456 , 862 , 1538 , 2170 , 2341 , 2787 , 3116 , 1061 , 2250 , 3243 , 2057 , 1246 , 1602 , 2650 , 2873 , 1534 , 934 , 2200 , 2863 , 2695 , 1813 , 2677 , 1649 , 1837 , 2365 , 862 , 1908 , 3606 , 2786 , 1350 , 795 , 1277 , 2298 , 4094 , 2670 , 38 , 1998 , 3267 , 1821 , 1343 , 3337 , 3390 , 395 , 1337 , 2179 , 2613 , 3527 , 1155 , 1301 , 2540 , 2497 , 1271 , 1686 , 3427 , 2847 , 1406 , 214 , 3257 , 4063 , 1577 , 1999 , 788 , 1112 , 4000 , 2568 , 1876 , 1445 , 1373 , 3179 , 2180 , 1885 , 823 , 2096 , 3886 , 2257 , 1092 , 1304 , 3081 , 2733 , 2397 , 1357 , 1 , 2286 , 3087 , 3101 , 2283 , 558 , 2845 , 2626 , 1898 , 1810 , 2037 , 2428 , 789 , 1080 , 1804 , 2868 , 3271 , 2718 , 939 , 1790 , 2831 , 1564 , 1560 , 3130 , 2106 , 1332 , 3124 , 2748 , 2733 , 564 , 1866 , 3100 , 1831 , 2286 , 1248 , 2347 , 3303 , 1857 , 2097 , 806 , 439 , 3754 , 3930 , 1990 , 1687 , 2180 , 606 , 1541 , 2132 , 1248 , 2178 , 2615 , 1824 , 1787 , 2164 , 3076 , 2147 , 827 , 3877 , 3155 , 681 , 2844 , 1745 , 1176 , 3874 , 2460 , 1162 };


int SM1i=0;
int SM2i=0;

volatile bool CLOCKSEEN1;
volatile bool CLOCKSEEN2;

bool DEBUG=0;

// put your setup code here, to run once:
void setup() {

  // inputs for four potentiometers:
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);

  // clocks 
  pinMode(pinC1, INPUT);
  pinMode(pinC2, INPUT);

  //switches
  pinMode(pinS1, INPUT);
  pinMode(pinS2, INPUT);

 if(DEBUG){
  Serial.begin(9600);
 }
   randomSeed(analogRead(A5));

   // FILL the initial TM lists
   for(int i=0;i<32;i++){
     TMEM1[i] = random(1,4094);
     TMEM2[i] = random(1,4094);
   }
   
   // now using a flip flop to decide is trigger has been handled already
   CLOCKSEEN1=false;
   CLOCKSEEN2=false;
   attachInterrupt(digitalPinToInterrupt(pinC1), ISR1, RISING);
   attachInterrupt(digitalPinToInterrupt(pinC2), ISR2, RISING);

  // leave a pin held high so user an probe for the software's running
   pinMode(12, OUTPUT);
   digitalWrite(12,HIGH);
  // also initialize triggers:
   CLOCKSEEN1=true;
   CLOCKSEEN2=true; // pretend it's fired once on startup.
    
}// done setup()

void ISR1() {
  CLOCKSEEN1 = true;
}
void ISR2() {
  CLOCKSEEN2 = true;
}


void loop() {

    if(CLOCKSEEN1){
      ProcessSide1();
      CLOCKSEEN1=false;
    }
    if(CLOCKSEEN2){
      ProcessSide2();
      CLOCKSEEN2=false;
    }
    
   delay(1);  // sleep might be pointless.....
}

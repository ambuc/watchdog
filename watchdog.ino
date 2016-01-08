#include <Servo.h>      //https://github.com/arduino/Arduino/blob/master/libraries/Servo/
#include "TimeAlarms.h" //http://www.pjrc.com/teensy/td_libs_TimeAlarms.html
#include "LedControl.h" //https://github.com/wayoda/LedControl
#include "TMRpcm.h"     //https://github.com/TMRh20/TMRpcm
#include "PCMConfig.h"  //--^

//DEFINE CONSTANTS
const int plus      = 1; // plus button
const int alarm     = 2; // alarm button
const int snooze    = 3; // snooze button
const int speaker   = 9; //speaker pin

//DEFINE VARIABLES
int pos = 0;  // variable to store the servo position
int prevmode; // the previous mode (global)
bool isAlarm = false;

//DEFINE OBJECTS
LedControl lc = LedControl(4,5,6,1); // LedControl(DIN,CLK,LOAD,...)
AlarmID_t your;   //create alarm object
Servo leftServo;  //create servo object
Servo rightServo;

//runs once, before loop
void setup() {  
  //initialize up pins and servos
  pinMode(plus,    INPUT);
  pinMode(alarm,   INPUT);
  pinMode(snooze,  INPUT);
  pinMode(speaker, INPUT);
   leftServo.attach(10);
  rightServo.attach(11); 

  //initialize the MAX7219 device
  lc.shutdown(0,false); //turn on display
  lc.setIntensity(0,5); //brightness 5
  lc.clearDisplay(0);

  //play the text intro three times
  animate_dogclock(3); 
  
  // set time to something (just a few seconds before noon)
  setTime(11,59,55,1,1,11);

  // create the alarm (for noon) which triggers function yourAlarm
  your = Alarm.alarmRepeat(12,00,00,yourAlarm);
}


//this is the loop, which is performed as fast as possible, constantly.
void loop(){

  //before mode update
  time_t t = now();   //update the time object t
  Alarm.delay(100);   //Alarm-friendly loop delay
  lc.clearDisplay(0); //clear the display each time

  //mode update
  int mode = get_mode(prevmode); //mode persist

  //after mode update
  check_inputs(mode, prevmode); //check for button input
  print(mode, prevmode); //print whatever (either text or the number)
  prevmode = mode; //assign new mode, if there is

  //is the alarm going off?
  if (isAlarm){
    //if so, move and wait
    leftServo.write(180); 
    rightServo.write(-90); 
    delay(10);
  } else {
    //if not, keep still
    leftServo.write(95);
    rightServo.write(93);
  }
}

//checks button input to see if we've just switched mode
int get_mode(int prevmode){
  //if we have two inputs, we're not swapping modes; no state change
  if(digitalRead(snooze)==HIGH && digitalRead(alarm )==HIGH){ return prevmode; }
  if(digitalRead(plus  )==HIGH && digitalRead(alarm )==HIGH){ return prevmode; }
  if(digitalRead(plus  )==HIGH && digitalRead(snooze)==HIGH){ return prevmode; }

  //there's only one input. what is it?
  if (digitalRead(snooze)==HIGH){
    return 2; //timeset
  } else if (digitalRead(alarm)==HIGH) {
    return 1; //alarmset
  } else {
    return 0; //normal
  }

}

//trigger boolean
void yourAlarm(){
  isAlarm = true;
}

//checks for input from buttons
void check_inputs(int mode, int prevmode){

  //if we're snoozing, turn off the alarm
  if(isAlarm && digitalRead(snooze)==HIGH){
    isAlarm = false;
    return;
  }

  //if we've just switched, don't do anything
  if(mode != prevmode){ return; }

  //if we were just at rest, we're still at rest. new input swaps mode, not triggers buttons
  if(prevmode == 0){ return; }

  //if we were just about to SET THE ALARM, then the other two buttons ought to trigger H+ and M+
  if(prevmode == 1){
    if(digitalRead(snooze)==HIGH){
      //set alarm H+
      your = Alarm.alarmRepeat(
          hour(Alarm.read(your))+1, 
        minute(Alarm.read(your)), 
        second(Alarm.read(your)),
        yourAlarm
      );  
    } else if(digitalRead(plus)==HIGH){
      //set alarm M+
      your = Alarm.alarmRepeat(
          hour(Alarm.read(your)), 
        minute(Alarm.read(your))+1, 
        second(Alarm.read(your)),
        yourAlarm
      ); 
    }
  } 
  //if we were just about to SET THE TIME, then the other two buttons ought to trigger H+ and M+
  if(prevmode == 2){
    if(digitalRead(alarm)==HIGH){
      // set time H+
      setTime(hour()+1,minute(),second(),day(),month(),year());
    } else if(digitalRead(plus)==HIGH){
      //set time M+
      setTime(hour(),minute()+1,second(),day(),month(),year());          
    }
  }
}

//mode controls what's going on.
// zero (0) is default  --> print the time 
// one  (1) is alarmset --> print the alarm
// two  (2) is timeset  --> print the time
void print(int mode, int prevmode){
  if(prevmode != mode){ //if we've just made changes,
    dispMode(mode); // print text instead
  } else {
    if(mode==1){
      putAlarm(); 
    } else {
      putTime();
    }      
  }
}

//based on the mode, prints text (cL0c, aLa, etc)
void dispMode(int mode){
  if(mode==0){
    feedFour('c', 'L', '0', 'c'); 
    Alarm.delay(100);
  } else if (mode==1) {
    feedFour('a',  'L' ,  'a' , ' '); 
    Alarm.delay(100);
  } else if (mode==2) {
    animate_timeset();
  }
}

//prints the time based on global time bject
void putTime(){
  int h = hourFormat12();
  int m = minute();
  int s = second();
  if (s%2 == 0){
    lc.setDigit(0,4,2,true); // just center dots ?
  }
  feedFour(h/10%10, h%10, m/10%10, m%10);
}

//prints the time of the upcoming alarm
void putAlarm(){
  int h = hourFormat12(Alarm.read(your));
  int m = minute(Alarm.read(your));
  int s = second(Alarm.read(your));
  if (s%2 == 0){
    lc.setDigit(0,4,2,true); // just center dots ?
  }
  feedFour(h/10%10, h%10, m/10%10, m%10);
}

//prints four characters to the lc object
void feedFour(byte a, byte b, byte c, byte d){
  lc.setChar(0, 3, a, false);
  lc.setChar(0, 2, b, false);
  lc.setChar(0, 1, c, false);
  lc.setChar(0, 0, d, false);
}

//cute little loading animation
void animate_timeset(){
  feedFour('-',  '_' , '_', '_'); Alarm.delay(30);
  feedFour(' ',  '-' , '_', '_'); Alarm.delay(30);
  feedFour('_',  ' ' , '-', '_'); Alarm.delay(30);
  feedFour('_',  '_' , ' ', '-'); Alarm.delay(30);
  feedFour('_',  '_' , '-', ' '); Alarm.delay(30);
  feedFour('_',  '-' , ' ', '_'); Alarm.delay(30);
  feedFour('-',  ' ' , '_', '_'); Alarm.delay(30);
}

//intro animation (recursive because why not?)
void animate_dogclock(int n){
  if(n==0){return;}
  feedFour('d',  0 ,  9 , ' '); Alarm.delay(250);
  feedFour('c', 'L', '0', 'c'); Alarm.delay(250);
  animate_dogclock(n-1);
}
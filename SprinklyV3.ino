// include libraries:
#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal.h>

//Initialize RTC shit
DS3231 myRTC;
bool century = false;
bool h12Flag;
bool pmFlag;
byte alarmDay, alarmHour, alarmMinute, alarmSecond, alarmBits;
bool alarmDy, alarmH12Flag, alarmPmFlag;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const byte rs = 13, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Setup of pins
const int clockPin = A2;  // Pin CL on Relay board
const int latchPin = A1;  // Pin LT on Relay board
const int dataPin = A0;   // Pin DS on Relay board
const byte powerPin = 3;
const byte UpPin = 4;
const byte DownPin = 7;
const byte AcceptPin = 6;
const byte AbortPin = 5;

byte data = 0;  //0-255, value for relays
int menu = 0; //value for which menu is shown
byte NewStart = 0; //Flag used once for fresh starting.

//LCD settings
int LCD = 255; // Used together with menu to decide if to redraw screen or not.

//Debounce settings
const unsigned long DebounceDelay = 50;
bool ButtonUpState = HIGH;
bool LastButtonUpState = HIGH;
unsigned long LastButtonUpDebounceTime = 0;
bool ButtonDownState = HIGH;
bool LastButtonDownState = HIGH;
unsigned long LastButtonDownDebounceTime = 0;
bool ButtonAcceptState = HIGH;
bool LastButtonAcceptState = HIGH;
unsigned long LastButtonAcceptDebounceTime = 0;
bool ButtonAbortState = HIGH;
bool LastButtonAbortState = HIGH;
unsigned long LastButtonAbortDebounceTime = 0;

//Variables related to runtimes
unsigned long Sprinkler1StartTime = 0;
unsigned long Sprinkler2StartTime = 0;
unsigned long Sprinkler3StartTime = 0;
unsigned long Sprinkler4StartTime = 0;
unsigned long Sprinkler5StartTime = 0;
unsigned long Sprinkler6StartTime = 0;
unsigned long Sprinkler7StartTime = 0;
unsigned long Sprinkler8StartTime = 0;
unsigned long Sprinkler1TimeRemaining = 0;
unsigned long Sprinkler2TimeRemaining = 0;
unsigned long Sprinkler3TimeRemaining = 0;
unsigned long Sprinkler4TimeRemaining = 0;
unsigned long Sprinkler5TimeRemaining = 0;
unsigned long Sprinkler6TimeRemaining = 0;
unsigned long Sprinkler7TimeRemaining = 0;
unsigned long Sprinkler8TimeRemaining = 0;
unsigned long Sprinkler1Finish = 0;
unsigned long Sprinkler2Finish = 0;
unsigned long Sprinkler3Finish = 0;
unsigned long Sprinkler4Finish = 0;
unsigned long Sprinkler5Finish = 0;
unsigned long Sprinkler6Finish = 0;
unsigned long Sprinkler7Finish = 0;
unsigned long Sprinkler8Finish = 0;
int Sprinkler1State = 1;
int Sprinkler2State = 1;
int Sprinkler3State = 1;
int Sprinkler4State = 1;
int Sprinkler5State = 1;
int Sprinkler6State = 1;
int Sprinkler7State = 1;
int Sprinkler8State = 1;
int SprinklerStates[] = {Sprinkler1State, Sprinkler2State, Sprinkler3State, Sprinkler4State, Sprinkler5State, Sprinkler6State, Sprinkler7State, Sprinkler8State};
int Sprinkler1Time = 4; // Watering time in minutes
int Sprinkler2Time = 4;
int Sprinkler3Time = 4;
int Sprinkler4Time = 4;
int Sprinkler5Time = 4;
int Sprinkler6Time = 4;
int Sprinkler7Time = 4;
int Sprinkler8Time = 4;
bool ForcedWatering = 0;
bool LongWatering = 1;

unsigned long MenuTime = 0;
bool LongDay = 0; // Flag to check if it's a long watering day or not.
bool HasRun = 0; // Flag to check if program has been run.

void setup() {
  Wire.begin();
  Serial.begin(9600);
  lcd.begin(20, 4);
  pinMode(UpPin, INPUT_PULLUP);
  pinMode(DownPin, INPUT_PULLUP);
  pinMode(AcceptPin, INPUT_PULLUP);
  pinMode(AbortPin, INPUT_PULLUP);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin,  OUTPUT);
  pinMode(clockPin, OUTPUT);
  data = 0;
  digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
  shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
  digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
  pinMode(3, OUTPUT);  //strömsätter reläna m. 24v
  digitalWrite(3, LOW);
  lcd.setCursor(0, 0);
  lcd.print("Watering system v0.2");
  lcd.setCursor(0, 1);
  lcd.print("2024-05-27");
  delay(1500);
  lcd.clear();
}


void loop() {

  if(NewStart == 0){
    data = 0;
    digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
    shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
    digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
    lcd.print("Relays set.");
    delay(1500);
    lcd.clear();
    NewStart = 1;
  }


  //Various timevariables
  byte Weekday = myRTC.getDoW();
  if(Weekday == 0 || Weekday == 3 || Weekday == 5){
    LongDay = 1;
  }else{
    LongDay = 0;
  }
  byte Hour = myRTC.getHour(h12Flag, pmFlag);
  byte Minute = myRTC.getMinute();

  if(Hour == 6 && HasRun == 1){ //Reset function after sprinklers should've finished.
    for (int i = 0; i < 9; i++) {
      if(SprinklerStates[i] == -1){
        SprinklerStates[i] = 1;
      }
    }
  HasRun = 0;
  }


  // Reading buttons
  int ButtonUpRead = digitalRead(UpPin);
  int ButtonDownRead = digitalRead(DownPin);
  int ButtonAcceptRead = digitalRead(AcceptPin);
  int ButtonAbortRead = digitalRead(AbortPin);

  //Look for statechange button UP
  if (ButtonUpRead != LastButtonUpState) {
    LastButtonUpDebounceTime = millis();
  }
  //Check how long state has been changed(Debounce)
  if ((millis() - LastButtonUpDebounceTime) > DebounceDelay) {
    if (ButtonUpRead != ButtonUpState) {
      ButtonUpState = ButtonUpRead;  //Statechange accepted as a button press.
      if (ButtonUpState == LOW && menu < 16) {
        menu = menu + 1;
      }
    }
  }
  LastButtonUpState = ButtonUpRead; // Resave buttonstate

  //Look for statechange button DOWN
  if (ButtonDownRead != LastButtonDownState) {
    LastButtonDownDebounceTime = millis();
  }
  //Check how long state has been changed(Debounce)
  if ((millis() - LastButtonDownDebounceTime) > DebounceDelay) {
    if (ButtonDownRead != ButtonDownState) {
      ButtonDownState = ButtonDownRead;  //Statechange accepted as a button press.
      if (ButtonDownState == LOW && menu > -2) {
        menu = menu - 1;
      }
    }
  }
  LastButtonDownState = ButtonDownRead; // Resave buttonstate

  //Look for statechange button ACCEPT
  if (ButtonAcceptRead != LastButtonAcceptState) {
    LastButtonAcceptDebounceTime = millis();
  }
  //Check how long state has been changed
  if ((millis() - LastButtonAcceptDebounceTime) > DebounceDelay) {
    if (ButtonAcceptRead != ButtonAcceptState) {
      ButtonAcceptState = ButtonAcceptRead;  //Statechange accepted as a button press.
      // How to act in menu for Long watering
      if (ButtonAcceptState == LOW && menu == -2) {
        LongWatering = 1; // Turns longwatering on.
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Forced watering
      if (ButtonAcceptState == LOW && menu == -1) {
        ForcedWatering = 1; // Starts forced watering cycle
        LCD = 0; //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 1
      if (ButtonAcceptState == LOW && menu == 1) {
        SprinklerStates[0] = 1; // Turns that section on for both forced and long watering.
        LCD = 0; //Forces LCD to refresh
      }
      if (ButtonAcceptState == LOW && menu == 2 && Sprinkler1Time < 60) {
        Sprinkler1Time = Sprinkler1Time + 5; // Adds 5 minutes to that sections runtime.
        LCD = 0; //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 2
      if (ButtonAcceptState == LOW && menu == 3) {
        SprinklerStates[1] = 1; // Turns that section on for both forced and long watering.
        LCD = 0; //Forces LCD to refresh
      }
      if (ButtonAcceptState == LOW && menu == 4 && Sprinkler2Time < 60) {
        Sprinkler2Time = Sprinkler2Time + 5; // Adds 5 minutes to that sections runtime.
        LCD = 0; //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 3
      if (ButtonAcceptState == LOW && menu == 5) {
        SprinklerStates[2] = 1; // Turns that section on for both forced and long watering.
        LCD = 0; //Forces LCD to refresh
      }
      if (ButtonAcceptState == LOW && menu == 6 && Sprinkler3Time < 60) {
        Sprinkler3Time = Sprinkler3Time + 5; // Adds 5 minutes to that sections runtime.
        LCD = 0; //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 4
      if (ButtonAcceptState == LOW && menu == 7) {
        SprinklerStates[3] = 1; // Turns that section on for both forced and long watering.
        LCD = 0; //Forces LCD to refresh
      }
      if (ButtonAcceptState == LOW && menu == 8 && Sprinkler4Time < 60) {
        Sprinkler4Time = Sprinkler4Time + 5; // Adds 5 minutes to that sections runtime.
        LCD = 0; //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 5
      if (ButtonAcceptState == LOW && menu == 9) {
        SprinklerStates[4] = 1; // Turns that section on for both forced and long watering.
        LCD = 0; //Forces LCD to refresh
      }
      if (ButtonAcceptState == LOW && menu == 10 && Sprinkler5Time < 60) {
        Sprinkler5Time = Sprinkler5Time + 5; // Adds 5 minutes to that sections runtime.
        LCD = 0; //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 6
      if (ButtonAcceptState == LOW && menu == 11) {
        SprinklerStates[5] = 1; // Turns that section on for both forced and long watering.
        LCD = 0; //Forces LCD to refresh
      }
      if (ButtonAcceptState == LOW && menu == 12 && Sprinkler6Time < 60) {
        Sprinkler6Time = Sprinkler6Time + 5; // Adds 5 minutes to that sections runtime.
        LCD = 0; //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 7
      if (ButtonAcceptState == LOW && menu == 13) {
        SprinklerStates[6] = 1; // Turns that section on for both forced and long watering.
        LCD = 0; //Forces LCD to refresh
      }
      if (ButtonAcceptState == LOW && menu == 14 && Sprinkler7Time < 60) {
        Sprinkler7Time = Sprinkler7Time + 5; // Adds 5 minutes to that sections runtime.
        LCD = 0; //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 8
      if (ButtonAcceptState == LOW && menu == 15) {
        SprinklerStates[7] = 1; // Turns that section on for both forced and long watering.
        LCD = 0; //Forces LCD to refresh
      }
      if (ButtonAcceptState == LOW && menu == 16 && Sprinkler8Time < 60) {
        Sprinkler8Time = Sprinkler8Time + 5; // Adds 5 minutes to that sections runtime.
        LCD = 0; //Forces LCD to refresh
      }
    }
  }
  LastButtonAcceptState = ButtonAcceptRead;

  //Look for statechange button ABORT
  if (ButtonAbortRead != LastButtonAbortState) {
    LastButtonAbortDebounceTime = millis();
  }
  //Check how long state has been changed
  if ((millis() - LastButtonAbortDebounceTime) > DebounceDelay) {
    if (ButtonAbortRead != ButtonAbortState) {
      ButtonAbortState = ButtonAbortRead;  //Statechange accepted as a button press. Following is how to act.
      // How to act in menu for Long watering
      if (ButtonAbortState == LOW && menu == -2) {
        LongWatering = 0; // Turns long watering cycle off.
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Forced watering
      if (ButtonAbortState == LOW && menu == -1) {
        ForcedWatering = 0; // Turns forced watering off(kinda useless since it runs and resets instantly)
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 1
      if (ButtonAbortState == LOW && menu == 1) {
        SprinklerStates[0] = 0; // Turns that section off for both forced and long watering.
        LCD = 0;  //Forces LCD to refresh
      }
      if (ButtonAbortState == LOW && menu == 2 && Sprinkler1Time > 5) {
        Sprinkler1Time = Sprinkler1Time - 5; // Removes 5 minutes to that sections runtime.
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 2
      if (ButtonAbortState == LOW && menu == 3) {
        SprinklerStates[1] = 0; // Turns that section off for both forced and long watering.
        LCD = 0;  //Forces LCD to refresh
      }
      if (ButtonAbortState == LOW && menu == 4 && Sprinkler2Time > 5) {
        Sprinkler2Time = Sprinkler2Time - 5; // Removes 5 minutes to that sections runtime.
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 3
      if (ButtonAbortState == LOW && menu == 5) {
        SprinklerStates[2] = 0; // Turns that section off for both forced and long watering.
        LCD = 0;  //Forces LCD to refresh
      }
      if (ButtonAbortState == LOW && menu == 6 && Sprinkler3Time > 5) {
        Sprinkler3Time = Sprinkler3Time - 5; // Removes 5 minutes to that sections runtime.
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 4
      if (ButtonAbortState == LOW && menu == 7) {
        SprinklerStates[3] = 0; // Turns that section off for both forced and long watering.
        LCD = 0;  //Forces LCD to refresh
      }
      if (ButtonAbortState == LOW && menu == 8 && Sprinkler4Time > 5) {
        Sprinkler4Time = Sprinkler4Time - 5; // Removes 5 minutes to that sections runtime.
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 5
      if (ButtonAbortState == LOW && menu == 9) {
        SprinklerStates[4] = 0; // Turns that section off for both forced and long watering.
        LCD = 0;  //Forces LCD to refresh
      }
      if (ButtonAbortState == LOW && menu == 10 && Sprinkler5Time > 5) {
        Sprinkler5Time = Sprinkler5Time - 5; // Removes 5 minutes to that sections runtime.
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 6
      if (ButtonAbortState == LOW && menu == 11) {
        SprinklerStates[5] = 0; // Turns that section off for both forced and long watering.
        LCD = 0;  //Forces LCD to refresh
      }
      if (ButtonAbortState == LOW && menu == 12 && Sprinkler6Time > 5) {
        Sprinkler6Time = Sprinkler6Time - 5; // Removes 5 minutes to that sections runtime.
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 7
      if (ButtonAbortState == LOW && menu == 13) {
        SprinklerStates[6] = 0; // Turns that section off for both forced and long watering.
        LCD = 0;  //Forces LCD to refresh
      }
      if (ButtonAbortState == LOW && menu == 14 && Sprinkler7Time > 5) {
        Sprinkler7Time = Sprinkler7Time - 5; // Removes 5 minutes to that sections runtime.
        LCD = 0;  //Forces LCD to refresh
      }
      // How to act in menu for Sprinkler 8
      if (ButtonAbortState == LOW && menu == 15) {
        SprinklerStates[7] = 0; // Turns that section off for both forced and long watering.
        LCD = 0;  //Forces LCD to refresh
      }
      if (ButtonAbortState == LOW && menu == 16 && Sprinkler8Time > 5) {
        Sprinkler8Time = Sprinkler8Time - 5; // Removes 5 minutes to that sections runtime.
        LCD = 0;  //Forces LCD to refresh
      }
    }
  }
  LastButtonAbortState = ButtonAbortRead; // Resaves buttonstate

  // Base menus, no button functionality, just drawing up status/cursors etc.
  if (menu == -2 && LCD != -2) { // Compares flags to decide if it should redraw or not.
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Long watering     -2");
    lcd.setCursor(0, 1);
    lcd.print("On/Off: ");
    lcd.print(LongWatering);
    lcd.setCursor(0,2);
    for (int i = 0; i < 8; i++) {
      lcd.print(SprinklerStates[i]);
    }
    LCD = -2;
  }
  if (menu == -1 && LCD != -1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Forced watering    -1");
    lcd.setCursor(0, 1);
    lcd.print("On/Off: ");
    lcd.print(ForcedWatering);
    LCD = -1;
  }
  if (menu == 0 && LCD != 0) {
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Forced: ");
    lcd.print(ForcedWatering);
    lcd.print(" Long: ");
    lcd.print(LongWatering);
    lcd.setCursor(0, 3);
    lcd.print("Active: ");
    for (int i = 0; i < 8; i++) { // Draws up active sections on main page
      if (SprinklerStates[i] == 1) {
        lcd.print(i + 1);
      } else {
        lcd.print(" ");
      }
    }
    LCD = 0;  // Flag to not update LCD for no reason.
    MenuTime = millis(); // Save time when menu was last drawn up.
  }

  if ((millis() - MenuTime) > 1000 && menu == 0) { // Function to update LCD every second for clock visuals.
    lcd.setCursor(0, 0);
    byte ClockDays = myRTC.getDate();
    if(ClockDays < 10){  // IF to compensate for rtc lib showing 1-9 instead of 01-09...
      lcd.print(0);
      lcd.print(myRTC.getDate(), DEC);  
    }else{
      lcd.print(myRTC.getDate(), DEC);
    }
    lcd.print("-");
    lcd.print(myRTC.getMonth(century));
    lcd.print("-20");
    lcd.print(myRTC.getYear());
    lcd.print("  ");

    byte ClockHours = myRTC.getHour(h12Flag, pmFlag);
    if(ClockHours < 10){  // IF to compensate for rtc lib showing 1-9 instead of 01-09...
      lcd.print(0);
      lcd.print(myRTC.getHour(h12Flag, pmFlag), DEC);  
    }else{
      lcd.print(myRTC.getHour(h12Flag, pmFlag), DEC);
    }
    lcd.print(":");
    
    byte ClockMinutes = myRTC.getMinute();
    if(ClockHours < 10){  // IF to compensate for rtc lib showing 1-9 instead of 01-09...
      lcd.print(0);
      lcd.print(myRTC.getMinute(), DEC);  
    }else{
      lcd.print(myRTC.getMinute(), DEC);
    }

    lcd.print(":");
    byte ClockSeconds = myRTC.getSecond();
    if(ClockSeconds < 10){  // IF to compensate for rtc lib showing 1-9 instead of 01-09...
      lcd.print(0);
      lcd.print(myRTC.getSecond(), DEC);  
    }else{
      lcd.print(myRTC.getSecond(), DEC);
    }
  }

  if (menu == 1 && LCD != 1) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 1     1/8");
    lcd.setCursor(0, 1);
    lcd.print(">On/Off: ");
    lcd.print(SprinklerStates[0]);
    lcd.setCursor(0, 2);
    lcd.print(" Time: ");
    lcd.print(Sprinkler1Time);
    LCD = 1;
  }
  if (menu == 2 && LCD != 2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 1     1/8");
    lcd.setCursor(0, 1);
    lcd.print(" On/Off: ");
    lcd.print(SprinklerStates[0]);
    lcd.setCursor(0, 2);
    lcd.print(">Time: ");
    lcd.print(Sprinkler1Time);
    LCD = 2;
  }

  if (menu == 3 && LCD != 3) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 2     2/8");
    lcd.setCursor(0, 1);
    lcd.print(">On/Off: ");
    lcd.print(SprinklerStates[1]);
    lcd.setCursor(0, 2);
    lcd.print(" Time: ");
    lcd.print(Sprinkler2Time);
    LCD = 3;
  }
  if (menu == 4 && LCD != 4) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 2     2/8");
    lcd.setCursor(0, 1);
    lcd.print(" On/Off: ");
    lcd.print(SprinklerStates[1]);
    lcd.setCursor(0, 2);
    lcd.print(">Time: ");
    lcd.print(Sprinkler2Time);
    LCD = 4;
  }
  if (menu == 5 && LCD != 5) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 3     3/8");
    lcd.setCursor(0, 1);
    lcd.print(">On/Off: ");
    lcd.print(SprinklerStates[2]);
    lcd.setCursor(0, 2);
    lcd.print(" Time: ");
    lcd.print(Sprinkler3Time);
    LCD = 5;
  }
  if (menu == 6 && LCD != 6) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 3     3/8");
    lcd.setCursor(0, 1);
    lcd.print(" On/Off: ");
    lcd.print(SprinklerStates[2]);
    lcd.setCursor(0, 2);
    lcd.print(">Time: ");
    lcd.print(Sprinkler3Time);
    LCD = 6;
  }
  if (menu == 7 && LCD != 7) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 4     4/8");
    lcd.setCursor(0, 1);
    lcd.print(">On/Off: ");
    lcd.print(SprinklerStates[3]);
    lcd.setCursor(0, 2);
    lcd.print(" Time: ");
    lcd.print(Sprinkler4Time);
    LCD = 7;
  }
  if (menu == 8 && LCD != 8) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 4     4/8");
    lcd.setCursor(0, 1);
    lcd.print(" On/Off: ");
    lcd.print(SprinklerStates[3]);
    lcd.setCursor(0, 2);
    lcd.print(">Time: ");
    lcd.print(Sprinkler4Time);
    LCD = 8;
  }
  if (menu == 9 && LCD != 9) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 5     5/8");
    lcd.setCursor(0, 1);
    lcd.print(">On/Off: ");
    lcd.print(SprinklerStates[4]);
    lcd.setCursor(0, 2);
    lcd.print(" Time: ");
    lcd.print(Sprinkler5Time);
    LCD = 9;
  }
  if (menu == 10 && LCD != 10) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 5     5/8");
    lcd.setCursor(0, 1);
    lcd.print(" On/Off: ");
    lcd.print(SprinklerStates[4]);
    lcd.setCursor(0, 2);
    lcd.print(">Time: ");
    lcd.print(Sprinkler5Time);
    LCD = 10;
  }
  if (menu == 11 && LCD != 11) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 6     6/8");
    lcd.setCursor(0, 1);
    lcd.print(">On/Off: ");
    lcd.print(SprinklerStates[5]);
    lcd.setCursor(0, 2);
    lcd.print(" Time: ");
    lcd.print(Sprinkler6Time);
    LCD = 11;
  }
  if (menu == 12 && LCD != 12) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 6     6/8");
    lcd.setCursor(0, 1);
    lcd.print(" On/Off: ");
    lcd.print(SprinklerStates[5]);
    lcd.setCursor(0, 2);
    lcd.print(">Time: ");
    lcd.print(Sprinkler6Time);
    LCD = 12;
  }
  if (menu == 13 && LCD != 13) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 7     7/8");
    lcd.setCursor(0, 1);
    lcd.print(">On/Off: ");
    lcd.print(SprinklerStates[6]);
    lcd.setCursor(0, 2);
    lcd.print(" Time: ");
    lcd.print(Sprinkler7Time);
    LCD = 13;
  }
  if (menu == 14 && LCD != 14) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 7     7/8");
    lcd.setCursor(0, 1);
    lcd.print(" On/Off: ");
    lcd.print(SprinklerStates[6]);
    lcd.setCursor(0, 2);
    lcd.print(">Time: ");
    lcd.print(Sprinkler7Time);
    LCD = 14;
  }
  if (menu == 15 && LCD != 15) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 8     8/8");
    lcd.setCursor(0, 1);
    lcd.print(">On/Off: ");
    lcd.print(SprinklerStates[7]);
    lcd.setCursor(0, 2);
    lcd.print(" Time: ");
    lcd.print(Sprinkler8Time);
    LCD = 15;
  }
  if (menu == 16 && LCD != 16) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" Sprinkler 8     8/8");
    lcd.setCursor(0, 1);
    lcd.print(" On/Off: ");
    lcd.print(SprinklerStates[7]);
    lcd.setCursor(0, 2);
    lcd.print(">Time: ");
    lcd.print(Sprinkler8Time);
    LCD = 16;
  }


  // LONG WATERING SECTION
  if(Hour > 18 || Hour < 6){ // Checks what time it is, don't water when there's strong sun.
    if(LongWatering == 1 && LongDay == 1){ // Checks if long watering is on and if it's the correct day

      //Sprinkler 1
      if (Sprinkler1State == 1 && data == 0) {  //Startar bevattningen
        LCD = 255; // Forces LCD refresh.
        digitalWrite(powerPin, HIGH);
        data = 1;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        Sprinkler1StartTime = millis() / 1000;
        Sprinkler1Finish = Sprinkler1StartTime + (Sprinkler1Time * 60);
        lcd.clear();
      }
      while (Sprinkler1State == 1 && data == 1) {  //Shows remaining time and reads abort button
        bool abort = digitalRead(AbortPin);
        if (abort == 0) {
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler1State = -1;
          lcd.clear();
          delay(500);
        }
        lcd.setCursor(0, 0);
        lcd.print("Section 1 started");
        lcd.setCursor(0, 1);
        lcd.print("Seconds remaining: ");
        lcd.setCursor(0, 2);
        lcd.print(Sprinkler1TimeRemaining);
        lcd.setCursor(0, 3);
        lcd.print("Press decr to abort");
        Sprinkler1TimeRemaining = Sprinkler1Finish - (millis()/1000);
        if (Sprinkler1TimeRemaining < 1) {  // In the while loop, watches remaining time and turns off section when done.
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler1State = -1;                         //-1 as to not get caught again until full reset.
        }
      }

      //Sprinkler 2
      if (Sprinkler2State == 1 && data == 0) {  //Startar bevattningen
        LCD = 255; // Forces LCD refresh.
        digitalWrite(powerPin, HIGH);
        data = 2;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        Sprinkler2StartTime = millis() / 1000;
        Sprinkler2Finish = Sprinkler2StartTime + (Sprinkler2Time * 60);
        lcd.clear();
      }
      while (Sprinkler2State == 1 && data == 2) {  //Shows remaining time and reads abort button
        bool abort = digitalRead(AbortPin);
        if (abort == 0) {
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler2State = -1;
          lcd.clear();
          delay(500);
        }
        lcd.setCursor(0, 0);
        lcd.print("Section 2 started");
        lcd.setCursor(0, 1);
        lcd.print("Seconds remaining: ");
        lcd.setCursor(0, 2);
        lcd.print(Sprinkler2TimeRemaining);
        lcd.setCursor(0, 3);
        lcd.print("Press decr to abort");
        Sprinkler2TimeRemaining = Sprinkler2Finish - (millis()/1000);
        if (Sprinkler2TimeRemaining < 1) {  // In the while loop, watches remaining time and turns off section when done.
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler2State = -1;                         //-1 as to not get caught again until full reset.
        }
      }

      //Sprinkler 3
      if (Sprinkler3State == 1 && data == 0) {  //Startar bevattningen
        LCD = 255; // Forces LCD refresh.
        digitalWrite(powerPin, HIGH);
        data = 4;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        Sprinkler3StartTime = millis() / 1000;
        Sprinkler3Finish = Sprinkler3StartTime + (Sprinkler3Time * 60);
        lcd.clear();
      }
      while (Sprinkler3State == 1 && data == 4) {  //Shows remaining time and reads abort button
        bool abort = digitalRead(AbortPin);
        if (abort == 0) {
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler3State = -1;
          lcd.clear();
          delay(500);
        }
        lcd.setCursor(0, 0);
        lcd.print("Section 3 started");
        lcd.setCursor(0, 1);
        lcd.print("Seconds remaining: ");
        lcd.setCursor(0, 2);
        lcd.print(Sprinkler3TimeRemaining);
        lcd.setCursor(0, 3);
        lcd.print("Press decr to abort");
        Sprinkler3TimeRemaining = Sprinkler3Finish - (millis()/1000);
        if (Sprinkler3TimeRemaining < 1) {  // In the while loop, watches remaining time and turns off section when done.
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler3State = -1;                         //-1 as to not get caught again until full reset.
        }
      }

      //Sprinkler 4
      if (Sprinkler4State == 1 && data == 0) {  //Startar bevattningen
        LCD = 255; // Forces LCD refresh.
        digitalWrite(powerPin, HIGH);
        data = 8;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        Sprinkler4StartTime = millis() / 1000;
        Sprinkler4Finish = Sprinkler4StartTime + (Sprinkler4Time * 60);
        lcd.clear();
      }
      while (Sprinkler4State == 1 && data == 8) {  //Shows remaining time and reads abort button
        bool abort = digitalRead(AbortPin);
        if (abort == 0) {
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler4State = -1;
          lcd.clear();
          delay(500);
        }
        lcd.setCursor(0, 0);
        lcd.print("Section 4 started");
        lcd.setCursor(0, 1);
        lcd.print("Seconds remaining: ");
        lcd.setCursor(0, 2);
        lcd.print(Sprinkler4TimeRemaining);
        lcd.setCursor(0, 3);
        lcd.print("Press decr to abort");
        Sprinkler4TimeRemaining = Sprinkler4Finish - (millis()/1000);
        if (Sprinkler4TimeRemaining < 1) {  // In the while loop, watches remaining time and turns off section when done.
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler4State = -1;                         //-1 as to not get caught again until full reset.
        }
      }

      //Sprinkler 5
      if (Sprinkler5State == 1 && data == 0) {  //Startar bevattningen
        LCD = 255; // Forces LCD refresh.
        digitalWrite(powerPin, HIGH);
        data = 16;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        Sprinkler5StartTime = millis() / 1000;
        Sprinkler5Finish = Sprinkler5StartTime + (Sprinkler5Time * 60);
        lcd.clear();
      }
      while (Sprinkler5State == 1 && data == 16) {  //Shows remaining time and reads abort button
        bool abort = digitalRead(AbortPin);
        if (abort == 0) {
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler5State = -1;
          lcd.clear();
          delay(500);
        }
        lcd.setCursor(0, 0);
        lcd.print("Section 5 started");
        lcd.setCursor(0, 1);
        lcd.print("Seconds remaining: ");
        lcd.setCursor(0, 2);
        lcd.print(Sprinkler5TimeRemaining);
        lcd.setCursor(0, 3);
        lcd.print("Press decr to abort");
        Sprinkler5TimeRemaining = Sprinkler5Finish - (millis()/1000);
        if (Sprinkler5TimeRemaining < 1) {  // In the while loop, watches remaining time and turns off section when done.
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler5State = -1;                         //-1 as to not get caught again until full reset.
        }
      }

      //Sprinkler 6
      if (Sprinkler6State == 1 && data == 0) {  //Startar bevattningen
        LCD = 255; // Forces LCD refresh.
        digitalWrite(powerPin, HIGH);
        data = 32;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        Sprinkler6StartTime = millis() / 1000;
        Sprinkler6Finish = Sprinkler6StartTime + (Sprinkler6Time * 60);
        lcd.clear();
      }
      while (Sprinkler6State == 1 && data == 32) {  //Shows remaining time and reads abort button
        bool abort = digitalRead(AbortPin);
        if (abort == 0) {
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler6State = -1;
          lcd.clear();
          delay(500);
        }
        lcd.setCursor(0, 0);
        lcd.print("Section 6 started");
        lcd.setCursor(0, 1);
        lcd.print("Seconds remaining: ");
        lcd.setCursor(0, 2);
        lcd.print(Sprinkler6TimeRemaining);
        lcd.setCursor(0, 3);
        lcd.print("Press decr to abort");
        Sprinkler6TimeRemaining = Sprinkler6Finish - (millis()/1000);
        if (Sprinkler6TimeRemaining < 1) {  // In the while loop, watches remaining time and turns off section when done.
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler6State = -1;                         //-1 as to not get caught again until full reset.
        }
      }

      //Sprinkler 7
      if (Sprinkler7State == 1 && data == 0) {  //Startar bevattningen
        LCD = 255; // Forces LCD refresh.
        digitalWrite(powerPin, HIGH);
        data = 64;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        Sprinkler7StartTime = millis() / 1000;
        Sprinkler7Finish = Sprinkler7StartTime + (Sprinkler7Time * 60);
        lcd.clear();
      }
      while (Sprinkler7State == 1 && data == 64) {  //Shows remaining time and reads abort button
        bool abort = digitalRead(AbortPin);
        if (abort == 0) {
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler7State = -1;
          lcd.clear();
          delay(500);
        }
        lcd.setCursor(0, 0);
        lcd.print("Section 7 started");
        lcd.setCursor(0, 1);
        lcd.print("Seconds remaining: ");
        lcd.setCursor(0, 2);
        lcd.print(Sprinkler7TimeRemaining);
        lcd.setCursor(0, 3);
        lcd.print("Press decr to abort");
        Sprinkler7TimeRemaining = Sprinkler7Finish - (millis()/1000);
        if (Sprinkler7TimeRemaining < 1) {  // In the while loop, watches remaining time and turns off section when done.
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler7State = -1;                         //-1 as to not get caught again until full reset.
        }
      }

      //Sprinkler 8
      if (Sprinkler8State == 1 && data == 0) {  //Startar bevattningen
        LCD = 255; // Forces LCD refresh.
        digitalWrite(powerPin, HIGH);
        data = 128;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        Sprinkler8StartTime = millis() / 1000;
        Sprinkler8Finish = Sprinkler8StartTime + (Sprinkler8Time * 60);
        lcd.clear();
      }
      while (Sprinkler8State == 1 && data == 128) {  //Shows remaining time and reads abort button
        bool abort = digitalRead(AbortPin);
        if (abort == 0) {
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler8State = -1;
          lcd.clear();
          delay(500);
        }
        lcd.setCursor(0, 0);
        lcd.print("Section 8 started");
        lcd.setCursor(0, 1);
        lcd.print("Seconds remaining: ");
        lcd.setCursor(0, 2);
        lcd.print(Sprinkler8TimeRemaining);
        lcd.setCursor(0, 3);
        lcd.print("Press decr to abort");
        Sprinkler8TimeRemaining = Sprinkler8Finish - (millis()/1000);
        if (Sprinkler8TimeRemaining < 1) {  // In the while loop, watches remaining time and turns off section when done.
          data = 0;
          digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
          shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
          digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
          Sprinkler8State = -1;                         //-1 as to not get caught again until full reset.
        }
      }

    digitalWrite(powerPin, LOW); // All sections should've been run through, turn off 24V power(saves power).
    } //end of (LongWatering == 1 && LongDay == 1)
  } //end of (Hour > 18 || Hour < 6)

  // FORCED WATERING SECTION
  if(ForcedWatering == 1){ //Watering cycle that runs every active section for a shorter time, directly when activated.
    digitalWrite(powerPin, HIGH); // Turn on 24V to solenoids/relays
    // Sprinkler 1
    if (SprinklerStates[0] == 1 && data == 0){  //Startar bevattningen
      LCD = 255; // Forces LCD refresh.
      data = 1;
      digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
      shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
      digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
      Sprinkler1StartTime = millis() / 1000;
      Sprinkler1Finish = Sprinkler1StartTime + ((Sprinkler1Time/4) * 60);
      lcd.clear();
    }
    while (SprinklerStates[0] == 1 && data == 1){  //Shows remaining time and reads abort button
      bool abort = digitalRead(AbortPin);
      if (abort == 0) {
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[0] = -1;
        lcd.clear();
        delay(500);
      }
      lcd.setCursor(0, 0);
      lcd.print("Section 1 started");
      lcd.setCursor(0, 1);
      lcd.print("Seconds remaining: ");
      lcd.setCursor(0, 2);
      lcd.print(Sprinkler1TimeRemaining);
      lcd.setCursor(0, 3);
      lcd.print("Press decr to abort");
      Sprinkler1TimeRemaining = Sprinkler1Finish - (millis()/1000);
      if (Sprinkler1TimeRemaining < 1){  // In the while loop, watches remaining time and turns off section when done.
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[0] = -1;                         //-1 as to not get caught again until full reset.
      }
    }

    // Sprinkler 2
    if (SprinklerStates[1] == 1 && data == 0){  //Startar bevattningen
      LCD = 255; // Forces LCD refresh.
      data = 2;
      digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
      shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
      digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
       Sprinkler2StartTime = millis() / 1000;
       Sprinkler2Finish = Sprinkler2StartTime + ((Sprinkler2Time/4) * 60);
       lcd.clear();
    }
    while (SprinklerStates[1] == 1 && data == 2){  //Shows remaining time and reads abort button
      bool abort = digitalRead(AbortPin);
      if (abort == 0){
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        Sprinkler2State = -1;
        lcd.clear();
        delay(500);
      }
      lcd.setCursor(0, 0);
      lcd.print("Section 2 started");
      lcd.setCursor(0, 1);
      lcd.print("Seconds remaining: ");
      lcd.setCursor(0, 2);
      lcd.print(Sprinkler2TimeRemaining);
      lcd.setCursor(0, 3);
      lcd.print("Press decr to abort");
      Sprinkler2TimeRemaining = Sprinkler2Finish - (millis()/1000);
      if (Sprinkler2TimeRemaining < 1){  // In the while loop, watches remaining time and turns off section when done.
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[1] = -1;                         //-1 as to not get caught again until full reset.
      }
    }

    // Sprinkler 3
    if (SprinklerStates[2] == 1 && data == 0){  //Startar bevattningen
      LCD = 255; // Forces LCD refresh.
      data = 4;
      digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
      shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
      digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
      Sprinkler3StartTime = millis() / 1000;
      Sprinkler3Finish = Sprinkler3StartTime + ((Sprinkler3Time/4) * 60);
      lcd.clear();
    }
    while (SprinklerStates[2] == 1 && data == 4){  //Shows remaining time and reads abort button
      bool abort = digitalRead(AbortPin);
      if (abort == 0){
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[2] = -1;
        lcd.clear();
        delay(500);
      }
      lcd.setCursor(0, 0);
      lcd.print("Section 3 started");
      lcd.setCursor(0, 1);
      lcd.print("Seconds remaining: ");
      lcd.setCursor(0, 2);
      lcd.print(Sprinkler3TimeRemaining);
      lcd.setCursor(0, 3);
      lcd.print("Press decr to abort");
      Sprinkler3TimeRemaining = Sprinkler3Finish - (millis()/1000);
      if (Sprinkler3TimeRemaining < 1){  // In the while loop, watches remaining time and turns off section when done.
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[2] = -1;                         //-1 as to not get caught again until full reset.
      }
    }

    // Sprinkler 4
    if (SprinklerStates[3] == 1 && data == 0){  //Startar bevattningen
      LCD = 255; // Forces LCD refresh.
      data = 8;
      digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
      shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
      digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
      Sprinkler4StartTime = millis() / 1000;
      Sprinkler4Finish = Sprinkler4StartTime + ((Sprinkler4Time/4) * 60);
      lcd.clear();
    }
    while (SprinklerStates[3] == 1 && data == 8){  //Shows remaining time and reads abort button
      bool abort = digitalRead(AbortPin);
      if (abort == 0){
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[3] = -1;
        lcd.clear();
        delay(500);
      }
      lcd.setCursor(0, 0);
      lcd.print("Section 4 started");
      lcd.setCursor(0, 1);
      lcd.print("Seconds remaining: ");
      lcd.setCursor(0, 2);
      lcd.print(Sprinkler4TimeRemaining);
      lcd.setCursor(0, 3);
      lcd.print("Press decr to abort");
      Sprinkler4TimeRemaining = Sprinkler4Finish - (millis()/1000);
      if (Sprinkler4TimeRemaining < 1){  // In the while loop, watches remaining time and turns off section when done.
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[3] = -1;                         //-1 as to not get caught again until full reset.
      }
    }

    // Sprinkler 5
    if (SprinklerStates[4] == 1 && data == 0){  //Startar bevattningen
      LCD = 255; // Forces LCD refresh.
      data = 16;
      digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
      shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
      digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
      Sprinkler5StartTime = millis() / 1000;
      Sprinkler5Finish = Sprinkler5StartTime + ((Sprinkler5Time/4) * 60);
      lcd.clear();
    }
    while (SprinklerStates[4] == 1 && data == 16){  //Shows remaining time and reads abort button
      bool abort = digitalRead(AbortPin);
      if (abort == 0) {
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[4] = -1;
        lcd.clear();
        delay(500);
      }
      lcd.setCursor(0, 0);
      lcd.print("Section 5 started");
      lcd.setCursor(0, 1);
      lcd.print("Seconds remaining: ");
      lcd.setCursor(0, 2);
      lcd.print(Sprinkler5TimeRemaining);
      lcd.setCursor(0, 3);
      lcd.print("Press decr to abort");
      Sprinkler5TimeRemaining = Sprinkler5Finish - (millis()/1000);
      if (Sprinkler5TimeRemaining < 1){  // In the while loop, watches remaining time and turns off section when done.
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[4] = -1;                         //-1 as to not get caught again until full reset.
      }
    }

    // Sprinkler 6
    if (SprinklerStates[5] == 1 && data == 0){  //Startar bevattningen
      LCD = 255; // Forces LCD refresh.
      data = 32;
      digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
      shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
      digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
      Sprinkler6StartTime = millis() / 1000;
      Sprinkler6Finish = Sprinkler6StartTime + ((Sprinkler6Time/4) * 60);
      lcd.clear();
    }
    while (SprinklerStates[5] == 1 && data == 32){  //Shows remaining time and reads abort button
      bool abort = digitalRead(AbortPin);
      if (abort == 0){
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[5] = -1;
        lcd.clear();
        delay(500);
      }
      lcd.setCursor(0, 0);
      lcd.print("Section 6 started");
      lcd.setCursor(0, 1);
      lcd.print("Seconds remaining: ");
      lcd.setCursor(0, 2);
      lcd.print(Sprinkler6TimeRemaining);
      lcd.setCursor(0, 3);
      lcd.print("Press decr to abort");
      Sprinkler6TimeRemaining = Sprinkler6Finish - (millis()/1000);
      if (Sprinkler6TimeRemaining < 1){  // In the while loop, watches remaining time and turns off section when done.
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[5] = -1;                         //-1 as to not get caught again until full reset.
      }
    }

    // Sprinkler 7
    if (SprinklerStates[6] == 1 && data == 0){  //Startar bevattningen
      LCD = 255; // Forces LCD refresh.
      data = 64;
      digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
      shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
      digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
      Sprinkler7StartTime = millis() / 1000;
      Sprinkler7Finish = Sprinkler7StartTime + ((Sprinkler7Time/4) * 60);
      lcd.clear();
    }
    while (SprinklerStates[6] == 1 && data == 64){  //Shows remaining time and reads abort button
      bool abort = digitalRead(AbortPin);
      if (abort == 0){
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[6] = -1;
        lcd.clear();
        delay(500);
      }
      lcd.setCursor(0, 0);
      lcd.print("Section 7 started");
      lcd.setCursor(0, 1);
      lcd.print("Seconds remaining: ");
      lcd.setCursor(0, 2);
      lcd.print(Sprinkler7TimeRemaining);
      lcd.setCursor(0, 3);
      lcd.print("Press decr to abort");
      Sprinkler7TimeRemaining = Sprinkler7Finish - (millis()/1000);
      if (Sprinkler7TimeRemaining < 1){  // In the while loop, watches remaining time and turns off section when done.
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[6] = -1;                         //-1 as to not get caught again until full reset.
      }
    }

    // Sprinkler 8
    if (SprinklerStates[7] == 1 && data == 0){  //Startar bevattningen
      LCD = 255; // Forces LCD refresh.
      data = 128;
      digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
      shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
      digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
      Sprinkler8StartTime = millis() / 1000;
      Sprinkler8Finish = Sprinkler8StartTime + ((Sprinkler8Time/4) * 60);
      lcd.clear();
    }
    while (SprinklerStates[7] == 1 && data == 128){  //Shows remaining time and reads abort button
      bool abort = digitalRead(AbortPin);
      if (abort == 0){
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[7] = -1;
        lcd.clear();
        delay(500);
      }
      lcd.setCursor(0, 0);
      lcd.print("Section 8 started");
      lcd.setCursor(0, 1);
      lcd.print("Seconds remaining: ");
      lcd.setCursor(0, 2);
      lcd.print(Sprinkler8TimeRemaining);
      lcd.setCursor(0, 3);
      lcd.print("Press decr to abort");
      Sprinkler8TimeRemaining = Sprinkler8Finish - (millis()/1000);
      if (Sprinkler8TimeRemaining < 1){  // In the while loop, watches remaining time and turns off section when done.
        data = 0;
        digitalWrite(latchPin, LOW);                  // Latch is low while we load new data to register
        shiftOut(dataPin, clockPin, MSBFIRST, data);  // Load the data to register using ShiftOut function
        digitalWrite(latchPin, HIGH);                 // Toggle latch to present the new data on register outputs
        SprinklerStates[7] = -1;                         //-1 as to not get caught again until full reset.
      }
    }
    //Still inside of if(ForcedWatering == 1){
    digitalWrite(powerPin, LOW); // Turns off 24V to save power.
    for (int i = 0; i < 9; i++) { // Reset all states so it can be run again.
      if (SprinklerStates[i] == -1) {
        SprinklerStates[i] = 1;
      }
    }
    lcd.setCursor(0,0);
    lcd.print("Clearing flags");
    lcd.setCursor(0,1);
    lcd.print(SprinklerStates[0]);
    lcd.print(SprinklerStates[1]);
    lcd.print(SprinklerStates[2]);
    lcd.print(SprinklerStates[3]);
    lcd.print(SprinklerStates[4]);
    lcd.print(SprinklerStates[5]);
    lcd.print(SprinklerStates[6]);
    lcd.print(SprinklerStates[7]);
    delay(2500);
    ForcedWatering = 0;
    lcd.clear();
  }
}
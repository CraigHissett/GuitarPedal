/*
 * ard-mididrum.ino
 * 
 * MIDI interface for 6 inputs as drum pads 'mono' (variable/analog from piezo sensor) with velocity value
 *                and for pedals 'stereo' ("expression" type with potentiometer inside)
 *                and for pedals 'mono' ("sustain" type with a switch inside NA or NC)
 *                
 * it uses Arduino Micro, USB port as MIDI-OUT device, OLED 128x64 display (setup and monitor),
 * rotary encoder + button for setup menu and options, 6 dimmable leds to monitor 6 inputs activity, 
 * MIDI-OUT serial port; parameters are memorized in the MPU EEPROM;
 * 
 * 
 * by Marco Zonca 8-12/2020
 * https://create.arduino.cc/projecthub/marcozonca/create-or-expand-your-drum-set-with-midi-interface-8d82f2?ref=search&ref_id=MIDI&offset=45
*/

/*table
 * 
 * octave note pitchNr
 * -------------------
 *  -2    C    0
 *  -2    C#   1
 *  -2    D    2
 *  -2    D#   3
 *  -2    E    4
 *  -2    F    5
 *  -2    F#   6    
 *  -2    G    7
 *  -2    G#   8
 *  -2    A    9
 *  -2    A#   10
 *  -2    B    11
 *  
 *  ...the same for the other octaves...
 *  
 * octave note pitchNr
 * -------------------
 *  -1   (C-B)   12-23
 *   0   (C-B)   24-35
 *   1   (C-B)   36-47
 *   2   (C-B)   48-59
 *   3   (C-B)   60-71
 *   4   (C-B)   72-83
 *   5   (C-B)   84-95
 *   6   (C-B)   96-107
 *   7   (C-B)   108-119
 *   8   (C-G)   120-127
 *   
 *   PitchNr [0 to 127]
 *   Octave = ((int(PitchNr / 12)) - 2)     [-2 to 8]
 *   Note-Number = (PitchNr-((Octave+2)*12) + 1) [1 to 12]
 *   
 *   ----------------------
 *   for pitchNr the maximum is 127
 *   for controlNr the maximum is 119
 *   
*/

#include "MIDIUSB.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define NUM_INPUTS  6
#define NUM_VALUES  9
#define IS_DEBUG false

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

// drums or pedals
//ADJUST THESE FOR FOOTSWITCH INPUT
const uint8_t input1 = 18;  // A0
const uint8_t input2 = 19;  // A1
const uint8_t input3 = 20;  // A2
const uint8_t input4 = 21;  // A3
const uint8_t input5 = 22;  // A4
const uint8_t input6 = 23;  // A5

// leds
const int led1Pin = 13;
const int led2Pin = 11;
const int led3Pin = 10;
const int led4Pin = 9;
const int led5Pin = 6;
const int led6Pin = 5;
const byte on = 255;
const byte off = 0;

const uint8_t inputs[NUM_INPUTS] = {input1, input2, input3, input4, input5, input6};
const uint8_t leds[NUM_INPUTS] = {led1Pin, led2Pin, led3Pin, led4Pin, led5Pin, led6Pin};
uint8_t prevValue[NUM_INPUTS] = {0, 0, 0, 0, 0, 0};


//CHANGE NOTE VALUES FOR CC COMMANDS
byte APitch = 48;
int AOctave = (int(APitch/12)-2);
byte ANote = (APitch-((AOctave+2)*12) + 1);
char tableNotes[41] = "C  C# D  D# E  F  F# G  G# A  A# B  ";
char atext[31]="";
char apark[31]="";

uint8_t drum_velocity = 0;
uint8_t velocity = 0;
uint8_t controlvalue = 0;
bool drum_isPressed = false;

uint8_t bType[NUM_INPUTS] = {1, 1, 1, 1, 2, 3};  // 1=piezo drum 2=switch pedal 3=trimmer pedal (default setting)

int KNOBinterval = 4000; // m/sec to enter Menu
int KNOBmenuInterval = 1000; // m/sec to navigate Menu

// rotary encoder
int menuPin = 4;
int encoderPin1 = 7;
int encoderPin2 = 12;
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
volatile long encoderPrev = 0;
volatile boolean IsMenu = false;
volatile long lastencoderValue = 0;
volatile int lastMSB = 0;
volatile int lastLSB = 0;
volatile int knob = 0;
volatile byte preknob = 1;
unsigned long prevKNOBmillis = 0;
byte menuLevel=0;  // 0-2
byte menu_L0_Input=0;  // 0-6 0=exit
byte menu_L1_Voice=0;  // 0-9 0=back
byte menu_L2_Value=0;  // 0-127
byte menu_memory[NUM_INPUTS][NUM_VALUES];


/*  Voices 0:Type          1=piezo/drum 2=switch/pedal 3=trimmer/pedal
 *         1:Reverse       0=no 1=yes (only pedals)
 *         2:Channel       1-16
 *         3:PitchNr       0-127 (only drums)
 *         4:Len           5-95 m/sec (only drums)
 *         5:Min           0-63
 *         6:Max           64-127
 *         7:AntiBouncing  0-63 (only pedals)
 *         8:ControlNr     0-119 (only pedals)
 *         
 */

const byte menu_L2_Value_piezo_min[NUM_VALUES]=  {  1, 255,  1,   0,   5,  0,  64, 255, 255};  // 255=not used
const byte menu_L2_Value_piezo_max[NUM_VALUES]=  {  3, 255, 16, 127,  95, 63, 127, 255, 255};
const byte menu_L2_Value_piezo_def[NUM_VALUES]=  {  1, 255,  1,  48,  50, 30, 100, 255, 255};
const byte menu_L2_Value_switch_min[NUM_VALUES]= {  1,   0,  1, 255, 255,  0,  64,   0,   0}; 
const byte menu_L2_Value_switch_max[NUM_VALUES]= {  3,   1, 16, 255, 255, 63, 127,  63, 119}; 
const byte menu_L2_Value_switch_def[NUM_VALUES]= {  2,   1,  1, 255, 255,  0,  70,  60, 102}; 
const byte menu_L2_Value_trimmer_min[NUM_VALUES]={  1,   0,  1, 255, 255,  0,  64,   0,   0}; 
const byte menu_L2_Value_trimmer_max[NUM_VALUES]={  3,   1, 16, 255, 255, 63, 127,  63, 119};
const byte menu_L2_Value_trimmer_def[NUM_VALUES]={  3,   0,  1, 255, 255,  0,  90,   2, 103};
const byte mId1=0xF1;
const byte mId2=0xF2;

void setup() {
  int mAdr=0;
  byte a=0;
  byte b=0;
  if (IS_DEBUG) Serial.begin(38400);
  Serial1.begin(31250);  // must be 31250 for MIDI standard
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Display Address 0x3C for 128x64 pixels
  pinMode(encoderPin1, INPUT_PULLUP);
  pinMode(encoderPin2, INPUT_PULLUP);
  pinMode(menuPin, INPUT_PULLUP);
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
  pinMode(led4Pin, OUTPUT);
  pinMode(led5Pin, OUTPUT);
  pinMode(led6Pin, OUTPUT);

  mAdr=0;  // read EEPROM first 2 ID bytes and load, if necessary, all default parameters
  if (EEPROM.read(mAdr) != mId1 || EEPROM.read(mAdr+1) != mId2)  {
    for (a=0;a<NUM_INPUTS;a++) {
      loadDefaults(a, bType[a]);
    }//for a
    updateMemory();
  } else {
    for (a=0;a<NUM_INPUTS;a++) {  // read EEPROM 54 bytes (addr=2to55)
      for (b=0;b<NUM_VALUES;b++) {
        mAdr=(2+(a*9))+b;
        menu_memory[a][b]=EEPROM.read(mAdr);
      }
    }
  }//if EEPROM.read()
    
  for (byte i=0;i<NUM_INPUTS;i++) {  // pullup for switches
    switch (menu_memory[i][0]) {  //0=type
      case 1:  // drum
      break;
      case 2:  // switch
        pinMode(inputs[i], INPUT_PULLUP);
      break;
      case 3:  // trimmer
      break;
    }//switch
  }//for i

  for (int i=1;i<=NUM_INPUTS;i++) {  // leds show
    if (i>1) { analogWrite(leds[i-2], off); }
    analogWrite(leds[i-1], on);
    delay(200);
  }
  analogWrite(leds[5], off); 

  showNormalDisplay();
}//setup()


void loop() {
  if (IsMenu==false) {
    for (byte i=0;i<NUM_INPUTS;i++) {  // ----------------- work on inputs
      switch (menu_memory[i][0]) {  //0=type
        case 1:  // drum
          readDrum(i); 
          break;
        case 2:  // switch
          readSwitch(i);
          break;
        case 3:  // trimmer
          readTrimmer(i);
          break;
      }  //switch
    }  // for i
    if ((prevKNOBmillis+KNOBinterval) < millis()) {  // ---- check menu
      if (digitalRead(menuPin) == LOW) {
        IsMenu=true;
        menuLevel=0;
        menu_L0_Input=0;
        if (IS_DEBUG) Serial.println("menu level 0 (input nr 0-6)");
        display.clearDisplay();
        printAt(2,2,false,"SETUP:      type:");
        printAt(2,4,false,"rev:     min :");
        printAt(2,5,false,"ch :     max :");
        printAt(2,6,false,"ctr:     note:");
        printAt(2,7,false,"len:     abnc:");
        displayChoices();
        display.display();
      }
      prevKNOBmillis=millis();
    }
   } else {
    updateEncoder(); 
  } // if IsMenu()
}  // loop()

void showNormalDisplay() {  //normal display
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Z-MIDI 6dp");
  printAt(1,4,false,"Press knob for setup");
  printAt(1,6,true,"P T CH RAW VAL NT/CC");
  display.display();  
}

void loadDefaults(byte PortNr, byte PortType) {
  byte a=0;
  byte b=0;
  a=PortNr;
  switch (PortType) {
    case 1:
      for (b=0;b<NUM_VALUES;b++) {
        menu_memory[a][b]=menu_L2_Value_piezo_def[b];
      }
    break;
    case 2:
      for (b=0;b<NUM_VALUES;b++) {
        menu_memory[a][b]=menu_L2_Value_switch_def[b];
      }
    break;
    case 3:
      for (b=0;b<NUM_VALUES;b++) {
        menu_memory[a][b]=menu_L2_Value_trimmer_def[b];
      }
    break;
  }//switch(a)
}//loadDefaults()

void updateMemory() {
  int mAdr=0;
  byte a=0;
  byte b=0;
  EEPROM.update(0,mId1);
  EEPROM.update(1,mId2);
  for (a=0;a<NUM_INPUTS;a++) {  // update EEPROM 54 bytes (addr=2to55)
    for (b=0;b<NUM_VALUES;b++) {
      mAdr=(2+(a*9))+b;
      EEPROM.update(mAdr,menu_memory[a][b]);
    }
  }
}

void displayChoices() {  // live display during navigating inside menu
  switch (menuLevel) {
    case 0:
      if (knob==0) {
        printAt(9,2,true,"exit");
      } else {
        sprintf(atext,"%-4d",knob);
        printAt(9,2,true,atext);
      }
    break;
    case 1:
      if (menu_L0_Input==0) {
        printAt(9,2,false,"exit");
      } else {
        sprintf(atext,"%-4d",menu_L0_Input);
        printAt(9,2,false,atext);
      }
      if (menu_memory[menu_L0_Input-1][1-1]==1) {  // ------------- 1=drum
        sprintf(atext,"%d",menu_memory[menu_L0_Input-1][1-1]);
        printAt(20,2,false,atext);
        printAt(7,4,false,"-");
        sprintf(atext,"%-2d ",menu_memory[menu_L0_Input-1][6-1]);
        printAt(17,4,false,atext);
        sprintf(atext,"%-2d",menu_memory[menu_L0_Input-1][3-1]);
        printAt(7,5,false,atext);
        sprintf(atext,"%-3d",menu_memory[menu_L0_Input-1][7-1]);
        printAt(17,5,false,atext);
        printAt(7,6,false,"---");
        APitch = menu_memory[menu_L0_Input-1][4-1];
        AOctave = ((int(APitch / 12)) - 2);
        ANote = (APitch-((AOctave+2)*12) + 1);
        s_substring(apark, sizeof(atext), tableNotes, sizeof(tableNotes), (ANote-1) * 3, ((ANote-1) * 3)+1);
        sprintf(atext,"%s/%+1d",apark, AOctave);
        printAt(17,6,false,atext);
        sprintf(atext,"%-2d",menu_memory[menu_L0_Input-1][5-1]);
        printAt(7,7,false,atext);
        printAt(17,7,false,"--");
      } else {   // ---------------------------------------------- 2=switch 3=trimmer
        sprintf(atext,"%d",menu_memory[menu_L0_Input-1][1-1]);
        printAt(20,2,false,atext);
        if (menu_memory[menu_L0_Input-1][2-1]==0) {  // reverse
          printAt(7,4,false,"N");
        } else {
          printAt(7,4,false,"Y");
        }
        sprintf(atext,"%-2d",menu_memory[menu_L0_Input-1][6-1]);
        printAt(17,4,false,atext);
        sprintf(atext,"%-2d",menu_memory[menu_L0_Input-1][3-1]);
        printAt(7,5,false,atext);
        sprintf(atext,"%-3d",menu_memory[menu_L0_Input-1][7-1]);
        printAt(17,5,false,atext);
        sprintf(atext,"%-3d",menu_memory[menu_L0_Input-1][9-1]);
        printAt(7,6,false,atext);
        printAt(17,6,false,"--/--");
        printAt(7,7,false,"--");
        sprintf(atext,"%-2d",menu_memory[menu_L0_Input-1][8-1]);
        printAt(17,7,false,atext);
      }
      printAt(8,2,false," ");  // clear pointers ">"
      printAt(19,2,false," ");
      printAt(6,4,false," ");
      printAt(6,5,false," ");
      printAt(16,6,false," ");
      printAt(6,7,false," ");
      printAt(16,4,false," ");
      printAt(16,5,false," ");
      printAt(16,7,false," ");
      printAt(6,6,false," ");
      switch(knob) {           // new pointer ">"
        case 0:
          printAt(8,2,false,">");
        break;
        case 1:
          printAt(19,2,false,">");
        break;
        case 2:
          printAt(6,4,false,">");
        break;
        case 3:
          printAt(6,5,false,">");
        break;
        case 4:
          printAt(16,6,false,">");
        break;
        case 5:
          printAt(6,7,false,">");
        break;
        case 6:
          printAt(16,4,false,">");
        break;
        case 7:
          printAt(16,5,false,">");
        break;
        case 8:
          printAt(16,7,false,">");
        break;
        case 9:
          printAt(6,6,false,">");
        break;
      }//switch(knob)
    break;
    case 2:
      switch(menu_L1_Voice) {  // print new voice's value
        case 0:
        break;
        case 1:
          sprintf(atext,"%d",knob);
          printAt(20,2,true,atext);
        break;
        case 2:
          if (knob==0) {  // reverse
            printAt(7,4,true,"N");
          } else {
            printAt(7,4,true,"Y");
          }
        break;
        case 3:
          sprintf(atext,"%-2d",knob);
          printAt(7,5,true,atext);
        break;
        case 4:
          APitch = knob;
          AOctave = ((int(APitch / 12)) - 2);
          ANote = (APitch-((AOctave+2)*12) + 1);
          s_substring(apark, sizeof(atext), tableNotes, sizeof(tableNotes), (ANote-1) * 3, ((ANote-1) * 3)+1);
          sprintf(atext,"%s/%+1d",apark, AOctave);
          printAt(17,6,true,atext);
        break;
        case 5:
          sprintf(atext,"%-2d",knob);
          printAt(7,7,true,atext);
        break;
        case 6:
          sprintf(atext,"%-2d",knob);
          printAt(17,4,true,atext);
        break;
        case 7:
          sprintf(atext,"%-3d",knob);
          printAt(17,5,true,atext);
        break;
        case 8:
          sprintf(atext,"%-2d",knob);
          printAt(17,7,true,atext);
        break;
        case 9:
          sprintf(atext,"%-3d",knob);
          printAt(7,6,true,atext);
        break;
      }//switch(menu_L1_Voice)
    break;
  }//switch(menuLevel)
}

void updateEncoder() {  // ---------------------------------- encoder menu
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit
  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;
  lastEncoded = encoded; //store this value for next time 
  if ((encoderValue - encoderPrev) > 8) {  // ------------------ right turn, clockwise ++
    knob ++;
    if (menuLevel==0) if (knob>NUM_INPUTS) knob=0;
    if (menuLevel==1) if (knob>9) knob=0;
    if (menuLevel==1) if (knob==2) if (menu_memory[menu_L0_Input-1][1-1]==1) knob=3;  // jump n.2 if drum
    if (menuLevel==1) if (knob==8) if (menu_memory[menu_L0_Input-1][1-1]==1) knob=0;  // jump n.8 if drum
    if (menuLevel==1) if (knob==9) if (menu_memory[menu_L0_Input-1][1-1]==1) knob=0;  // jump n.9 if drum
    if (menuLevel==1) if (knob==4) if (menu_memory[menu_L0_Input-1][1-1]==2 || menu_memory[menu_L0_Input-1][1-1]==3) knob=6;  // jump n.4 if pedal
    if (menuLevel==1) if (knob==5) if (menu_memory[menu_L0_Input-1][1-1]==2 || menu_memory[menu_L0_Input-1][1-1]==3) knob=6;  // jump n.5 if pedal
    if (menuLevel==2) if (menu_memory[menu_L0_Input-1][1-1]==1) if (knob > menu_L2_Value_piezo_max[menu_L1_Voice-1]) knob=menu_L2_Value_piezo_min[menu_L1_Voice-1];
    if (menuLevel==2) if (menu_memory[menu_L0_Input-1][1-1]==2) if (knob > menu_L2_Value_switch_max[menu_L1_Voice-1]) knob=menu_L2_Value_switch_min[menu_L1_Voice-1];
    if (menuLevel==2) if (menu_memory[menu_L0_Input-1][1-1]==3) if (knob > menu_L2_Value_trimmer_max[menu_L1_Voice-1]) knob=menu_L2_Value_trimmer_min[menu_L1_Voice-1];
    if (menuLevel==2) if (menu_L1_Voice==1) loadDefaults((menu_L0_Input-1),knob);  // load defaults if changing PortType 
    encoderPrev=encoderValue;
    displayChoices();
    display.display();
    if (IS_DEBUG) Serial.println(knob);
  }
  if ((encoderValue - encoderPrev) < -8) {  // ------------------ left turn, anticlockwise --
    knob --;
    if (menuLevel==0) if (knob<0) knob=NUM_INPUTS;
    if (menuLevel==1) if (knob<0) knob=9;
    if (menuLevel==1) if (knob==2) if (menu_memory[menu_L0_Input-1][1-1]==1) knob=1;  // jump n.2 if drum
    if (menuLevel==1) if (knob==8) if (menu_memory[menu_L0_Input-1][1-1]==1) knob=7;  // jump n.8 if drum
    if (menuLevel==1) if (knob==9) if (menu_memory[menu_L0_Input-1][1-1]==1) knob=7;  // jump n.9 if drum
    if (menuLevel==1) if (knob==4) if (menu_memory[menu_L0_Input-1][1-1]==2 || menu_memory[menu_L0_Input-1][1-1]==3) knob=3;  // jump n.4 if pedal
    if (menuLevel==1) if (knob==5) if (menu_memory[menu_L0_Input-1][1-1]==2 || menu_memory[menu_L0_Input-1][1-1]==3) knob=3;  // jump n.5 if pedal
    if (menuLevel==2) if (menu_memory[menu_L0_Input-1][1-1]==1) if (knob < menu_L2_Value_piezo_min[menu_L1_Voice-1]) knob=menu_L2_Value_piezo_max[menu_L1_Voice-1];
    if (menuLevel==2) if (menu_memory[menu_L0_Input-1][1-1]==2) if (knob < menu_L2_Value_switch_min[menu_L1_Voice-1]) knob=menu_L2_Value_switch_max[menu_L1_Voice-1];
    if (menuLevel==2) if (menu_memory[menu_L0_Input-1][1-1]==3) if (knob < menu_L2_Value_trimmer_min[menu_L1_Voice-1]) knob=menu_L2_Value_trimmer_max[menu_L1_Voice-1];
    if (menuLevel==2) if (menu_L1_Voice==1) loadDefaults((menu_L0_Input-1),knob);  // load defaults if changing PortType 
    encoderPrev=encoderValue;
    displayChoices();
    display.display();
    if (IS_DEBUG) Serial.println(knob);
  }
  if (((prevKNOBmillis+KNOBmenuInterval) < millis()) && (digitalRead(menuPin) == LOW)) {  // pressed knob
    switch (menuLevel) {
      case 0:
        if (knob==0) {  // exit from menu
          showNormalDisplay();
          updateMemory();
          IsMenu=false;
          if (IS_DEBUG) Serial.println("menu off");
        } else {  //  chosen input nr
          menuLevel=1;
          menu_L0_Input=knob;
          knob=0;
          displayChoices();
          display.display();
          if (IS_DEBUG) Serial.print("chosen: ");
          if (IS_DEBUG) Serial.print(menu_L0_Input);
          if (IS_DEBUG) Serial.println(", menu level 1 (voice 0-9)");
        }
        break;
      case 1:
        if (knob==0) {  // back previous level 0
          menuLevel=0;
          knob=menu_L0_Input;
          displayChoices();
          display.display();
          if (IS_DEBUG) Serial.println("back menu level 0 (input nr 0-6)");
        } else {  //  chosen voice
          menuLevel=2;
          menu_L1_Voice=knob;
          knob=menu_memory[menu_L0_Input-1][menu_L1_Voice-1];  // propose value in memory
          displayChoices();
          display.display();
          if (IS_DEBUG) Serial.print("chosen: ");
          if (IS_DEBUG) Serial.print(menu_L1_Voice);
          if (IS_DEBUG) Serial.println(", menu level 2 (VALUE 0-127)");
        }
        break;
      case 2:
        menuLevel=1;  //   save value & back previous level 1
        menu_L2_Value=knob;
        menu_memory[menu_L0_Input-1][menu_L1_Voice-1]=menu_L2_Value;
        knob=menu_L1_Voice;
        displayChoices();
        display.display();
        if (IS_DEBUG) Serial.print("chosen: ");
        if (IS_DEBUG) Serial.print(menu_L2_Value);
        if (IS_DEBUG) Serial.println(", save VALUE & back level 1 (voice 0-9)");
        break;
    }  // switch menuLevel
    prevKNOBmillis=millis();
  }  // if millis() 
}  // updateEncoder()


void printAt(const byte col, const byte row, const bool blackonwhite, const char thetext[31]) {  // ------- printAt
  // display: 21 col. x 8 row [setcursor(x,y)] x=(col-1)*6 y=(row-1)*8
  // x=0,6,12,18,24,30,36,42,48,54,60,66,72,78,84,90,96,102,108,114,120 
  // y=0,8,16,24,32,40,48,56
  display.setTextSize(1);
  if (blackonwhite==true) {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // b/w
  } else {
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK); // normal
  }
  display.setCursor((col-1)*6,(row-1)*8);
  display.print(thetext);
}  // printAt()


void readSwitch(byte btnr) {  // ---------------------------------- switch
  int val = 0;
  val = map(analogRead(inputs[btnr]), 0, 1023, 0, 127);
  int raw = val;
  if (val > menu_memory[btnr][6]) {  //6=maxValue
    val = menu_memory[btnr][6];
  }
  if (val < menu_memory[btnr][5]) {  //5=minValue
    val = menu_memory[btnr][5];
  }
  controlvalue = map(val, menu_memory[btnr][5], menu_memory[btnr][6], 0, 127);  // expand value  5=minValue 6=maxValue
  if (controlvalue < (128/2)) {  // .. off
    if (menu_memory[btnr][1]==0) {  //1=reverse
      controlvalue=0;
    } else {
      controlvalue=127;
    }
  } else {  // ....................... on
    if (menu_memory[btnr][1]==0) {  //1=reverse
      controlvalue=127;
    } else {
      controlvalue=0;
    }
  }
  if (controlvalue <= (prevValue[btnr]-menu_memory[btnr][7]) || controlvalue >= (prevValue[btnr]+menu_memory[btnr][7])) {  //7=antibouncing
    controlChange((menu_memory[btnr][2]-1), menu_memory[btnr][8], controlvalue);  //8=controlNr 2=channel

    if (IS_DEBUG) Serial.print(btnr+1);
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(menu_memory[btnr][2]);  //2=channel
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(menu_memory[btnr][8]);  //8=controlNr
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(raw);
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.println(controlvalue);

    sprintf(atext,"%d ",btnr+1);  // live monitor
    printAt(1,7,false,atext);
    printAt(3,7,false,"S");
    sprintf(atext,"%d  ",menu_memory[btnr][2]);  //2=channel
    printAt(5,7,false,atext);
    sprintf(atext,"%d   ",raw);
    printAt(8,7,false,atext);
    sprintf(atext,"%d   ",controlvalue);
    printAt(12,7,false,atext);
    sprintf(atext,"%d     ",menu_memory[btnr][8]);  //8=controlNr
    printAt(16,7,false,atext);
    display.display();

    prevValue[btnr]=controlvalue;
    analogWrite(leds[btnr], (controlvalue*2));
  }
}  // readSwitch()

void readTrimmer(byte btnr) {  // ---------------------------------- trimmer
  int val = 0;
  val = map(analogRead(inputs[btnr]), 0, 1023, 0, 127);
  int raw = val;
  if (val > menu_memory[btnr][6]) {  //6=maxValue
    val = menu_memory[btnr][6];
  }
  if (val < menu_memory[btnr][5]) {  //5=minValue
    val = menu_memory[btnr][5];
  }
  if (menu_memory[btnr][1]==0) { controlvalue = map(val, menu_memory[btnr][5], menu_memory[btnr][6], 0, 127);}  // expand value if reverse=no 5=minValue 6=maxValue
  if (menu_memory[btnr][1]==1) { controlvalue = map(val, menu_memory[btnr][5], menu_memory[btnr][6], 127, 0);}  // expand value if reverse=yes 5=minValue 6=maxValue
  if (controlvalue <= (prevValue[btnr]-menu_memory[btnr][7]) || controlvalue >= (prevValue[btnr]+menu_memory[btnr][7])) {  //7=antibouncing
    controlChange((menu_memory[btnr][2]-1), menu_memory[btnr][8], controlvalue);  //8=controlNr 2=channel

    if (IS_DEBUG) Serial.print(btnr+1);
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print((menu_memory[btnr][2]));  //2=channel
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(menu_memory[btnr][8]);  //8=controlNr
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(raw);
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.println(controlvalue);

    sprintf(atext,"%d ",btnr+1);  // live monitor
    printAt(1,7,false,atext);
    printAt(3,7,false,"T");
    sprintf(atext,"%d  ",menu_memory[btnr][2]);  //2=channel
    printAt(5,7,false,atext);
    sprintf(atext,"%d   ",raw);
    printAt(8,7,false,atext);
    sprintf(atext,"%d   ",controlvalue);
    printAt(12,7,false,atext);
    sprintf(atext,"%d     ",menu_memory[btnr][8]);  //8=controlNr
    printAt(16,7,false,atext);
    display.display();

    prevValue[btnr]=controlvalue;
    analogWrite(leds[btnr], (controlvalue*2));
  }
}  // readTrimmer()

void readDrum(byte btnr) {  // -------------------------------------- drum
  int val=0;
  val = map(analogRead(inputs[btnr]), 0, 1023, 0, 127);
  int raw = val;
  while (val >= menu_memory[btnr][5]) {  //5=minValue as threshold
    drum_isPressed = true;
    if (val >= drum_velocity) {  // take the peak of wave
      drum_velocity=val;
    }
    val = map(analogRead(inputs[btnr]), 0, 1023, 0, 127);
  }//while
  if (drum_isPressed == true) {
    if (drum_velocity > menu_memory[btnr][6]) {  //6=maxValue
      drum_velocity = menu_memory[btnr][6];
    }
    velocity = map(drum_velocity, menu_memory[btnr][5], menu_memory[btnr][6], 1, 127);  // expand velocity 5=minValue 6=maxValue
    playNoteOn((menu_memory[btnr][2]-1), menu_memory[btnr][3], velocity);  //3=pitchNr 2=channel
    
    if (IS_DEBUG) Serial.print(btnr+1);
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print((menu_memory[btnr][2]));  //2=channel
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(menu_memory[btnr][3]);  //3=pitchNr
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(raw);
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(velocity);
    if (IS_DEBUG) Serial.print(" --> ");

    sprintf(atext,"%d ",btnr+1);  // live monitor
    printAt(1,7,false,atext);
    printAt(3,7,false,"P");
    sprintf(atext,"%d  ",menu_memory[btnr][2]);  //2=channel
    printAt(5,7,false,atext);
    sprintf(atext,"%d   ",raw);
    printAt(8,7,false,atext);
    sprintf(atext,"%d   ",velocity);
    printAt(12,7,false,atext);
    sprintf(atext,"%d     ",menu_memory[btnr][3]);  //3=pitchNr
    APitch = menu_memory[btnr][3];
    AOctave = ((int(APitch / 12)) - 2);
    ANote = (APitch-((AOctave+2)*12) + 1);
    s_substring(apark, sizeof(atext), tableNotes, sizeof(tableNotes), (ANote-1) * 3, ((ANote-1) * 3)+1);
    sprintf(atext,"%s/%d ",apark,AOctave);
    printAt(16,7,false,atext);
    display.display();

    if (IS_DEBUG) Serial.print(APitch);
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(AOctave);
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.print(ANote);
    if (IS_DEBUG) Serial.print(" ");
    if (IS_DEBUG) Serial.println(atext);
    
    prevValue[btnr]=velocity;
    drum_velocity=0;
    drum_isPressed = false;
    analogWrite(leds[btnr], (velocity*2));
    delay(menu_memory[btnr][4]);  //4=timeinterval
    analogWrite(leds[btnr], off);
  }  // if(drum1_isPressed)
}  // readDrum()


// First parameter is the event type (0x0B = control change) 4 bits
// Second parameter is the event type, combined with the channel, above 4 bits + channel 4 bits
// Third parameter is the control number (0-119).
// Fourth parameter is the control value (0-127).
// --------------------------------------------------------------
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t midiCc = {0x0B, byte (0xB0 | channel), control, value};
  MidiUSB.sendMIDI(midiCc);  // to USB MIDI
  MidiUSB.flush();
  Serial1.write(0xB0 | channel);  // to serial1 MIDI
  Serial1.write(control);
  Serial1.write(value);
}

// First parameter is the event type (0x09 = note on, 0x08 = note off) 4 bits
// Second parameter is note-on/note-off, combined with the channel, above 4 bits + channel 4 bits
// Channel between 0-15. Typically reported to the user as 1-16
// Third parameter is the note number, the pitch, (48 = middle C).
// Fourth parameter is the velocity (0 = none, 64 = normal, 127 = louder/faster).
// --------------------------------------------------------------
void playNoteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, byte (0x90 | channel), pitch, velocity};
  MidiUSB.sendMIDI(noteOn);  // to USB MIDI
  MidiUSB.flush();
  Serial1.write(0x90 | channel);  // to serial1 MIDI
  Serial1.write(pitch);
  Serial1.write(velocity);
}
void playNoteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, byte (0x80 | channel), pitch, velocity};
  MidiUSB.sendMIDI(noteOff);  // to USB MIDI
  MidiUSB.flush();
  Serial1.write(0x80 | channel);  // to serial1 MIDI
  Serial1.write(pitch);
  Serial1.write(velocity);
}

//------------------------------------------------------------------------- SUBSTRING
bool s_substring(char *dest_string, const int dest_sizeof, const char *source_string, 
 const int source_sizeof, const int source_from, const int source_to) {  // copies source(from, to) to dest
  if ((source_from < 0) || (source_to < source_from) || ((source_to - source_from + 1) > (dest_sizeof - 1)) 
    || (source_to >= (source_sizeof-1)) || ((source_to - source_from + 1) > (strlen(source_string)))) {
    dest_string[0]=0;  // NUL
    return true;  // err 1
  } else {
    int _Count=0;
    for (int i=source_from;i<(source_to+1);i++) {
      dest_string[_Count]=source_string[i];
      _Count++;
    }
    dest_string[_Count]=0;  // ends with NUL
    return false;  // ok 0
  }
}  // end s_substring()

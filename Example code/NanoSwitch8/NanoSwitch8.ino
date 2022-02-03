// Switch8 nano
/*
  Arduino Nano based Midi switcher for use with Boss GT 1000 (Core)
  It uses a I2C display for showing bank and switch info
  Developed by Patrick Klaassen (patrick.klaassen@gmail.com)

  System allows for 10 banks with 8 presets. Default MidiChannel is 1.

  The system sends a program change message on channel 1 for the given preset button
  Preset button (D2) 1 in bank 0 sends program change 0 on channel 1
  Preset button (D3) 2 in bank 0 sends program change 1 on channel 1
  etc.
  Preset button 1 in bank 1 sends program change 10 on channel 1
  Preset button 2 in bank 1 sends program change 11 on channel 1
  etc.

  For going a bank down you need to press the bank down button (D10) twice. The first is to prevent a accidental press so you can 
  press preset 8 without a bank change
  This feature is not present on bank up button (D11) since that button is not close to another preset button.

  The buttons are connected in a 1 row 10 col matrix.
  
Pin Layout
  Midi signal TRS Tip 220ohm to TX1 midi out
  Midi Ring 220ohm to power (5V)
  Midi Sleeve GND
  Vin - Connects with Boss GT1000 power supply (9V)
  3V3 - power to Oled display
  A4 SDA Oled
  A5 SCL Oled
  D2/D11 Switch col inputs 
  D12 Switch row input


Functions/Subroutines
- void printText(String strText)
- void printData(String strBank, String strPreset)
- void programChange(int intPreset)
- void handleBankUp()
- void handleBankDown()
- void setMute() Currently not in use
- void keypadEvent(KeypadEvent key)

System routines
- void setup()
- void loop()
 
 */


#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <MIDI.h>


// OLED section
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


const byte rows = 1; /*change it the same value as numberOfPedal variable */
const byte cols = 10; // You need 10 for the 8 loop, Bank/up/reverb and bank/down/crunch channel buttons
char keys[rows][cols] = {
{'a','b','c','d','e','f','g','h','i','j'}
// a - preset/channel 1
// b - preset/channel 2
// c - preset/channel 3
// d - preset/channel 4
// e - preset/channel 5
// f - preset/channel 6
// g - preset/channel 7
// h - preset/channel 8
// i - bank switching
// j - bank switching
};
byte rowPins[rows] = {12}; // 30 looper, 31 store, 32 preset
//byte colPins[cols] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11};  //1,2,3,4,5,6,7,8,up,down
byte colPins[cols] = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2};  //1,2,3,4,5,6,7,8,up,down
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);
boolean debug = false;
int intCurrentBank = 0;
int intCurrentPreset = -1;
int intMidiChannel = 1;
int intMute=0;
int intBankPressed = 0;

MIDI_CREATE_DEFAULT_INSTANCE();


void printText(String strText) {
  display.clearDisplay();
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 4);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.println(strText);
  display.setTextSize(1);
  display.setCursor(50, 25);
  display.println("switch8 nano");
  display.display();
  delay(1000);
}


void printData(String strBank, String strPreset) {
  display.clearDisplay();

  display.setTextSize(3);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 1);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  String strText = "B:"+strBank +" P:"+strPreset;
  display.println(strText);
  display.setTextSize(1);
  display.setCursor(50, 25);
  display.println("switch8 nano");
  display.display();
}


void programChange(int intPreset){
  // Create midi preset number
  if (intMute==1){
    setMute();
  }
  intBankPressed = 0;
  int intMidiValue = intPreset + (intCurrentBank*10);
  MIDI.sendProgramChange(intMidiValue,intMidiChannel);
  // TODO Update display
  printData(String(intCurrentBank), String(intPreset+1));
}

void handleBankUp(){
  if (intBankPressed == 1)
  {
    intCurrentBank++;
    if (intCurrentBank==10){
      intCurrentBank = 0;
    }
    printData(String(intCurrentBank), "X");
  } else {
    intBankPressed = 1;
  }

  
}

void handleBankDown(){
  intCurrentBank--;
  if (intCurrentBank==-1){
    intCurrentBank = 9;
  }
  printData(String(intCurrentBank), "X");
  intBankPressed = 1;
}

void setMute(){
  // Create midi preset number

  switch (intMute)
  {
    case 0:
      MIDI.sendControlChange(1,127,intMidiChannel);
      printText("Mute   B:" + String(intCurrentBank));
      intMute=1;
      break;
    case 1:
      MIDI.sendControlChange(1,0,intMidiChannel);
      if (intCurrentPreset!=-1){
        printData(String(intCurrentBank), String(intCurrentPreset));
      } else {
        printData(String(intCurrentBank), "X");
      }
      intMute=0;
      break;
  }
}


/******************************************************/
//take care of some special events
void keypadEvent(KeypadEvent key){
  if (debug) Serial.println("key event");
  if (debug) Serial.println(key);
  switch (keypad.getState())
  {
    case PRESSED:
      if (debug) Serial.println("key pressed event");
      if (debug) Serial.println(key);
      switch (key)
      {
        case 'a': /* 'a' to 'h' for 8 pedals- adapt it to the number you needs */
          programChange(0);
          break;
        case 'b':
          programChange(1); 
          break;
        case 'c':
          programChange(2);
          break;
        case 'd':
          programChange(3);
          break;
        case 'e':
          programChange(4);
          break;
        case 'f':
          programChange(5);
          break;
        case 'g':
          programChange(6);
          break;
        case 'h':
          programChange(7);
          break;
      }
      break;
    case RELEASED:
      if (debug) Serial.println("key released event");
      if (debug) Serial.println(key);
      // saveState check savestate
      switch (key)
      {
        case 'i': // depending on mode either amp Reverb switch or bank down
          if (intMute==0){
            handleBankUp();  
          }
          break;
        case 'j': // depending on mode either amp Channel switch or bank up
          handleBankDown();
          break;
      }
      break;
    case HOLD:
      if (debug) Serial.println("key hold event");
      if (debug) Serial.println(key);
      switch (key)
      {
        case 'i': // depending on mode either amp Reverb switch or bank down
          if (intMute==0){
            setMute();   
          }
          break;
      }
      break;
  }
}



void setup() {
  Serial.begin(19200);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  display.dim(true);
  printText("Starting"); 

  delay(500);
  printText("init keys"); 
  keypad.addEventListener(keypadEvent); //add an event listener for this keypad, meaning all the buttons on the Switch8
  keypad.setHoldTime(500);
  keypad.setDebounceTime(80);
  delay(500);
  printText("init midi");
  MIDI.begin(); // MIDI init
  MIDI.turnThruOff();
  delay(500);
  printText("Ready"); 
}

void loop(){

  // Trigger keypad events
  char key = keypad.getKey();

}

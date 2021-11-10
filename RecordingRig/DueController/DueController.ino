#include <Control_Surface.h>
#include <LiquidCrystal.h>

// Instantiate a MIDI Interface to use
USBMIDI_Interface midi;


// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


Bank<2> bank(4); // A bank with four channels, and 2 bank settings

//latching switch variables
const uint8_t velocity = 0b1111111; // Maximum velocity (0b1111111 = 0x7F = 127)
const uint8_t C4 = 60;              // Note number 60 is defined as middle C in the MIDI specification
const unsigned long duration = 100; // The duration of the MIDI note (in milliseconds)


/*
// Instantiate an analog multiplexer
CD74HC4051 mux {
  A0,       // Analog input pin
  {3, 4, 5} // Address pins S0, S1, S2
};
*/

// Create a new instance of the class 'DigitalLatch', called 'toggleSwitch', on pin 2, that sends MIDI messages with note 'C4' (60) on channel 1, with velocity 127 and a duration of 100ms
//DigitalLatch toggleSwitch(2, C4, 1, velocity, duration);

Bankable::CCPotentiometer potentiometer1 {
  {bank, BankType::CHANGE_CHANNEL},     // bank configuration
  A0,                                   // analog pin
  {MIDI_CC::Channel_Volume, CHANNEL_1}, // address
};



SwitchSelector selector {bank, 11};

// Initialize the Control Surface
void setup() {
    // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("CraigMIDI");
  
  Control_Surface.begin();
  pinMode(LED_BUILTIN, OUTPUT);
}

// Update the Control Surface
void loop() {
  Control_Surface.loop();
  digitalWrite(LED_BUILTIN, digitalRead(11));
}

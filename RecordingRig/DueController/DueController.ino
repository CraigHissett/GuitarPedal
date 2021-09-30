#include <Control_Surface.h>

// Instantiate a MIDI Interface to use
USBMIDI_Interface midi;

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

// Instantiate a CCButton object
CCButton button {
  // Push button on pin 5:
  5,
  // General Purpose Controller #1 on MIDI channel 1:
  {MIDI_CC::General_Purpose_Controller_1, CHANNEL_1},
};

// Instantiate an array of CCPotentiometer objects
CCPotentiometer potentiometers[] {
  {A0,        // Analog pin connected to potentiometer 1
   0x10},     // Controller number of the first potentiometer
  {A1,        // Analog pin connected to potentiometer 2
   0x11},     // Controller number of the second potentiometer
  {A2, 0x12}, // Etc.
  {A3, 0x13},
  {A4, 0x14},
  {A5, 0x15},
};

// Initialize the Control Surface
void setup() {
  Control_Surface.begin();
}

// Update the Control Surface
void loop() {
  Control_Surface.loop();
}

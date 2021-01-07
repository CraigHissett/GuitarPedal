#include <midi_serialization.h>
#include <usbmidi.h>

// Link to FBV4 Pin out:
// http://www.harmonicappliances.com/floorboard/floorboard.html

/* 
Switch (or combination)         Resistance    Voltage  
Tap Tempo / Tuner               0             0.040  
Channel Select / Effect On/Off  100           0.490  
Wah (under pedal)               220           0.900  
Channel A / EQ                  370           1.38  
Channel B / Trem/Chorus         570           1.83  
Channel C / Delay               810           2.23  
Channel D / Reverb              1.1k          2.67  
Bank Up + Bank Down             2.7k          3.65  
Bank Up                         3.7k          3.88  
Bank Down                       5k            4.1  
none                            20k           ~5  
*/

/*
Ethernet pin out:
1 GND
2 Wah
3 GND
4 Volume
5 LEDs
6 Switches
7 5V
8 5V
*/

void sendCC(uint8_t channel, uint8_t control, uint8_t value) {
  USBMIDI.write(0xB0 | (channel & 0xf));
  USBMIDI.write(control & 0x7f);
  USBMIDI.write(value & 0x7f);
}

void sendNote(uint8_t channel, uint8_t note, uint8_t velocity) {
  USBMIDI.write((velocity != 0 ? 0x90 : 0x80) | (channel & 0xf));
  USBMIDI.write(note & 0x7f);
  USBMIDI.write(velocity &0x7f);
}

const int ANALOG_PIN_COUNT = 2;
const int BUTTON_PIN_COUNT = 0;

// Change the order of the pins to change the ctrl or note order.
int buttonPin = A0;

int ccValues = 1;

int readCC(int pin) {
  // Convert from 10bit value to 7bit.
  return analogRead(pin) >> 3;
}

void setup() {
  // Initialize initial values.
    pinMode(buttonPin, INPUT);
    ccValues = readCC(buttonPin);
  }

void loop() {
  while (USBMIDI.available()) {
    // We must read entire available data, so in case we receive incoming
    // MIDI data, the host wouldn't get stuck.
    u8 b = USBMIDI.read();
  }

    int value = readCC(buttonPin);

    // Send CC only if th has changed.
    if (ccValues != value) 
    {
      if( value >= 280 and value <= 380 ) {  // 370
         sendNote(0, 64 + 1, 1 ? 127 : 0);
      }
      else if( value >= 480 and value <= 580 ) {  // 570
         sendNote(0, 64 + 2, 2 ? 127 : 0);
      }
      else if( value >= 780 and value <= 880 ) {  // 810
         sendNote(0, 64 + 3, 3 ? 127 : 0);
      }
      else if( value >= 1050 and value <= 1150 ) {  // 1.1k
         sendNote(0, 64 + 4, 4 ? 127 : 0);
      }
      else {
        Serial.println(value);
      }
      ccValues = value;
    }

  // Flush the output.
  USBMIDI.flush();
}

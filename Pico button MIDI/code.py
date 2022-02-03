import time
import board
import busio
import usb_midi

import adafruit_midi
from adafruit_midi.control_change import ControlChange
from adafruit_bus_device.i2c_device import I2CDevice
import adafruit_dotstar

from digitalio import DigitalInOut, Direction, Pull


# RGB MIDI controller example for Pimoroni RGB Keypad for Raspberry Pi Pico

# Prerequisites
#
# Requires Adafruit CircuitPython: https://learn.adafruit.com/getting-started-with-raspberry-pi-pico-circuitpython
#
# Also requires the following CircuitPython libs: adafruit_midi, adafruit_bus_device, adafruit_dotstar
# (drop them into the lib folder)
#
# Save this code in code.py on your Raspberry Pi Pico CIRCUITPY drive


# Pull CS pin low to enable level shifter
cs = DigitalInOut(board.GP17)
cs.direction = Direction.OUTPUT
cs.value = 0

# Set up APA102 pixels
num_pixels = 16
pixels = adafruit_dotstar.DotStar(board.GP18, board.GP19, num_pixels, brightness=0.1, auto_write=True)

# Set up I2C for IO expander (addr: 0x20)
i2c = busio.I2C(board.GP5, board.GP4)
device = I2CDevice(i2c, 0x20)

# Set USB MIDI up on channel 0
midi = adafruit_midi.MIDI(midi_out=usb_midi.ports[1], out_channel=0)

# Function to map 0-255 to position on colour wheel
def colourwheel(pos):
    if pos < 0 or pos > 255:
        return (0, 0, 0)
    if pos < 85:
        return (255 - pos * 3, pos * 3, 0)
    if pos < 170:
        pos -= 85
        return (0, 255 - pos * 3, pos * 3)
    pos -= 170
    return (pos * 3, 0, 255 - pos * 3)

# List to store the button states and toggle states
held = [0] * 16
toggled = [0] * 16

# Keep reading button states, setting pixels, sending notes
while True:
    with device:
        # Read from IO expander, 2 bytes (8 bits) correspond to the 16 buttons
        device.write(bytes([0x0]))
        result = bytearray(2)
        device.readinto(result)
        b = result[0] | result[1] << 8

        # Loop through the buttons
        for i in range(16):
            if not (1 << i) & b:  # Pressed state
                pixels[i] = colourwheel(i * 16)  # Map pixel index to 0-255 range
                if not held[i]:
                    if not toggled[i]:
                        #send on CC
                        midi.send(ControlChange(60 + i, 100))
                        toggled[i] = 1
                    else:
                        #send off CC
                        midi.send(ControlChange(60 + i, 60))
                        toggled[i] = 0
                held[i] = 1
            else:  # Released state
                pixels[i] = (0, 0, 0)  # Turn pixel off
                held[i] = 0  # Set held state to off

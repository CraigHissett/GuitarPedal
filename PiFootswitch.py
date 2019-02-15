#!/usr/bin/python3
# -*- coding: utf-8 -*-
# based on script by William Hofferbert
#
# https://github.com/whofferbert/rpi-midi-rotary-encoder
#
# watches raspberry pi GPIO pins and translates that
# behavior into midi data. Midi data is accessible to
# other clients through a virtual midi device that is
# created with amidithru, via os.system

import RPi.GPIO as GPIO
import time, datetime
import mido
import os
import re

# midi device naming and setup
name = "GuitarRotaryEncoder"

#Pins needed:
#P1 - 3.3v
#3-SDA|5-SCL
#8-tx|10-rx

#set up pi GPIO pins for Footswitches
B1 = 29
B2 = 31
B3 = 33
B4 = 35
B5 = 37

# set up pi GPIO pins for rotary encoder
sw_pin = 19
dt_pin = 21
clk_pin = 23

# button midi info; cc#12 = effect control 1 
button_state = 0
button_channel = 0
button_cc_num = 12
# don't let button signals trigger until X ms have passed
button_stagger_time = 220

# knob midi info; cc#7 = volume, default position near half
position = 63
rotary_increment = 1
rotary_channel = 0
rotary_cc_num = 7

# wait some seconds for other software after reboot
init_sleep_secs = 10

#
# main stuff below
# TODO maybe use the pythonic if __name__ == "__main__":
#

GPIO.setmode(GPIO.BOARD)
GPIO.setwarnings(False)

# wait some seconds, so we don't step on MODEP's toes
time.sleep(init_sleep_secs)

# set up backend
mido.set_backend('mido.backends.rtmidi')

# system command to set up the midi thru port
# TODO would be nice to do this in python, but
# rtmidi has issues seeing ports it has created
runCmd = "amidithru '" + name + "' &"
os.system(runCmd)

# regex to match on rtmidi port name convention
#GuitarRotaryEncoder:GuitarRotaryEncoder 132:0
# TODO is it necessary to write:  "\s+(\d+)?:\d+)"  instead?
nameRegex = "(" + name + ":" + name + "\s+\d+:\d+)"
matcher = re.compile(nameRegex)
newList = list(filter(matcher.match, mido.get_output_names()))
# all to get the name of the thing we just made
output_name = newList[0]

# starting time
last_time = ret_mili_time()

print('System start/restart - ' + str(datetime.datetime.now()))

#Switches to be connected to pins and 3.3v pin
GPIO.setup(B1, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
GPIO.setup(B2, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
GPIO.setup(B3, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
GPIO.setup(B4, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)
GPIO.setup(B5, GPIO.IN, pull_up_down = GPIO.PUD_DOWN)

# Set up the GPIO channels
#GPIO.setup(dt_pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) # dt
#GPIO.setup(sw_pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) # sw
#GPIO.setup(clk_pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN) # clk

#This function will run when the button is triggered
def Notifier(channel):
  if channel==B1:
    print('Button 1')
  elif channel==B2:
    print('Button 2')          
  elif channel==B3:
    print('Button 3')
  elif channel==B4:
    print('Button 4')
  elif channel==B5:
    print('Button 5')
    
GPIO.add_event_detect(B1, GPIO.RISING)
GPIO.add_event_detect(B2, GPIO.RISING)
GPIO.add_event_detect(B3, GPIO.RISING)
GPIO.add_event_detect(B4, GPIO.RISING)
GPIO.add_event_detect(B5, GPIO.RISING)

# rotary encoder
#GPIO.add_event_detect(clk_pin,GPIO.BOTH,callback=rotary_callback)

def send_cc(channel, ccnum, val):
  msg = mido.Message('control_change', channel=channel, control=ccnum, value=val)
  output = mido.open_output(output_name)
  output.send(msg)

def ret_mili_time():
  current_milli_time = int(round(time.time() * 1000))
  return current_milli_time

while True:
  #print('Looping')
  if GPIO.event_detected(B1):
    time.sleep(0.005) # debounce for 5mSec
    # only show valid edges
    if GPIO.input(B1)==1:
      Notifier(B1)
      send_cc(0, 65, 127)
  if GPIO.event_detected(B2):
    time.sleep(0.005)
    if GPIO.input(B2)==1:
      Notifier(B2)
      send_cc(0, 66, 127)
  if GPIO.event_detected(B3):
    time.sleep(0.005)
    if GPIO.input(B3)==1:
      Notifier(B3)
      send_cc(0, 67, 127)
  if GPIO.event_detected(B4):
    time.sleep(0.005)
    if GPIO.input(B4)==1:
      Notifier(B4)
      send_cc(0, 68, 127)
  if GPIO.event_detected(B5):
    time.sleep(0.005)
    if GPIO.input(B5)==1:
      Notifier(B5)
      send_cc(0, 69, 127)
  time.sleep(0.5)
        
GPIO.cleanup()

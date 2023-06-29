#!/usr/bin/python
import time
from Batbot_Control.py import BatBot

print("Servo Test Entered. This unit testing case is still under development")
time.sleep(3)


#---------------------- Main Code Below ---------------------------------

bb = BatBot() #this code is bring run on the Jetson, so there is no need for an external computer to "need to connect to the Jetson" since it is stand alone

bb.moveEar(180, 'A')
time.sleep(3000)
bb.moveEar(0, 'A')

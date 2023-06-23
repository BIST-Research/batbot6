## Copy
# Best Best version ever, with filter
# 5EA821FD53374E4D4C202020FF0F2204
# Import packages
import serial
import serial.tools.list_ports
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib
import sys
import bb_log

from m4 import M4

matplotlib.use('TkAgg')
#%matplotlib notebook
          
bat_log = bb_log.get_log()

serial_number = int(sys.argv[1])
page_size = int(sys.argv[2])
baud = int(sys.argv[3])
# Communicate with Arduino
ser = M4(serial_number, page_size, baud, bat_log)

# Create empty list to store the data
sensor1_data = []
sensor2_data = []
fig, ax = plt.subplots()

# Animation function
def update_plot(frame):
    # Read the data from the serial port
    line = ser.sercom.readline().decode().strip()
    data = line.split(',')

    
    sensor1 = float(data[0])
    sensor2 = float(data[1])
  
    sensor1_data.append(sensor1)
    sensor2_data.append(sensor2)

    # Update the plot
    ax.clear()
    ax.plot(sensor1_data[-50:])
    ax.plot(sensor2_data[-50:])

ani = FuncAnimation(fig, update_plot, interval=1)

plt.show()

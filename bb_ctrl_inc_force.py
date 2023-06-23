import matplotlib.pyplot as plt
import numpy as np
import serial
import serial.tools.list_ports
import time
import math
import os
import sys
import logging
import yaml

import bb_log

from datetime import datetime
from m4 import M4

bat_log = bb_log.get_log()

def bin2dec(bin_data):
    return [((y << 8) | x) for x, y in zip(bin_data[::2], bin_data[1::2])]
    
def get_timestamp_now():
    return datetime.now().strftime('%Y%m%d_%H%M%S%f')[:-3]


class BatBot:
    
    def __init__(self):
    
        self.parent_directory = os.path.dirname(os.path.abspath(__file__))
                
        with open('bb_conf.yaml') as fd:
        
            bb_conf = yaml.safe_load(fd)
                        
            self.echo_sercom = M4(bb_conf['echo']['serial_number'], bb_conf['echo']['page_size'], bat_log)
            
            self.force_sercom = M4(bb_conf['force']['serial_number'], bb_conf['force']['page_size'], bat_log)
            
            self.data_directory = self.parent_directory + f"/{bb_conf['data_directory']}"
            
            if not os.path.exists(self.data_directory):
                os.makedirs(self.data_directory)
                
            self.run_directory = self.data_directory + f"/{get_timestamp_now()}"
            os.makedirs(self.run_directory)
        
    def start_run(self):
        self.echo_sercom.write([0x10])
        self.force_sercom.write([0x10])
        
    def wait_run(self):
    
        echo_ready = False
        force_ready = False
        
        while True:
        
            self.echo_sercom.write([0x20])
                
            if self.echo_sercom.read(1) == b'\x01':
                break
                
        while True:
            
            self.force_sercom.write([0x20])
                
            if self.force_sercom.read(1) == b'\x01':
                break
                
    def _get_data(self, inst, channel):
        
        inst.write([0x30 | channel])
        
        npages = inst.read(1)[0]
        
        return inst.read(2 * npages * inst.page_size)
        
    def run(self):
        
        self.start_run()
        self.wait_run()
        
        echo_right = self._get_data(self.echo_sercom, 0x00)
        echo_left = self._get_data(self.echo_sercom, 0x01)
        
        force_right = self._get_data(self.force_sercom, 0x00)
        force_left = self._get_data(self.force_sercom, 0x01)
                
        timestamp = get_timestamp_now()
        
        dump_path = f"{self.run_directory}/{timestamp}.bin"
        
        with open(dump_path, 'wb') as fp:
            fp.write(echo_right + echo_left + force_right + force_left)
            #fp.write(echo_right + echo_left)

        
        return [echo_right, echo_left, force_left, force_right]
        #return [echo_right, echo_left]
        
    def send_amp_stop(self):
        self.echo_sercom.write([0xff])
        
    def send_amp_start(self):
        self.echo_sercom.write([0xfe])

    
#Jilun Code for Analysing Force Data
# Constrain function
def constrain(value):
    return min(max(value, 0), 100)

# Filter Function
numReadings = 200 
readings1 = [0] * numReadings   
readings2 = [0] * numReadings
total1 = 0       
total2 = 0
readIndex = 0  
def smooth(reading, readings, total, numReadings):
    global readIndex
    total = total - readings[readIndex] 
    readings[readIndex] = reading          
    total = total + readings[readIndex]    
    readIndex = (readIndex + 1) % numReadings 
    return total / numReadings 
        

if __name__ == '__main__':


    nruns = 0
    plot_interval = 3
    
    if len(sys.argv) < 2:
        nruns = -1
    else:
        nruns = int(sys.argv[1])
    
    instance = BatBot()
    

    nruns_idx = 0
    time_start = datetime.now()

    f, ((echo_left_ax, echo_right_ax), (force_left_ax, force_right_ax)) = plt.subplots(2, 2, sharey=False)

    #f, (echo_left_ax, echo_right_ax) = plt.subplots(1, 2, sharey=False)

    echo_left_total, echo_right_total, force_left_total, force_right_total = [],[],[],[]
    #echo_left_specgram, echo_right_specgram = [],[]
    
    #echo_left_total, echo_right_total = [],[]
    
    instance.send_amp_start()
    
    while True:

        try:
            if nruns_idx == nruns:
                break
                
            raw_data = instance.run()
            unraw_data = []
                        
            for r in raw_data:
                unraw_data.append(bin2dec(r))
                
            echo_right, echo_left, force_left, force_right = unraw_data
            #Transforming the data
            for i in range(len(force_left)):
                val1 = (force_left[i]*-1+2560.0)/0.00001
                val2 = (force_right[i]*-1+2560.0)/0.00001
                val1 = smooth(val1, readings1, total1, numReadings)
                val2 = smooth(val2, readings2, total2, numReadings)
                force_left[i] = val1
                force_right[i] = val2
            #Adding to total
            echo_left_total = np.append(echo_left_total, echo_left)
            echo_right_total = np.append(echo_right_total, echo_right)
            force_left_total = np.append(force_left_total, force_left)
            force_right_total = np.append(force_right_total, force_right)
            
            if nruns_idx % plot_interval == 0 and nruns_idx != 0:
                elapsed = datetime.now() - time_start

                 # Clear previous lines (for speed)
                echo_left_ax.clear()
                echo_right_ax.clear()
                force_left_ax.clear()
                force_right_ax.clear()

                 # Plot
                echo_left_ax.plot(echo_left_total)
                echo_right_ax.plot(echo_right_total)
                force_left_ax.plot(force_left_total)
                force_right_ax.plot(force_right_total)

                # Leave a status message
                echo_left_ax.set_title('{} echo runs - {}'.format(nruns_idx, str(elapsed)[:-7]))
                echo_right_ax.set_title('{} runs/min'.format(int(nruns_idx/max(elapsed.seconds,1)*60)))
                echo_left_ax.set_ylim(0, 4096)
                echo_right_ax.set_ylim(0, 4096)
                force_left_ax.set_title('Force Data')
                force_left_ax.set_ylim(0, 100)
                force_right_ax.set_ylim(0, 100)
                
                #Showing the plot
                plt.show(block=False)

                #Deleting old data
                echo_right_total, echo_left_total, force_left_total, force_right_total = [],[],[],[]
                #echo_right_total, echo_left_total = [],[]

                # Show the plot without blocking (there's no separate UI
                # thread)
                plt.pause(0.001)

                       
            nruns_idx += 1
                
        except KeyboardInterrupt:
            print("")
            bat_log.info("Interrupted")
            break
        
    instance.send_amp_stop()
    time_finish = datetime.now() - time_start
    bat_log.info(f"{nruns} runs took {time_finish}")
        
        
            
            
            
        
        
        
    
    

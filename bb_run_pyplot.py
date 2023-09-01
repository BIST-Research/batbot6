#
# Date created: 4/3/23
# Author: Ben Westcott
#

import numpy as np
import serial
import serial.tools.list_ports
import time
import math
import os
import sys
import logging
import yaml
from bb_plot import BBPlotter

from scipy import signal
from batbot import BatBot
from batbot import bin2dec

import bb_log

from datetime import datetime
from m4 import M4

bat_log = bb_log.get_log()

if __name__ == '__main__':
    
    nruns = 0
    
    if len(sys.argv) < 2:
        nruns = -1
    else:
        nruns = int(sys.argv[1])
        
    bb = BatBot(bat_log)
    
    # first calibration run
    initial_calib_data = [bin2dec(r) for r in bb.run()]
    
    plotter = BBPlotter(bat_log, bb.echo_book, initial_calib_data)
    
    nruns_idx = 0
    time_start = datetime.now()
    
    echo0_total, echo1_total = [],[]
    
    

    bb.send_sweep_freqs()

    bb.send_amp_start()
    
    while True:

        try:
            if nruns_idx == nruns:
                break
            
            echo_data = [bin2dec(r) for r in bb.run()]

            echo1_total = np.append(echo1_total, echo_data[1])
            echo0_total = np.append(echo0_total, echo_data[0])
            
            if nruns_idx % plotter.echo_calibration_interval == 0:
                plotter.echo_calibration_run(echo_data)

            if nruns_idx % plotter.echo_plot_interval == 0:
                time_elapsed = datetime.now() - time_start
                
                plotter.update_super_title(time_elapsed, nruns_idx)
                plotter.update_plots([echo0_total, echo1_total])
                
                echo0_total, echo1_total = [],[]
                

            nruns_idx += 1   
        

        except KeyboardInterrupt:
            print("")
            bat_log.info("Interrupted")
            break

    bb.send_amp_stop()
    time_finish = datetime.now() - time_start
    bat_log.info(f"{nruns} runs took {time_finish}")

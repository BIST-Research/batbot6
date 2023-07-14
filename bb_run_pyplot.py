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
from scipy import signal
from batbot import BatBot
from batbot import bin2dec

import bb_log

from datetime import datetime
from m4 import M4

bat_log = bb_log.get_log()

if __name__ == '__main__':
    
    nruns = 0

    if(len(sys.argv)) < 2:
        nruns =  -1
    else:
        nruns = int(sys.argv[1])
    
    bb = BatBot(bat_log)
    
    echo_plot_interval = bb.echo_book['plot_interval']
    
    echo_sampling_rate = 1/bb.echo_book['sampling_period']
    
    echo_n_fft = bb.echo_book['n_fft']
    
    echo_y_max = bb.echo_book['y_max']
    echo_y_min = bb.echo_book['y_min']
    
    echo_y_lims = [bb.echo_book['y_min'], bb.echo_book['y_max']]
    
    echo_y_spec_lims = [bb.echo_book['y_spec_min'], bb.echo_book['y_spec_max']]
    
    nruns_idx = 0
    time_start = datetime.now()

    f, ((echo_left_ax, echo_left_spec), (echo_right_ax, echo_right_spec)) = plt.subplots(2, 2, sharey=False)

    echo_left_total, echo_right_total = [],[]
    
    bb.send_sweep_freqs()

    bb.send_amp_start()

    while True:

        try:
            if nruns_idx == nruns:
                break
            
            raw_data = bb.run()
            unraw_data = []

            for r in raw_data:
                unraw_data.append(bin2dec(r))

            echo_right, echo_left = unraw_data
            #sos = signal.butter(2, [40E3, 150E3], 'bp', fs=r_samp, output='sos')
            #filt_right = signal.sosfilt(sos, echo_right)
            #filt_left = signal.sosfilt(sos, echo_left)

            #print(echo_right)
            #echo_left_total = np.append(echo_left_total, echo_left)
            #echo_right_total = np.append(echo_right_total, echo_right)
            echo_left_total = np.append(echo_left_total, echo_left)
            echo_right_total = np.append(echo_right_total, echo_right)

            if nruns_idx % echo_plot_interval == 0:

                elapsed = datetime.now() - time_start

                echo_left_ax.clear()
                echo_right_ax.clear()

                echo_left_ax.plot(echo_left_total)
                echo_left_ax.set_ylim(echo_y_lims)

                echo_right_ax.plot(echo_right_total)
                echo_right_ax.set_ylim(echo_y_lims)

                echo_left_ax.set_title('{} echo runs - {}'.format(nruns_idx, str(elapsed)[:-7]))
                echo_right_ax.set_title('{} runs/min'.format(int(nruns_idx/max(elapsed.seconds,1)*60)))

                echo_right_spec.specgram(echo_right_total, echo_n_fft, echo_sampling_rate)
                
                echo_right_spec.set_ylim(echo_y_spec_lims)

                
                echo_left_spec.specgram(echo_left_total, echo_n_fft, echo_sampling_rate)
                echo_left_spec.set_ylim(echo_y_spec_lims)

                plt.show(block=False)

                echo_right_total, echo_left_total = [],[]

                plt.pause(0.001)

            nruns_idx += 1   
        

        except KeyboardInterrupt:
            print("")
            bat_log.info("Interrupted")
            break

    bb.send_amp_stop()
    time_finish = datetime.now() - time_start
    bat_log.info(f"{nruns} runs took {time_finish}")

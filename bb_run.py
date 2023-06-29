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

            self.echo_sercom = M4(bb_conf['echo']['serial_number'], bb_conf['echo']['num_adc_samples'], bat_log)

            self.data_directory = self.parent_directory+ f"/{bb_conf['data_directory']}"

            if not os.path.exists(self.data_directory):
                os.makedirs(self.data_directory)

            self.run_directory = self.data_directory + f"/{get_timestamp_now()}"
            os.makedirs(self.run_directory)

    def start_run(self):
        self.echo_sercom.write([0x10])
            
    def run(self):
        
        self.start_run()
        
        # 2 for adc0 and adc1
        # Other 2 because we get a bytestream, but ADCs capture halfwords (uint16_t)
        raw = self.echo_sercom.read(2 * 2 * self.echo_sercom.num_adc_samples)
        
        timestamp = get_timestamp_now()
        
        dump_path = f"{self.run_directory}/{timestamp}.bin"
        
        # dump binary
        with open(dump_path, 'wb') as fp:
            fp.write(raw)
        
        return [raw[:len(raw)//2], raw[len(raw)//2:]]

    def send_amp_stop(self):
        self.echo_sercom.write([0xff])

    def send_amp_start(self):
        self.echo_sercom.write([0xfe])

if __name__ == '__main__':
    
    nruns = 0
    plot_interval = 3

    if(len(sys.argv)) < 2:
        nruns =  -1
    else:
        nruns = int(sys.argv[1])
    
    instance = BatBot()

    r_samp = 1/(2.4E-6)
    n_fft = 1024
    y_max = 4096

    nruns_idx = 0
    time_start = datetime.now()

    f, ((echo_left_ax, echo_left_spec), (echo_right_ax, echo_right_spec)) = plt.subplots(2, 2, sharey=False)

    echo_left_total, echo_right_total = [],[]

    instance.send_amp_start()

    while True:

        try:
            if nruns_idx == nruns:
                break
            
            raw_data = instance.run()
            unraw_data = []

            for r in raw_data:
                unraw_data.append(bin2dec(r))

            
            
            echo_right, echo_left = unraw_data

            #print(echo_right)
            echo_left_total = np.append(echo_left_total, echo_left)
            echo_right_total = np.append(echo_right_total, echo_right)

            if nruns_idx % plot_interval == 0 and nruns_idx != 0:

                elapsed = datetime.now() - time_start

                echo_left_ax.clear()
                echo_right_ax.clear()

                echo_left_ax.plot(echo_left_total)
                echo_left_ax.set_ylim(0, y_max)

                echo_right_ax.plot(echo_right_total)
                echo_right_ax.set_ylim(0, y_max)

                echo_left_ax.set_title('{} echo runs - {}'.format(nruns_idx, str(elapsed)[:-7]))
                echo_right_ax.set_title('{} runs/min'.format(int(nruns_idx/max(elapsed.seconds,1)*60)))

                echo_right_spec.specgram(echo_right_total, n_fft, r_samp, noverlap=n_fft//2)
                echo_right_spec.set_ylim(0, r_samp//2)
                
                echo_left_spec.specgram(echo_left_total, n_fft, r_samp, noverlap=n_fft//2)
                echo_right_spec.set_ylim(0, r_samp//2)       

                plt.show(block=False)

                echo_right_total, echo_left_total = [],[]

                plt.pause(0.001)

            nruns_idx += 1   
        

        except KeyboardInterrupt:
            print("")
            bat_log.info("Interrupted")
            break

    instance.send_amp_stop()
    time_finish = datetime.now() - time_start
    bat_log.info(f"{nruns} runs took {time_finish}")
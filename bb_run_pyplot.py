import matplotlib.pyplot as plt
import matplotlib.gridspec as gs
import matplotlib.colors as colors
import matplotlib.ticker as mticker
from matplotlib import mlab as mlab
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

def calibration_run(data, Fs, NFFT, noverlap):
    cs, cf, ct = mlab.specgram(data, Fs=Fs, NFFT=NFFT, noverlap=noverlap)
    return cs
    
def get_ylims(data, padding):
    return min(data) - padding, max(data) + padding

if __name__ == '__main__':
    
    nruns = 0

    if(len(sys.argv)) < 2:
        nruns =  -1
    else:
        nruns = int(sys.argv[1])
    
    bb = BatBot(bat_log)
    
    echo_plot_interval = bb.echo_book['plot_interval']
    
    echo_sampling_rate = 1/bb.echo_book['sampling_period']
    echo_adc_samples = bb.echo_book['num_adc_samples']
    
    echo_n_fft = bb.echo_book['n_fft']
    echo_n_overlap = bb.echo_book['n_overlap']
    
    echo_y_time_padding = bb.echo_book['y_time_padding']
    echo_y_spec_padding = bb.echo_book['y_spec_padding']
        
    echo_calibration_interval = bb.echo_book['calibration_interval']
    
    echo_spec_cmap = bb.echo_book['spec_color_map']
    
    nruns_idx = 0
    time_start = datetime.now()
    
    figure = plt.figure(figsize=(25,10))
    plt_grid = gs.GridSpec(2, 4, width_ratios=[10, 10, 0.1, 10], wspace=0.4)
    
    echo_left_ax = figure.add_subplot(plt_grid[0,0])
    echo_right_ax = figure.add_subplot(plt_grid[1,0])
    
    echo_left_spec = figure.add_subplot(plt_grid[0,1])
    echo_right_spec = figure.add_subplot(plt_grid[1,1])

    echo_left_spec_cax = figure.add_subplot(plt_grid[0,2])
    echo_right_spec_cax = figure.add_subplot(plt_grid[1,2])
    
    echo_left_psd = figure.add_subplot(plt_grid[0,3])
    echo_right_psd = figure.add_subplot(plt_grid[1,3])
    
    echo_left_total, echo_right_total = [],[]

    bb.send_sweep_freqs()

    bb.send_amp_start()
    
    # first calibration run
    calib_right, calib_left = [bin2dec(r) for r in bb.run()]
    
    calib_sr = calibration_run(calib_right, echo_sampling_rate, echo_n_fft, echo_n_overlap)
    calib_sl = calibration_run(calib_left, echo_sampling_rate, echo_n_fft, echo_n_overlap)
    
    sos = signal.butter(10, [10E3, 200E3], 'bp', fs=echo_sampling_rate, output='sos')
    
    echo_record_period = echo_adc_samples/echo_sampling_rate
    
    echo_x_range_ms = np.arange(0, echo_record_period, 0.002)

    
    while True:

        try:
            if nruns_idx == nruns:
                break
            
            echo_right, echo_left = [bin2dec(r) for r in bb.run()]
            #echo_right = signal.sosfilt(sos, echo_right)

            #print(echo_right)
            #echo_left_total = np.append(echo_left_total, echo_left)
            #echo_right_total = np.append(echo_right_total, echo_right)
            echo_left_total = np.append(echo_left_total, echo_left)
            echo_right_total = np.append(echo_right_total, echo_right)
            
            if nruns_idx % echo_calibration_interval == 0:
                calib_sr = calibration_run(echo_right, echo_sampling_rate, echo_n_fft, echo_n_overlap)
                calib_sl = calibration_run(echo_left, echo_sampling_rate, echo_n_fft, echo_n_overlap)

            if nruns_idx % echo_plot_interval == 0:
            
                elapsed = datetime.now() - time_start

                echo_left_ax.clear()
                echo_right_ax.clear()
                echo_left_spec.clear()
                echo_right_spec.clear()
                echo_left_spec_cax.clear()
                echo_right_spec_cax.clear()
                echo_right_psd.clear()
                echo_left_psd.clear()
                
                echo_left_ax.set_xlabel('Time (usec)')
                echo_left_ax.set_ylabel('Amplitude (mV)')
                echo_right_ax.set_xlabel('Time (usec)')
                echo_right_ax.set_ylabel('Amplitude (mV)')
                echo_left_spec.set_xlabel('Time (sec)')
                echo_left_spec.set_ylabel('Frequency (Hz)')
                echo_right_spec.set_xlabel('Time (sec)')
                echo_right_spec.set_ylabel('Frequency (Hz)')

                
                echo_left_ax.set_title("ADC0")
                echo_left_ax.plot(echo_left_total)
                echo_left_ax.set_ylim(get_ylims(echo_left_total, echo_y_time_padding))
                #echo_left_ax.set_xticklabels(echo_x_range_ms)


                echo_right_ax.set_title("ADC1")
                echo_right_ax.plot(echo_right_total)
                echo_right_ax.set_ylim(get_ylims(echo_right_total, echo_y_time_padding))
                #echo_right_ax.set_xticklabels(echo_x_range_ms)

                
                figure.suptitle(f"{nruns_idx} echos - {str(elapsed)[:-7]}\n{int(nruns_idx/max(elapsed.seconds,1)*60)} echos/min")

                #sr, fr, tr, imr = echo_right_spec.specgram(echo_right_total, echo_n_fft, echo_sampling_rate, cmap='jet', vmin=min_val_r, vmax=max_val_r)
                
                
                sr, fr, tr = mlab.specgram(echo_right_total, NFFT=echo_n_fft, Fs=echo_sampling_rate, noverlap=echo_n_overlap)
                
                pcmr = echo_right_spec.pcolormesh(tr, fr, sr, cmap=echo_spec_cmap, shading='auto', norm=colors.LogNorm(vmin=calib_sr.min(), vmax=calib_sr.max()))
                
                echo_right_spec.set_ylim(10E3, 200E3)
                #echo_right_spec.set_xticklabels(echo_x_range_ms)

                sl, fl, tl = mlab.specgram(echo_left_total, NFFT=echo_n_fft, Fs=echo_sampling_rate, noverlap=echo_n_overlap)
                
                pcml = echo_left_spec.pcolormesh(tl, fl, sl, cmap=echo_spec_cmap, shading='auto', norm=colors.LogNorm(vmin=calib_sl.min(), vmax=calib_sl.max()))
                
                echo_left_spec.set_ylim(10E3, 200E3)
                #echo_left_spec.set_xticklabels(echo_x_range_ms)

                figure.colorbar(pcmr, cax=echo_right_spec_cax)
                figure.colorbar(pcml, cax=echo_left_spec_cax)
                
                echo_right_psd.clear()
                                
                echo_right_psd.psd(echo_right_total, echo_n_fft, echo_sampling_rate, noverlap=echo_n_overlap, color='g')
                
                echo_left_psd.clear()
               
                echo_left_psd.psd(echo_left_total, echo_n_fft, echo_sampling_rate, noverlap=echo_n_overlap, color='g')
                
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

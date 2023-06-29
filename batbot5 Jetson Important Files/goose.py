from matplotlib.pyplot import figure, plot, show, draw, pause, subplot, xlabel, ylabel, title
from datetime import datetime
import serial
import struct
import math
import matplotlib.pyplot as plt
import time
import numpy as np

# Show all packets
VERBOSE = 0

# Native USB port of Due (use Device Manager to find)
PORT = '/dev/ttyACM1'

def read(ser, n):
    data = ser.read(size=n)
    if VERBOSE:
        print('> ' + ''.join('{:02x} '.format(b) for b in data))
    return data

def write(ser, data):
    ser.write(bytearray(data))
    if VERBOSE:
        print('< ' + ''.join('{:02x} '.format(b) for b in data))

#from scipy.signal import butter, lfilter, freqz


#def butter_lowpass(cutoff, fs, order=5):
#    return butter(order, cutoff, fs=fs, btype='high', analog=False)

#def butter_lowpass_filter(data, cutoff, fs, order=5):
#    b, a = butter_lowpass(cutoff, fs, order=order)
#    y = lfilter(b, a, data)
#   return y



f  = []

#filter2 is a bandpass filter from 30 to 70 kHz
filt = np.load('filter2.npy')

#used for printing in distance instead of time
distanceArray = np.array(list(range(0,10000))).astype('float')*(0.17/400.0)

def main():
    ser = serial.Serial()
    ser.port = PORT
    ser.baudrate = 115200 # arbitrary
    ser.setRTS(True)
    ser.setDTR(True)
    ser.open()



    
    print('Communicating over port {}'.format(ser.name))

    # Hello
    write(ser, [0, 0, 0, 0, 0])
    opcode, response_len = struct.unpack('<BI', read(ser, 5))
    if opcode != 0x80 or response_len != 2:
        print('Unexpected packet! opcode=0x{:02x}, response_len={}'
               .format(opcode, response_len))
        return

    version = struct.unpack('<BB', read(ser, response_len))
    print('Connected to device (version {})'.format(version))

    '''
    # Queue data
    chirp = generate_chirp()
    queue_data = [0] * (1 + 4 + 4096 + 1)

    queue_data[0] = 1
    queue_data[1] = (4096 >> 0) & 0xff
    queue_data[2] = (4096 >> 8) & 0xff
    queue_data[3] = (4096 >> 16) & 0xff
    queue_data[4] = (4096 >> 24) & 0xff
    for i in range(2048):
        queue_data[5 + 2*i + 0] = (chirp[i] >> 0) & 0xff
        queue_data[5 + 2*i + 1] = (chirp[i] >> 8) & 0xff
    
    write(ser, queue_data)
    opcode, response_len = struct.unpack('<BI', read(ser, 5))
    if opcode != 0x81 or response_len != 0:
        print('unexpected! opcode=0x{:02x}, response_len={:04}'
               .format(opcode, response_len))
        return

    print('queue_data'.format(version))
    '''

    
    while True:
        try:
            time.sleep(0.3)
            # Initiate data collection
            write(ser, [2, 0, 0, 0, 0])


            opcode, response_len = struct.unpack('<BI', read(ser, 5))
            #if opcode != 0x82 or response_len != 0:
            #    print('Unexpected packet! opcode=0x{:02x}, response_len={}'
            #           .format(opcode, response_len))
            #    return

            print('Collecting data... ', end='')

            # Response that data collection has finished
            opcode, response_len = struct.unpack('<BI', read(ser, 5))
            if opcode != 0x82:
                print('Unexpected packet! opcode=0x{:02x}, response_len={}'
                       .format(opcode, response_len))
                return

            print('done! ({} data points)'.format(response_len // 2))

            # Record the data
            output_path = datetime.now().strftime('./static/%Y%m%d_%H%M%S.txt')
            #output_path = "output.txt"
            raw_data = read(ser, response_len)
            full_data = []
            with open(output_path, 'w') as f:
                for i in range(0, len(raw_data), 2):
                    data = raw_data[i] | (raw_data[i + 1] << 8)
                    full_data.append(data)
                    f.write('{}\n'.format(data))

                full_data1 = full_data[0:20002:2]
                full_data2 = full_data[1:20001:2]
                #full_data1 = full_data[0:9999];
                #full_data2 = full_data[10000:];
                m1 = sum(full_data1)/len(full_data1);
                m2 = sum(full_data2)/len(full_data1);
                for i in range(len(full_data1)):
                    full_data1[i] -= m1
                    full_data2[i] -= m2
                max1 = max(full_data1)
                max2 = max(full_data2)
                full_data1 = np.array(full_data1).astype('float')
                full_data2 = np.array(full_data2).astype('float')
                full_data1 = full_data1/max1
                full_data2 = full_data2/max2

                filt_data2 = np.convolve(full_data2, filt,mode='same')

                #plt.figure()
                plt.clf()
                subplot(2,1,1)
                #plot(distanceArray,full_data1,'b-')
                #pause(1)
                #ylabel('Amplitude (nrm.)')
                #xlabel('Distance of Echo [m]')
                #title('Left')
                #subplot(2,2,2)
                plot(distanceArray[2000:9900],filt_data2[2000:9900])
                title('Mic')
                xlabel('Distance of Echo [m]')
                #ylabel('Amplitude')
                #ax3 = subplot(2,2,3)
                #ax3.specgram(full_data1,Fs=400, NFFT = 1024, noverlap = 1000, cmap = 'jet')
                #ax3.set_title("Spectrogram")
                #ax3.set_ylim(15,120)
                #ax3.set_ylabel('Frequency [kHz]')
                #ax3.set_xlabel('Time [ms]')
                ax4 = subplot(2,1,2)
                spec,b,c,d = ax4.specgram(filt_data2[2000:9900],Fs=400, NFFT = 1024, noverlap = 1000, cmap = 'jet')
                logged = 10*np.log10(spec)
                logged -= np.max(logged)
                logged[np.where(logged<-50)]=-50
                ax4.pcolormesh(c,b,logged,cmap='jet')
                ax4.set_ylim(40,60)
                ax4.set_xlabel('Time [ms]')
                
                plt.tight_layout()
                pause(0.5)
                plt.show(block=False)
                #plt.close()
                
           
                    
        except KeyboardInterrupt:
            break

    print('Closing connection')
    ser.close()

if __name__ == '__main__':
    main()

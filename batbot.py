import os
import yaml
from datetime import datetime
import serial
import serial.tools.list_ports
from m4 import M4
import threading

def get_timestamp_now():
    return datetime.now().strftime('%Y%m%d_%H%M%S%f')[:-3]

def bin2dec(bin_data):
    return [((y << 8) | x) for x, y in zip(bin_data[::2], bin_data[1::2])]

class AsyncWrite(threading.Thread):
    def __init__(self, filename, buf):
        
        threading.Thread.__init__(self)
        
        self.filename = filename
        self.buf = buf
        
    def run(self):
        with open(self.filename, 'wb') as fp:
            fp.write(self.buf)
            

class BatBot:

    def __init__(self, bat_log):
        
        self.parent_directory = os.path.dirname(os.path.abspath(__file__))
                
        with open('bb_conf.yaml') as fd:

            bb_conf = yaml.safe_load(fd)
            
            self.echo_book = bb_conf['echo']

            self.echo_sercom = M4(self.echo_book['serial_number'], self.echo_book['num_adc_samples'], bat_log)

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
        

        with open(dump_path, 'wb') as fp:
            fp.write(raw)
        
        # dump binary
        #return raw
        return [raw[:len(raw)//2], raw[len(raw)//2:]]

    def send_amp_stop(self):
        self.echo_sercom.write([0xff])

    def send_amp_start(self):
        self.echo_sercom.write([0xfe])
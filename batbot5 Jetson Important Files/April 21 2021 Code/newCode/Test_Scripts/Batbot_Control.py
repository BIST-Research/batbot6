"""
Performs runs indefinitely, saving results and occassionally plotting them
"""

# import matplotlib.pyplot as plt
from datetime import datetime
import numpy as np
import serial
import serial.tools.list_ports
import struct
import time
import math
import os

# Number of runs between plot updates. Plotting almost doubles the
# duration of each run, so keep this large
PLOT_INTERVAL = 10

# COM port of the M4; leave None for cross-platform auto-detection
# PORT = "/dev/ttyTHS1"  #hardware pins 8 and 10
PORT = None




class BatBot:
    """
    Bare minimum example (collect a single run):

        bb = BatBot()
        left, right = bb.collect_data()
        plt.plot(left)
        plt.plot(right)
        plt.show()

    """
    
    def __init__(self, port=None):
        """
        Connect to a device

        :param port: optional, COM port of M4
        """
        # Try to deduce the serial port
        if not port:
            port = self.guess_port()

        # Connect to the device
        self.ser = serial.Serial(port)

        # Attempt to reset the device
        self.reset()

        print(f'Connected to M4 on {port}')

    @staticmethod
    def guess_port():
        """
        Discover any locally connected M4s

        :return: COM port name of discovered M4
        """
        # Vendor and product ID of SAMD51 USB Host
        VID = 0x239a
        PID = 0x8031

        # Try to detect any M4s by USB IDs
        available_ports = serial.tools.list_ports.comports()
        possible_ports = [port.device for port in available_ports \
                          if (port.vid == VID and port.pid == PID)]

        # Yell at the user if no M4 was found
        if not any(possible_ports):
            raise Exception('M4 not found: verify that it is properly connected')

        return possible_ports[0]


    def reset(self):
        """
        Trigger a hardware reset using the serial's DTR; highly
        dependent on hardware configuration
        """
        self.ser.setDTR(False)
        time.sleep(1)
        self.ser.flushInput()
        self.ser.setDTR(True)


    def write(self, packet):
        self.ser.write(packet)


    def read(self, length):
        return self.ser.read(length)


    def _start_run(self):
        self.write([0x10])


    def set_motion_profile(self,moveservo,movemotor,movevalves,servocode):
        """
        Define and send a motion profile to the M4 for the TVCU to use for valves 
        BYTE 1: Valve motion profile (which to turn on and off)
        BYTE 2: PWM profile (duty cycle to use)
        """
        if movevalves == 1:
            TVCU_OPCODE = [0x01] #TODO: This seem hard-coded to specific motion profiles. Is this correct?? I thought these values
            #based on the documentation, dictated certain valve movements
            valve_motion_profile = [0xFF]
            valve_pwm_profile = [0xFF]
            self.write(TVCU_OPCODE)
            self.write(valve_motion_profile)
            self.write(valve_pwm_profile)

        """
        Define and send a motion profile for the TMCU to use for servos and motors
        BYTE 1: Servo
        BYTE 2: Stepper (R)
        BYTE 3: Stepper (L)
        """
        if moveservo == 1:
            print("TRANSMITTIG TMCU SERVO CODE")
            TMCU_SERVO_OPCODE = [0x02]
            servo_motion_profile = servocode
            self.write(TMCU_SERVO_OPCODE)
            self.write(servo_motion_profile)

        if movemotor == 1:
            TMCU_MOTOR_OPCODE =[0x03]
            r_stepper_profile = [0xFF]
            l_stepper_profile = [0xFF]
            self.write(TMCU_MOTOR_OPCODE)      
            self.write(r_stepper_profile)
            self.write(l_stepper_profile)


    def _wait_for_run_to_complete(self):
        while True:
            self.write([0x20])

            if self.read(1) == b'\x01':
                return


    def _get_data(self, ch):
        self.write([0x30 | ch])
        
        num_pages = self.read(1)[0]
        raw_data = self.read(num_pages * 8192)  # This contains the amount of data to be collected per page
        return [((y << 8) | x) for x, y in zip(raw_data[::2], raw_data[1::2])]


    def collect_data(self):
        """
        Perform a full data collection run

        :return: collected data split into separate channels
        """
        # Set the motion profile before data collection

        #TODO: If I (Henry) recall correctly, the goal was to move this out so we could implement more comprehensive unit testing
        # self.set_motion_profile(1,0,0,[0xF0]) 

        # Start the run by sending 0x10 to the M4 to signal start of data collection
        self._start_run()

        # Query the M4 status in a loop
        self._wait_for_run_to_complete()

        # Once the status is OK, get the data from the M4
        left_ch = self._get_data(ch=0)
        right_ch = self._get_data(ch=1)

        return (left_ch, right_ch)

#---------------------------- More Methodization in order to increase ability to run unit testing --------------------------------
    def moveEar(self, degrees, which): #degrees is self-explanatory, but "which" used A and B
        #Example Code
            # self.set_motion_profile(1,0,0,[0xF0]) -- Looks like it was meant to initialize both the left and right ears
            # the opcode works as follows: 
            #   The first '1' tells the program that we are expecting a servo code. Treated like a boolean
            #   The next number, '0', is treated like a boolean, no motor code expected
            #   The third number dictates if the valves are moved or not. Seems to be a boolean
            #set_motion_profile(moveservo,movemotor,movevalves,servocode) #moves servo 1 to the proper positon
        
        # First, convert int to appropriate binary value
        MAX_BINARY = int(0b111111)
        DEGREES_TOTAL = int(180)
        servoPositionBinary = bin(round((int(degrees)/DEGREES_TOTAL) * MAX_BINARY))
        print ("Final binary value for designated input: " + str(servoPositionBinary))
        # Determine where this code needs to go
        servoBinary = bin(0)
        if which=="a":
            print("Servo A")
            servoBinary += bin(1) << 8
        elif which == "b":
            print("Servo B")
            servoBinary += bin(1) << 7
        # Add servoPositionBinary to the servoBinary command
        servoBinary += servoPositionBinary
        self.set_motion_profile(1,0,0, servoBinary)
        return servoBinary
    

#------------------------------------------- Main Loop Below -----------------------------------------------

if __name__ == '__main__':

    """
    Main thread of the code. Runs the plotting functionality.
    """

    # Connect to M4 and make an instance of the BatBot object called "bb"
    bb = BatBot(port=PORT)
    
    print('-' * 60)

    # Ask for name of output folder
    print('Enter name of folder: ', end='')
    folder_name = input()

    print('-' * 60)    
    print(f'Saving to {os.path.abspath(folder_name)}')
    print('-' * 60)

    # Ask user if they want to limit number of runs
    print('Enter number of runs to perform (inf for continuous runs): ')
    n_runs = input()
    if n_runs != "inf":
	nruns = int(n_runs)
    
'''
    # Create a subplot for each channel
    f, (ax1, ax2) = plt.subplots(1, 2, sharey=True)
    ax1.set_xlim([0, 10000])
    ax1.set_ylim([0, 4096])
    ax2.set_xlim([0, 10000])
    ax2.set_ylim([0, 4096])
'''
	
    num_runs = 0
    trial_start = datetime.now()

    # Loop data collection indefinitely; press Ctrl-C and close the plot
    # to stop elegantly
    while True:
        try:
            run_start = time.time()*10**9

            # Collect data
            left, right = bb.collect_data()

            # Create output folder and file
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S%f')[:-3]
            output_folder = folder_name + '/'
            output_filename = timestamp + '.txt'
            output_path = output_folder + output_filename
            if not os.path.exists(output_folder):
                os.makedirs(output_folder)

            # Write output to file
            with open(output_path, 'w') as f:
                for data in left + right:
                    f.write('{}\n'.format(data))

            num_runs += 1
'''
            # Periodically plot an incoming signal
            if num_runs % PLOT_INTERVAL == 0:
                elapsed = datetime.now() - trial_start

                # Leave a status message
                ax1.set_title('{} runs - {}'.format(num_runs, str(elapsed)[:-7]))
                ax2.set_title('{} runs/min'.format(int(num_runs/max(elapsed.seconds,1)*60)))

                # Clear previous lines (for speed)
                ax1.lines = ax2.lines = []

                # Plot
                ax1.plot(left)
                ax2.plot(right)

                # Show the plot without blocking (there's no separate UI
                # thread)
                plt.show(block=False)
                plt.pause(0.001)
'''
            # *** Future singal processing and other kinds of things can go here in the code ***
            if num_runs == nruns:
                break

        except KeyboardInterrupt:
            print('Interrupted')
            break

    print('-' * 60)

    elapsed = datetime.now() - trial_start
    print(f'{num_runs} runs took {elapsed}')

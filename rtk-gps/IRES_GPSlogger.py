# Serial Communications Logging Script
# GOAL: connect to arduino via serial connection, then log the incoming data to a .csv file
# all the wiring should be completed before running this script, refer to GitHub respository
# GPS Antenna --> uBlox ZED-F9P board --> Arduino (over I2C) --> PC or Jetson (via USB)

import datetime
import serial
import csv
import os
import signal
import sys

# connect via serial port (if you are on a PC, you can check this in device manager)
ser = serial.Serial('/dev/ttyACM1', 115200) 	# likely need to change the com port number and serial rate (Do I need to specify which serial port? 'COM5')
# do a ls /dev/tty* check before runnning since keyboards and other devices an change ACM ports

# Define the signal handler function 
def signal_handler(sig, frame):

#Close the file and serial port before exiting
	file.close()
	ser.close()
	sys.exit(0)

signal.signal(signal.SIGTERM, signal_handler)
#	test (replacing SIGTERM with SIGINT)

# place to save the raw data files
directory_path = "raw_data"

# check if the directory exists
if not os.path.exists(directory_path):
    # create the directory
    os.makedirs(directory_path)
    print("Directory created: ", directory_path)

# for naming the csv, pull in the current datetime
current_datetime = datetime.datetime.now()
formatted_datetime = current_datetime.strftime("%Y%m%d%H%M%S")

# create csv_filename and output to user
csv_filename = directory_path + "/logged_gps_" + formatted_datetime + ".csv"
print("Saving csv as " + csv_filename + " in the current directory")
print("logging data... use 'ctrl+c' to stop")


# open csv in append mode ('a')
with open(csv_filename, 'a', newline='') as file:

	# create a csv writer object
	writer = csv.writer(file)

	# write the column headings for the csv
	header = ['Elapsed .ino Time (millisecs)','Lat (deg *10^-7)','Long (deg *10^-7)','Altitude (mm)','Accuracy (mm)']
	writer.writerow(header)


	try: 		# allows us to throw the keyboard exception properly

		while True:

			# check if serial data is available (are bytes in serial buffer?)
			if ser.in_waiting:
			
				# grab the data from the serial buffer
				data = ser.readline().decode().rstrip()
								
	
				# print data as an output (for debugging... comment out later)
				#print(f"Received data: {data}")

				# write the data into a new line of the csv
				writer.writerow(data.split(","))
				#time.sleep(0.1) #Adding small delay to allow for processing and writing

	except KeyboardInterrupt:
		print("Logging stopped by user!")

	finally:
		print("Closing the file and serial connection in the finally block.")
		file.close()
		ser.close()

# added in finally (replaced original which just stated ser.close()

# Serial Communications Logging Script
# GOAL: connect to arduino via serial connection, then log the incoming data to a .csv file
# all the wiring should be completed before running this script, refer to GitHub respository
# GPS Antenna --> uBlox ZED-F9P board --> Arduino (over I2C) --> PC or Jetson (via USB)

import datetime
import serial
import csv
import os
import sys
import signal
import time


# signal handling function for shutting down the process
def signal_handler(sig, frame):
    # close the csv file before exiting
    file.close()
    print("Logging stopped by user!")
    print("recorded " + str(num_gps_pts) + " GPS points") 
    print("failed to get GPS " + str(gps_fails) + " times") 
    sys.exit(0)

# stages the shutting down process
signal.signal(signal.SIGTERM, signal_handler)


# connect via serial port (if you are on a PC, you can check this in device manager)
ser = serial.Serial('/dev/ttyACM0', 115200) 	# likely need to change the com port number and serial rate

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
print('******** READ BELOW! ********')
print("1. saving csv as " + csv_filename + " in the current directory")
print("2. use 'ctrl+c' to stop execution of this script")
print("3. when GPS data is logged to the csv, the coordinates are outputted below")
print("4. if you don't see coordinates below --> NO data for you! something is WRONG!")
print('*****************************')
time.sleep(2)

# define counter for number of samples recorded
num_gps_pts = 0
gps_fails = 0

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
				print(f"received data: {data}")

				# write the data into a new line of the csv
				writer.writerow(data.split(","))

				# increment GPS counter
				num_gps_pts = num_gps_pts + 1

			else:
				print('no GPS data, trying again')
				gps_fails = gps_fails + 1
				time.sleep(1)



	except KeyboardInterrupt:
		print("\n*****************************")
		print("logging stopped by user!")
		print("recorded " + str(num_gps_pts) + " GPS points")
		print("failed to get GPS " + str(gps_fails) + " times")

ser.close()

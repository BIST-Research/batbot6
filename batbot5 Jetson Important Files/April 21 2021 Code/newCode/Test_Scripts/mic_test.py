#!/usr/bin/python
import time
from Jetson.Batbot_Control import BatBot

print("Mic Test Entered. This unit testing case is still under development")
#time.sleep(5)

BatBot.collect_data()

            # # Create output folder and file
            # timestamp = datetime.now().strftime('%Y%m%d_%H%M%S%f')[:-3]
            # output_folder = folder_name + '/'
            # output_filename = timestamp + '.txt'
            # output_path = output_folder + output_filename
            # if not os.path.exists(output_folder):
            #     os.makedirs(output_folder)

            # # Write output to file
            # with open(output_path, 'w') as f:
            #     for data in left + right:
            #         f.write('{}\n'.format(data))

#---------------------- Main Code Below ---------------------------------

#bb = BatBot() #this code is bring run on the Jetson, so there is no need for an external computer to "need to connect to the Jetson" since it is stand alone




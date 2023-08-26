#
# Date created: 4/3/23
# Author: Ben Westcott
#

from batbot import BatBot
from batbot import AsyncWrite
from batbot import get_timestamp_now
from batbot import bin2dec
import time
import logging
import sys
import bb_log
from datetime import datetime

bat_log = bb_log.get_log()

if __name__ == '__main__':
    
    nruns = 0
    progress_interval = 50
    
    if(len(sys.argv)) < 2:
        nruns = -1
    else:
        nruns = int(sys.argv[1])
        
    bb = BatBot(bat_log)
    
    nruns_idx = 0
    time_start = datetime.now()
    
    bb.send_amp_start()
    
    raw_data = []
    
    while True:
        try:
            if nruns_idx == nruns:
                break
            
        #    if nruns_idx >= 1:
       #         writer = AsyncWrite(f"{bb.run_directory}/{get_timestamp_now()}.bin", raw_data)
        #        writer.start()
                
            raw_data = bb.run()
            #unraw = []
            #for r in raw_data:
            #   unraw.append(bin2dec(r))
            
            
            #print(len(unraw[1]))
            
            if nruns_idx % progress_interval == 0:
                if nruns == -1:
                    bat_log.info(f"{nruns_idx}")
                else:
                    bat_log.info(f"{nruns_idx}/{nruns}")
            
          #  if nruns >= 1:
          #      writer.join()
            
            nruns_idx += 1
        
        
        except KeyboardInterrupt:
            print("")
            bat_log.info("Interrupted")
            break

    bb.send_amp_stop()
    time_finish = datetime.now() - time_start
    
    print_nruns = nruns
    if nruns == -1 or nruns != nruns_idx:
        print_nruns = nruns_idx
        
    bat_log.info(f"{print_nruns} runs took {time_finish}")

    

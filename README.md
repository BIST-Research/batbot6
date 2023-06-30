# BatBot6
## Getting started

code for batbot6

NOTE: Since this is a private repository, to have git privileges, you must generate a personal access token under your profile settings.

Use the following to clone the repository for the first time:

```
git clone https://{your-access-key}@github.com/BIST-Research/batbot6
```

After which (on a machine you trust), you can do

```
git config credential.helper store
```

Which should allow you to execute git commands normally without explicitly providing your access token.

## Basic Usage


### echo and listen
`python3.8 bb_ctrl.py 200`

Will execute 200 chirp and listen operations. Each execution of bb_ctrl creates a timestamped folder within `data_dst` and, for example if you do 200 runs, this timestamped folder, `./data_dst/202309...` will contain 200 binaries. Each binary contains data for both ears, and thus it is split down the middle, the first half containing right ear data and the second half containing left ear data. 


#### How do I know if it is working?

First, if you encouter an error about not being able to find device with serial number xyz, then make sure that the associated microcontrollers are connected (via USB at the moment) and have power (green LED is on). If it is both connected and on, unplug the microcontroller, plug it back in and press the reset button in the middle of the board and run the script again. 

If a matplot window appears, then the BatBot is recieving data from the microcontroller. 

If you do not see some slanted lines on the spectrograms (these are the chirps), then it is likely that the main sonar amplification board is not recieving power. 

The amp board should be recieving power from the 24v regulator, and when the script is running, the green enable LED on the amp board shuld be lit up. You can also listen for some faint clicking coming from the transducers. This indicates that the chirps are being amplified and sent out through the waveguide. 

### Recording GPS data

`cd rtk-gps`
`python3.8 Example1_GetPositionAccuracy.py`

This should create a 'data' folder and create a .csv file of the GPS coordinates. Note that you need a whole setup for this to work!

Keep in mind the serial device connection for the ubuntu jetson is /dev/ttyACM0 and not the typical 'COMx' you see on Windows devices. You can view connected devices from terminal using `ls /dev/tty*`. Also don't forget to adjust / and \ when moving code between Windows and Ubuntu systems.


## Basic theory

A [linear chirp](https://en.wikipedia.org/wiki/Chirp_spectrum) is defined by the function:

```math
x(t) = e^{j\left(\Delta \Omega\,t + \cfrac{\Delta \Omega}{2T}\, t^{2} + \phi\right)}
```

In discrete time, for $`k=\Delta \Omega`$ and $`t = f[n]`$, taking the real part of $`x(t)`$:

```math
x[n] = \cos\left(k f[n] + \cfrac{kf^{2}[n]}{2} + \phi\right)
 ```

```math 
f[n] = \cfrac{nT}{N} \qquad k = \cfrac{\omega_{1} - \omega_{0}}{T}
```

Where $`\omega_{0}`$ and $`\omega_{1}`$ are the initial and final frequencies respectively. The rest of this function follows typical notation for time/spatially varying waves. 

A Han window is used to attenuate harmonics outside of the scope of interest:

```math
w[n] = \sin^{2}\, \omega n 
```

So, other than some linear scaling and biasing to produce integers within the capabilities of the DAC, the output chirp is:

```math
y[n] = x[n]w[n]
```


# Git Flow

This repo has branches dedicated to specific devices. 
- 'vehicle' lives on the batbot6 [only those with the vehicle can make changes to this branch]
- 'vehicle-test' is where code should be put to test new features on the batbot. After a successful test, this branch should be merged into 'vehicle'.
- 'bench' is for those at VTech who are developi
- when developing new features from VTech, create a branch off of 'bench', develop the feature on that branch, then merge it back into 'bench' when finished. 

In conclusion, features should generally flow from bench --> vehicle-test --> vehicle. Those with the physical batbot are the only ones working on the vehicle branch, the vehicle-test is the interface between the two.

For now, main is just chilling... don't know what do to with it but also don't want to delete it in case we need to revert.

### git Branch Commands
1. `git branch` --> check what branch you are on (not your local machine will not pull in all the branches automatically)
2. `git checkout <branch_name>` --> checkout a branch to do work on it. always be aware of which branch you are on... this is also how you bring a remote branch down onto your computer. This is also how you create a new branch.
3. once you are on a branch, you can use all the same git commands and `add`, `commit`, `pull`, and `push`.

Pro-tip, download GitKraken to make branching and merging much easier! It's not free usually, but you students can get the free student developer kit through GitHub which includes a GitKraken membership.

[Read more](https://danielkummer.github.io/git-flow-cheatsheet/) on this Git Flow here, we are roughly sticking to this approach.

# System Overview

### Jetson Nano
The Jetson Nano is a mini-computer with connections to attach peripherials (mouse, monitor, keyboard, etc.) Similar to a raspberry pi, but with greater computing power because it uses a GPU (graphics processing unit). You can use a Jetson as a regular computer, it runs on the Ubuntu operating system, and is considered an embedded computing device. It serves as the 'brains' of the batbot and houses all the scripts that are executed.  

### Power Distribution Board
This board is the center power distribution center. You plug a LiPo battery into this board, and it provides the correct amount of power to each subsystem (jetson, transducers, etc.)

### Transducers
The transducers are what send out the ultrasonic (?) sound. Transducers are essentially fancy speakers.

### Microphones 
for recording the ultrasonic sounds???

### Microcontrollers
There are several microcontrollers (MCU) on the batbot. MCUs are used to control the transducers are other computers. The jetson can send commands to the MCUs very serial connection (USB) but cannot push new code to the MCUs (limitations to building the arduino IDE on the jetson). 

# Powering Up the System

There are two ways to power up the system, and they are NOT equal. 
1. Using a LiPo battery. A fully charged LiPo connects to a power distribution board, which then sends power to all the sub-systems in the bat bot
2. If you want to solely power the Jetson, you can unplug the default connection on the Jetson and plug in a separate power cable directly from the wall outlet to the jetson (note: this ONLY powers the jetson, not necessarily all the other key components)
	- keep in mind the Jetson will be in headless mode unless you plug in an HDMI cable first (then give it power)
	- A green LED indicator on the opposite side of the Jetson board from the power jack should turn on if it has power
	- if you want to power the Jetson via USB-C cable, you need to remove the tiny little jumper cable next to the barrel connector 

### LiPo Batteries

Regarding LiPo batteries, the power distribution board can handle 18-36V, and 18V should really be the lowest allowable voltage for everything to operate. LiPo's also have weird rating conventions... but for a 4S (4 cell) LiPo, you can expect the minimum voltage to be 12V, nominal voltage to be 14.8V (what it's rated to), storage voltage to be 15.2V, and max voltage to be 16.8V. Yes, that means you would charge a 14.8V battery to 16.8V to use it.

Generalizing, each cell (S) of a lipo has a nominal voltage of 3.7V, min voltage of 3V and max voltage of 4.2V.  

### SSH Instructions

New to SSH? Bascially, SSH allows you to access another computer's command line, so you can control it. To do SSH, you need to have a connection (either physically w/ an ethernet cable, or through a Wi-Fi router with both devices connected to the same network). 

Once you have established the connection, open up a terminal (ubuntu/mac) or command prompt (windows) and run 
```ssh batbot@batbot-desktop```
and enter the password to the batbot when prompted. This approach only gives you access to the command line, and thus you can't view any graphics. 

If you want to SSH AND view graphics, you need to get a VNC client. VNC = virtual network computing, which allows for desktop graphics sharing over a network so you can actively view the Jetson's screen from your own laptop. 
1. power the jetson, connect via SSH over Wi-Fi or ethernet (see previous notes on this)
2. download a VNC client (recommended: “VNC Viewer”) 
3. open a command prompt / terminal on your computer and forward the VNC connection to your localhost “ssh -L 59000:localhost:5901 -C -N -l batbot batbot-desktop.local” 
4. if it goes right it should ask you to verify fingerprint and then to enter a password (do so)
5. if password was correct, the process should just hang forever (this sets up a networking tunnel for you to use!)
6. minimize that window, and now you can use the VNC for graphics by SSH'ing in again 
TO DO: BEN UPDATE THIS ???

If you simply need to SSH in without graphics  
1. plug in the wi-fi router (the network should be named 'batbot')
2. power on the jetson, which should automatically connect to the 'batbot' wifi router
3. connect your computer to 'batbot' wifi
4. windows: open up command prompt, then type: ssh batbot@batbot-desktop
- enter password when prompted, then you should be in!

For easy file sharing over SSH, download [Bitvise SSH Client](https://www.bitvise.com/download-area). This software enables you to connect via SSH with a GUI and view files on both your PC and the SSH'd device simultaneously.  
1. plug in the wi-fi router (the network should be named 'batbot')
2. power on the jetson, which should automatically connect to the 'batbot' wifi router
3. connect your computer to 'batbot' wifi
4. open bitvise SSH
- server --> host: batbot-desktop
- authentication --> username: batbot
- click 'Log in' at the bottom. If this is your first time using Bitvise to connect to a device, it'll exchange some SSH keys and you should select 'Accept and Save'
5. a terminal should open up, and you're good to do any terminal commands from here!
6. File Sharing --> after you have a working terminal, on the left hand side of the Bitvise GUI select 'New SFTP Window'. This should open a new window in which you can see your file and the batbot jetson's file and easily transfer files over!
 
# Code Overview

### Scripts 
bb_ctrl.py --> main script, it pulls in the configurations in bb_conf.yaml and uses m4.py to create MCU objects. The output is .bin (binary files) that can be used in post-processing

m4.py --> object used to instantiate a MCU object (for our microcontrollers!)
   - send a serial number and determine connection

bb_conf.yaml --> configs for each microcontroller
	- if change any MCUs need to update this
	- page size is how much 
	- m4.py uses the values contained within this script

### Running

1. open up a terminal within the jetson (you should already be SSH'd in) [ctrl+alt+t]
2. run ```python3.8 bb_ctrl.py```
	- this starts sending echolocation chirps, records them, and saves them!
	- with no argument, it runs continuously until you kill the process with ctrl+c
	- with an argument, such as ```python3.8 bb_ctrl.py 500```, it runs for 500 chirps then stops (500 chirps takes about 2 minutes).
3. the binary data (outputs) will be saved into a data_dst file for each time you run the bb_ctrl.py script

###  Is it working?

What to look for to ensure things are operational:
1. green LED on the amplifier board w/ fan should be lit up 
2. listen for a clicking sound (an artifact of the amplifier being used), should hear a tik-tik-tik for each chirp. If you DON'T hear these clicks, something is wrong (check your wiring)
3. when you boot up the program, you should see visuals on the jetson's desktop (if you aren't using graphics fowarding, you won't see this obviously). It should show the raw time waveform (that gets saved to binary files) and then a live spectrogram as well.


# Post Processing
TBD. Contact Ibrahim and Adam.

# Use Guide for GPS-RTK2 ZED-F9P

## Steps for setting up U-Center on Computer
Follow these steps in order to download the proper software
	- following this link:https://www.u-blox.com/en/product/u-center to the u-center website
	- Download u-center 2, v23.03.54868 (for M10 and and F10T products only
	- Download u-center, v22.07 (for F9/M9 products)
	- Note: You will use both softwards for the GPS system 

## Steps for Connecting RTK2-GPS to Computer

Ensure you have all of these materials
	- SparkFun GPS-RTK2 Board - ZED-F9P
	- USB A --> USB C cord
	- GNSS Multi-Band Magnetic Mount Antenna - 5m (SMA)
	- Interface Cable SMA to U.FL
	- Grounding plate for Magnetic Antenna
	- Tripod

1. Connect The Antenna Mount to the Magnetic Grounding Plate
2. Attach the Antenna and Grounding plate to the Tripod
3. Connect U.Fl to SMA cable to the RTK2 GPS board
	- the port will be labeled "Active L1/L2 Antenna"
4. Connect the SMA Cables bewteen the Antenna and the U.FL Interface Cable
5. Use the USB A --> USB C cable to connect the RTK2 board to your computer
6. Open u-center 2 and navigate to "Devices"
	- click "Add Device" and select which ever COM port appears
	- you can keep automatic autobauding or select your own
		- Note 9,600 is good for initial tests/debuggin but 38,400 works well

# Navigating u-center 2

u-center 2 has two default tabs (Consoles and Views)

Consoles shows 2 tabs:
	- shows message views in both packet and binary
Views shows 4 tabs
	- Satellite Position View 
	- Satellite Signal View 
	- Map View
	- Data View

On the top bar there are three commands: Play log, Record log, Convert log

Play log is used to replay recorded logs
Record log is used to record data gathered by the RTK2 GPS
Convert log is used to convert the data from .ubx to .uc2

# Using the RTK2-GPS

## Part 1: u-center 2
Click "Add Device" and add the specific COM that the RTK2-GPS is plugged into
Select a Baud rate of 38,400 (or 9,600 if debugging) 
Wait for data to show up in the Console and Views Tabs (usually better to be outside)
Click "Record log"
Once you're finished recording end the record and save the log file
Press the Convert log file and select your saved log file

## Part 2: u-center
Open u-center (different from u-center 2)
follow tutorial for converting .uc2 files into .csv files: https://www.youtube.com/watch?v=zOANryt7UiM



# Arduino w/ RTK-GPS Module

The [setup guide](https://learn.sparkfun.com/tutorials/gps-rtk2-hookup-guide/all) refers to this [Arduino Library](https://github.com/sparkfun/SparkFun_u-blox_GNSS_v3). You can easily install the arduino library by opening up your Arduino IDE, go to 'Tools' --> 'Manage Libraries' --> search for 'SparkFun u-blox GNSS v3' and install it.

[RTKlib](https://www.rtklib.com/)

# Echo embedded code
## VSCode, PlatformIO and uploading code

First, install [VSCode](https://code.visualstudio.com/Download) and the [PlatformIO](https://platformio.org/install/ide?install=vscode) extension. 

Create a new project in PlatformIO. If the PIO screen does not show when opening VSCode, check the left side of the toolbar at the bottom of the window for a house icon and click it. It may take a few seconds after opening VSCode for PIO to launch so give it a few seconds for the house to show. 

Select the board being used. The MCU sent inside bb6 is the ```Adafruit Itsybitsy M4```. Note that the board selection dropdown can also be typed in to filter results.

The name you choose for the project doesn't matter.

Once the project is created, you should see a file navigation window on the left side of the window. If you view inside the ```src``` directory, there should be an autogenerated "main.cpp" file. You can delete this. The source code includes the main structure, so keeping this one will cause namespace issues. 

Now, navigate to the git folder and copy the contents of ```echo_src/src/``` to the PlatformIO project folder. For linux and MacOS users, PIO's default project locations are at ```~/Documents/PlatformIO/```.

Now, go back to VSCode, and the ```src``` directory should be holding the project source code. Look for a check mark icon in the bottom toolbar to compile the code for errors. 

Next, connect the microcontroller and verify that PIO sees it by pressing the PIO home button (bottom tool bar) and going to the "devices" section in the left hand toolbar. If it doesn't show, see the "Updating the MCU bootloader" section of this readme.

Press the arrow icon in the bottom toolbar to upload the code. If the upload process hangs on ```waiting for new upload port``` and then fails, double tap the reset button on the MCU and try reuploading. 

NOTE: The main python script was renamed from ```bb_ctrl.py``` to ```bb_run.py```

## Working with the code
There are a few variables within ```ml_main.cpp``` that can be played around with:

### ```uint16_t num_adc_samples = 25000```
This variable effectively allows to change the record time of the microphones. Currently, they are set to record for ```25000 samples * 2.4E-6 s/sample = 60 ms```. Not sure how far you can push this value, but if you choose to push passed 65355 = 2**16 - 1, then you'll have to change the variable from a halfword (```uint16_t```) to a word (```uint32_t```).

**If you change this value, you must also change it in ```bb_conf.yaml```!**

###  ```TCC_set_period``` w/in the ```wait_timer_init``` function
This variable sets the wait time between the chirp and when the microphones start listening. This value cna be set as follows:


```math
T_{\text{wait}} = \cfrac{P * v}{f_{\text{gclk}}}
```

Where $`T_{wait}`$ is the time to wait between chirp and record, $`P`$ is the timer prescaler, $`v`$ is the period division value (second argument to ```TCC_set_period``` macro) and $`f_{GCLK}`$ is the clock frequency fed to the peripheral.

For example (and how I have it originally set): $`P = 256`$, $`f_{\text{gclk}} = 120\,\,\text{MHz}`$ and if I want a 100ms wait time, then $`v = 46875`$

### f0 and f1 w/in the ```generate_chirp``` function
These values indicate the range of the linear sweep of frequencies present in the chirp. **Keep in mind that at roughly a 400kHz sampling rate (actual sampling rate is ```1/(2.4us)```), setting these variables beyond 200kHz cant be accurately picked up by the ADCS**

****STILL WORKING ON WRITING STAY TUNED****

## Updating the MCU bootloader
Go [here](https://github.com/adafruit/uf2-samdx1/releases/tag/v3.15.0) to download the most recent bootloader. Do not follow the links in the instruction page below since they're not regularly updated to point at the most updated versions. 

Follow the instructions [here](https://learn.adafruit.com/introducing-adafruit-itsybitsy-m4/update-the-uf2-bootloader) to update the MCU's bootloader. 





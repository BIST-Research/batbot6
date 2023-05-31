# BatBot6
## Getting started

code for batbot6

NOTE: Since this is a private repository, to have git privileges, you must generate a personal access token under your profile settings.

Use the following to clone the repository for the first time:

```
git https://{your-access-key}@github.com/BIST-Research/batbot6
```

After which (on a machine you trust), you can do

```
git config credential.helper store
```

Which should allow you to execute git commands normally without explicitly providing your access token

==========================================================================================================================================

## Basic Usage

```python3.8 bb_ctrl.py 200```

Will execute 200 chirp and listen operations. Each execution of bb_ctrl creates a timestamped folder within ```data_dst``` and, for example if you do 200 runs, this timestamped folder, ```./data_dst/202309...``` will contain 200 binaries. Each binary contains data for both ears, and thus it is split down the middle, the first half containing right ear data and the second half containing left ear data. 

### How do i know if it is working?

First, if you encouter an error about not being able to find device with serial number xyz, then make sure that the associated microcontrollers are connected (via USB at the moment) and have power (green LED is on). If it is both connected and on, unplug the microcontroller, plug it back in and press the reset button in the middle of the board and run the script again. 

If a matplot window appears, then the BatBot is recieving data from the microcontroller. If you do not see some slanted lines on the spectrograms (these are the chirps), then it is likely that the main sonar amplification board is not recieving power. 

For reference, we use the following for creating a chirp:

```math
x[n] = \cos\left(\omega \cfrac{\omega t_{1} n}{N}\right)
 ```

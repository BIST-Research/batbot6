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

=======================================================================================================
## Basic Usage

```python3.8 bb_ctrl.py 200```

Will execute 200 chirp and listen operations. Each execution of bb_ctrl creates a timestamped folder within ```data_dst``` and, for example if you do 200 runs, this timestamped folder, ```./data_dst/202309...``` will contain 200 binaries. Each binary contains data for both ears, and thus it is split down the middle, the first half containing right ear data and the second half containing left ear data. 

### How do i know if it is working?

First, if you encouter an error about not being able to find device with serial number xyz, then make sure that the associated microcontrollers are connected (via USB at the moment) and have power (green LED is on). If it is both connected and on, unplug the microcontroller, plug it back in and press the reset button in the middle of the board and run the script again. 

If a matplot window appears, then the BatBot is recieving data from the microcontroller. 

If you do not see some slanted lines on the spectrograms (these are the chirps), then it is likely that the main sonar amplification board is not recieving power. 

The amp board should be recieving power from the 24v regulator, and when the script is running, the green enable LED on the amp board shuld be lit up. You can also listen for some faint clicking coming from the transducers. This indicates that the chirps are being amplified and sent out through the wvaeguide. 

For reference, a linear chirp is defined as the function:

```math
x(t) = e^{j\left(\Delta \Omega\,t + \cfrac{\Delta \Omega}{2T}\, t^{2} + \phi\right)}
```

In discrete time, for $`k=\Delta \Omega`$ and $`t = f[n]`$, taking $`\fraktur{R}\{x(t)\}`$:

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



 

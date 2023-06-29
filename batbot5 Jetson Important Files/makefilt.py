import numpy as np
import scipy.signal
f1 = scipy.signal.firwin(5001,43000,pass_zero='highpass',fs=400000)
f2 = scipy.signal.firwin(5001,52000,pass_zero='lowpass',fs=400000)
f = np.convolve(f1,f2,'same')
np.save('filter3',f)

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Jun 23 12:33:23 2020

@author: devan
"""


import os
from PyQt5 import QtGui, QtCore
from PyQt5.QtCore import Qt
import pyqtgraph as pg
import numpy as np
from scipy.signal import spectrogram, butter, lfilter
from scipy import signal
import time
import colormaps as cm
from enum import Enum
from rangeslider import RangeSlider

os.environ['DISPLAY']=':0' # fixes display error with launch on startup

class SizePolicy(Enum):
    FIX = QtGui.QSizePolicy.Fixed
    MIN = QtGui.QSizePolicy.Minimum
    MAX = QtGui.QSizePolicy.Maximum
    PREF = QtGui.QSizePolicy.Preferred
    EXPAND = QtGui.QSizePolicy.Expanding
    MINEXPAND = QtGui.QSizePolicy.MinimumExpanding

class Align(Enum):
    CENTER = QtCore.Qt.AlignCenter
    LEFT = QtCore.Qt.AlignLeft
    RIGHT = QtCore.Qt.AlignRight
    
    
class Window(QtGui.QWidget): 
    
    ### Setting up the Window and graphical aspects ###
    def __init__(self):
        super().__init__()
        pg.setConfigOptions(imageAxisOrder='row-major') 
            # flips image correct way, originally rotated 90deg
            
        self.setWindowIcon(QtGui.QIcon('VT_BIST_LOGO.ico'))
        self.setWindowTitle('BatBot')
        
        ## constants
        self.width = 800        # width of the window (pixels)
        self.height = 430       # height of the window (pixels)
        self.plotWidth = 375    # width of each plot (pixels)
        self.N = int(10e3)      # number of data points per file
        self.fs = int(400e3)    # sampling frequency (Hz)
        self.beginTime = 0      # start time of sampling
        self.stopTime = 0.025   # stop time of sampling (seconds)
        self.maxFreq = 200e3    # maximum recorded frequency (Hz)
        
        ## strings for labels
        self.nString = 'N: '        # elapsed number of iterations
        self.tString = 't: '        # elapsed time since recording start
        self.fsString = 'Fs: '      # sampling frequency
        self.fpsString = 'fps: '    # frames per second of the displayed plots
        self.nMaxString = 'Nmax: '  # maximum number of echoes
        
        ## variables
        self.file = '20181111_121335587.txt'# IMPORTANT: initial displayed data
        self.fps = 0.0                      # frames per second
        self.tPassed = 0                    # time elapsed
        # array of sample time points
        self.timeArray = np.linspace(self.beginTime, self.stopTime, self.N)
        self.n = 0                          #
        self.sigWindow = 'blackman'     # default signal window
        self.lowcut = 20e3      # low end of passed frequency range
        self.highcut = 100e3    # high end of passed frequency range
        self.nperseg = 256      # number of freq points per time segment
        self.noverlap = self.nperseg//5 # overlap of time segments
        self.nfft = self.nperseg       # not really sure
        self.dBCutoff = 70
        
# =============================================================================
#         #timer for automatic execution
#         self.timer = QtCore.QTimer()
#         self.timer.setInterval(500) #in ms
#         #self.timer.timeout.connect(self.fpsUpdate)
#         self.timer.start()
# =============================================================================
        
        ## create the main grid layout
        self.resize(self.width, self.height)
        self.mainLayout = QtGui.QGridLayout()
        self.setLayout(self.mainLayout)
        
        self.createInfoWidget()
        self.createSettings()
        
        self.displayInfo()
        
        ## sg settings menu
        #self.displayInfo() # create the info display
        self.labels() # self defined method
        # window starts with sgBtn toggled so spectrogram appears
        self.sgBtn.toggle()
        
    def createInfoWidget(self):
        ## creates the information display
        # groups all the information into one widget
        self.infoWidget = QtGui.QWidget()
        self.infoLayout =  QtGui.QGridLayout()
        self.infoWidget.setLayout(self.infoLayout)
        
        
        ## create plot layout
        self.view = pg.GraphicsView() # widget to display plots
        self.plotLayout = pg.GraphicsLayout()
        self.view.setCentralItem(self.plotLayout)
        ## add plots
        self.mainLayout.addWidget(self.view)
        
        ## add info widget
        self.mainLayout.addWidget(self.infoWidget, 1, 0, 1, 2)
        
        # spectrogram radio button
        self.sgBtn = QtGui.QRadioButton('Spectrogram')
        self.sgBtn.toggled.connect(self.plotSelection)
        
        # spectrum radio button
        self.smBtn = QtGui.QRadioButton('Spectrum')
        self.smBtn.toggled.connect(self.plotSelection)
        
        # oscillogram radio button
        self.ogBtn = QtGui.QRadioButton('Oscillogram')
        self.ogBtn.toggled.connect(self.plotSelection)
        
        
        self.leftPlot = self.plotLayout.addPlot()   #create plots 
        self.rightPlot = self.plotLayout.addPlot()
        #self.leftPlot.setEnabled(False)     # disables plot interaction
        #self.rightPlot.setEnabled(False)
        self.leftPlot.setFixedWidth(self.plotWidth)
        self.rightPlot.setFixedWidth(self.plotWidth)
        
        ## Widgets
        
        # button for the settings menu
        self.menuBtn = QtGui.QPushButton('Settings')
        
        # Start/Stop button
        self.startBtn = QtGui.QPushButton('Start/Stop')
        self.startBtn.setFixedWidth(self.plotWidth//4)
        self.startBtn.setStyleSheet("background-color: green")
        self.startBtn.clicked.connect(self.run)
        self.startBtn.setCheckable(True)
        
        # upload waveform button
        self.uploadBtn = QtGui.QPushButton('Upload Waveform')
        self.uploadBtn.setFixedWidth(3*self.plotWidth//10)
        self.uploadBtn.clicked.connect(self.waveformUpload)
        
        # blank input for number of echoes
        self.iterationInput = QtGui.QLineEdit('Infinity')
        MIN = SizePolicy.MIN.value
        self.iterationInput.setSizePolicy(MIN, MIN)
        
        
        ## Labels
        self.iterationLabel = QtGui.QLabel(self.nString)
        self.durationLabel = QtGui.QLabel(self.tString)
        #self.durationLabel.setHorizontalStretch(3)
        self.rateLabel = QtGui.QLabel(self.fsString)
        self.fpsLabel = QtGui.QLabel(self.fpsString)
        self.nMaxLabel = QtGui.QLabel(self.nMaxString)
        
        #self.sgBtn.setSizePolicy(MIN, MIN) |
        #self.smBtn.setSizePolicy(MIN, MIN) # dont seem to change anything
        #self.ogBtn.setSizePolicy(MIN, MIN) |
        
        ## Alignments
        left = Align.LEFT.value
        right = Align.RIGHT.value
        
        ## Add widgets to the layout in their proper positions
        # left a gap on column 1, 3, 5
        self.infoLayout.addWidget(self.uploadBtn, 4, 0)
        self.infoLayout.addWidget(self.startBtn, 4, 2)
        self.infoLayout.setColumnMinimumWidth(3, 20)
        self.infoLayout.addWidget(self.sgBtn, 3, 4, 1, 1, alignment=left)
        self.infoLayout.addWidget(self.smBtn, 4, 4, 1, 1, alignment=left)
        self.infoLayout.addWidget(self.ogBtn, 5, 4, 1, 1, alignment=left)
        self.infoLayout.setColumnMinimumWidth(5, 20)
        self.infoLayout.addWidget(self.iterationLabel, 3, 6, alignment=left)
        self.infoLayout.addWidget(self.durationLabel, 4, 6)
        self.infoLayout.addWidget(self.rateLabel, 5, 6)
        self.infoLayout.addWidget(self.nMaxLabel, 3, 7, alignment=right)
        self.infoLayout.addWidget(self.iterationInput, 3, 8)
        self.infoLayout.addWidget(self.fpsLabel, 4, 8)
        self.infoLayout.addWidget(self.menuBtn, 5, 8)
        
        #self.infoLayout.setContentsMargins(5, 0, 5, 5)
    
    def displayInfo(self):
        self.sgSettingsWgt.hide()
        self.smSettingsWgt.hide()
        self.ogSettingsWgt.hide()
        self.infoWidget.show()
    
    def createSettings(self):
        ## UNIVERSAL ##
        # dropdown box for signal windows
        self.windowSelect = QtGui.QComboBox()
        self.windowSelect.addItems(['blackman','hamming',
                              'hann','bartlett','flattop','parzen',
                              'bohman','blackmanharris','nuttall',
                              'barthann','boxcar','triang'])
        self.windowSelect.currentTextChanged.connect(self.saveSettings)
        self.windowLabel = QtGui.QLabel('Window:')
        
        self.cutSlider = RangeSlider()
        self.cutSlider.setRange(self.lowcut*1e-3, self.highcut*1e-3)
        self.cutSlider.setRangeLimit(0, 200)
        self.cutSlider.setTickInterval(20)
        self.cutSlider.startValueChanged.connect(self.saveSettings)
        self.cutSlider.stopValueChanged.connect(self.saveSettings)
        self.highcutLabel = QtGui.QLabel('{:.0f} kHz  ----\ '.format(self.highcut*1e-3))
        self.lowcutLabel = QtGui.QLabel('/----  {:.0f} kHz'.format(self.lowcut*1e-3))
        self.cutLabel = QtGui.QLabel('Band-pass Range')
        
        # default button
        self.defaultBtn = QtGui.QPushButton('Default')
        self.defaultBtn.clicked.connect(self.defaultSettings)
        
        # return to information display
        self.closeBtn = QtGui.QPushButton('Close')
        self.closeBtn.clicked.connect(self.displayInfo)
        
        ## SPECTROGRAM ##
        ## widget for grouping items w/ grid layout
        self.sgSettingsWgt = QtGui.QWidget()
        self.mainLayout.addWidget(self.sgSettingsWgt, 1, 0, 1, 2)
        self.sgSettingsLayout = QtGui.QGridLayout()
        self.sgSettingsWgt.setLayout(self.sgSettingsLayout)
        
        
        # dropdown box for length of window (powers of 2)
        self.lengthSelect = QtGui.QComboBox()
        self.lengthSelect.addItems(['128','256','512','1024','2048',
                                    '4096','8192'])
        self.lengthSelect.setCurrentText('256')
        self.lengthSelect.currentTextChanged.connect(self.saveSettings)
        self.lengthLabel = QtGui.QLabel("Length:")
        
        # slider for adjusting overlap (as % of window length)
        self.noverlapSlider = QtGui.QSlider(orientation=2) # 2=vertical
        self.noverlapSlider.setRange(20, 99)
        #self.noverlapSlider.setSingleStep(int(self.nperseg/10)) #unknown
        self.noverlapSlider.setPageStep(10)
        self.noverlapSlider.setSliderPosition(int(100*self.noverlap/self.nperseg))
        self.noverlapSlider.valueChanged.connect(self.saveSettings)
# =============================================================================
# Styling of the noverlap slider, not needed
# self.noverlapSlider.setStyleSheet(
# "QSlider::groove:horizontal {"\
# "background: qlineargradient("\
# "x1:0, y1:0, x2:0, y2:1, stop:0 #B1B1B1, stop:1 #c4c4c4);"\
# "border: 1px solid; border-radius: 4px; height: 10px; margin: 0px;}\n"
# 
# "QSlider::sub-page:horizontal {"\
# "background: qlineargradient("\
# "x1: 0, y1: 0, x2: 0, y2: 1,stop: 0 #66e, stop: 1 #bbf);"\
# "background: qlineargradient("\
# "x1: 0, y1: 0.2, x2: 1, y2: 1,stop: 0 #bbf, stop: 1 #55f);"\
# "border: 1px solid #777; height: 10px; border-radius: 4px;}"
# 
# "QSlider::add-page:horizontal {" \
# "background: #fff;" \
# "border: 1px solid #777;" \
# "height: 10px;" \
# "border-radius: 4px;}"
# 
# "QSlider::handle:horizontal {"\
# "background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"\
# "    stop:0 #eee, stop:1 #ccc);"\
# "border: 1px solid #777;"\
# "width: 18px;"\
# "margin-top: -3px;"\
# "margin-bottom: -3px;"\
# "border-radius: 4px;}"
# "")
# =============================================================================
        noverlapDisp = self.noverlapSlider.sliderPosition()
        self.noverlapLabel = QtGui.QLabel('overlap: {}%'.format(str(noverlapDisp)))
        
        
        
        
        ## SPECTRUM ##
        ## widget for grouping items w/ grid layout
        self.smSettingsWgt = QtGui.QWidget()
        self.mainLayout.addWidget(self.smSettingsWgt, 1, 0, 1, 2)
        self.smSettingsLayout = QtGui.QGridLayout()
        self.smSettingsWgt.setLayout(self.smSettingsLayout)
        
        # Range slider
        self.dbSlider = QtGui.QSlider(orientation=1) #1==horizontal
        self.dbSlider.setRange(0, 70)
        #self.dbSlider.setSingleStep(int(self.nperseg/10)) #unknown
        self.dbSlider.setPageStep(10)
        self.dbSlider.setSliderPosition(self.dBCutoff)
        self.dbSlider.valueChanged.connect(self.saveSettings)
        self.dbLabel = QtGui.QLabel("Range: {} dB".format(self.dBCutoff))
        
        
        ## doesn't seem to change anything
# =============================================================================
#         MIN = SizePolicy.MIN.value
#         
#         self.windowSelect.setSizePolicy(MIN, MIN)
#         windowLabel.setSizePolicy(MIN, MIN)
# =============================================================================
        
        ## OSCILLOGRAM ##
        # the main widget
        self.ogSettingsWgt = QtGui.QWidget()
        self.mainLayout.addWidget(self.ogSettingsWgt, 1, 0, 1, 2)
        self.ogSettingsLayout = QtGui.QGridLayout()
        self.ogSettingsWgt.setLayout(self.ogSettingsLayout)
        
    def saveSettings(self):
        self.sigWindow = self.windowSelect.currentText()
        self.nperseg = int(self.lengthSelect.currentText())
        #print(self.nperseg)
        self.noverlap = (self.noverlapSlider.sliderPosition()/100)*self.nperseg
        #print(self.noverlap)
        self.nfft = self.nperseg # can be changed
        noverlapDisp = self.noverlapSlider.sliderPosition()
        self.noverlapLabel.setText('overlap: {}%'.format(str(noverlapDisp)))
        start, stop = self.cutSlider.getRange()
        self.lowcut = start*1e3
        self.highcut = stop*1e3
        self.lowcutLabel.setText('/----  {} kHz'.format(str(start)))
        self.highcutLabel.setText('{} kHz  ----\ '.format(str(stop)))
        self.dBCutoff = self.dbSlider.sliderPosition()
        self.dbLabel.setText("Range: {} dB".format(self.dBCutoff))
        
        self.reloadPlots()
        
    def defaultSettings(self):
        self.sigWindow = 'blackman'
        self.windowSelect.setCurrentText(self.sigWindow)
        self.nperseg = 256
        self.lengthSelect.setCurrentText(str(self.nperseg))
        self.noverlap = self.nperseg//5
        self.noverlapSlider.setSliderPosition(100*self.noverlap/self.nperseg)
        self.nfft = self.nperseg
        self.lowcut = 20e3
        self.highcut = 100e3
        start = self.lowcut * 1e-3
        stop = self.highcut * 1e-3
        self.cutSlider.setRange(start, stop)
        self.lowcutLabel.setText('{} kHz'.format(str(start)))
        self.highcutLabel.setText('{} kHz'.format(str(stop)))
        
        self.reloadPlots()
        
        
    def dispSgSettings(self):
        CENTER = Align.CENTER.value
        RIGHT = Align.RIGHT.value
        
        self.sgSettingsLayout.addWidget(self.windowLabel, 1, 0)
        self.sgSettingsLayout.addWidget(self.windowSelect, 1, 1)
        self.sgSettingsLayout.addWidget(self.lengthLabel, 2, 0)
        self.sgSettingsLayout.addWidget(self.lengthSelect, 2, 1)
        self.sgSettingsLayout.addWidget(self.noverlapLabel, 0, 2, CENTER)
        self.sgSettingsLayout.addWidget(self.noverlapSlider, 1, 2, 3, 1, CENTER)
        #self.sgSettingsLayout.setRowMinimumHeight(1, 50)
        self.sgSettingsLayout.addWidget(self.lowcutLabel, 0, 3)
        self.sgSettingsLayout.addWidget(self.highcutLabel, 0, 5, RIGHT)
        self.sgSettingsLayout.addWidget(self.cutLabel, 0, 4, CENTER)
        self.sgSettingsLayout.addWidget(self.cutSlider, 1, 3, 1, 3)
        self.sgSettingsLayout.setColumnMinimumWidth(4, 20)
        self.sgSettingsLayout.setColumnMinimumWidth(6, 20)
        self.sgSettingsLayout.addWidget(self.defaultBtn, 0, 7)
        self.sgSettingsLayout.addWidget(self.closeBtn, 1, 7)
        
        self.infoWidget.hide()
        self.sgSettingsWgt.show()
    
    
        
    def dispSmSettings(self):
        RIGHT = Align.RIGHT.value
        CENTER = Align.CENTER.value
        
        self.smSettingsLayout.addWidget(self.windowLabel, 0, 0)#, RIGHT)
        self.smSettingsLayout.addWidget(self.windowSelect, 0, 1)
        self.smSettingsLayout.addWidget(self.defaultBtn, 0, 6)
        self.smSettingsLayout.addWidget(self.closeBtn, 1, 6)
        self.smSettingsLayout.addWidget(self.dbLabel, 0, 2)
        self.smSettingsLayout.addWidget(self.dbSlider, 1, 2, 1, 3)
        self.smSettingsLayout.addWidget(self.lowcutLabel, 2, 1, RIGHT)
        self.smSettingsLayout.addWidget(self.cutLabel, 2, 0, 1, 2, CENTER)
        self.smSettingsLayout.addWidget(self.highcutLabel, 2, 6)
        self.smSettingsLayout.addWidget(self.cutSlider, 2, 2, 1, 3)
        
        self.infoWidget.hide()
        self.smSettingsWgt.show()
        
        
        
        
    def dispOgSettings(self):
        RIGHT = Align.RIGHT.value
        
        self.ogSettingsLayout.addWidget(self.cutLabel, 1, 0)
        self.ogSettingsLayout.addWidget(self.lowcutLabel, 1, 1, RIGHT)
        self.ogSettingsLayout.addWidget(self.cutSlider, 1, 2, 1, 3)
        self.ogSettingsLayout.addWidget(self.highcutLabel, 1, 5)
        self.ogSettingsLayout.addWidget(self.defaultBtn, 0, 6)
        self.ogSettingsLayout.addWidget(self.closeBtn, 1, 6)
        
        self.infoWidget.hide()
        self.ogSettingsWgt.show()
    
    ### Start of data processing ###
        
    def amplitude(self, filename):
        self.data = []
        self.ldata = []
        self.rdata = []
        file = open(filename, newline='\n')
        for _ in file:
            i = int(_.strip())
            j = (((i - 0) * (1.65 - (-1.65))) / (4095-0))
            self.data.append(j)
        self.lavg = sum(self.data[0:self.N])/len(self.data)//2
        self.ravg = sum(self.data[self.N:])/len(self.data)//2
        for _ in range(len(self.data)//2):
            self.ldata.append(self.data[_] - self.lavg)
        for _ in range(len(self.data)//2, len(self.data)):
            self.rdata.append(self.data[_] - self.ravg)
        return self.ldata, self.rdata
    
    def butter_bandpass(self, lowcut, highcut, fs, order=5):
        nyq = 0.5 * fs
        low = lowcut / nyq
        high = highcut / nyq
        b, a = butter(order, [low, high], btype='band')
        return b, a
    
    def butter_bandpass_filter(self, data, lowcut, highcut, fs, order=5):
        b, a = self.butter_bandpass(lowcut, highcut, fs, order=order)
        y = lfilter(b, a, data)
        return y
    
    def build_og(self, leftPlot, rightPlot, lamp, ramp):
        leftPlot.clear()
        rightPlot.clear()
        feedback_cutoff = 15
        Time = self.timeArray[feedback_cutoff:]
        lamp = self.butter_bandpass_filter(lamp, self.lowcut, 
                                          self.highcut, self.fs)
        ramp = self.butter_bandpass_filter(ramp, self.lowcut, 
                                          self.highcut, self.fs)
        lamp = lamp[feedback_cutoff:]
        ramp = ramp[feedback_cutoff:]
        leftPlot.plot(x=Time, y=lamp)
        rightPlot.plot(x=Time, y=ramp)
        leftPlot.setLabel('bottom', 'Time', units='s')
        rightPlot.setLabel('bottom', 'Time', units='s')
        
        # attemps to fix SI Prefix issues
        #laxis = leftPlot.getAxis('left')
        #laxis.enableAutoSIPrefix(False)
        #leftPlot.setAxisItems({'left': laxis})
        
        leftPlot.setLabel('left', 'Amplitude', units='V')
        
        leftPlot.autoRange()
        rightPlot.autoRange()
    
    def ft(self, lamp, ramp):
        window = self.windowSelection()
        lfreq = np.abs(np.fft.rfft(lamp * window))
        rfreq = np.abs(np.fft.rfft(ramp * window))
        lowcut = int((self.lowcut/self.maxFreq) * len(lfreq))  # first data point is 0 and offsets rest of data
        highcut = int((self.highcut/self.maxFreq) * len(lfreq))
        lyf = lfreq[lowcut:highcut] 
        ryf = rfreq[lowcut:highcut]
        yf = np.concatenate((lyf, ryf))
        base_dB = max(yf)
        ldB = []
        rdB = []
        for v in lyf:
            ldB.append(20*np.log10(v/base_dB)) #convert to dB scale
        for v in ryf:
            rdB.append(20*np.log10(v/base_dB)) #convert to dB scale
        return ldB, rdB
        
    def build_sm(self, leftPlot, rightPlot, lamp, ramp):
        leftPlot.clear()
        rightPlot.clear()
        ldB, rdB = self.ft(lamp, ramp)
        xf = np.linspace(0.0, self.fs//2, self.N//2)
        lowcut = int((self.lowcut/self.maxFreq) * len(xf))
        highcut = int((self.highcut/self.maxFreq) * len(xf))
        xf = xf[lowcut:highcut]
        #xf = xf[self.N//20:self.N//4]
        #yf = freq[self.N//20:self.N//4] #from 20kHz to 100kHz
        leftPlot.plot(x=xf, y=ldB)
        rightPlot.plot(x=xf, y=rdB)
        
        leftPlot.setLabel('bottom', 'Frequency', units='Hz')
        rightPlot.setLabel('bottom', 'Frequency', units='Hz')
        leftPlot.setLabel('left', 'Amplitude', units='dB')
        
        leftPlot.autoRange()
        rightPlot.autoRange()
        leftPlot.setRange(xRange=(self.lowcut, self.highcut), yRange=(-1*self.dBCutoff, 0))
        rightPlot.setRange(yRange=(-1*self.dBCutoff, 0))
        #rows = len(lyf)
        #columns = len(xf)
        #yscale = 1
        #xscale = 200/columns
        #xaxis = pg.AxisItem('bottom', text='Frequency', units='Hz')
        #xaxis.setRange(0, 200e3)
        #leftPlot.setAxisItems({'bottom': xaxis})
        #leftPlot.scale(xscale, yscale)
        #rightPlot.scale(xscale, yscale)
            
    def build_sg(self, leftPlot, rightPlot, ldata, rdata):
        leftPlot.clear()
        rightPlot.clear()
        
        ldata = np.array(ldata)
        rdata = np.array(rdata)
        
        ## creating the spectrogram
        # most efficient nperseg & nfft values are powers of 2
        # spectrogram returns to Sxx as (frequency x time)
        f, t, lSxx = spectrogram(ldata, self.fs, self.sigWindow,
                                nperseg=self.nperseg, 
                                noverlap=self.noverlap,
                                nfft=self.nfft)
        f, t, rSxx = spectrogram(rdata, self.fs, self.sigWindow,
                                nperseg=self.nperseg, 
                                noverlap=self.noverlap,
                                nfft=self.nfft)
        
        ## Tried to convert Sxx to logscale but coloring is incorrect
        #Sxx = np.concatenate([lSxx,rSxx])
        #lSxx = 20 * np.log10(lSxx/np.max(Sxx))
        #rSxx = 20 * np.log10(rSxx/np.max(Sxx))
        
        lImg = pg.ImageItem() #spectrogram is an image
        rImg = pg.ImageItem()
        leftPlot.addItem(lImg) #add sg to widget
        rightPlot.addItem(rImg)
        
        ## color mapping
        nColors = 63
        #pos = np.linspace(0, 1, nColors)
        #pos = [x**(1/3) for x in pos]
        pos = np.logspace(-4, 0, nColors)
        color = cm.jet #63x4 array of rgba values
        
        cmap = pg.ColorMap(pos, color)

        lut = cmap.getLookupTable(0.0, 1.0, 256)
        lImg.setLookupTable(lut)
        rImg.setLookupTable(lut)
        
        
        ## cutting the frequency range to 20kHz-100kHz
        fRes = (f[1] - f[0])/2
        fLowcut = self.binarySearch(f, self.lowcut, fRes)
        fHighcut = self.binarySearch(f, self.highcut, fRes)
        lSxx = lSxx[fLowcut:fHighcut]
        rSxx = rSxx[fLowcut:fHighcut]
        f = f[fLowcut:fHighcut]
        
        #print(np.shape(rSxx))
        #print(len(t))
        
        ## empty array for the image
        rows = len(f)
        columns = len(t)
        self.lImg_array = np.zeros((rows, columns))
        self.rImg_array = np.zeros((rows, columns))
        
        ## scale the axes
        yscale = (max(f)-min(f))*1e-3 /rows # scale is wrong
        xscale = max(t)/columns
        lImg.scale(xscale, yscale)
        rImg.scale(xscale, yscale)
        
        ## Attempts to fix yAxis
        #yAxis = pg.AxisItem('left')
        #yAxis.setRange(self.lowcut*1e-3, self.highcut*1e-3)
        #leftPlot.setAxisItems({'left': yAxis})
        #leftPlot.setRange(yRange=(20,100))
        #leftPlot.getAxis('left').setRange(20,100)
        #leftPlot.getAxis('left').linkToView(None)
        #leftPlot.setTransformOriginPoint(QtCore.QPointF(0, 200))
        
        ## normalize together
        nSxx = np.concatenate([lSxx,rSxx])
        Min = nSxx.min()
        Max = nSxx.max()
        lImg.setImage(lSxx, autoLevels=False, levels=(Min, Max))
            #level values are estimated from appearance
        rImg.setImage(rSxx, autoLevels=False, levels=(Min, Max))
        
        leftPlot.autoRange()
        rightPlot.autoRange()
        #leftPlot.setLimits(xMin=0, xMax=t[-1], yMin=0, yMax=f[-1])
        #rightPlot.setLimits(xMin=0, xMax=t[-1], yMin=0, yMax=f[-1])
        
        leftPlot.setLabel('bottom', 'Time', units='s')
        rightPlot.setLabel('bottom', 'Time', units='s')
        leftPlot.setLabel('left', 'Frequency', units='kHz')
            
    def run(self):
        #startTime = time.time()
        self.startBtn.setStyleSheet("background-color: red")
        jStr = self.iterationInput.text()
        try:
            self.j = int(jStr)
        except ValueError:
            self.j = 'inf'
        basepath = '/home/devan/Coding/MuellerLab/BatBotGUI/Data'
            #/home/devan/BatBot/Data #for Jetson
        data = []
        with os.scandir(basepath) as entries:
            for entry in entries:
                if entry.is_file():
                    data.append(entry.path)
        while self.startBtn.isChecked():
            if self.j != 'inf':
                # self.n initialized in __init__
                if self.n >= self.j:
                    break
            tStart = time.time()
            if self.useUpload == False:
                self.file = data[self.n]
            #tAmp = time.time()
            lamp, ramp = self.amplitude(self.file) # if useUpload True, file is
                # set in the waveformUpload function
            #print("Amplitude: {}".format(time.time() - tAmp))
            if self.sgBtn.isChecked():
                #tSg = time.time()
                self.build_sg(self.leftPlot, self.rightPlot, lamp, ramp)
                #print("Spectrogram: {}".format(time.time() - tSg))
            elif self.smBtn.isChecked():
                #tSm = time.time()
                self.build_sm(self.leftPlot, self.rightPlot, lamp, ramp)
                #print("Spectrum: {}".format(time.time() - tSm))
            elif self.ogBtn.isChecked():
                #tOg = time.time()
                self.build_og(self.leftPlot, self.rightPlot, lamp, ramp)
                #self.build_og(self.rightPlot, self.timeArray, ramp, 'right')
                #print("Oscillogram: {}".format(time.time() - tOg))
            #self.tPassed = time.time() - startTime
            tOnce = time.time() - tStart
            self.fps = 1/tOnce
            self.labels()
            #self.fpsUpdate() #temporary
            self.n += 1
            app.processEvents()
        self.n = 0
        self.useUpload = False
        #endTime = time.time()
        #self.tTotal = endTime - startTime
        #print("Total: {}".format(self.tTotal))
        #startTime = endTime
        self.startBtn.setStyleSheet("background-color: green")
        self.startBtn.setChecked(False)
        
        
    def plotSelection(self):
        self.whichSettings()
        lamp, ramp = self.amplitude(self.file)
        if self.sgBtn.isChecked(): 
            self.build_sg(self.leftPlot, self.rightPlot, lamp, ramp)
        elif self.smBtn.isChecked():
            self.build_sm(self.leftPlot, self.rightPlot, lamp, ramp)
        elif self.ogBtn.isChecked():
            self.build_og(self.leftPlot, self.rightPlot, lamp, ramp)
            #self.build_og(self.rightPlot, self.timeArray, ramp, 'right')
            
    def reloadPlots(self):
        lamp, ramp = self.amplitude(self.file)
        if self.sgBtn.isChecked():
            self.build_sg(self.leftPlot, self.rightPlot, lamp, ramp)
        elif self.smBtn.isChecked():
            self.build_sm(self.leftPlot, self.rightPlot, lamp, ramp)
        elif self.ogBtn.isChecked():
            self.build_og(self.leftPlot, self.rightPlot, lamp, ramp)
    
    def whichSettings(self):
        self.menuBtn.disconnect()
        if self.sgBtn.isChecked():
            self.menuBtn.clicked.connect(self.dispSgSettings)
        elif self.smBtn.isChecked():
            self.menuBtn.clicked.connect(self.dispSmSettings)
        elif self.ogBtn.isChecked():
            self.menuBtn.clicked.connect(self.dispOgSettings)
    
    def waveformUpload(self):
        filepath = QtGui.QFileDialog.getOpenFileName(self, 'Open file', 
                                                     '/home/devan/BatBot/Data',
                                                     "Text Files (*.txt)") #Jetson filepath
        self.file = filepath[0]
        self.useUpload = True
        
    def labels(self):
        self.nString = 'N: {}'.format(self.n)
        minutes = self.tPassed//60
        sec = self.tPassed % 60
        self.tString = 't: {m:.0f} min {sec:.2f} sec'.format(m=minutes,sec=sec)
        self.fsString = 'Fs: {:.0f} kHz'.format(self.fs/1000)
        self.iterationLabel.setText(self.nString)
        self.durationLabel.setText(self.tString)
        self.rateLabel.setText(self.fsString)
        self.fpsString = 'fps: {:.1f} Hz'.format(self.fps)
        self.fpsLabel.setText(self.fpsString)
        
    def binarySearch(self, data, target, res):
        first = 0
        last = len(data) - 1
        index = -1
        while (first <= last) and (index == -1):
            mid = (first+last)//2
            if data[mid] >= target - res and data[mid] <= target + res:
                index = mid
            else: 
                if target < data[mid]:
                    last = mid - 1
                else: 
                    first = mid + 1
        return index
    
    def windowSelection(self):
        if self.sigWindow == 'blackman':
            return np.blackman(self.N)
        elif self.sigWindow == 'hamming':
            return np.hamming(self.N)
        elif self.sigWindow == 'hann':
            return np.hanning(self.N)
        elif self.sigWindow == 'bartlett':
            return np.bartlett(self.N)
        elif self.sigWindow == 'flattop':
            return signal.flattop(self.N)
        elif self.sigWindow == 'parzen':
            return signal.parzen(self.N)
        elif self.sigWindow == 'bohman':
            return signal.bohman(self.N)
        elif self.sigWindow == 'blackmanharris':
            return signal.blackmanharris(self.N)
        elif self.sigWindow == 'nuttall':
            return signal.nuttall(self.N)
        elif self.sigWindow == 'barthann':
            return signal.barthann(self.N)
        elif self.sigWindow == 'boxcar':
            return signal.boxcar(self.N)
        elif self.sigWindow == 'triang':
            return signal.triang(self.N)
        

if __name__ == '__main__':
    app = QtGui.QApplication([])
    w = Window()
    w.show()
    #w.showMaximized() #for Jetson
    import sys
    if (sys.flags.interactive != 1) or not hasattr(QtCore, 'PYQT_VERSION'):
        QtGui.QApplication.instance().exec_()
    

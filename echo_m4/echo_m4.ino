// Needs Adafruit's ZeroDMA library

// JETSON_SERIAL goes to Jetson
// Serial2 goes to Teensy 1

// This section includes Adafruit's ZeroDMA library. You will need to get the files on your computer in order to compile.
#include <Adafruit_ZeroDMA.h>
#include <wiring_private.h>  // for access to pinPeripheral

/*
 * Configuration
 */

#define JETSON_SERIAL Serial    // Jetson connection over USB port for hi-buad rate output (120-180 runs/s)
/*#define TVCU_SERIAL Serial2    // Mega Pins 16 and 17, TVCU connection over pins 0 and 1 for valve control
#define TMCU_SERIAL Serial1     // Mega pins 18 and 19, TMCU connection over pins 16 and 17 for motor and servo control, as well as data feedback from Nylon sensors*/

// Duration of waveforms in ms. The program size is directly proportional to
// this value, and RAM size is the biggest bottleneck. 
// Just leave this as is...
constexpr int DURATION = 30;

const int freq = 1.05e6;

// Duration of a single chirp
const double chirp_duration = 5E-3;

// Total number of samples per waveform
// Must be multiple of dac_block_size
const int num_dac_samples = freq * chirp_duration;

static DmacDescriptor descriptor1 __attribute__((aligned(16)));

/*x`x`
 * Configuration that you probably don't want to touch
 */

// Number of bytes per page (double the number of uint16_t readings)
// Here Brandon is defining a "Page" as a set of data  (4096 bytes)
constexpr int PAGE_SIZE = 2048;
// It takes ~4.4ms to collect 4096/2=2048 readings, so round up to the nearest
// multiple of 4.4
constexpr int NUM_PAGES = (DURATION + 4.4) / 4.4;

/*
 * DMA buffers
 */
//__attribute__ ((section(".dmabuffers"), used)) static uint16_t dac_buffer[num_dac_samples];

// Create buffers for DMAing data around -- the .dmabuffers section is supposed
// to optimize the memory location for the DMA controller
// FYI --- A buffer is a pre-allocated set of memory (an array), with a name, for example "my_buffer", that you can fill with data.
__attribute__ ((section(".dmabuffers"), used))   // This ".dmabuffers" thing is from the Adafruit zeroDMA library
uint16_t left_in_buffers[NUM_PAGES][PAGE_SIZE],  // Here 3 buffers (arrays) are built. Each one is an array of unsigned integer, and the array's size is NUM_PAGES x PAGE_SIZE
  right_in_buffers[NUM_PAGES][PAGE_SIZE], dac_buffer[num_dac_samples];

// Index of the page currently being accessed
uint16_t left_in_index, right_in_index;  //These essentially function as counters.

// ZeroDMA job instances for both ADCs and the DAC
// Here, three "ZeroDMA jobs" are initialized. The 'jobs' aren't started here, but later on they will be used to transfer data from the microphones to the UART.
Adafruit_ZeroDMA left_in_dma, right_in_dma, out_dma;

#define AMP_FORCE_STOP() (TCC0->CTRLBSET.reg |= TCC_CTRLBSET_CMD_STOP)
#define AMP_FORCE_RETRIGGER() (TCC0->CTRLBSET.reg |= TCC_CTRLBSET_CMD_RETRIGGER)


void setup() {
  //TODO Serial doesn't work for the first second or two - DON'T WORRY ABOUT THIS, the delay takes care of it.
  //JETSON_SERIAL.begin(128000);  // only use this line if you are using wires to hook up the jetson.
  // TVCU_SERIAL.begin(9600);
  delay(2000);

  generate_chirp();
  
  // Initialize all of the buffers - this section just fills the buffers with a bunch of zeros.
  for (auto i = 0; i < NUM_PAGES; i++)
    for (auto j = 0; j < PAGE_SIZE; j++) {
      left_in_buffers[i][j] = 0;
      right_in_buffers[i][j] = 0;
      //out_buffers[i][j] = i * 500;
    }
  left_in_index = right_in_index = 0; 

  // Initialize peripherals -> These functions are defined at the bottom. Essentially just setting up the board's peripherals
  clock_init();
  adc_init(A2, ADC1);
  adc_init(A5, ADC0);

  ADC1->INPUTCTRL.bit.MUXPOS = ADC_INPUTCTRL_MUXPOS_AIN1_Val;
  ADC0->INPUTCTRL.bit.MUXPOS = ADC_INPUTCTRL_MUXPOS_AIN2_Val;
  dac_init();
  dma_init();
  //timer_init();
  //amp_enable_timer();

  // disable amplifier
  digitalWrite(12, HIGH);
  
  // Trigger both ADCs to enter free-running mode
  ADC0->SWTRIG.bit.START = 1;
  ADC1->SWTRIG.bit.START = 1;
}


void loop() {

  // This flag turns to true when the data is ready to send via UART
  static auto data_ready = false;

  if (JETSON_SERIAL.available()) {  
    // If the Jetson is sending any data over...
    uint8_t opcode = JETSON_SERIAL.read();                                         // ... Read in the data that the Jetson sending over

//    if(opcode!=NULL)
//    {
//      //Serial.write(opcode);
//      Serial.println(opcode);
//    }
    
    // Start run
    if (opcode == 0x10) {                                                   // If the M4 send over the OPCODE "0x10"...
      
      data_ready = false;                                                   // Start the counters at 0
      left_in_index = 0;
      right_in_index = 0;

      // NOTE: Below, the "DMA->CTRL.bit.DMAENABLE = 0" syntax writes the VALUE 0 to the REGISTER POSITION "DMAENABLE" of the REGISTER "CTRL" of the "DMAC" PERIPHERAL
      // Similar syntax can be used to write to any other register, e.g. DAC->CTRLA.bit.ENABLE = 1 or something...
      // The names of the registers can be found in the SAMD51 datasheet
      //DAC->CTRLA.bit.ENABLE = 1;
      DMAC->Channel[2].CHCTRLA.bit.ENABLE = 1;

      
      // Start all DMA jobs
      DMAC->CTRL.bit.DMAENABLE = 0;                                         // Temporarily disables the DMA so that it's properties can be rewritten. 

      out_dma.startJob();
      left_in_dma.startJob();                                               // This line starts the DMA job for the buffer "left_in_dma" , i.e. the left ear mic.
      right_in_dma.startJob(); 
      
      DMAC->CTRL.bit.DMAENABLE = 1;                                         // Now that DMA is configured, re-enable it 
    }
    // Check run status                                                     
    else if (opcode == 0x20) {                                              // If the incoming OPCODE is '0x20' then the M4 will return the 'data_ready' flag (true/false)
      JETSON_SERIAL.write(data_ready);                                             // Outputs TRUE or FALSE to the python script
    }
    // Retreive left buffer                                                 // Once the OPCODE '0x30' is recieved, the M4 will send the left ear's data (contained in the buffer)
    else if (opcode == 0x30) {
      JETSON_SERIAL.write(NUM_PAGES - left_in_index);
      while (left_in_index < NUM_PAGES) {
        JETSON_SERIAL.write(
          reinterpret_cast<uint8_t *>(left_in_buffers[left_in_index++]),
          sizeof(uint16_t) * PAGE_SIZE);
      }
    }
    // Retreive right buffer
    else if (opcode == 0x31) {                                              // Similarly, once OPCODE '0x31' is recieved, the M4 sends the right ear's data
      JETSON_SERIAL.write(NUM_PAGES - right_in_index);
      while (right_in_index < NUM_PAGES) {
        JETSON_SERIAL.write(
          reinterpret_cast<uint8_t *>(right_in_buffers[right_in_index++]),
          sizeof(uint16_t) * PAGE_SIZE);
      }

    }
      else if(opcode == 0xff){
        digitalWrite(12, HIGH);
      }

      else if(opcode == 0xfe){
        digitalWrite(12, LOW);
      }
    //AMP_FORCE_RETRIGGER();
  }

  /*
   * Process events and stuff with soft deadlines
   */

  // Check if the run is complete yet
  if (!data_ready && left_in_index == NUM_PAGES &&
      right_in_index == NUM_PAGES) {
    data_ready = true;                                                     // If the data wasn't ready but the 3 indices have reached their limits, then the data is ready...
                                                                           // ... So data_ready is set to TRUE. This will trigger the M4 to ask for the data that is now done...
                                                                           // ... being collected.
    left_in_index = 0;
    right_in_index = 0;                        // This line resets all of the indices to 0
  }
}



/*
 * DMA descriptor callbacks (called when a descriptor is finished). These
 * just increment the page index so we can track job progress to know when
 * the run is finished
 */

void dma_left_complete(Adafruit_ZeroDMA *dma) {
  left_in_index++;
}

void dma_right_complete(Adafruit_ZeroDMA *dma) {
  right_in_index++;
}



/*
 * Peripheral configuration
 */

void clock_init() {
  // Create a new generic 48MHz clock for our timer: GCLK7
  GCLK->GENCTRL[7].reg = GCLK_GENCTRL_DIV(1) |       // Divide the 48MHz clock source by divisor 1: 48MHz/1 = 48MHz
                         GCLK_GENCTRL_IDC |          // Set the duty cycle to 50/50 HIGH/LOW
                         GCLK_GENCTRL_GENEN |        // Enable GCLK7
                         GCLK_GENCTRL_SRC_DFLL;      // Select 48MHz DFLL clock source
  while (GCLK->SYNCBUSY.bit.GENCTRL7);

  // Have TCC0 use GCLK7
  GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK7;
  
  // Have ADC1 use GCLK1 (ADC0 is already configured to GCLK0 by Arduino)
  GCLK->PCHCTRL[ADC1_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1_Val | (1 << GCLK_PCHCTRL_CHEN_Pos);

  GCLK->PCHCTRL[42].reg = GCLK_PCHCTRL_CHEN |        // Enable the DAC peripheral channel
                            GCLK_PCHCTRL_GEN_GCLK7;    // Connect generic clock 7 to DAC
}
//
void timer_init() {
  TCC0->CTRLA.reg = TC_CTRLA_PRESCALER_DIV4 |        // Set prescaler to 8, 48MHz/8 = 6MHz
                    TC_CTRLA_PRESCSYNC_PRESC;        // Set the reset/reload to trigger on prescaler clock                 

  TCC0->WAVE.reg = TC_WAVE_WAVEGEN_NPWM;             // Set-up TCC0 timer for Normal (single slope) PWM mode (NPWM)
  while (TCC0->SYNCBUSY.bit.WAVE);

  TCC0->PER.reg = 12;                            // Set-up the PER (period) register 50Hz PWM
  while (TCC0->SYNCBUSY.bit.PER);
  
  TCC0->CC[3].reg = 11;                           // Set-up the CC (counter compare), channel 0 register for 50% duty-cycle
  while (TCC0->SYNCBUSY.bit.CC0);

  TCC0->CTRLA.bit.ENABLE = 1;                        // Enable timer TCC0
  while (TCC0->SYNCBUSY.bit.ENABLE);

  const uint32_t port_group = 0;
  const uint32_t pin = 23;

 
  PORT->Group[port_group].PINCFG[pin].reg |= (PORT_PINCFG_PMUXEN | PORT_PINCFG_DRVSTR);

  //PA23, F -> 0x5
  PORT->Group[port_group].PMUX[pin >> 1].reg |= PORT_PMUX_PMUXO(0x6); 
}

void amp_enable_timer(void){

  TCC1->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV2 | TCC_CTRLA_PRESCSYNC_PRESC;

  TCC1->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;
  while(TCC1->SYNCBUSY.reg);

  TCC1->PER.reg = 10;
  while(TCC1->SYNCBUSY.reg);

  TCC1->CC[7].reg = 9;
  while(TCC1->SYNCBUSY.reg);

  
  TCC1->CTRLA.bit.ENABLE = 1;
  while(TCC1->SYNCBUSY.reg);
  
  const uint32_t port_group = 0;
  const uint32_t pin = 23;

  PORT->Group[port_group].PINCFG[pin].reg |= (PORT_PINCFG_PMUXEN | PORT_PINCFG_DRVSTR);

  //PA23, F -> 0x5
  PORT->Group[port_group].PMUX[pin >> 1].reg |= PORT_PMUX_PMUXO(0x5);

}

void dma_init() {
  //static Adafruit_ZeroDMA left_in_dma, right_in_dma, out_dma;

  static DmacDescriptor *left_in_descs[NUM_PAGES], *right_in_descs[NUM_PAGES];
  
  // Create left ADC channel DMA job
  {
    left_in_dma.allocate();
  
    for (auto i = 0; i < NUM_PAGES; i++) {
      left_in_descs[i] = left_in_dma.addDescriptor(
        (void *)(&ADC0->RESULT.reg),
        left_in_buffers[i],
        PAGE_SIZE,
        DMA_BEAT_SIZE_HWORD,
        false,
        true);
      left_in_descs[i]->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;
    }

    //left_in_dma.loop(true);
    left_in_dma.setTrigger(0x44); // Trigger on ADC0 read completed
    left_in_dma.setAction(DMA_TRIGGER_ACTON_BEAT);
    left_in_dma.setCallback(dma_left_complete);
  }

  // Create right ADC channel DMA job
  {
    right_in_dma.allocate();
  
    for (auto i = 0; i < NUM_PAGES; i++) {
      right_in_descs[i] = right_in_dma.addDescriptor(
        (void *)(&ADC1->RESULT.reg),
        right_in_buffers[i],
        PAGE_SIZE,
        DMA_BEAT_SIZE_HWORD,
        false,
        true);
      right_in_descs[i]->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;
    }
    right_in_dma.setTrigger(0x46); // Trigger on ADC1 read completed
    right_in_dma.setAction(DMA_TRIGGER_ACTON_BEAT);
    right_in_dma.setCallback(dma_right_complete);
  }

  {
    out_dma.allocate();
  
    auto desc = out_dma.addDescriptor(
      dac_buffer,                                     // Source                                                                                                
      (void *)(&DAC->DATA[0]),                        // Destination
      num_dac_samples,
      DMA_BEAT_SIZE_HWORD,
      true,
      false);
    desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;
    out_dma.loop(true);
    out_dma.setTrigger(0x16);
    out_dma.setAction(DMA_TRIGGER_ACTON_BEAT);
    out_dma.setCallback(stop_callback);
    out_dma.startJob();
  }
}


const uint32_t dac_pin = g_APinDescription[A0].ulPin;
const EPortType dac_port_grp = g_APinDescription[A0].ulPort;

void dac_init() {
  // Disable DAC
  DAC->CTRLA.bit.ENABLE = 0;
  while (DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST);

  // Use an external reference voltage (see errata; the internal reference is busted)
  DAC->CTRLB.reg = DAC_CTRLB_REFSEL_VREFPB;
  while (DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST);

  PORT->Group[dac_port_grp].PINCFG[dac_pin].reg |= PORT_PINCFG_DRVSTR;

  // Enable channel 0
  DAC->DACCTRL[0].bit.ENABLE = 1;
  while (DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST);

  // Enable DAC
  DAC->CTRLA.bit.ENABLE = 1;
  while (DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST);
}

void adc_init(int inpselCFG, Adc *ADCx) {
  // Configure the ADC pin (cheating method)
  pinPeripheral(inpselCFG, PIO_ANALOG);
  
  ADCx->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND;   // No Negative input (Internal Ground)
  while( ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_INPUTCTRL );
  
  //ADCx->INPUTCTRL.bit.MUXPOS = g_APinDescription[inpselCFG].ulADCChannelNumber; // Selection for the positive ADC input
  while( ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_INPUTCTRL );
  
  ADCx->CTRLA.bit.PRESCALER = ADC_CTRLA_PRESCALER_DIV4_Val; // Frequency set. SAMD51 Datasheet pp. 1323. f(CLK_ADC) = fGLCK/2^(1+4) = 1.5MHz
  while( ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLB );         // Gives sampling rate 1.5MHz/(12+4) ~= 125 kHz? The prescaler might need to be changed to 2 if data is messy...
  
  ADCx->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_12BIT_Val;
  while( ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLB );
  
  ADCx->SAMPCTRL.reg = 0x0;                        
  while( ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_SAMPCTRL );
  
  ADCx->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 |    // 1 sample only (no oversampling nor averaging)
            ADC_AVGCTRL_ADJRES(0x0ul);   // Adjusting result by 0
  while( ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_AVGCTRL );
  
  ADCx->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_AREFA_Val; // 1/2 VDDANA = 0.5* 3V3 = 1.65V
  while(ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_REFCTRL);
  
  ADCx->CTRLB.reg |= 0x02;  ; //FREERUN
  while( ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLB);
  
  ADCx->CTRLA.bit.ENABLE = 0x01;
  while( ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_ENABLE );
}


// Generate the waveform for an enveloped, linear cosine sweep
void generate_chirp()
{ 
  // Duration of chirp
  const double t1 = chirp_duration;

  // Phase shift (rads)
  const double phi = 0;

  // Initial frequency (Hz)
  const double f0 = 150e3;
  // Final frequency (Hz)
  const double f1 = 80e3;

  // "Chirpyness" or rate of frequency change
  const double k = (f1 - f0) / t1;
  
  for (int i = num_dac_samples - 1; i >= 0; --i)
  {
    double t = t1 * ((double)i / num_dac_samples);
    // Create a chirp from the frequency f(t) = f0 + kt
    double chirp = cos(phi + 2*PI * (f0*t + k/2 * t*t));
    // Create a Hanning window to envelope the chirp
    double window = 0.5 * (1 - cos(2*PI * i/(num_dac_samples - 1)));
    // Move the signal across a difference reference
    dac_buffer[i] = 4096/2 + 4096/2 * (chirp * window);
  }
}

void stop_callback(Adafruit_ZeroDMA *dma) {
     DMAC->Channel[2].CHCTRLA.bit.ENABLE = 0;
     //DAC->CTRLA.bit.ENABLE = 0;, maybe we could address it like this? test later
     
}

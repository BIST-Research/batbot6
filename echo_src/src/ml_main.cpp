/*
 * Author: Ben Westcott
 * Date created: 8/10/22
 */

#include <header/sine_tables.hpp>
#include <header/ml_port.hpp>
#include <header/ml_clocks.hpp>
#include <header/ml_adc.hpp>
#include <header/ml_dmac.hpp>
#include <header/ml_tcc.hpp>
#include <header/ml_ac.hpp>
#include <wiring_private.h>

#define JETSON_SERIAL Serial

#define JOB_STATUS_LED_CHANNEL                   ML_TCC0_CH3
#define JOB_STATUS_LED_PIN                       ML_M4GC_TCC0_CH3_PIN
#define JOB_STATUS_LED_PMUX_MASK                 ML_M4GC_TCC0_CH3_PMUX_msk

void job_led_toggle(void)
{

    static const EPortType port_grp = g_APinDescription[JOB_STATUS_LED_CHANNEL].ulPort;
    static const uint32_t pin = g_APinDescription[JOB_STATUS_LED_CHANNEL].ulPin;
    static boolean state = false;

    PORT->Group[port_grp].PINCFG[pin].bit.PMUXEN = state;

    state = !state;
}

void amp_disable(void)
{
  digitalWrite(10, HIGH);
}

void amp_enable(void)
{
  digitalWrite(10, LOW);
}

#define EMIT_RESONATOR_TIMER_CHANNEL          ML_TCC0_CH0
#define EMIT_RESONATOR_TIMER_PIN              ML_M4GC_TCC0_CH0_PIN
#define EMIT_RESONATOR_TIMER_PMUX_MASK        ML_M4GC_TCC0_CH0_PMUX_msk

#define EMIT_RESONATOR_INVERTED_TIMER_CHANNEL   ML_TCC0_CH1
#define EMIT_RESONATOR_INVERTED_TIMER_PIN       ML_M4GC_TCC0_CH1_PIN
#define EMIT_RESONATOR_INVERTED_TIMER_PMUX_MASK ML_M4GC_TCC0_CH1_PMUX_msk

void emit_resonator_timer_init(void)
{


  TCC_DISABLE(TCC0);
  TCC_sync(TCC0);

  TCC_SWRST(TCC0);
  TCC_sync(TCC0);



  TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 |
                    TCC_CTRLA_PRESCSYNC_PRESC;  

  TCC0->WAVE.reg = TCC_WAVE_WAVEGEN_NFRQ |                           
                   TCC_WAVE_RAMP_RAMP2   |                
                   TCC_WAVE_POL0 | 
                   TCC_WAVE_POL1;

  TCC_set_period(TCC0, 30); 

  TCC_channel_capture_compare_set(TCC0, EMIT_RESONATOR_TIMER_CHANNEL, 15);
  peripheral_port_init(EMIT_RESONATOR_TIMER_PMUX_MASK, EMIT_RESONATOR_TIMER_PIN, OUTPUT_PULL_DOWN, DRIVE_OFF);

  TCC_channel_capture_compare_set(TCC0, EMIT_RESONATOR_INVERTED_TIMER_CHANNEL, 15);
  peripheral_port_init(EMIT_RESONATOR_INVERTED_TIMER_PMUX_MASK, EMIT_RESONATOR_INVERTED_TIMER_PIN, OUTPUT_PULL_DOWN, DRIVE_OFF);

  TCC_channel_capture_compare_set(TCC0, JOB_STATUS_LED_CHANNEL, 15);
  peripheral_port_init(JOB_STATUS_LED_PMUX_MASK, JOB_STATUS_LED_PIN, OUTPUT_PULL_DOWN, DRIVE_ON);
  
}

// 1Mhz sampling freq, chirp duration of 5E-3
// sample rate = (resolution + 1 + samplen)/(ADC_GCLK / PRESCALE) = PRESCALE*(res + 1 + smp)/ADC_GCLK
// PRESCALE = ADC_GCLK * SAMPLE_RATE /(res + 1 + samp)
// PRESCALE = 120MHz * 1us/(12 + 1 + 2) = 8 -> ADC_CTRLA_PRESCALER_DIV8
// NUM_SAMPLES = 1Mhz * 5ms = 5000 samples
const int sample_freq = 400E3;
const double chirp_duration = 4E-3;
const uint16_t num_samples = 2000;   //Changed for serial output

static uint16_t chirp_out_buffer[num_samples];

uint32_t generate_chirp(const int f0, const int f1)
{

  const double t1 = chirp_duration;

  const double phi = 0;

  //const int f0 = 150E3;
  //const int f1 = 20E3;

  const double k = (f1 - f0) / t1;

  for(int i = num_samples - 1; i >= 0; --i)
  {

    // time step
    double t = t1 * ((double)i / num_samples);

    // time domain model for a bat chirp
    //double chirp = cos(phi + 2*PI * (k * t + k/2 + t * t));
    double chirp = cos(phi + 2*PI*((k/2)*t*t + f0*t));

    // Hanning window - remove harmonics from frequency distribution, leave fundemental frequencies
    double window = 0.5 * (1 - cos(2*PI * i/(num_samples - 1)));

    // fill DMA buffer 
    chirp_out_buffer[i] = (uint16_t)((4096/2) * (1 + chirp * window));
  }
  //chirp_out_buffer[4095] = 0;
  return (uint32_t)&chirp_out_buffer[0] + num_samples * sizeof(uint16_t);
}

// number of arrays of ADC samples we need
const uint32_t num_pages = 7;               //Can be 8, needs to be 7 for compatibility

// DMAC looks for the base descriptor when serving a request
static DmacDescriptor base_descriptor[3] __attribute__((aligned(16)));
static volatile DmacDescriptor wb_descriptor[3] __attribute__((aligned(16)));

#define DMAC_EMIT_MODULATOR_TIMER_CHANNEL     0x02

#define EMIT_MODULATOR_TIMER_CHANNEL             ML_TCC1_CH0
#define EMIT_MODULATOR_TIMER_PIN                 ML_M4GC_TCC1_CH0_PIN
#define EMIT_MODULATOR_TIMER_PMUX_MASK           ML_M4GC_TCC1_CH0_PMUX_msk

#define EMIT_MODULATOR_INVERTED_TIMER_CHANNEL    ML_TCC1_CH3
#define EMIT_MODULATOR_INVERTED_TIMER_PIN        ML_M4GC_TCC1_CH3_PIN
#define EMIT_MODULATOR_INVERTED_TIMER_PMUX_MASK  ML_M4GC_TCC1_CH3_PMUX_msk  

const uint32_t chirp_out_dmac_channel_settings = DMAC_CHCTRLA_BURSTLEN_SINGLE | //check when testing evsys
                                                 DMAC_CHCTRLA_TRIGACT_BURST |
                                                 //DMAC_CHCTRLA_TRIGSRC(DAC_DMAC_ID_EMPTY_0);
                                                 DMAC_CHCTRLA_TRIGSRC(TC3_DMAC_ID_OVF);

const uint16_t chirp_out_dmac_descriptor_settings = DMAC_BTCTRL_VALID |
                                           //         DMAC_BTCTRL_EVOSEL_BURST | //check when testing evsys
                                                    DMAC_BTCTRL_BLOCKACT_BOTH | //check when testing evsys
                                                    DMAC_BTCTRL_BEATSIZE_HWORD |
                                                    DMAC_BTCTRL_SRCINC;

uint32_t chirp_out_source_address;


void emit_modulator_timer_init(void)
{

  //ML_SET_GCLK7_PCHCTRL(TCC1_GCLK_ID);

  TCC_DISABLE(TCC1);
  TCC_sync(TCC1);

  TCC_SWRST(TCC1);
  TCC_sync(TCC1);

  TCC1->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV1 |
                    TCC_CTRLA_PRESCSYNC_PRESC;
  
  TCC1->CTRLA.bit.DMAOS = 0x00;

  TCC1->WAVE.reg = TCC_WAVE_WAVEGEN_NPWM;

  TCC_set_period(TCC1, 1200);

  TCC_channel_capture_compare_set(TCC1, EMIT_MODULATOR_TIMER_CHANNEL, 128);

  peripheral_port_init(EMIT_MODULATOR_TIMER_PMUX_MASK, EMIT_MODULATOR_TIMER_PIN, OUTPUT_PULL_DOWN, DRIVE_OFF);

  DMAC_channel_init(
    DMAC_EMIT_MODULATOR_TIMER_CHANNEL,
    chirp_out_dmac_channel_settings,
    DMAC_CHPRILVL_PRILVL_LVL0
  );

  //check when testing evsys
  DMAC_channel_intenset(DMAC_EMIT_MODULATOR_TIMER_CHANNEL, DMAC_2_IRQn, DMAC_CHINTENSET_SUSP, 0);

  DMAC_descriptor_init(
    chirp_out_dmac_descriptor_settings,
    num_samples,
    chirp_out_source_address,
    (uint32_t) &TCC1->CCBUF[EMIT_MODULATOR_TIMER_CHANNEL],
    (uint32_t) &base_descriptor[DMAC_EMIT_MODULATOR_TIMER_CHANNEL],
    &base_descriptor[DMAC_EMIT_MODULATOR_TIMER_CHANNEL]
  );
}

void dac_sample_timer_init(void)
{

  GCLK->PCHCTRL[TC3_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK4;

  TC3->COUNT8.CTRLA.bit.ENABLE = 0x00;
  while(TC3->COUNT8.SYNCBUSY.bit.ENABLE != 0U);

  TC3->COUNT8.CTRLA.bit.SWRST = 0x01;
  while(TC3->COUNT8.SYNCBUSY.bit.SWRST);

  TC3->COUNT8.CTRLA.reg = 
  (
    TC_CTRLA_MODE_COUNT16 |
    TC_CTRLA_PRESCSYNC_PRESC |
    TC_CTRLA_PRESCALER_DIV1 
  );

  TC3->COUNT8.WAVE.reg = TC_WAVE_WAVEGEN_MFRQ;
  //TC3->COUNT8.PER.reg = TC_COUNT8_PER_PER(48);
  TC3->COUNT8.CC[0].reg = 30;

  while(TC3->COUNT8.SYNCBUSY.bit.CC0);

  TC3->COUNT8.CTRLA.bit.ENABLE=1;
  while(TC3->COUNT8.SYNCBUSY.bit.ENABLE);

  peripheral_port_init(PORT_PMUX_PMUXE(PF_E), 7, OUTPUT_PULL_DOWN, DRIVE_ON);

}

void wait_timer_init(void)
{

  ML_SET_GCLK7_PCHCTRL(TCC2_GCLK_ID);

  TCC_DISABLE(TCC2);
  TCC_sync(TCC2);

  TCC_SWRST(TCC2);
  TCC_sync(TCC2);

  //120Meg/512 = 468750
  TCC2->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV256 | 
                    TCC_CTRLA_PRESCSYNC_PRESC;

  TCC2->WAVE.reg = TCC_WAVE_WAVEGEN_NFRQ | TCC_WAVE_POL0;

  TCC_set_period(TCC2, 5);

  TCC_SET_ONESHOT(TCC2);
  TCC_sync(TCC2);

  TCC_intenset(TCC2, TCC2_0_IRQn, TCC_INTENSET_OVF, 0);

  TCC2->CC[0].reg |= TCC_CC_CC(2);

  // GC port
  //perip2heral_port_init(PORT_PMUX_PMUXE(0x5), 28, OUTPUT_PULL_DOWN, DRIVE_ON);
  // D4 --> PA14 --> periph F
  peripheral_port_init(PORT_PMUX_PMUXE(0x5), 4, OUTPUT_PULL_DOWN, DRIVE_ON);
}

inline void state_timer_retrigger(void)
{
  TC2->COUNT16.CTRLBSET.reg |= TC_CTRLBSET_CMD_RETRIGGER;
  while(TC2->COUNT16.SYNCBUSY.bit.CTRLB);
}

void state_timer_init(void)
{

  ML_SET_GCLK7_PCHCTRL(TC2_GCLK_ID);

  TC2->COUNT16.CTRLA.bit.ENABLE = 0;
  while(TC2->COUNT16.SYNCBUSY.bit.ENABLE);

  //TC2->COUNT16.CTRLA.bit.SWRST = 1;
  //while(TC2->COUNT16.SYNCBUSY.bit.SWRST);

  // ((2**16 - 1) * 2)/120Meg = 1,09 ms
  TC2->COUNT16.CTRLA.reg = 
  (
    TC_CTRLA_PRESCALER_DIV2 |
    TC_CTRLA_MODE_COUNT16 |
    TC_CTRLA_PRESCSYNC_PRESC
  );

  TC2->COUNT16.WAVE.reg |= TC_WAVE_WAVEGEN_NFRQ;

  TC2->COUNT16.CTRLBSET.reg |= TC_CTRLBSET_ONESHOT;
  while(TC2->COUNT16.SYNCBUSY.bit.CTRLB);

  // (2**16)/2 = 32768
  TC2->COUNT16.CC[0].reg |= TC_COUNT16_CC_CC(32768);
  while(TC2->COUNT16.SYNCBUSY.bit.CC0);

  TC2->COUNT16.CTRLA.bit.ENABLE = 1;
  while(TC2->COUNT16.SYNCBUSY.bit.ENABLE);

  TC2->COUNT16.CTRLBSET.reg |= TC_CTRLBSET_CMD_STOP;
  while(TC2->COUNT16.SYNCBUSY.bit.CTRLB);

  // D7 --> PA18 --> periph E
  //peripheral_port_init_alt(PF_E, PP_EVEN, 7, OUTPUT_PULL_DOWN, DRIVE_ON);
  peripheral_port_init(PORT_PMUX_PMUXE(PF_E), 0, OUTPUT_PULL_DOWN, DRIVE_ON);

}

/*
 *
 * The below function allows us to hook up a debounced button
 * to a pin on the M4, and trigger an interrupt with it.
 * 
 * This is useful for debugging purposes
 * 
 * It essentially routes the input of an M4 pin to an analog
 * comparator which can detect the button press and will
 * trigger an interrupt
 * 
 */

#define AC_CHANNEL 0x00

void hardware_int_trigger_init(void)
{

    ML_SET_GCLK7_PCHCTRL(AC_GCLK_ID);

    ML_AC_DISABLE();
    AC_sync();

    ML_AC_SWRST();
    AC_sync();

    AC->COMPCTRL[AC_CHANNEL].reg |= (AC_COMPCTRL_MUXPOS_PIN3|
                                     AC_COMPCTRL_MUXNEG_GND  | 
                                     AC_COMPCTRL_SPEED_HIGH  |
                                     AC_COMPCTRL_HYST_HYST150|
                                     AC_COMPCTRL_FLEN_MAJ5   |
                                     AC_COMPCTRL_INTSEL_TOGGLE |
                                     AC_COMPCTRL_OUT_SYNC);

    AC->COMPCTRL[AC_CHANNEL].bit.SINGLE = 0x0;
   // AC->COMPCTRL[0].bit.HYSTEN = 0x1;
   // AC->COMPCTRL[0].bit.SWAP = 0x1;

   // AC->SCALER[0].reg = AC_SCALER_VALUE(20); 

    AC->INTENSET.reg |= AC_INTENSET_COMP0;

    //peripheral_port_init(ML_M4E_AC_AIN0_PMUX_msk, ML_M4E_AC_AIN0_PIN, ANALOG, DRIVE_OFF);
    peripheral_port_init(PORT_PMUX_PMUXO(PF_B), 2, ANALOG, DRIVE_OFF);

    NVIC_SetPriority(AC_IRQn, 0);
    NVIC_EnableIRQ(AC_IRQn);

    AC->COMPCTRL[AC_CHANNEL].reg |= AC_COMPCTRL_ENABLE;
    AC_sync();

    ML_AC_ENABLE();
    AC_sync();

}

#define DAC_DMAC_CHANNEL 0x02

void dac_init(void)
{
  // Disable DAC
  DAC->CTRLA.bit.ENABLE = 0;
  DAC->CTRLA.bit.SWRST = 1;
  while (DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST);

  // Use an external reference voltage (see errata; the internal reference is busted)
  DAC->CTRLB.reg = DAC_CTRLB_REFSEL_VDDANA;
  while (DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST);

  DAC->DACCTRL[0].reg |= DAC_DACCTRL_CCTRL_CC1M;

  const uint32_t dac_pin = g_APinDescription[A0].ulPin;
  const EPortType dac_port_grp = g_APinDescription[A0].ulPort;
  
  PORT->Group[dac_port_grp].PINCFG[dac_pin].reg |= PORT_PINCFG_DRVSTR;

  // Enable channel 0
  DAC->DACCTRL[0].bit.ENABLE = 1;
  while(DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST);

  DMAC_channel_init
  (
    DAC_DMAC_CHANNEL,
    chirp_out_dmac_channel_settings,
    DMAC_CHPRILVL_PRILVL_LVL0
  );

  //check when testing evsys
  DMAC_channel_intenset(2, DMAC_2_IRQn, DMAC_CHINTENSET_SUSP, 0);

  DMAC_descriptor_init
  (
    chirp_out_dmac_descriptor_settings,
    num_samples,
    chirp_out_source_address,
    (uint32_t) &DAC->DATA[0],
    (uint32_t) &base_descriptor[2],
    &base_descriptor[2]
  );
}

const uint16_t num_adc_samples = 30000;

#define PASTE_FUSE(REG) ((*((uint32_t *) (REG##_ADDR)) & (REG##_Msk)) >> (REG##_Pos))

const uint32_t refbuf = PASTE_FUSE(ADC0_FUSES_BIASREFBUF);
const uint32_t r2r = PASTE_FUSE(ADC0_FUSES_BIASR2R);
const uint32_t comp = PASTE_FUSE(ADC0_FUSES_BIASCOMP);

const uint32_t adc0_dmac_channel_settings = 
(
  DMAC_CHCTRLA_TRIGACT_BURST |
  DMAC_CHCTRLA_TRIGSRC(ADC0_DMAC_ID_RESRDY)
);

const uint16_t adc0_dmac_descriptor_settings = 
(
  DMAC_BTCTRL_BEATSIZE_HWORD |
  DMAC_BTCTRL_DSTINC |
  DMAC_BTCTRL_EVOSEL_BURST |
  DMAC_BTCTRL_BLOCKACT_BOTH |
  DMAC_BTCTRL_VALID
);

#define ADC0_DMAC_CHANNEL 0x00

uint16_t adc0_samples[num_adc_samples];


void adc0_init(void)
{
  ML_SET_GCLK7_PCHCTRL(ADC0_GCLK_ID);

  DMAC_channel_init
  (
    ADC0_DMAC_CHANNEL,
    adc0_dmac_channel_settings,
    DMAC_CHPRILVL_PRILVL_LVL0
  );

  DMAC_descriptor_init
  (
    adc0_dmac_descriptor_settings,
    num_adc_samples,
    (uint32_t)&ADC0->RESULT.reg,
    (uint32_t)adc0_samples + sizeof(uint16_t) * num_adc_samples,
    (uint32_t)&base_descriptor[ADC0_DMAC_CHANNEL],
    &base_descriptor[ADC0_DMAC_CHANNEL]
  );

  DMAC_channel_intenset
  (
    ADC0_DMAC_CHANNEL,
    DMAC_0_IRQn,
    DMAC_CHINTENSET_SUSP,
    0
  );

  ADC0->CTRLA.reg |= ADC_CTRLA_SWRST;
  ADC_sync(ADC0);

  ADC0->CALIB.reg = 
  (
    ADC0_FUSES_BIASREFBUF(refbuf) |
    ADC0_FUSES_BIASCOMP(comp) |
    ADC0_FUSES_BIASR2R(r2r)
  );

  ADC0->CTRLA.reg |= ADC_CTRLA_PRESCALER_DIV8;

  ADC0->SAMPCTRL.reg |= ADC_SAMPCTRL_SAMPLEN(3U - 1);

  ADC0->REFCTRL.reg |= ADC_REFCTRL_REFSEL_INTVCC1;

  ADC0->INPUTCTRL.reg = 
  (
    ADC_INPUTCTRL_MUXPOS_AIN2 |
    ADC_INPUTCTRL_MUXNEG_GND
  );

  ADC0->CTRLB.reg =
  (
    ADC_CTRLB_RESSEL_12BIT |
    ADC_CTRLB_WINMODE(0U) |
    ADC_CTRLB_FREERUN
  );

  ADC0->INTFLAG.reg = ADC_INTFLAG_MASK;
  
  ADC_sync(ADC0);

}

const uint32_t adc1_dmac_channel_settings = 
(
  DMAC_CHCTRLA_TRIGACT_BURST |
  DMAC_CHCTRLA_TRIGSRC(ADC1_DMAC_ID_RESRDY)
);

const uint16_t adc1_dmac_descriptor_settings = 
(
  DMAC_BTCTRL_BEATSIZE_HWORD |
  DMAC_BTCTRL_DSTINC |
  DMAC_BTCTRL_EVOSEL_BURST |
  DMAC_BTCTRL_BLOCKACT_BOTH |
  DMAC_BTCTRL_VALID
);

#define ADC1_DMAC_CHANNEL 0x01

uint16_t adc1_samples[num_adc_samples];

void adc1_init(void)
{
  ML_SET_GCLK7_PCHCTRL(ADC1_GCLK_ID);

  DMAC_channel_init
  (
    ADC1_DMAC_CHANNEL,
    adc1_dmac_channel_settings,
    DMAC_CHPRILVL_PRILVL_LVL0
  );

  DMAC_descriptor_init
  (
    adc1_dmac_descriptor_settings,
    num_adc_samples,
    (uint32_t)&ADC1->RESULT.reg,
    (uint32_t)adc1_samples + sizeof(uint16_t) * num_adc_samples,
    (uint32_t)&base_descriptor[ADC1_DMAC_CHANNEL],
    &base_descriptor[ADC1_DMAC_CHANNEL]
  );

  DMAC_channel_intenset
  (
    ADC1_DMAC_CHANNEL,
    DMAC_1_IRQn,
    DMAC_CHINTENSET_SUSP,
    0
  );

  ADC1->CTRLA.reg |= ADC_CTRLA_SWRST;
  ADC_sync(ADC1);

  ADC1->CALIB.reg = 
  (
    ADC1_FUSES_BIASREFBUF(refbuf) |
    ADC1_FUSES_BIASCOMP(comp) |
    ADC1_FUSES_BIASR2R(r2r)
  );

  ADC1->CTRLA.reg |= ADC_CTRLA_PRESCALER_DIV8;

  ADC1->SAMPCTRL.reg |= ADC_SAMPCTRL_SAMPLEN(3U - 1);

  ADC1->REFCTRL.reg |= ADC_REFCTRL_REFSEL_INTVCC1;

  ADC1->INPUTCTRL.reg = 
  (
    ADC_INPUTCTRL_MUXPOS_AIN1 |
    ADC_INPUTCTRL_MUXNEG_GND
  );

  ADC1->CTRLB.reg =
  (
    ADC_CTRLB_RESSEL_12BIT |
    ADC_CTRLB_WINMODE(0U) |
    ADC_CTRLB_FREERUN
  );

  ADC1->INTFLAG.reg = ADC_INTFLAG_MASK;
  
  ADC_sync(ADC1);
}

typedef enum {IDLE, EMIT, WAIT, LISTEN} data_acquisition_state;
typedef enum {START_JOB = 0x10, AMP_STOP = 0xff, AMP_START = 0xfe, GET_RUN_INFO = 0xfd} host_command;

data_acquisition_state dstate = IDLE;

void setup(void) 
{

//#ifndef MODE_HARD_TRIG
  JETSON_SERIAL.begin(115200);
  //while(!Serial);

//#endif
  int f0 = 150E3;
  int f1 = 10E3;

  chirp_out_source_address = generate_chirp(f0, f1);

  MCLK_init();
  GCLK_init();

  amp_disable();

  DMAC_init(&base_descriptor[0], &wb_descriptor[0]);

  //emit_resonator_timer_init();

#ifdef MODE_HARD_TRIG

  state_timer_init();
  hardware_int_trigger_init();

#endif
  //emit_modulator_timer_init();
  dac_init();

  wait_timer_init();

  dac_sample_timer_init();

  job_led_toggle();

  adc0_init();
  adc1_init();

  //TCC_ENABLE(TCC0);
  //TCC_sync(TCC0);

  //TCC_ENABLE(TCC1);
  //TCC_sync(TCC1);

  //TCC_FORCE_STOP(TCC1);
  //TCC_sync(TCC1);
  
  ML_ADC_ENABLE(ADC0);
  ADC_sync(ADC0);

  ML_ADC_ENABLE(ADC1);
  ADC_sync(ADC1);

  ML_ADC_SWTRIG_START(ADC0);
  ADC_sync(ADC0);

  ML_ADC_SWTRIG_START(ADC1);
  ADC_sync(ADC1);

  DAC->CTRLA.bit.ENABLE = 1;
  while(DAC->SYNCBUSY.bit.ENABLE || DAC->SYNCBUSY.bit.SWRST);

  TCC_ENABLE(TCC2);
  TCC_sync(TCC2);

  TCC_FORCE_STOP(TCC2);
  TCC_sync(TCC2);

  ML_DMAC_ENABLE();

  ML_DMAC_CHANNEL_ENABLE(DMAC_EMIT_MODULATOR_TIMER_CHANNEL);
  ML_DMAC_CHANNEL_SUSPEND(DMAC_EMIT_MODULATOR_TIMER_CHANNEL);

  ML_DMAC_CHANNEL_ENABLE(ADC0_DMAC_CHANNEL);
  ML_DMAC_CHANNEL_SUSPEND(ADC0_DMAC_CHANNEL);

  ML_DMAC_CHANNEL_ENABLE(ADC1_DMAC_CHANNEL);
  ML_DMAC_CHANNEL_SUSPEND(ADC1_DMAC_CHANNEL);

}

#define N_ADC_SAMPLES 30000

boolean emit_start_intflag = false;
boolean emit_stop_intflag = false;
boolean wait_stop_intflag = false;
boolean adc0_done_intflag = false;
boolean adc1_done_intflag = false;

void loop(void) 
{
  
  switch(dstate)
  {

    case IDLE: 
    {

      if(emit_start_intflag)
      {

        //TCC_FORCE_RETRIGGER(TCC1);
        //TCC_sync(TCC1);

        ML_DMAC_CHANNEL_RESUME(DMAC_EMIT_MODULATOR_TIMER_CHANNEL);

        dstate = EMIT;       

        emit_start_intflag = false;
      }
      
      break;  
    }

    case EMIT: 
    { 

      if(emit_stop_intflag)
      {

        //TCC_FORCE_STOP(TCC1);
        //TCC_sync(TCC1);

        TCC_FORCE_RETRIGGER(TCC2);
        TCC_sync(TCC2);
      
        dstate = WAIT;

        emit_stop_intflag = false;
      }
      
      break; 
    }

    case WAIT: 
    { 

      if(wait_stop_intflag)
      {

        ML_DMAC_CHANNEL_RESUME(ADC0_DMAC_CHANNEL);
        ML_DMAC_CHANNEL_RESUME(ADC1_DMAC_CHANNEL);

        dstate = LISTEN;

        wait_stop_intflag = false;

      }
      break;

    }

    case LISTEN: 
    {

      if(adc0_done_intflag & adc1_done_intflag)
      {

        adc0_done_intflag = adc1_done_intflag = false;

        uint16_t chunk_size = 2*N_ADC_SAMPLES/8;

        uint8_t *chunk_ptr0 = reinterpret_cast<uint8_t *>(&adc0_samples[0]);
        for(uint16_t i=0; i < 8; i++, chunk_ptr0 += chunk_size)
        {
          JETSON_SERIAL.write(chunk_ptr0, sizeof(uint8_t) * chunk_size);
        }

        uint8_t *chunk_ptr1 = reinterpret_cast<uint8_t *>(&adc1_samples[0]);
        for(uint16_t i=0; i < 8; i++, chunk_ptr1 += chunk_size)
        {
          JETSON_SERIAL.write(chunk_ptr1, sizeof(uint8_t) * chunk_size);
        }

        //TC2->COUNT16.CTRLBSET.reg |= TC_CTRLBSET_CMD_RETRIGGER;
        //while(TC2->COUNT16.SYNCBUSY.bit.CTRLB);

        job_led_toggle();

        dstate = IDLE;

      }
      break;

    }
  }

#ifndef MODE_HARD_TRIG

  if(JETSON_SERIAL.available())
  {

    host_command opcode = (host_command)JETSON_SERIAL.read();

    if(opcode <= START_JOB)
    {

      if(dstate == IDLE && opcode == START_JOB)
      {
        emit_start_intflag = true;

        job_led_toggle();
      }
    }

    else if(opcode == AMP_STOP)
    {
      amp_disable();
    }

    else if(opcode == AMP_START)
    {
      amp_enable();
    }
    else if(opcode == GET_RUN_INFO)
    {

      int f0_recv = 0;
      int f1_recv = 0;

      for(int8_t j = (32 - 8) ; j >= 0; j -= 8)
      {
        f0_recv |= JETSON_SERIAL.read() << j;
        f1_recv |= JETSON_SERIAL.read() << j;
      }

      generate_chirp(f0_recv, f1_recv);
    }
  }
#endif

}

void DMAC_0_Handler(void)
{

  if(ML_DMAC_CHANNEL_IS_SUSP(ADC0_DMAC_CHANNEL))
  {
    ML_DMAC_CHANNEL_CLR_SUSP_INTFLAG(ADC0_DMAC_CHANNEL);

    adc0_done_intflag = true;
  }

}

void DMAC_1_Handler(void)
{

  if(ML_DMAC_CHANNEL_IS_SUSP(ADC1_DMAC_CHANNEL))
  {

    ML_DMAC_CHANNEL_CLR_SUSP_INTFLAG(ADC1_DMAC_CHANNEL);

    adc1_done_intflag = true;

  }

}


void DMAC_2_Handler(void)
{
  
  if(ML_DMAC_CHANNEL_IS_SUSP(DMAC_EMIT_MODULATOR_TIMER_CHANNEL))
  {

    ML_DMAC_CHANNEL_CLR_SUSP_INTFLAG(DMAC_EMIT_MODULATOR_TIMER_CHANNEL);

    emit_stop_intflag = true;
  }

}

void TCC2_0_Handler(void)
{

  if(TCC_IS_OVF(TCC2))
  {

    TCC_CLR_OVF_INTFLAG(TCC2);

    wait_stop_intflag = true;
  }
}


void AC_Handler(void)
{
  
  static int ac_trig_cnt = 0;

  ML_AC_CLR_COMP0_INTFLAG();
  
  if(ac_trig_cnt != 0 && dstate == IDLE)
  {
      emit_start_intflag = true;
  } 
  
  ac_trig_cnt++;

}



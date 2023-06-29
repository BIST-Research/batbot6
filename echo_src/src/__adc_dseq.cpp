#include <Arduino.h>

const uint16_t nsamples = 2 * 6250;

uint16_t adc_samples[nsamples];

#define PASTE_FUSE(REG) ((*((uint32_t *) (REG##_ADDR)) & (REG##_Msk)) >> (REG##_Pos))

const uint32_t input_ctrl[] = {ADC_INPUTCTRL_MUXPOS_AIN2, ADC_INPUTCTRL_MUXPOS_AIN15};

const uint32_t refbuf = PASTE_FUSE(ADC0_FUSES_BIASREFBUF);
const uint32_t r2r = PASTE_FUSE(ADC0_FUSES_BIASR2R);
const uint32_t comp = PASTE_FUSE(ADC0_FUSES_BIASCOMP);

#define DMAC_DSEQ_CHANNEL 0x00

const uint32_t dseq_dmac_channel_settings = DMAC_CHCTRLA_TRIGSRC(ADC0_DMAC_ID_SEQ) | 
                                            DMAC_CHCTRLA_TRIGACT_BURST;
                                    
const uint16_t dseq_dmac_descriptor_beat_settings = DMAC_BTCTRL_BEATSIZE_HWORD |
                                                    DMAC_BTCTRL_SRCINC         |
                                                    DMAC_BTCTRL_VALID;

#define DMAC_RESULT_CHANNEL 0x01                                   

const uint32_t result_dmac_channel_settings = DMAC_CHCTRLA_TRIGSRC(ADC0_DMAC_ID_RESRDY) |
                                            DMAC_CHCTRLA_TRIGACT_BURST;

const uint16_t result_dmac_descriptor_beat_settings = DMAC_BTCTRL_BEATSIZE_HWORD |
                                                      DMAC_BTCTRL_DSTINC         |
                                                      DMAC_BTCTRL_EVOSEL_BURST  |
                                                      DMAC_BTCTRL_VALID          |
                                                      DMAC_BTCTRL_BLOCKACT_BOTH;

void adc_init(void){
/*
  ML_SET_GCLK1_PCHCTRL(ADC0_GCLK_ID);

  DMAC_channel_init(
    DMAC_DSEQ_CHANNEL, 
    dseq_dmac_channel_settings, 
    DMAC_CHPRILVL_PRILVL_LVL0
  );

  DMAC_descriptor_init(
    dseq_dmac_descriptor_beat_settings,
    2,
    (uint32_t)input_ctrl + sizeof(uint32_t) * 2,
    (uint32_t) &ADC0->DSEQDATA.reg,
    (uint32_t) &base_descriptor[DMAC_DSEQ_CHANNEL],
    &base_descriptor[DMAC_DSEQ_CHANNEL]
  );

  DMAC_channel_init(
    DMAC_RESULT_CHANNEL, 
    result_dmac_channel_settings, 
    DMAC_CHPRILVL_PRILVL_LVL0
  );

  DMAC_descriptor_init(
    result_dmac_descriptor_beat_settings,
    nsamples,
    (uint32_t)&ADC0->RESULT.reg,
    (uint32_t)adc_samples + sizeof(uint16_t) * nsamples,
    (uint32_t)&base_descriptor[DMAC_RESULT_CHANNEL],
    &base_descriptor[DMAC_RESULT_CHANNEL]
  );*/

  //DMAC_channel_intenset(DMAC_RESULT_CHANNEL, DMAC_1_IRQn, DMAC_CHINTENSET_SUSP, 0);

  //ML_ADC_DISABLE(ADC0);
  //ML_ADC_SWRST(ADC0);

  ADC0->CALIB.reg = (ADC0_FUSES_BIASREFBUF(refbuf) | ADC0_FUSES_BIASCOMP(comp) | ADC0_FUSES_BIASR2R(r2r));

  ADC0->INPUTCTRL.reg = ADC_INPUTCTRL_MUXPOS_AIN15 |
                        ADC_INPUTCTRL_MUXNEG_GND;
  //ADC_sync(ADC0);

  //ADC0->INPUTCTRL.bit.DIFFMODE = 0x1;

  ADC0->SAMPCTRL.reg = ADC_SAMPCTRL_SAMPLEN(0U);
  //ADC_sync(ADC0);

  ADC0->DSEQCTRL.reg = ADC_DSEQCTRL_AUTOSTART | 
                       ADC_DSEQCTRL_INPUTCTRL;

  //ADC0->AVGCTRL.reg = ADC_AVGCTRL_ADJRES(0x1) | ADC_AVGCTRL_SAMPLENUM_4;

  ADC0->CTRLA.reg = ADC_CTRLA_PRESCALER_DIV8; //750 kHz
  ADC0->CTRLB.reg = ADC_CTRLB_RESSEL_12BIT;

  ADC0->REFCTRL.reg = ADC_REFCTRL_REFSEL_INTVCC0;
  //ADC_sync(ADC0);

}

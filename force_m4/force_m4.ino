
#include <Adafruit_ZeroDMA.h>
#include <wiring_private.h>

const int samp_duration = 30;
const int num_pages = (samp_duration + 4.4)/4.4;
const int page_size = 2048;

__attribute__ ((section(".dmabuffers"), used)) 
uint16_t right_in_buf[num_pages][page_size], left_in_buf[num_pages][page_size];

const int b_buf_size = sizeof(uint16_t)*num_pages*page_size;

static DmacDescriptor tmp_descriptor __attribute__((aligned(16)));

uint16_t right_in_idx, left_in_idx;

Adafruit_ZeroDMA right_in_dma, left_in_dma;

void dma_left_complete(Adafruit_ZeroDMA *dma){
  left_in_idx++;
}

void dma_right_complete(Adafruit_ZeroDMA *dma){
  right_in_idx++;
}

void clock_init(void){

  GCLK->GENCTRL[7].reg = GCLK_GENCTRL_DIV(1) |
                         GCLK_GENCTRL_IDC    |
                         GCLK_GENCTRL_GENEN  |
                         GCLK_GENCTRL_SRC_DFLL;

  while(GCLK->SYNCBUSY.bit.GENCTRL7);

  GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK7;

  GCLK->PCHCTRL[ADC1_GCLK_ID].reg = GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1;
  
}

void timer_init(void){

  TCC0->CTRLA.reg = TCC_CTRLA_PRESCALER_DIV4 | TCC_CTRLA_PRESCSYNC_PRESC;

  TCC0->WAVE.reg = TC_WAVE_WAVEGEN_NPWM;             // Set-up TCC0 timer for Normal (single slope) PWM mode (NPWM)
  while (TCC0->SYNCBUSY.bit.WAVE);

  TCC0->PER.reg = 12;                            // Set-up the PER (period) register 50Hz PWM
  while (TCC0->SYNCBUSY.bit.PER);
  
  TCC0->CC[0].reg = 6;                           // Set-up the CC (counter compare), channel 0 register for 50% duty-cycle
  while (TCC0->SYNCBUSY.bit.CC0);

  TCC0->CTRLA.bit.ENABLE = 1;                        // Enable timer TCC0
  while (TCC0->SYNCBUSY.bit.ENABLE);
}

void dma_init(void){
  
   static DmacDescriptor *left_in_descs[num_pages], *right_in_descs[num_pages];
  
  // Create left ADC channel DMA job
  {
    left_in_dma.allocate();
  
    for (auto i = 0; i < num_pages; i++) {
      left_in_descs[i] = left_in_dma.addDescriptor(
        (void *)(&ADC0->RESULT.reg),
        left_in_buf[i],
        page_size,
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
  
    for (auto i = 0; i < num_pages; i++) {
      right_in_descs[i] = right_in_dma.addDescriptor(
        (void *)(&ADC1->RESULT.reg),
        right_in_buf[i],
        page_size,
        DMA_BEAT_SIZE_HWORD,
        false,
        true);
      right_in_descs[i]->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;
    }
    right_in_dma.setTrigger(0x46); // Trigger on ADC1 read completed
    right_in_dma.setAction(DMA_TRIGGER_ACTON_BEAT);
    right_in_dma.setCallback(dma_right_complete);
  }
  
}

void adc_init(int inpselCFG, Adc *ADCx){
  
  // Configure the ADC pin (cheating method)
  pinPeripheral(inpselCFG, PIO_ANALOG);
  
  ADCx->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND;   // No Negative input (Internal Ground)
  while( ADCx->SYNCBUSY.reg & ADC_SYNCBUSY_INPUTCTRL );
  
  ADCx->INPUTCTRL.bit.MUXPOS = g_APinDescription[inpselCFG].ulADCChannelNumber; // Selection for the positive ADC input
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

void setup() {

  //Serial.begin(128000);
  while(!Serial);

  bzero((void*)&right_in_buf[0], b_buf_size);
  bzero((void*)&left_in_buf[0], b_buf_size);

  left_in_idx = right_in_idx = 0;

  clock_init();
  adc_init(A1, ADC0);
  adc_init(A3, ADC1);
  dma_init();
  timer_init();

  ADC0->SWTRIG.bit.START = 1;
  ADC1->SWTRIG.bit.START = 1;
  
}

void loop() {

  static auto data_ready = false;

  if(Serial.available()){

    uint8_t opcode = Serial.read();

    if(opcode == 0x10){

      data_ready = false;
      left_in_idx = 0;
      right_in_idx = 0;

      left_in_dma.startJob();
      right_in_dma.startJob();
      
    }

    else if(opcode == 0x20){
      Serial.write(data_ready);
    }

    else if(opcode == 0x30){
      
      Serial.write(num_pages - left_in_idx);

      while(left_in_idx < num_pages){

        Serial.write(
          reinterpret_cast<uint8_t *>(left_in_buf[left_in_idx++]), 
          sizeof(uint16_t) * page_size);
          
      }
    }

    else if(opcode == 0x31){
      
      Serial.write(num_pages - right_in_idx);

      while(right_in_idx < num_pages){

        Serial.write(
          reinterpret_cast<uint8_t *>(right_in_buf[right_in_idx++]), 
          sizeof(uint16_t) * page_size);
          
      }
    }
  }

  if(!data_ready && left_in_idx == num_pages && right_in_idx == num_pages){

    data_ready = true;

    left_in_idx = right_in_idx = 0;
    
  }

}

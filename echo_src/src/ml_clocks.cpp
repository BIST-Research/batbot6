#include "header/ml_clocks.hpp"

/*
 * Initializes the main clock, the clock used by the CPU.
 */
void MCLK_init(void){

  //DPLL1_init();                         


  // initial main clk division of 1
  MCLK->CPUDIV.reg = ML_MCLK_CPUDIV1;

  // set peripheral bus access, so we can access their registers in setup and main loop
  MCLK->AHBMASK.reg |= MCLK_AHBMASK_DMAC;
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_CCL;
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_AC;
  MCLK->APBAMASK.reg |= MCLK_APBAMASK_TC0;
  MCLK->APBBMASK.reg |= MCLK_APBBMASK_TC2;
  MCLK->APBBMASK.reg |= MCLK_APBBMASK_TC3;

}


/*
 * Initializes digital phase locked loop 1. 
 * This is a clock generator which takes in a 32 kHz clock
 * And uses a phase locked loop and a frequency multiplier to 
 * Obtain the frequency we want. This clock signal is then fed to 
 * The generic clock controller 7 (GCLK7) to feed our peripherals.
 * 
 * We need to setup DPLL1 because we want to feed the analog to
 * digital converter (ADC0) with a specific clock frequency which
 * cannot be obtained with normal clock divisions.
 * 
 * f_dppln = f_ckr * (LDR + 1 + LDRFRAC/32)
 * 
 * where f_ckr = 32 kHz in our case 
 * (this is set by with OSCCTRL_DPLLCTRLB_REFCLK_XOSC32)
 * 
 * We want f_dpll1 = 104 MHz, so LDR = 3249 and LDRFRAC = 0
 * 
 */

#define DPLL1_INTEGER_MULTIPLICATION_VALUE (3249)

void DPLL1_init(void){

    OSCCTRL->Dpll[1].DPLLCTRLB.reg = OSCCTRL_DPLLCTRLB_REFCLK_XOSC32;

    OSCCTRL->Dpll[1].DPLLRATIO.reg = OSCCTRL_DPLLRATIO_LDR(DPLL1_INTEGER_MULTIPLICATION_VALUE);

    OSCCTRL->Dpll[1].DPLLCTRLA.bit.ENABLE = 0x1;

}

/*
 * Initializes the generic clock (GCLK7) to be used by peripherals (TCC, ADC, AC, etc)
 */
void GCLK_init(void){ 


  // GCLK divider, GCLK7_FREQ = ML_MCLK_UNDIV/(ML_MCLK_CPUDIV * ML_GCLK_GENCTRL_DIV) 
  // 50/50 duty 
  // Source multiplexer selects DPLL1
  // Output enable, Generator enable
  GCLK->GENCTRL[ML_GCLK_CH].reg = GCLK_GENCTRL_DIV(1) |                
                                  GCLK_GENCTRL_IDC |                  
                                  GCLK_GENCTRL_SRC_DPLL0 |             
                                  GCLK_GENCTRL_GENEN;

  // wait for GEN7 sync
  while(GCLK->SYNCBUSY.bit.GENCTRL7);


  //GCLK->PCHCTRL[25].reg = 0;
  GCLK->PCHCTRL[TCC0_GCLK_ID].reg = GCLK_PCHCTRL_CHEN |        // Enable the TCC0 perhipheral channel
                                  GCLK_PCHCTRL_GEN_GCLK7;    // Route generic clock 1 to TCC0

  GCLK->PCHCTRL[DAC_GCLK_ID].reg = GCLK_PCHCTRL_CHEN |        // Enable the TCC0 perhipheral channel
                                  GCLK_PCHCTRL_GEN_GCLK4;  

}



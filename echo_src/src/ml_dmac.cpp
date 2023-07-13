/*
 * Author: Ben Westcott
 * Date created: 3/6/23
 */

#include "header/ml_dmac.hpp"

void DMAC_channel_intenset(const uint8_t channel, const IRQn_Type IRQn, const uint8_t intmsk, const uint32_t priority_level){

    DMAC->Channel[channel].CHINTENSET.reg |= intmsk;
    
    NVIC_SetPriority(IRQn, priority_level);

    NVIC_EnableIRQ(IRQn);
}

void DMAC_init(DmacDescriptor *base_descriptor, volatile DmacDescriptor *writeback_descriptor){

  // disable and software reset 
  ML_DMAC_DISABLE();
  ML_DMAC_SWRST();

  // This is the base address in memory of where transfer descriptors exist
  DMAC->BASEADDR.reg = (uint32_t) base_descriptor;

  // SRAM addr of write-back section written to WRBADDR reg
  DMAC->WRBADDR.reg = (uint32_t) writeback_descriptor;

  // Instruct the DMAC arbitor to respect channel priority levels 0 through 3
  DMAC->CTRL.reg |= DMAC_CTRL_LVLEN0 | DMAC_CTRL_LVLEN1 | DMAC_CTRL_LVLEN2 | DMAC_CTRL_LVLEN3;

}

/*
 *
 * Initializes a given channel of the DMAC.
 * Essentially, we can split up the different "responsibilities" of the DMAC
 * into different channels, i.e. one channel for moving data from the ADC to memory 
 * and another for moving data from memeory to a TCC instance
 * 
 */
void DMAC_channel_init(const uint8_t channel, const uint32_t settings, const uint8_t priority_level){

  ML_DMAC_CHANNEL_DISABLE(channel);
  ML_DMAC_CHANNEL_SWRST(channel);

  DMAC->Channel[channel].CHCTRLA.reg = settings;

  // If the DMAC has two or more channels which need data moved, the priority level of the
  // channel tells the DMAC which to serve first
  DMAC->Channel[channel].CHPRILVL.reg = priority_level;

}


/*
 *
 * Initializes descriptors to be used by the direct memory access controller (DMAC)
 * 
 * A descriptor is a datastructure which holds important information about the 
 * data that needs to be moved from one place to another
 * 
 */
void DMAC_descriptor_init(const uint16_t btsettings, 
                                const uint16_t btcnt, 
                                const uint32_t srcaddr, 
                                const uint32_t dstaddr, 
                                const uint32_t descaddr,
                                DmacDescriptor *cpy) {

  DmacDescriptor descriptor;

  descriptor.BTCTRL.reg = btsettings;

  // number of beats, ie bytes, halfwords or words to send
  // This is typically the length of the buffer we want to fill, or take data from
  descriptor.BTCNT.reg = btcnt;

  // Location in memory of data to send. SRCADDR accepts last memory location of data
  descriptor.SRCADDR.reg = srcaddr;

  // Send values to TCC0 count register, more specifically the update buffer, 
  // so byte placed in CCBUF from DMAC will be seen in CC reg next clock cycle
  descriptor.DSTADDR.reg = dstaddr;

  // What descriptor to point to when the DMAC is finished serving the current descriptor.
  // Typically we want to just point back to the same descriptor
  descriptor.DESCADDR.reg = descaddr;

  // copy setup descriptor ptr into base descriptor allocation
  memcpy((void *)cpy, &descriptor, sizeof(DmacDescriptor));

}
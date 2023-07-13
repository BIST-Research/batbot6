/*
 * Author: Ben Westcott
 * Date created: 3/6/23
 */

#include <Arduino.h>

#define ML_DMAC_ENABLE() (DMAC->CTRL.reg |= DMAC_CTRL_DMAENABLE)

#define ML_DMAC_DISABLE() (DMAC->CTRL.reg &= ~DMAC_CTRL_DMAENABLE)

#define ML_DMAC_SWRST() (DMAC->CTRL.reg |= DMAC_CTRL_SWRST)


#define ML_DMAC_CHANNEL_ENABLE(channel) (DMAC->Channel[channel].CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE)

#define ML_DMAC_CHANNEL_DISABLE(channel) (DMAC->Channel[channel].CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE)

#define ML_DMAC_CHANNEL_SWRST(channel) (DMAC->Channel[channel].CHCTRLA.reg |= DMAC_CHCTRLA_SWRST)


#define ML_DMAC_CHANNEL_IS_SUSP(channel) (DMAC->Channel[channel].CHINTFLAG.bit.SUSP)

#define ML_DMAC_CHANNEL_CLR_SUSP_INTFLAG(channel) (DMAC->Channel[channel].CHINTFLAG.bit.SUSP = 0x1)

#define ML_DMAC_CHANNEL_RESUME(channel) (DMAC->Channel[channel].CHCTRLB.reg |= DMAC_CHCTRLB_CMD_RESUME)

#define ML_DMAC_CHANNEL_SUSPEND(channel) (DMAC->Channel[channel].CHCTRLB.reg |= DMAC_CHCTRLB_CMD_SUSPEND)

void DMAC_channel_intenset(const uint8_t channel, const IRQn_Type IRQn, const uint8_t intmsk, const uint32_t priority_level);

void DMAC_init(DmacDescriptor *base_descriptor, volatile DmacDescriptor *writeback_descriptor);

void DMAC_channel_init(const uint8_t channel, const uint32_t settings, const uint8_t priority_level);

void DMAC_descriptor_init(const uint16_t btsettings, 
                                const uint16_t btcnt, 
                                const uint32_t srcaddr, 
                                const uint32_t dstaddr, 
                                const uint32_t descaddr,
                                DmacDescriptor *cpy);
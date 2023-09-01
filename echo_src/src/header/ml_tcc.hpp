/*
 * Author: Ben Westcott
 * Date created: 3/8/23
 */
#include <Arduino.h>

#define ML_TCC0_CH0 0x0
#define ML_TCC0_CH1 0x1
#define ML_TCC0_CH2 0x2
#define ML_TCC0_CH3 0x3

#define ML_TCC1_CH0 0x0
#define ML_TCC1_CH1 0x1
#define ML_TCC1_CH2 0x2
#define ML_TCC1_CH3 0x3
#define ML_TCC1_CH7 0x7

// REQUIRES R/W SYNC
#define TCC_ENABLE(instance) (instance->CTRLA.reg |= TCC_CTRLA_ENABLE)

// REQUIRES R/W SYNC
#define TCC_DISABLE(instance) (instance->CTRLA.reg &= ~TCC_CTRLA_ENABLE)

// REQUIRES R/W SYNC
#define TCC_SWRST(instance) (instance->CTRLA.reg |= TCC_CTRLA_SWRST)

// REQUIRES R/W SYNC
#define TCC_SET_ONESHOT(instance) (instance->CTRLBSET.reg |= TCC_CTRLBSET_ONESHOT)

// REQUIRES R/W SYNC
#define TCC_CLR_ONESHOT(instance) (instance->CTRLBCLR.reg |= TCC_CTRLBCLR_ONESHOT)

// REQUIRES R/W SYNC
#define TCC_FORCE_STOP(instance) (instance->CTRLBSET.reg |= TCC_CTRLBSET_CMD_STOP)

// REQUIRES R/W SYNC
#define TCC_FORCE_RETRIGGER(instance) (instance->CTRLBSET.reg |= TCC_CTRLBSET_CMD_RETRIGGER)

#define TCC_IS_OVF(instance) (instance->INTFLAG.bit.OVF)

#define TCC_CLR_OVF_INTFLAG(instance) (instance->INTFLAG.bit.OVF = 0x1)

void TCC_sync(Tcc *instance);

void TCC_set_period(Tcc *instance, uint32_t value);

void TCC_channel_capture_compare_set(Tcc *instance, const uint8_t channel, const uint8_t value);

void TCC_intenset(Tcc *instance, const IRQn_Type IRQn, const uint8_t interrupt_mask, const uint32_t priority_level);

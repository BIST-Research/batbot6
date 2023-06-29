/*
 * Author: Ben Westcott
 * Date created: 3/12/23
 */

#include <Arduino.h>

#define ML_AC_ENABLE()(AC->CTRLA.reg |= AC_CTRLA_ENABLE)

#define ML_AC_DISABLE()(AC->CTRLA.reg &= ~AC_CTRLA_ENABLE)

#define ML_AC_SWRST()(AC->CTRLA.reg |= AC_CTRLA_SWRST)

#define ML_AC_SET_CONT_MEASURE(channel)(AC->COMPCTRL[channel].reg &= ~AC_COMPCTRL_SINGLE)

#define ML_AC_SET_SINGLE_MEASURE(channel)(AC->COMPCTRL[channel].reg |= AC_COMPCTRL_SINGLE)

#define ML_AC_CLR_COMP0_INTFLAG() (AC->INTFLAG.bit.COMP0 = 0x1)

#define ML_AC_IS_COMP0_INT() (AC->INTFLAG.bit.COMP0)

void AC_sync(void) { while(AC->SYNCBUSY.reg); }

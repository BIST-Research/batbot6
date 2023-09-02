/*
 * Author: Ben Westcott
 * Date created: 1/27/23
 */
#include <Arduino.h>

#define ML_MCLK_UNDIV 120000000
#define ML_MCLK_CPUDIV1 (MCLK_CPUDIV_DIV(MCLK_CPUDIV_DIV_DIV1_Val))

void MCLK_init(void);

/*
        GCLK definitions
*/

#define ML_GCLK_CH 7

// Channel enable, GCLK7, WRTLCK - disable future writing to reg
#define ML_GCLK7_PCHCTRL (GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK7 )
#define ML_GCLK1_PCHCTRL (GCLK_PCHCTRL_CHEN | GCLK_PCHCTRL_GEN_GCLK1)

#define ML_SET_GCLK7_PCHCTRL(id)(GCLK->PCHCTRL[id].reg = ML_GCLK7_PCHCTRL)
#define ML_SET_GCLK1_PCHCTRL(id)(GCLK->PCHCTRL[id].reg = ML_GCLK1_PCHCTRL)

void GCLK_init(void);
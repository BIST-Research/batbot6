/*
 * Author: Ben Westcott
 * Date created: 1/27/23
 */
#include <Arduino.h>

typedef enum
{
    PF_A, PF_B, PF_C, PF_D, PF_E, PF_F, PF_G, 
    PF_H, PF_I, PF_J, PF_K, PF_L, PF_M, PF_N
} ml_port_function;

typedef enum
{
    PP_ODD, PP_EVEN
} ml_port_parity;

typedef enum 
{ 
    DRIVE_OFF = 0x00, 
    DRIVE_ON = 0x01
} ml_port_drive_strength;

typedef enum 
{

    INPUT_STANDARD = 0x02,
    INPUT_PULL_DOWN = 0x06,
    INPUT_PULL_UP = 0x16,
    OUTPUT_TOTEMPOLE_INPUT_DISABLED = 0x01,
    OUTPUT_TOTEMPOLE_INPUT_ENABLED = 0x03,
    OUTPUT_PULL_DOWN = 0x04,
    OUTPUT_PULL_UP = 0x14,
    ANALOG = 0x00

} ml_port_config;

void peripheral_port_init
(
    const uint8_t pmux_mask,
    const uint8_t m4_pin, 
    const ml_port_config config, 
    const ml_port_drive_strength drive
);

void peripheral_port_init_alt
(
    const ml_port_function func,
    const ml_port_parity parity,
    const uint8_t m4_pin,
    const ml_port_config config,
    const ml_port_drive_strength drive
);                    
/*
 *
 * M4 Express pin definitions
 * 
 */

#define ML_M4E_TCC0_CH0_PIN 7
#define ML_M4E_TCC0_CH0_XPIN 12
#define ML_M4E_TCC0_CH0_PMUX 0x6
#define ML_M4E_TCC0_CH0_PMUX_msk (PORT_PMUX_PMUXE(ML_M4E_TCC0_CH0_PMUX))

#define ML_M4E_TCC0_CH1_PIN 4
#define ML_M4E_TCC0_CH1_XPIN 13
#define ML_M4E_TCC0_CH1_PMUX 0x6
#define ML_M4E_TCC0_CH1_PMUX_msk (PORT_PMUX_PMUXO(ML_M4E_TCC0_CH1_PMUX))

#define ML_M4E_TCC0_CH3_PIN 0
#define ML_M4E_TCC0_CH3_XPIN 23
#define ML_M4E_TCC0_CH3_PMUX 0x6
#define ML_M4E_TCC0_CH3_PMUX_msk (PORT_PMUX_PMUXO(ML_M4E_TCC0_CH3_PMUX))


// PA16 -> Peripheral function F
#define ML_M4E_TCC1_CH0_PIN 13
#define ML_M4E_TCC1_CH0_XPIN 16
#define ML_M4E_TCC1_CH0_PMUX 0x5
#define ML_M4E_TCC1_CH0_PMUX_msk (PORT_PMUX_PMUXE(ML_M4E_TCC1_CH0_PMUX))

// PA17 -> Peripheral function F
#define ML_M4E_TCC1_CH1_PIN 12
#define ML_M4E_TCC1_CH1_XPIN 17
#define ML_M4E_TCC1_CH1_PMUX 0x5
#define ML_M4E_TCC1_CH1_PMUX_msk (PORT_PMUX_PMUXO(ML_M4E_TCC1_CH1_PMUX))

//PA23 -> Peripheral function F
#define ML_M4E_TCC1_CH7_PIN 0
#define ML_M4E_TCC1_CH7_XPIN 23
#define ML_M4E_TCC1_CH7_PMUX 0x5
#define ML_M4E_TCC1_CH7_PMUX_msk (PORT_PMUX_PMUXO(ML_M4E_TCC1_CH7_PMUX))

// PA02 -> Peripheral function B
#define ML_M4E_ADC0_AIN0_PIN A0
#define ML_M4E_ADC0_AIN0_XPIN 2
#define ML_M4E_ADC0_AIN0_PMUX 0x1
#define ML_M4E_ADC0_AIN0_PMUX_msk (PORT_PMUX_PMUXE(ML_M4E_ADC0_AIN0_PMUX))

// PB08 -> Peripheral function B
#define ML_M4E_ADC1_AIN0_PIN A4
#define ML_M4E_ADC1_AIN0_XPIN 8
#define ML_M4E_ADC1_AIN0_PMUX 0x1
#define ML_M4E_ADC1_AIN0_PMUX_msk (PORT_PMUX_PMUXE(ML_M4E_ADC1_AIN0_PMUX))

// PA04 --> Peripheral function B
#define ML_M4E_AC_AIN0_PIN A3
#define ML_M4E_AC_AIN0_XPIN 4
#define ML_M4E_AC_AIN0_PMUX 0x1
#define ML_M4E_AC_AIN0_PMUX_msk (PORT_PMUX_PMUXE(ML_M4E_AC_AIN0_PMUX))

/*
 *
 * M4 GrandCentral pin definitions
 * 
 */

// For D45, TCC0.0: PC12 -> peripheral function F and PMUXE
#define ML_M4GC_TCC0_CH0_PIN 45
#define ML_M4GC_TCC0_CH0_XPIN 10
#define ML_M4GC_TCC0_CH0_PMUX 0x5                       
#define ML_M4GC_TCC0_CH0_PMUX_msk (PORT_PMUX_PMUXE(ML_M4GC_TCC0_CH0_PMUX))

// For D44, TCC0.1: PC11 -> peripheral function F and PMUXO
#define ML_M4GC_TCC0_CH1_PIN 44
#define ML_M4GC_TCC0_CH1_XPIN 11
#define ML_M4GC_TCC0_CH1_PMUX 0x5
#define ML_M4GC_TCC0_CH1_PMUX_msk (PORT_PMUX_PMUXO(ML_M4GC_TCC0_CH1_PMUX))

// For D23, TCC1.3: PA15 -> peripheral function G and PMUXO
#define ML_M4GC_TCC1_CH3_PIN 23
#define ML_M4GC_TCC1_CH3_XPIN 15
#define ML_M4GC_TCC1_CH3_PMUX 0x6
#define ML_M4GC_TCC1_CH3_PMUX_msk (PORT_PMUX_PMUXO(ML_M4GC_TCC1_CH3_PMUX))

#define ML_M4GC_TCC1_CH0_PIN 8
#define ML_M4GC_TCC1_CH0_XPIN 18 //PB18, TCC1.0 : function f, PMUXE
#define ML_M4GC_TCC1_CH0_PMUX 0x5
#define ML_M4GC_TCC1_CH0_PMUX_msk (PORT_PMUX_PMUXE(ML_M4GC_TCC1_CH0_PMUX))

// For D3, TCC0.3: PC19 -> peripheral function F and PMUXO
#define ML_M4GC_TCC0_CH3_PIN 3
#define ML_M4GC_TCC0_CH3_XPIN 19
#define ML_M4GC_TCC0_CH3_PMUX 0x5
#define ML_M4GC_TCC0_CH3_PMUX_msk (PORT_PMUX_PMUXO(ML_M4GC_TCC0_CH3_PMUX))


// CCL_WO[0] - PB23: D11, peripheral function N
#define ML_M4GC_CCL_CH0_PIN 11
#define ML_M4GC_CCL_CH0_XPIN 23
#define ML_M4GC_CCL_CH0_PMUX 0xD
#define ML_M4GC_CCL_CH0_PMUX_msk (PORT_PMUX_PMUXO(ML_M4GC_CCL_CH0_PMUX))

// PA05, A1: peripheral function B, PMUXO
#define ML_M4GC_ADC0_AIN5_PIN A1
#define ML_M4GC_ADC0_AIN5_XPIN 5
#define ML_M4GC_ADC0_AIN5_PMUX 0x1
#define ML_M4GC_ADC0_AIN5_PMUX_msk (PORT_PMUX_PMUXO(ML_M4GC_ADC0_AIN5_PMUX))

// PB04, A7: peripheral function B, PMUXE
#define ML_M4GC_ADC1_AIN6_PIN A7
#define ML_M4GC_ADC1_AIN6_XPIN 4
#define ML_M4GC_ADC1_AIN6_PMUX 0x1
#define ML_M4GC_ADC1_AIN6_PMUX_msk (PORT_PMUX_PMUXE(ML_M4GC_ADC1_AIN6_PMUX))

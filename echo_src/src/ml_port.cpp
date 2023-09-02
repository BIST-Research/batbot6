/*
 * Author: Ben Westcott
 * Date created: 3/12/23
 */

#include <header/ml_port.hpp>

#define CONFIG_PULLEN(config)       (((config & 0x4) >> 2))
#define CONFIG_INEN(config)         (((config & 0x2) >> 1))
#define CONFIG_DIRSET(config)       (((config & 0x1) >> 0))
#define CONFIG_OUT(config)          (((config & 0x10) >> 4))

void peripheral_port_init(const uint8_t pmux_mask, 
                          const uint8_t m4_pin, 
                          const ml_port_config config, 
                          const ml_port_drive_strength drive){

    const EPortType port_group = g_APinDescription[m4_pin].ulPort;
    const uint32_t pin = g_APinDescription[m4_pin].ulPin;

    PORT->Group[port_group].PINCFG[pin].reg |= PORT_PINCFG_PMUXEN;

    PORT->Group[port_group].PINCFG[pin].reg |= (CONFIG_PULLEN(config) ? PORT_PINCFG_PULLEN : 0x0);

    PORT->Group[port_group].PINCFG[pin].reg |= (CONFIG_INEN(config) ? PORT_PINCFG_INEN : 0x0);

    PORT->Group[port_group].PINCFG[pin].reg |= (drive ? PORT_PINCFG_DRVSTR : 0x0);

    PORT->Group[port_group].PMUX[pin >> 1].reg |= pmux_mask;

    PORT->Group[port_group].DIR.reg |= (CONFIG_DIRSET(config) ? PORT_DIRSET_DIRSET(pin) : 0x0);

    PORT->Group[port_group].OUT.reg |= (CONFIG_OUT(config) ? PORT_OUT_OUT(pin) : 0x0);

}

void peripheral_port_init_alt
(
    const ml_port_function func,
    const ml_port_parity parity,
    const uint8_t m4_pin, 
    const ml_port_config config, 
    const ml_port_drive_strength drive
)
{

    const EPortType port_group = g_APinDescription[m4_pin].ulPort;
    const uint32_t pin = g_APinDescription[m4_pin].ulPin;

    uint8_t pmux_mask = parity ? PORT_PMUX_PMUXE((uint8_t)func) : PORT_PMUX_PMUXO((uint8_t)func);

    PORT->Group[port_group].PINCFG[pin].reg |= PORT_PINCFG_PMUXEN;

    PORT->Group[port_group].PINCFG[pin].reg |= (CONFIG_PULLEN(config) ? PORT_PINCFG_PULLEN : 0x0);

    PORT->Group[port_group].PINCFG[pin].reg |= (CONFIG_INEN(config) ? PORT_PINCFG_INEN : 0x0);

    PORT->Group[port_group].PINCFG[pin].reg |= (drive ? PORT_PINCFG_DRVSTR : 0x0);

    PORT->Group[port_group].PMUX[pin >> 1].reg |= pmux_mask;

    PORT->Group[port_group].DIR.reg |= (CONFIG_DIRSET(config) ? PORT_DIRSET_DIRSET(pin) : 0x0);

    PORT->Group[port_group].OUT.reg |= (CONFIG_OUT(config) ? PORT_OUT_OUT(pin) : 0x0);

}
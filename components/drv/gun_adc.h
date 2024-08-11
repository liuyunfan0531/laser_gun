#ifndef _GUN_ADC_H
#define _GUN_ADC_H

#include <stdio.h>

void gun_adc_init(void);
uint32_t gun_get_bat(uint8_t chrg_state);

#endif

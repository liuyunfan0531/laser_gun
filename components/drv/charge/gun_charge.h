#ifndef _CHARGE_H
#define _CHARGE_H

#include <stdio.h>

typedef enum{
	GUN_BATTERY_LEVEL_0,
	GUN_BATTERY_LEVEL_1,
	GUN_BATTERY_LEVEL_2,
    GUN_BATTERY_LEVEL_3,
	GUN_BATTERY_LEVEL_4
}gun_battery_level;

typedef struct{
	uint8_t gun_charge_stat;
	uint8_t gun_charing;	
    gun_battery_level gun_bat_level;
	uint32_t gun_chg_vol;
}gun_charge_t;

gun_charge_t gun_charge_val;
// gun_charge_t charge_info;

void gun_charge_init(void);

#endif

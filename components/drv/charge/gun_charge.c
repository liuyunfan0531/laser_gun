#include "gun_charge.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gun_adc.h"

#define GUN_BAT_CHARGE_PIN              GPIO_NUM_5
#define GUN_BAT_CHRG_PIN                GPIO_NUM_6
#define GUN_BAT_READ_CHARGE_PIN         gpio_get_level(GUN_BAT_CHARGE_PIN)
#define GUN_BAT_READ_CHRG_PIN           gpio_get_level(GUN_BAT_CHRG_PIN)
#define GUN_VOL_PERIOD				    120	//120*500ms

static void gun_charge_gpio_init(void)
{
    gpio_config_t chg_config = {
		.mode = GPIO_MODE_INPUT,
		.intr_type = GPIO_INTR_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
	};
	chg_config.pin_bit_mask = 1ULL << GUN_BAT_CHARGE_PIN;
    gpio_config(&chg_config);

	chg_config.pin_bit_mask = 1ULL << GUN_BAT_CHARGE_PIN;
	gpio_config(&chg_config);
}

static int gun_get_charge_gpio_val(void)
{
	return GUN_BAT_READ_CHARGE_PIN;
}

static int gun_get_chrg_gpio_val(void)
{
	return GUN_BAT_READ_CHRG_PIN;
}

uint8_t gun_read_charge_status(void)
{
	uint8_t reg_value, chg_stat;

	reg_value = gun_get_charge_gpio_val();
	gun_charge_val.gun_charing = gun_get_chrg_gpio_val();
	
	if(reg_value)
		chg_stat = 1;
	else
		chg_stat = 0;

	return chg_stat;
}

static uint8_t count = 0; 
static void gun_det_plug(void *arg)
{
	uint8_t chg_stat = 0;
    for (; ;) 
	{
        gun_charge_val.gun_charge_stat = gun_read_charge_status();
		if(count == 0)
			//获取电压值
			gun_charge_val.gun_chg_vol = gun_get_bat(gun_charge_val.gun_charge_stat);
		
		if(count >= GUN_VOL_PERIOD){
			count = 1;
			//获取电压值
			gun_charge_val.gun_chg_vol = gun_get_bat(gun_charge_val.gun_charge_stat);
		}
		//检测更新充电状态
		if(chg_stat != gun_charge_val.gun_charge_stat){
			if(count)
				//获取电压值
				gun_charge_val.gun_chg_vol = gun_get_bat(gun_charge_val.gun_charge_stat);

			//更新充电状态
			chg_stat = gun_charge_val.gun_charge_stat;
		}
		count++;
		vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void gun_charge_init(void)
{
	gun_charge_gpio_init();

	xTaskCreate(gun_det_plug, "gun_det_plug", 4096, NULL, 10, NULL);
}

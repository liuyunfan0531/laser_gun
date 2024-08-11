#include "gun_adc.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_adc_cal.h"
#include "gun_charge.h"

static const char *TAG = "gun_adc";

#define NO_OF_SAMPLES       64          //Multisampling
#define ADC_PIN             GPIO_NUM_4

static esp_adc_cal_characteristics_t *adcChar;

static const adc_unit_t unit = ADC_UNIT_1;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc1_channel_t channel_chg = ADC1_CHANNEL_3;

static const uint32_t dischg_bat[][2] = 
{
	{3950, 4},
	{3900, 3},
	{3800, 2},
	{3700, 1},
};

static const uint32_t chg_bat[][2] = 
{
	{4150, 4},
	{4100, 3},
	{3950, 2},
	{3830, 1}
};

static uint32_t gun_get_chg_vol(void)
{
    uint32_t adc_reading_chg = 0;

    for (int i = 0; i < NO_OF_SAMPLES; i++){
        adc_reading_chg += adc1_get_raw(channel_chg);
    }

    adc_reading_chg /= NO_OF_SAMPLES;

    uint32_t chg_value = esp_adc_cal_raw_to_voltage(adc_reading_chg, adcChar);

    return chg_value;
}

static uint32_t gun_get_dischg_bat(void)
{
    uint8_t i;
	uint8_t table_size = sizeof(dischg_bat)/sizeof(dischg_bat[0]);
    uint32_t vol;

    vol = gun_get_chg_vol();
    vol = vol * 2;
    ESP_LOGI(TAG, "vol = %d", vol);

    gun_charge_val.gun_chg_vol = vol;

    for(i = 0; i < table_size; i++)
	{
		if(vol >= dischg_bat[i][0]){
			break;
		}
	}

    if(i < table_size)
        gun_charge_val.gun_bat_level = dischg_bat[i][1];
    else
        gun_charge_val.gun_bat_level = dischg_bat[table_size-1][1];

    return vol;
}

static uint32_t gun_get_chg_bat(void)
{
    uint8_t i;
	uint8_t table_size = sizeof(chg_bat)/sizeof(chg_bat[0]);
    uint32_t vol;

    vol = gun_get_chg_vol();
    vol = vol * 2;
    ESP_LOGI(TAG, "vol = %d", vol);

    gun_charge_val.gun_chg_vol = vol;

    for(i = 0; i < table_size; i++)
	{
		if(vol >= chg_bat[i][0]){
			break;
		}
	}

    if(i < table_size)
        gun_charge_val.gun_bat_level = chg_bat[i][1];
    else
        gun_charge_val.gun_bat_level = chg_bat[table_size-1][1];

    return vol;
}

void gun_adc_init(void)
{
    adc1_config_width(width);
    adc1_config_channel_atten(channel_chg, atten);

    adc_vref_to_gpio(unit, ADC_PIN);

    adcChar = (esp_adc_cal_characteristics_t*)calloc(10, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, 
                                                            atten,
                                                            width,
                                                            1100,
                                                            adcChar);
    if (val_type == ESP_ADC_CAL_VAL_DEFAULT_VREF){
        ESP_LOGI(TAG, "使用默认参考电压");
    }else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF){
        ESP_LOGI(TAG, "使用 eFuse 中的电压");
    }
}

uint32_t gun_get_bat(uint8_t chrg_state)
{
    uint32_t bat = 0;

#if 0
    //usb插入
    if(chrg_state)
    {
        //是否处于充电状态
        if(!gun_charge_val.gun_charing)   
            bat = gun_get_chg_bat();
        else
            bat = gun_get_dischg_bat();
    }
    else//usb拔出
    {
        bat = gun_get_dischg_bat();
    }

    return bat;
#else
    bat = gun_get_dischg_bat();
#endif
    return bat;
}

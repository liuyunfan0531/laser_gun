#include "gun_presskey.h"
#include "driver/gpio.h"
#include "gun_multi_button.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gun_infrared.h"

static const char *TAG = "gun_presskey";

static Button botton_shoot; 
static Button botton_map;
static Button botton_panel;

#define PIN_LOW                 0x00
#define PIN_HIGH                0x01 

#define GUN_SHOOT_PIN                 GPIO_NUM_46
#define GUN_READ_SHOOT_PIN            gpio_get_level(GUN_SHOOT_PIN)
#define GUN_MAP_PIN                   GPIO_NUM_9
#define GUN_READ_MAP_PIN              gpio_get_level(GUN_MAP_PIN)
#define GUN_PANEL_PIN                 GPIO_NUM_10
#define GUN_READ_PANEL_PIN            gpio_get_level(GUN_PANEL_PIN)

static uint8_t read_button_shoot(void)
{
	return GUN_READ_SHOOT_PIN;
}

static uint8_t read_button_map(void)
{
	return GUN_READ_MAP_PIN;
}

static uint8_t read_button_panel(void)
{
	return GUN_PANEL_PIN;
}

static void button_cbk(void *button)
{
    uint8_t event_val = 0;
	
	event_val = get_button_event((struct Button *)button);

    if((struct Button *)button == &botton_shoot) {
        if(event_val == SINGLE_CLICK) {
            ESP_LOGI(TAG, "-------SHOOT SINGLE_CLICK-------");
            //处理相关逻辑
            gun_ir_tx_task();
        }
    } else if((struct Button *)button == &botton_map) {
        if(event_val == SINGLE_CLICK) {
            ESP_LOGI(TAG, "-------MAP SINGLE_CLICK-------");
            //处理相关逻辑

        }
    } else if((struct Button *)button == &botton_panel) {
        if(event_val == SINGLE_CLICK) {
            ESP_LOGI(TAG, "-------PANEL SINGLE_CLICK-------");
            //处理相关逻辑

        }
    } else {

    }
}

static void presskey_thread_entry(void *arg)
{
    for(; ;)
    {
        button_ticks();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void gun_presskey_config(void)
{
    xTaskCreate(presskey_thread_entry, "presskey_thread_entry", 2048, NULL, 9, NULL);

    button_init(&botton_shoot, read_button_shoot, PIN_LOW); 
	button_attach(&botton_shoot, SINGLE_CLICK, button_cbk);	    //注册单击事件
    button_start(&botton_shoot);

    button_init(&botton_map, read_button_map, PIN_LOW); 
	button_attach(&botton_map, SINGLE_CLICK, button_cbk);	    //注册单击事件
    button_start(&botton_map);

    button_init(&botton_panel, read_button_panel, PIN_LOW);
    button_attach(&botton_panel, SINGLE_CLICK, button_cbk);	    //注册单击事件
    button_start(&botton_panel);
}

void gun_presskey_init(void)
{
    gpio_config_t chg_config = {
		.mode = GPIO_MODE_INPUT,
		.intr_type = GPIO_INTR_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
	};
	chg_config.pin_bit_mask = 1ULL << GUN_SHOOT_PIN;
    gpio_config(&chg_config);
    chg_config.pin_bit_mask = 1ULL << GUN_MAP_PIN;
    gpio_config(&chg_config);
    chg_config.pin_bit_mask = 1ULL << GUN_PANEL_PIN;
    gpio_config(&chg_config);
}


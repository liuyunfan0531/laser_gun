#ifndef _GUN_WS2812_H_
#define _GUN_WS2812_H_

#include <stdio.h>
#include "gun_spi.h"

typedef enum{
    WS2812_WRITE,
    WS2812_GREEN,
    WS2812_RED,
    WS2812_BLUE,
    WS2812_YELLOW,
    WS2812_PURPLE,
    WS2812_CYAN
}ws2812_select_color;

typedef enum{
    WS2812_EFFECT_ON = 1,
    WS2812_EFFECT_BREATH,
    WS2812_EFFECT_BLINK
}ws2812_effect_t;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
}ws2812_color_t;

typedef struct{
    spi_setting_t spi_setting;
    ws2812_color_t grb;
    uint8_t led_nums;
}ws2812_config_t;

void gun_ws2812_init(void);
void gun_ws812_flush_data(ws2812_select_color color_index);
void gun_ws2812_set_breath(ws2812_select_color color_index);
void gun_ws2812_set_pixel(ws2812_color_t color, uint8_t* buffer, uint16_t index);
void ws2812_control_task(void* arg);

#endif

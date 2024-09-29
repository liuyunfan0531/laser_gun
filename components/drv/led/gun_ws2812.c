#include "gun_ws2812.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>
#include "driver/gpio.h"
#include "freertos/task.h"
#include "gun_charge.h"

static const char *TAG = "gun_ws2812";

#define WS2812_TH           0xE0//0xE0 F8
#define WS2812_TL           0x80//0x80 E0

#define WS2812_LED_NUMS     8
#define WS2812_DATA_BITS    8   //一个颜色8位
#define WS2812_DATA_BYTES   3   //一颗灯三个字节
#define WS2812_RESET_BYTE   30

#define SPI_MASTER_FREQ     4500000     //4.5MHz
#define GUN_ESP_LED         GPIO_NUM_47

uint8_t *g_ws2812_bit_buffer = NULL;
  
static ws2812_config_t ws2812_config = {
    .spi_setting = {
        .clock_speed_hz = SPI_MASTER_FREQ,
        .dma_channel = SPI_DMA_CH_AUTO, 
        .host = SPI2_HOST,
        .max_transfer_sz = WS2812_LED_NUMS*WS2812_DATA_BYTES*WS2812_DATA_BITS,
        .mosi = GUN_ESP_LED,
    },
    .led_nums = WS2812_LED_NUMS
};

static uint8_t grb_data[7][3] = {
    {0xff, 0xff, 0xff},     //0
    {0xff, 0x00, 0x00},     //1
    {0x00, 0xff, 0x00},     //2
    {0x00, 0x00, 0xff},     //3
    {0xff, 0xff, 0x00},     //4
    {0x00, 0xff, 0xff},     //5
    {0xff, 0x00, 0xff},     //6
};

void gun_ws2812_set_pixel(ws2812_color_t color, uint8_t* buffer, uint16_t index)
{
    for(uint8_t i = 0; i < WS2812_DATA_BITS; i++){
        buffer[index++] = (color.green & (1 << (7 - i))) ? WS2812_TH : WS2812_TL;
    }

    for(uint8_t i = 0; i < WS2812_DATA_BITS; i++){
        buffer[index++] = (color.red & (1 << (7 - i))) ? WS2812_TH : WS2812_TL;
    }

    for(uint8_t i = 0; i < WS2812_DATA_BITS; i++){
        buffer[index++] = (color.blue & (1 << (7 - i))) ? WS2812_TH : WS2812_TL;
    }
}

void gun_ws812_flush_data(ws2812_select_color color_index)
{
    uint16_t buff_size = ws2812_config.led_nums*WS2812_DATA_BYTES*WS2812_DATA_BITS + WS2812_RESET_BYTE; 
    uint16_t index_g = WS2812_RESET_BYTE;

    ws2812_config.grb.green = grb_data[color_index][0];
    ws2812_config.grb.red = grb_data[color_index][1];
    ws2812_config.grb.blue = grb_data[color_index][2];

    memset(g_ws2812_bit_buffer, 0x00, buff_size);

    for(uint8_t i = 0; i < ws2812_config.led_nums; i++){
        gun_ws2812_set_pixel(ws2812_config.grb, g_ws2812_bit_buffer, index_g);
        index_g += WS2812_DATA_BYTES*WS2812_DATA_BITS;
    }

    gun_spi_data_queue(g_ws2812_bit_buffer, buff_size);
}

void gun_ws2812_set_breath(ws2812_select_color color_index)
{
    uint16_t buff_size = ws2812_config.led_nums*WS2812_DATA_BYTES*WS2812_DATA_BITS + WS2812_RESET_BYTE; 
    uint16_t index_g = WS2812_RESET_BYTE;
    uint8_t green = 0, red = 0, blue = 0;
    static bool breath_flag = false;
    static uint8_t brightness = 255;

    green = grb_data[color_index][0];
    red = grb_data[color_index][1];
    blue = grb_data[color_index][2];

    memset(g_ws2812_bit_buffer, 0x00, buff_size);
    
    if(!breath_flag){
        brightness -= 10;
        if(brightness <= 5)
            breath_flag = true;
    } else {
        brightness += 10;
        if(brightness >= 255){
            breath_flag = false;
            brightness = 255;
        }
    }

    for(uint8_t i = 0; i < ws2812_config.led_nums; i++){
        ws2812_config.grb.green = (brightness * green) / 255;
        ws2812_config.grb.red = (brightness * red) / 255;
        ws2812_config.grb.blue = (brightness * blue) / 255;
        // ESP_LOGI(TAG, "ws2812_config.grb.green = %d, ws2812_config.grb.red = %d, ws2812_config.grb.blue = %d", ws2812_config.grb.green, ws2812_config.grb.red, ws2812_config.grb.blue);
        gun_ws2812_set_pixel(ws2812_config.grb, g_ws2812_bit_buffer, index_g);
        index_g += WS2812_DATA_BYTES*WS2812_DATA_BITS;
    }

    gun_spi_data_queue(g_ws2812_bit_buffer, buff_size);
    index_g = WS2812_DATA_BYTES*WS2812_DATA_BITS;
    vTaskDelay(300 / portTICK_PERIOD_MS);
}

void ws2812_control_task(void* arg)
{
    gun_charge_t charge_info;
    ws2812_effect_t ws2812_effect = WS2812_EFFECT_ON;
    ws2812_select_color color_index = WS2812_WRITE;

    for(; ;)
    {
        gun_charge_info(&charge_info);
        if(charge_info.gun_charing == 1) {
            ws2812_effect = WS2812_EFFECT_BREATH;
            color_index = WS2812_WRITE;
        }

        switch(ws2812_effect)
        {
            case WS2812_EFFECT_ON: 
                gun_ws812_flush_data(color_index);
                break;
            case WS2812_EFFECT_BREATH:
                gun_ws2812_set_breath(color_index);
                break;
            case WS2812_EFFECT_BLINK:

                break;
        }

       vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

void gun_ws2812_init(void)
{
    gun_spi_init(&ws2812_config.spi_setting);
    g_ws2812_bit_buffer = heap_caps_malloc(ws2812_config.led_nums*WS2812_DATA_BYTES*WS2812_DATA_BITS + WS2812_RESET_BYTE, MALLOC_CAP_DMA);

    xTaskCreate(ws2812_control_task, "ws2812_control_task", 2560, NULL, 3, NULL);
}

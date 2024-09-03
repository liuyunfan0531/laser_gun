#include "gun_ws2812.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>
#include "driver/gpio.h"
#include "freertos/task.h"

static const char *TAG = "gun_ws2812";

#define WS2812_TH           0xE0
#define WS2812_TL           0x80

#define WS2812_LED_NUMS     3
#define WS2812_DATA_BITS    8   //一个颜色8位
#define WS2812_DATA_BYTES   3   //一颗灯三个字节

#define SPI_MASTER_FREQ     4500000     //4.5MHz
#define GUN_ESP_LED         GPIO_NUM_47

uint8_t *g_ws2812_bit_buffer = NULL;
  
static ws2812_config_t ws2812_config = {
    .spi_setting = {
        .clock_speed_hz = SPI_MASTER_FREQ,
        .dma_channel = SPI_DMA_CH_AUTO,
        .host = SPI3_HOST,
        .max_transfer_sz = WS2812_LED_NUMS*WS2812_DATA_BYTES*WS2812_DATA_BITS,
        .mosi = GUN_ESP_LED,
    },
    .led_nums = WS2812_LED_NUMS
};

static uint8_t grb_data[8][3] = {
    {0xff, 0xff, 0xff},
    {0xff, 0x00, 0x00},
    {0x00, 0xff, 0x00},
    {0x00, 0x00, 0xff},
    {0xff, 0xff, 0x00},
    {0x00, 0xff, 0xff},
    {0xff, 0x00, 0xff},
    {0x00, 0x00, 0x00}
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

// char t_data = 0x00;

void gun_ws812_flush_data(ws2812_select_color color_index)
{
    uint16_t buff_size = ws2812_config.led_nums*WS2812_DATA_BYTES*WS2812_DATA_BITS; 
    uint16_t index = 0;

    ws2812_config.grb.green = grb_data[color_index][0];
    ws2812_config.grb.red = grb_data[color_index][1];
    ws2812_config.grb.blue = grb_data[color_index][2];

    // gun_spi_data_queue(&t_data, sizeof(t_data)); //帮我测下这里能不能拉低(发送0x00) 把后面的都注释掉

    for(uint8_t i = 0; i < ws2812_config.led_nums; i++)
    {
        gun_ws2812_set_pixel(ws2812_config.grb, g_ws2812_bit_buffer, index);
        index += WS2812_DATA_BYTES * WS2812_DATA_BITS;
    }

    gun_spi_data_queue(g_ws2812_bit_buffer, buff_size);
}

void gun_ws2812_set_breath()
{
    // for(){

    // }

    // for(){

    // }
}

void ws2812_control_task(void* arg)
{
    ws2812_effect_t ws2812_effect = WS2812_EFFECT_ON;
    ws2812_select_color color_index = WS2812_READ;
    for(; ;)
    {
        switch(ws2812_effect)
        {
            case WS2812_EFFECT_ON: 
                gun_ws812_flush_data(color_index);
                break;
            case WS2812_EFFECT_BREATH:

                break;
        }

       vTaskDelay(30 / portTICK_PERIOD_MS);
    }
}

void gun_ws2812_init(void)
{
    gun_spi_init(&ws2812_config.spi_setting);
    g_ws2812_bit_buffer = heap_caps_malloc(ws2812_config.led_nums*WS2812_DATA_BYTES*WS2812_DATA_BITS, MALLOC_CAP_DMA);
    memset(g_ws2812_bit_buffer, 0x00, ws2812_config.led_nums*WS2812_DATA_BYTES*WS2812_DATA_BITS);
    
    // xTaskCreate(ws2812_control_task, "ws2812_control_task", 2048, NULL, 3, NULL);
}

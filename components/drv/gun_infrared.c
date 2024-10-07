#include "gun_infrared.h"
#include "driver/gpio.h"
#include "driver/rmt.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "gun_infrared";

#define RMT_TX_GPIO_NUM         48
#define RMT_TX_CHANNEL          RMT_CHANNEL_0
#define RMT_RX_GPIO_NUM         45
#define RMT_RX_CHANNEL          RMT_CHANNEL_4

#define RMT_CLK_DIV             100
#define RMT_TICK_10_US          (80000000/RMT_CLK_DIV/100000)
#define RMT_ITEM32_TIMEOUT_US   10000

#define HEADER_HIGH_9000US      9000                         /*!< NEC protocol header: positive 9ms */
#define HEADER_LOW_4500US       4500                         /*!< NEC protocol header: negative 4.5ms*/ 
#define NEC_BIT_ONE_HIGH_US     560
#define NEC_BIT_ONE_LOW_US      1690
#define NEC_BIT_ZERO_HIGH_US    560
#define NEC_BIT_ZERO_LOW_US     560

#define NEC_DATA_ITEM_NUM       33

static SemaphoreHandle_t sp_rmt_mutex;

typedef struct{
    rmt_channel_t channel;      
    gpio_num_t gpio_num;        
    rmt_mode_t mode;
}ir_config_t;

typedef struct{
    uint8_t user_id;
    uint8_t war_situation;
}ir_data_t;

ir_data_t s_ir_tx_data, s_ir_rx_data;

/* (high_us / 10) 相当于把时间从微妙转化成以10微妙为单位时间 
   RMT_TICK_10_US相当于是周期 10微妙对应8个周期 */
static void nec_fill_item(rmt_item32_t *item, uint16_t high_us, uint16_t low_us)
{
    item->level0 = 1;
    item->duration0 = (high_us / 10) * RMT_TICK_10_US;
    item->level1 = 0;
    item->duration1 = (low_us / 10) * RMT_TICK_10_US;
}

static void nec_fill_item_head(rmt_item32_t *item)
{
    nec_fill_item(item, HEADER_HIGH_9000US, HEADER_LOW_4500US);
}

static void nec_fill_item_bit_one(rmt_item32_t *item)
{
    nec_fill_item(item, NEC_BIT_ONE_HIGH_US, NEC_BIT_ONE_LOW_US);
}

static void nec_fill_item_bit_zero(rmt_item32_t *item)
{
    nec_fill_item(item, NEC_BIT_ZERO_HIGH_US, NEC_BIT_ZERO_LOW_US);
}

/*
    构建一个NEC协议数据包
    引导码 + 数据码 + 数据反码
*/
static void nec_buid_item(rmt_item32_t *item, uint8_t ir_tx_user_id, uint8_t ir_tx_war_situation)
{
    //引导码
    nec_fill_item_head(item);
    item++;                         //指向下一个item

    //数据码 身份信息
    for(uint8_t i = 0; i < 8; i++) {
        if(ir_tx_user_id & (1 << (7 - i))) {
            nec_fill_item_bit_one(item);
        } else {
            nec_fill_item_bit_zero(item);
        }
        item++;
    }
    // 战局信息
    for(uint8_t i = 0; i < 8; i++) {
        if(ir_tx_war_situation & (1 << (7 - i))) {
            nec_fill_item_bit_one(item);
        } else {
            nec_fill_item_bit_zero(item);
        }
        item++;
    }

    //数据码反码 身份信息
   for(uint8_t i = 0; i < 8; i++) {
        if(!(ir_tx_user_id & (1 << (7 - i)))) {
            nec_fill_item_bit_one(item); 
        } else {
            nec_fill_item_bit_zero(item);
        }
        item++; // 更新指针
    }
    // 战局信息
    for(uint8_t i = 0; i < 8; i++) {
        if(!(ir_tx_war_situation & (1 << (7 - i)))) {
            nec_fill_item_bit_one(item); 
        } else {
            nec_fill_item_bit_zero(item);
        }
        item++; // 更新指针
    }
}

static void gun_ir_tx_config(ir_config_t *sp_ir_config)
{
    rmt_config_t rmt_tx = {
        .rmt_mode = sp_ir_config->mode,
        .channel = sp_ir_config->channel,
        .gpio_num = sp_ir_config->gpio_num,
        .mem_block_num = 1,         //可以存64个item
        .clk_div = RMT_CLK_DIV,
        .tx_config = {
            .loop_en = false,       //关闭循环发射，只发射一次,
            .carrier_freq_hz = 38000,    //38KHz
            .carrier_level = RMT_CARRIER_LEVEL_HIGH,     
            .carrier_en = true,
            .idle_level = RMT_IDLE_LEVEL_LOW,   //空闲状态低电平
            .carrier_duty_percent = 50,         //载波占空比为50
            .idle_output_en = true              //输出使能
        }
    };
    rmt_config(&rmt_tx);
    //使能红外发射驱动
    rmt_driver_install(rmt_tx.channel, 0, 0);
    ESP_LOGI(TAG, "install rmt driver");
}

void gun_ir_tx_task(void)
{
    rmt_item32_t *item;
    uint8_t item_num = NEC_DATA_ITEM_NUM;

    //获取互斥锁
    if(xSemaphoreTake(sp_rmt_mutex, 0) == pdTRUE) {
        item = (rmt_item32_t *)malloc(sizeof(rmt_item32_t) * item_num);
        memset(item, 0, sizeof(rmt_item32_t) * item_num);
        
        nec_buid_item(item, s_ir_tx_data.user_id, s_ir_tx_data.war_situation);
        rmt_write_items(RMT_TX_CHANNEL, item, item_num, true);      //将item写入通道对应的RAM并进入阻塞
        rmt_wait_tx_done(RMT_TX_CHANNEL, portMAX_DELAY);            //等待发送完成

        ESP_LOGI(TAG, "--rmt send item finish--");
        free(item);
        item = NULL;

        //释放互斥锁
        xSemaphoreGive(sp_rmt_mutex);
    } 
}

void gun_ir_tx_init(void)
{
    ir_config_t tx_config = {
        .channel = RMT_TX_CHANNEL,
        .gpio_num = RMT_TX_GPIO_NUM,
        .mode = RMT_MODE_TX,
    };

    gun_ir_tx_config(&tx_config);
    ESP_LOGI(TAG, "----init rmt tx----");

    sp_rmt_mutex = xSemaphoreCreateBinary();
    if (sp_rmt_mutex != NULL) {
		xSemaphoreGive(sp_rmt_mutex); 
	}

    //测试 填充数据
    //实际数据由蓝牙下发
    s_ir_tx_data.user_id = 0x01;
    s_ir_tx_data.war_situation = 0x02;
}
/*--------------------------------------------------------------------------------------------------------------------------------*/

static bool check_if_one_item(rmt_item32_t *item)
{
    if(item->level0 == 0 
       && item->duration0 >= (((NEC_BIT_ONE_HIGH_US / 10) * RMT_TICK_10_US) - 100)
       && item->level1 == 1 
       && item->duration1 >= (((NEC_BIT_ONE_LOW_US / 10) * RMT_TICK_10_US) - 100)) {
        return true;
    }
    return false;
}

static bool check_if_zero_item(rmt_item32_t *item)
{
    if(item->level0 == 0 
       && item->duration0 >= (((NEC_BIT_ZERO_HIGH_US / 10) * RMT_TICK_10_US) - 100)
       && item->level1 == 1 
       && item->duration1 >= (((NEC_BIT_ZERO_LOW_US / 10) * RMT_TICK_10_US)- 100)) {
        return true;
    }
    return false;
}
static void parse_items(rmt_item32_t *item)
{
    item++;     //跳过引导码

    for(uint8_t i = 0; i < 8; i++)
    {
        if(check_if_one_item(item)) {
            s_ir_rx_data.user_id |= (1 << (7 - i));
        } else if(check_if_zero_item(item)) {
            s_ir_rx_data.user_id |= (0 << (7 - i));
        }
        item++;
    }

    for(uint8_t i = 0; i < 8; i++)
    {
        if(check_if_one_item(item)) {
            s_ir_rx_data.war_situation |= (1 << (7 - i));
        } else if(check_if_zero_item(item)) {
            s_ir_rx_data.war_situation |= (0 << (7 - i));
        }
        item++;
    }

    ESP_LOGI(TAG, "-----user_id = %d, war_situation = %d-----", s_ir_rx_data.user_id, s_ir_rx_data.war_situation);
}

void gun_ir_rx_task(void *arg)
{
    size_t rx_size = 0;
    RingbufHandle_t rb = NULL;

    rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rb);     //获取红外接收器接收的数据 放在ringbuff中
    rmt_rx_start(RMT_RX_CHANNEL, true);
    for(; ;)
    {
        rmt_item32_t *item = (rmt_item32_t *)xRingbufferReceive(rb, &rx_size, portMAX_DELAY);

        if(item != NULL) {
            if(rx_size == (NEC_DATA_ITEM_NUM * sizeof(rmt_item32_t))) {
                ESP_LOGI(TAG, "-----rx_size = %d-----", rx_size);
                rmt_rx_stop(RMT_RX_CHANNEL);
                parse_items(item);
                rmt_rx_start(RMT_RX_CHANNEL, true);
            }
            vRingbufferReturnItem(rb, (void*) item);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

static void gun_ir_rx_config(ir_config_t *sp_ir_config)
{
    rmt_config_t rmt_rx = {
        .rmt_mode = sp_ir_config->mode,
        .channel = sp_ir_config->channel,
        .gpio_num = sp_ir_config->gpio_num,
        .mem_block_num = 1,             //可以存64个item
        .clk_div = RMT_CLK_DIV,
        .rx_config = {
           .filter_en = true,           //开启滤波
           .filter_ticks_thresh = 100,  //滤波阈值 250us以下的信号不接受 
           .idle_threshold = (RMT_ITEM32_TIMEOUT_US / 10) * RMT_TICK_10_US
        }
    };
    rmt_config(&rmt_rx);
    rmt_driver_install(rmt_rx.channel, 1000, 0);
}

void gun_ir_rx_init(void)
{
    ir_config_t rx_config = {
        .channel = RMT_RX_CHANNEL,
        .gpio_num = RMT_RX_GPIO_NUM,
        .mode = RMT_MODE_RX,
    };

    gun_ir_rx_config(&rx_config);
    ESP_LOGI(TAG, "----init rmt rx----");

    xTaskCreate(gun_ir_rx_task, "gun_ir_rx_task", 2048, NULL, 9, NULL);
}

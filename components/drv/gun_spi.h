#ifndef _GUN_SPI_H
#define _GUN_SPI_H

#include <stdio.h>
#include "driver/spi_master.h"

typedef struct{
    uint8_t dma_channel;
    spi_host_device_t host;
    int mosi;
    uint16_t max_transfer_sz;
    uint32_t clock_speed_hz;
}spi_setting_t;

void gun_spi_init(spi_setting_t *spi_setting);
void gun_spi_data_queue(void *data, int lenth);

#endif

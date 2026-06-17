#ifndef __DHT11_H
#define __DHT11_H

#include "ch32v30x.h"

#define DHT11_PORT  GPIOA
#define DHT11_PIN   GPIO_Pin_11

typedef struct {
    uint8_t  humi_int;    /* humidity integer % */
    uint8_t  humi_dec;    /* humidity decimal (always 0 for DHT11) */
    uint8_t  temp_int;    /* temperature integer °C */
    uint8_t  temp_dec;    /* temperature decimal (always 0 for DHT11) */
    uint8_t  checksum;    /* sum of bytes 0-3 */
    float    temperature; /* parsed float */
    float    humidity;    /* parsed float */
    uint8_t  valid;       /* 1=valid data, 0=checksum failed */
} DHT11_Data;

extern DHT11_Data dht11;

void DHT11_Init(void);
uint8_t DHT11_Read(DHT11_Data *data);

#endif

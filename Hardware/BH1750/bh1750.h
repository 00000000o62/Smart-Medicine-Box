#ifndef __BH1750_H
#define __BH1750_H

#include "ch32v30x.h"

#define BH1750_ADDR  0x23    /* 7-bit I2C address (ADDR pin = LOW) */

/* I2C pins (software bit-bang) */
#define BH_SCL_PORT  GPIOA
#define BH_SCL_PIN   GPIO_Pin_12
#define BH_SDA_PORT  GPIOA
#define BH_SDA_PIN   GPIO_Pin_15

void BH1750_Init(void);
uint8_t BH1750_Read(float *lux);

#endif

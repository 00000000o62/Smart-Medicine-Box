#ifndef __SPI_H
#define __SPI_H

#include "ch32v30x.h"

void SPI2_Init(void);
uint8_t SPI_WriteByte(SPI_TypeDef *SPIx, uint8_t Byte);
void SPI_SetSpeed(SPI_TypeDef *SPIx, uint8_t SpeedSet);

#endif

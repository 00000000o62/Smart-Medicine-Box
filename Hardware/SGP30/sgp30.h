#ifndef __SGP30_H
#define __SGP30_H

#include "ch32v30x.h"

typedef struct {
    uint16_t tvoc_ppb;   /* TVOC in ppb */
    uint16_t co2_ppm;    /* eCO2 in ppm */
    uint16_t ethanol;    /* raw ethanol signal */
    uint16_t h2;         /* raw H2 signal */
    uint8_t  ready;      /* 1=chip initialized and measuring */
    uint32_t baseline;   /* IAQ baseline for persistence */
} SGP30_Data;

extern SGP30_Data sgp30;

void SGP30_Init(void);
uint8_t SGP30_Measure(SGP30_Data *data);
uint8_t SGP30_Probe(void);
void SGP30_SetHumidity(float temp_c, float rh_pct);

#endif

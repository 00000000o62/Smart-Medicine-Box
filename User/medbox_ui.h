#ifndef __MEDBOX_UI_H
#define __MEDBOX_UI_H

#include "ch32v30x.h"

extern float    display_temp;
extern float    display_humi;
extern uint16_t display_tvoc;
extern uint16_t display_co2;
extern uint8_t  wifi_connected;
extern char     wifi_ip[16];
extern uint8_t  sensor_ok;   /* 1=at least one valid DHT11 reading */
extern float    display_lux;  /* BH1750 light sensor (lux) */

void UI_Init(void);
void UI_DrawAll(void);
void UI_UpdateClock(void);
void UI_UpdateSensors(void);

#endif

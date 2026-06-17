#ifndef __MEDBOX_SCHEDULE_H
#define __MEDBOX_SCHEDULE_H

#include "ch32v30x.h"

#define MAX_SLOTS       7
#define MAX_COMPARTMENTS 7

typedef enum {
    MED_PENDING = 0,
    MED_TAKEN,
    MED_MISSED
} MedStatus;

typedef struct {
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  compartment;
    uint8_t  status;
    char     name[20];
} MedSlot;

typedef struct {
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
    uint32_t ticks;
} MedTime;

extern MedSlot schedule[MAX_SLOTS];
extern uint8_t schedule_count;
extern MedTime current_time;
extern uint8_t alarm_active;
extern uint8_t alarm_slot_index;

/* Micro switch counter */
extern uint32_t pill_count;

void MedTime_Init(void);
void MedTime_Update(void);
void MedSchedule_Init(void);
uint8_t MedSchedule_CheckAlarm(void);
void MedSchedule_ConfirmDose(uint8_t index);
void Compartment_Open(uint8_t comp);
void Button_Init(void);
void Button_Poll(void);

#endif

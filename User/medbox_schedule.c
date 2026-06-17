#include "medbox_schedule.h"
#include "ch32v30x_gpio.h"
#include "ch32v30x_rcc.h"
#include <string.h>
#include <stdio.h>

/* Global schedule — hardcoded demo data */
MedSlot schedule[MAX_SLOTS];
uint8_t schedule_count = 0;
MedTime current_time;
uint8_t alarm_active = 0;
uint8_t alarm_slot_index = 0;

/* Micro switch counter */
uint32_t pill_count = 0;

/* Compartment GPIO pins: PA1-PA7 */
static const uint16_t comp_pins[7] = {
    GPIO_Pin_1, GPIO_Pin_2, GPIO_Pin_3, GPIO_Pin_4,
    GPIO_Pin_5, GPIO_Pin_6, GPIO_Pin_7
};

void MedTime_Init(void)
{
    /* Default start time: 2026-05-18 08:00:00 */
    current_time.year   = 2026;
    current_time.month  = 5;
    current_time.day    = 18;
    current_time.hour   = 8;
    current_time.minute = 0;
    current_time.second = 0;
    current_time.ticks  = 0;
}

void MedTime_Update(void)
{
    /* Called every 100ms from main loop */
    current_time.ticks++;
    if(current_time.ticks >= 10)
    {
        current_time.ticks = 0;
        current_time.second++;
        if(current_time.second >= 60)
        {
            current_time.second = 0;
            current_time.minute++;
            if(current_time.minute >= 60)
            {
                current_time.minute = 0;
                current_time.hour++;
                if(current_time.hour >= 24)
                {
                    current_time.hour = 0;
                    current_time.day++;
                    if(current_time.day > 30) /* simplified */
                    {
                        current_time.day = 1;
                        current_time.month++;
                        if(current_time.month > 12)
                        {
                            current_time.month = 1;
                            current_time.year++;
                        }
                    }
                }
            }
        }
    }
}

void MedSchedule_Init(void)
{
    /* Initialize compartment GPIOs */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4
                                | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* All compartments closed (active low logic — set high = locked) */
    GPIO_SetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4
                     | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);

    /* Demo medication schedule */
    schedule_count = 4;

    schedule[0].hour = 8;
    schedule[0].minute = 0;
    schedule[0].compartment = 1;
    schedule[0].status = MED_PENDING;
    strcpy(schedule[0].name, "Vitamin C");

    schedule[1].hour = 12;
    schedule[1].minute = 0;
    schedule[1].compartment = 2;
    schedule[1].status = MED_PENDING;
    strcpy(schedule[1].name, "Aspirin");

    schedule[2].hour = 14;
    schedule[2].minute = 0;
    schedule[2].compartment = 3;
    schedule[2].status = MED_PENDING;
    strcpy(schedule[2].name, "Ibuprofen");

    schedule[3].hour = 20;
    schedule[3].minute = 0;
    schedule[3].compartment = 1;
    schedule[3].status = MED_PENDING;
    strcpy(schedule[3].name, "Calcium");
}

uint8_t MedSchedule_CheckAlarm(void)
{
    uint8_t i;
    for(i = 0; i < schedule_count; i++)
    {
        if(schedule[i].status == MED_PENDING
           && current_time.hour == schedule[i].hour
           && current_time.minute == schedule[i].minute
           && current_time.second == 0)
        {
            alarm_active = 1;
            alarm_slot_index = i;
            return 1;
        }
    }
    return 0;
}

void MedSchedule_ConfirmDose(uint8_t index)
{
    if(index < schedule_count)
    {
        schedule[index].status = MED_TAKEN;
        Compartment_Open(schedule[index].compartment);
        alarm_active = 0;
    }
}

void Compartment_Open(uint8_t comp)
{
    if(comp >= 1 && comp <= 7)
    {
        GPIO_ResetBits(GPIOA, comp_pins[comp - 1]);
    }
}

/* Micro switch: PA0, active-low, GPIO polling with 3-state debounce */
void Button_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA0 = input with internal pull-up. Switch connects PA0→GND when pressed */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*
 * 3-state debounce state machine (called every 100ms from main loop):
 *   IDLE(0) → PRESSED(1) → CONFIRMED(2) → back to IDLE after release
 */
#define BTN_IDLE       0
#define BTN_PRESSED    1
#define BTN_CONFIRMED  2

void Button_Poll(void)
{
    static uint8_t  btn_state = BTN_IDLE;
    static uint16_t btn_ticks = 0;
    uint8_t pin_now = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);

    switch(btn_state)
    {
    case BTN_IDLE:
        if(pin_now == 0)                /* press detected */
        {
            btn_state = BTN_PRESSED;
            btn_ticks = 0;
        }
        break;

    case BTN_PRESSED:
        btn_ticks++;
        if(btn_ticks >= 3)              /* 300ms debounce */
        {
            if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
            {
                pill_count++;           /* confirmed press */
                btn_state = BTN_CONFIRMED;
                printf("Button pressed! Count=%lu\r\n", (unsigned long)pill_count);
            }
            else
            {
                btn_state = BTN_IDLE;   /* glitch — ignore */
            }
        }
        break;

    case BTN_CONFIRMED:
        if(pin_now == 1)                /* wait for release */
            btn_state = BTN_IDLE;
        break;
    }
}

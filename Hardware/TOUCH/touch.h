#ifndef __TOUCH_H
#define __TOUCH_H

#include "ch32v30x.h"

#define TP_PRES_DOWN    0x80
#define TP_CATH_PRES    0x40

/* Touch device struct */
typedef struct
{
    uint8_t (*init)(void);
    uint8_t (*scan)(uint8_t);
    void    (*adjust)(void);
    uint16_t x0;
    uint16_t y0;
    uint16_t x;
    uint16_t y;
    uint8_t  sta;
    float    xfac;
    float    yfac;
    int16_t  xoff;
    int16_t  yoff;
    uint8_t  touchtype;
}_m_tp_dev;

extern _m_tp_dev tp_dev;

/* Touch pin macros — CH32V307 GPIOC */
#define PEN_PORT    GPIOC
#define PEN_PIN     GPIO_Pin_10
#define DOUT_PORT   GPIOC
#define DOUT_PIN    GPIO_Pin_2
#define TDIN_PORT   GPIOC
#define TDIN_PIN    GPIO_Pin_3
#define TCLK_PORT   GPIOC
#define TCLK_PIN    GPIO_Pin_0
#define TCS_PORT    GPIOC
#define TCS_PIN     GPIO_Pin_13

#define PEN_READ()      GPIO_ReadInputDataBit(PEN_PORT, PEN_PIN)
#define DOUT_READ()     GPIO_ReadInputDataBit(DOUT_PORT, DOUT_PIN)
#define TDIN_SET()      GPIO_SetBits(TDIN_PORT, TDIN_PIN)
#define TDIN_CLR()      GPIO_ResetBits(TDIN_PORT, TDIN_PIN)
#define TCLK_SET()      GPIO_SetBits(TCLK_PORT, TCLK_PIN)
#define TCLK_CLR()      GPIO_ResetBits(TCLK_PORT, TCLK_PIN)
#define TCS_SET()       GPIO_SetBits(TCS_PORT, TCS_PIN)
#define TCS_CLR()       GPIO_ResetBits(TCS_PORT, TCS_PIN)

/* API prototypes */
uint8_t TP_Init(void);
uint8_t TP_Scan(uint8_t tp);
void    TP_Adjust(void);
uint16_t TP_Read_AD(uint8_t CMD);
uint8_t TP_Read_XY2(uint16_t *x, uint16_t *y);
void    TP_Drow_Touch_Point(uint16_t x, uint16_t y, uint16_t color);
void    TP_Draw_Big_Point(uint16_t x, uint16_t y, uint16_t color);

#endif

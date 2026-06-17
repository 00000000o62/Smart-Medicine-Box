#ifndef __LCD_H
#define __LCD_H

#include "ch32v30x.h"
#include <stdlib.h>

/* LCD device struct */
typedef struct
{
    uint16_t width;
    uint16_t height;
    uint16_t id;
    uint8_t  dir;
    uint16_t wramcmd;
    uint16_t setxcmd;
    uint16_t setycmd;
}_lcd_dev;

extern _lcd_dev lcddev;

/* Display orientation: 0=0°, 1=90°, 2=180°, 3=270° */
#define USE_HORIZONTAL  0

/* Resolution */
#define LCD_W 240
#define LCD_H 320

/* Colors */
extern uint16_t POINT_COLOR;
extern uint16_t BACK_COLOR;

/* Pin definitions — CH32V307 GPIOB */
#define LCD_LED_PORT    GPIOB
#define LCD_LED_PIN     GPIO_Pin_9
#define LCD_CS_PORT     GPIOB
#define LCD_CS_PIN      GPIO_Pin_11
#define LCD_RS_PORT     GPIOB
#define LCD_RS_PIN      GPIO_Pin_10
#define LCD_RST_PORT    GPIOB
#define LCD_RST_PIN     GPIO_Pin_12

/* Pin control macros */
#define LCD_LED_ON()    GPIO_SetBits(LCD_LED_PORT, LCD_LED_PIN)
#define LCD_LED_OFF()   GPIO_ResetBits(LCD_LED_PORT, LCD_LED_PIN)
#define LCD_CS_SET()    GPIO_SetBits(LCD_CS_PORT, LCD_CS_PIN)
#define LCD_CS_CLR()    GPIO_ResetBits(LCD_CS_PORT, LCD_CS_PIN)
#define LCD_RS_SET()    GPIO_SetBits(LCD_RS_PORT, LCD_RS_PIN)
#define LCD_RS_CLR()    GPIO_ResetBits(LCD_RS_PORT, LCD_RS_PIN)
#define LCD_RST_SET()   GPIO_SetBits(LCD_RST_PORT, LCD_RST_PIN)
#define LCD_RST_CLR()   GPIO_ResetBits(LCD_RST_PORT, LCD_RST_PIN)

/* Color constants */
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define BRED        0XF81F
#define GRED        0XFFE0
#define GBLUE       0X07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define BROWN       0XBC40
#define BRRED       0XFC07
#define GRAY        0X8430
#define DARKBLUE    0X01CF
#define LIGHTBLUE   0X7D7C
#define GRAYBLUE    0X5458
#define LIGHTGREEN  0X841F
#define LIGHTGRAY   0XEF5B
#define LGRAY       0XC618
#define LGRAYBLUE   0XA651
#define LBBLUE      0X2B12

/* API prototypes */
void LCD_Init(void);
void LCD_Clear(uint16_t Color);
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos);
void LCD_DrawPoint(uint16_t x, uint16_t y);
void LCD_SetWindows(uint16_t xStar, uint16_t yStar, uint16_t xEnd, uint16_t yEnd);
void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue);
void LCD_WR_DATA(uint8_t data);
void LCD_WriteRAM_Prepare(void);
void Lcd_WriteData_16Bit(uint16_t Data);
void LCD_direction(uint8_t direction);
void LCD_WR_REG(uint8_t data);

#endif

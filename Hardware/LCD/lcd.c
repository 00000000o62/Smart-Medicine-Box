#include "lcd.h"
#include "spi.h"
#include "debug.h"
#include <stdlib.h>

_lcd_dev lcddev;
uint16_t POINT_COLOR = 0x0000;
uint16_t BACK_COLOR = 0xFFFF;

void LCD_WR_REG(uint8_t data)
{
    LCD_CS_CLR();
    LCD_RS_CLR();
    SPI_WriteByte(SPI2, data);
    LCD_CS_SET();
}

void LCD_WR_DATA(uint8_t data)
{
    LCD_CS_CLR();
    LCD_RS_SET();
    SPI_WriteByte(SPI2, data);
    LCD_CS_SET();
}

void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
{
    LCD_WR_REG(LCD_Reg);
    LCD_WR_DATA(LCD_RegValue);
}

void LCD_WriteRAM_Prepare(void)
{
    LCD_WR_REG(lcddev.wramcmd);
}

void Lcd_WriteData_16Bit(uint16_t Data)
{
    LCD_CS_CLR();
    LCD_RS_SET();
    SPI_WriteByte(SPI2, Data >> 8);
    SPI_WriteByte(SPI2, Data);
    LCD_CS_SET();
}

void LCD_DrawPoint(uint16_t x, uint16_t y)
{
    LCD_SetCursor(x, y);
    Lcd_WriteData_16Bit(POINT_COLOR);
}

void LCD_Clear(uint16_t Color)
{
    uint32_t i, m;
    LCD_SetWindows(0, 0, lcddev.width - 1, lcddev.height - 1);
    LCD_CS_CLR();
    LCD_RS_SET();
    for(i = 0; i < lcddev.height; i++)
    {
        for(m = 0; m < lcddev.width; m++)
        {
            Lcd_WriteData_16Bit(Color);
        }
    }
    LCD_CS_SET();
}

void LCD_GPIOInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = LCD_LED_PIN | LCD_RS_PIN | LCD_CS_PIN | LCD_RST_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void LCD_RESET(void)
{
    LCD_RST_CLR();
    Delay_Ms(100);
    LCD_RST_SET();
    Delay_Ms(50);
}

void LCD_Init(void)
{
    SPI2_Init();
    LCD_GPIOInit();
    LCD_RESET();

    /* ILI9341 initialization sequence */
    LCD_WR_REG(0xCF);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xD9);
    LCD_WR_DATA(0x30);

    LCD_WR_REG(0xED);
    LCD_WR_DATA(0x64);
    LCD_WR_DATA(0x03);
    LCD_WR_DATA(0x12);
    LCD_WR_DATA(0x81);

    LCD_WR_REG(0xE8);
    LCD_WR_DATA(0x85);
    LCD_WR_DATA(0x10);
    LCD_WR_DATA(0x7A);

    LCD_WR_REG(0xCB);
    LCD_WR_DATA(0x39);
    LCD_WR_DATA(0x2C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x34);
    LCD_WR_DATA(0x02);

    LCD_WR_REG(0xF7);
    LCD_WR_DATA(0x20);

    LCD_WR_REG(0xEA);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA(0x1B);

    LCD_WR_REG(0xC1);
    LCD_WR_DATA(0x12);

    LCD_WR_REG(0xC5);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x26);

    LCD_WR_REG(0xC7);
    LCD_WR_DATA(0xB7);

    LCD_WR_REG(0x36);
    LCD_WR_DATA(0x48);

    LCD_WR_REG(0x3A);
    LCD_WR_DATA(0x55);

    LCD_WR_REG(0xB1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x1A);

    LCD_WR_REG(0xB6);
    LCD_WR_DATA(0x0A);
    LCD_WR_DATA(0x22);

    LCD_WR_REG(0xF2);
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0x26);
    LCD_WR_DATA(0x01);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x1D);
    LCD_WR_DATA(0x1A);
    LCD_WR_DATA(0x0A);
    LCD_WR_DATA(0x0D);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x49);
    LCD_WR_DATA(0x66);
    LCD_WR_DATA(0x3B);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x11);
    LCD_WR_DATA(0x01);
    LCD_WR_DATA(0x09);
    LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x04);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x18);
    LCD_WR_DATA(0x1D);
    LCD_WR_DATA(0x02);
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x04);
    LCD_WR_DATA(0x36);
    LCD_WR_DATA(0x13);
    LCD_WR_DATA(0x4C);
    LCD_WR_DATA(0x07);
    LCD_WR_DATA(0x13);
    LCD_WR_DATA(0x0F);
    LCD_WR_DATA(0x2E);
    LCD_WR_DATA(0x2F);
    LCD_WR_DATA(0x05);

    LCD_WR_REG(0x2B);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x01);
    LCD_WR_DATA(0x3F);

    LCD_WR_REG(0x2A);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0xEF);

    LCD_WR_REG(0x11);
    Delay_Ms(120);

    LCD_WR_REG(0x29);

    LCD_direction(USE_HORIZONTAL);
    LCD_LED_ON();
    LCD_Clear(WHITE);
}

void LCD_SetWindows(uint16_t xStar, uint16_t yStar, uint16_t xEnd, uint16_t yEnd)
{
    LCD_WR_REG(lcddev.setxcmd);
    LCD_WR_DATA(xStar >> 8);
    LCD_WR_DATA(0x00FF & xStar);
    LCD_WR_DATA(xEnd >> 8);
    LCD_WR_DATA(0x00FF & xEnd);

    LCD_WR_REG(lcddev.setycmd);
    LCD_WR_DATA(yStar >> 8);
    LCD_WR_DATA(0x00FF & yStar);
    LCD_WR_DATA(yEnd >> 8);
    LCD_WR_DATA(0x00FF & yEnd);

    LCD_WriteRAM_Prepare();
}

void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
    LCD_SetWindows(Xpos, Ypos, Xpos, Ypos);
}

void LCD_direction(uint8_t direction)
{
    lcddev.setxcmd = 0x2A;
    lcddev.setycmd = 0x2B;
    lcddev.wramcmd = 0x2C;

    switch(direction)
    {
        case 0:
            lcddev.width = LCD_W;
            lcddev.height = LCD_H;
            LCD_WriteReg(0x36, (1 << 3) | (0 << 6) | (0 << 7));
            break;
        case 1:
            lcddev.width = LCD_H;
            lcddev.height = LCD_W;
            LCD_WriteReg(0x36, (1 << 3) | (0 << 7) | (1 << 6) | (1 << 5));
            break;
        case 2:
            lcddev.width = LCD_W;
            lcddev.height = LCD_H;
            LCD_WriteReg(0x36, (1 << 3) | (1 << 6) | (1 << 7));
            break;
        case 3:
            lcddev.width = LCD_H;
            lcddev.height = LCD_W;
            LCD_WriteReg(0x36, (1 << 3) | (1 << 7) | (1 << 5));
            break;
        default:
            break;
    }
}

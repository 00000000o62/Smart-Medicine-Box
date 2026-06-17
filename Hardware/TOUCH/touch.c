#include "touch.h"
#include "lcd.h"
#include "gui.h"
#include "debug.h"
#include <stdlib.h>
#include <math.h>

/* Default touch calibration — will be overwritten by TP_Adjust() */
_m_tp_dev tp_dev = {
    TP_Init,
    TP_Scan,
    TP_Adjust,
    0, 0, 0, 0, 0,
    0.066,   /* xfac — LCD x = raw_x * xfac + xoff */
    0.088,   /* yfac */
    -15,     /* xoff */
    -18,     /* yoff */
    0
};

/* XPT2046 read commands */
static uint8_t CMD_RDX = 0xD0;
static uint8_t CMD_RDY = 0x90;

void TP_Write_Byte(uint8_t num)
{
    uint8_t count;
    for(count = 0; count < 8; count++)
    {
        if(num & 0x80) TDIN_SET();
        else TDIN_CLR();
        num <<= 1;
        TCLK_CLR();
        TCLK_SET();
    }
}

uint16_t TP_Read_AD(uint8_t CMD)
{
    uint8_t count;
    uint16_t Num = 0;
    TCLK_CLR();
    TDIN_CLR();
    TCS_CLR();
    TP_Write_Byte(CMD);
    Delay_Us(6);
    TCLK_CLR();
    Delay_Us(1);
    TCLK_SET();
    TCLK_CLR();
    for(count = 0; count < 16; count++)
    {
        Num <<= 1;
        TCLK_CLR();
        TCLK_SET();
        if(DOUT_READ()) Num++;
    }
    Num >>= 4;
    TCS_SET();
    return Num;
}

#define READ_TIMES 5
#define LOST_VAL   1

static uint16_t TP_Read_XOY(uint8_t xy)
{
    uint16_t i, j;
    uint16_t buf[READ_TIMES];
    uint16_t sum = 0;
    uint16_t temp;
    for(i = 0; i < READ_TIMES; i++) buf[i] = TP_Read_AD(xy);
    for(i = 0; i < READ_TIMES - 1; i++)
    {
        for(j = i + 1; j < READ_TIMES; j++)
        {
            if(buf[i] > buf[j])
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }
    sum = 0;
    for(i = LOST_VAL; i < READ_TIMES - LOST_VAL; i++) sum += buf[i];
    temp = sum / (READ_TIMES - 2 * LOST_VAL);
    return temp;
}

static uint8_t TP_Read_XY(uint16_t *x, uint16_t *y)
{
    uint16_t xtemp, ytemp;
    xtemp = TP_Read_XOY(CMD_RDX);
    ytemp = TP_Read_XOY(CMD_RDY);
    *x = xtemp;
    *y = ytemp;
    return 1;
}

#define ERR_RANGE 50

uint8_t TP_Read_XY2(uint16_t *x, uint16_t *y)
{
    uint16_t x1, y1, x2, y2;
    uint8_t flag;
    flag = TP_Read_XY(&x1, &y1);
    if(flag == 0) return 0;
    flag = TP_Read_XY(&x2, &y2);
    if(flag == 0) return 0;
    if(((x2 <= x1 && x1 < x2 + ERR_RANGE) || (x1 <= x2 && x2 < x1 + ERR_RANGE))
    && ((y2 <= y1 && y1 < y2 + ERR_RANGE) || (y1 <= y2 && y2 < y1 + ERR_RANGE)))
    {
        *x = (x1 + x2) / 2;
        *y = (y1 + y2) / 2;
        return 1;
    }
    return 0;
}

void TP_Drow_Touch_Point(uint16_t x, uint16_t y, uint16_t color)
{
    POINT_COLOR = color;
    LCD_DrawLine(x - 12, y, x + 13, y);
    LCD_DrawLine(x, y - 12, x, y + 13);
    LCD_DrawPoint(x + 1, y + 1);
    LCD_DrawPoint(x - 1, y + 1);
    LCD_DrawPoint(x + 1, y - 1);
    LCD_DrawPoint(x - 1, y - 1);
    gui_circle(x, y, color, 6, 0);
}

void TP_Draw_Big_Point(uint16_t x, uint16_t y, uint16_t color)
{
    POINT_COLOR = color;
    LCD_DrawPoint(x, y);
    LCD_DrawPoint(x + 1, y);
    LCD_DrawPoint(x, y + 1);
    LCD_DrawPoint(x + 1, y + 1);
}

uint8_t TP_Scan(uint8_t tp)
{
    if(PEN_READ() == 0)
    {
        if(tp)
            TP_Read_XY2(&tp_dev.x, &tp_dev.y);
        else if(TP_Read_XY2(&tp_dev.x, &tp_dev.y))
        {
            tp_dev.x = (uint16_t)(tp_dev.xfac * tp_dev.x + tp_dev.xoff);
            tp_dev.y = (uint16_t)(tp_dev.yfac * tp_dev.y + tp_dev.yoff);
        }
        if((tp_dev.sta & TP_PRES_DOWN) == 0)
        {
            tp_dev.sta = TP_PRES_DOWN | TP_CATH_PRES;
            tp_dev.x0 = tp_dev.x;
            tp_dev.y0 = tp_dev.y;
        }
    }
    else
    {
        if(tp_dev.sta & TP_PRES_DOWN)
        {
            tp_dev.sta &= ~(1 << 7);
        }
        else
        {
            tp_dev.x0 = 0;
            tp_dev.y0 = 0;
            tp_dev.x = 0xffff;
            tp_dev.y = 0xffff;
        }
    }
    return tp_dev.sta & TP_PRES_DOWN;
}

void TP_Adjust(void)
{
    uint16_t pos_temp[4][2];
    uint8_t  cnt = 0;
    uint16_t d1, d2;
    uint32_t tem1, tem2;
    float fac;
    uint16_t outtime = 0;

    POINT_COLOR = BLUE;
    BACK_COLOR = WHITE;
    LCD_Clear(WHITE);
    POINT_COLOR = BLACK;
    LCD_ShowString(10, 40, 16, (uint8_t *)"Touch calibration", 1);
    LCD_ShowString(10, 56, 16, (uint8_t *)"Tap cross to calibrate", 1);

    TP_Drow_Touch_Point(20, 20, RED);
    tp_dev.sta = 0;
    tp_dev.xfac = 0;

    while(1)
    {
        tp_dev.scan(1);
        if((tp_dev.sta & 0xc0) == TP_CATH_PRES)
        {
            outtime = 0;
            tp_dev.sta &= ~(1 << 6);
            pos_temp[cnt][0] = tp_dev.x;
            pos_temp[cnt][1] = tp_dev.y;
            cnt++;
            switch(cnt)
            {
                case 1:
                    TP_Drow_Touch_Point(20, 20, WHITE);
                    TP_Drow_Touch_Point(lcddev.width - 20, 20, RED);
                    break;
                case 2:
                    TP_Drow_Touch_Point(lcddev.width - 20, 20, WHITE);
                    TP_Drow_Touch_Point(20, lcddev.height - 20, RED);
                    break;
                case 3:
                    TP_Drow_Touch_Point(20, lcddev.height - 20, WHITE);
                    TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, RED);
                    break;
                case 4:
                    tem1 = abs(pos_temp[0][0] - pos_temp[1][0]);
                    tem2 = abs(pos_temp[0][1] - pos_temp[1][1]);
                    tem1 *= tem1; tem2 *= tem2;
                    d1 = (uint16_t)sqrt((double)(tem1 + tem2));

                    tem1 = abs(pos_temp[2][0] - pos_temp[3][0]);
                    tem2 = abs(pos_temp[2][1] - pos_temp[3][1]);
                    tem1 *= tem1; tem2 *= tem2;
                    d2 = (uint16_t)sqrt((double)(tem1 + tem2));
                    fac = (float)d1 / d2;

                    if(fac < 0.95 || fac > 1.05 || d1 == 0 || d2 == 0)
                    {
                        cnt = 0;
                        TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);
                        TP_Drow_Touch_Point(20, 20, RED);
                        continue;
                    }

                    tem1 = abs(pos_temp[0][0] - pos_temp[2][0]);
                    tem2 = abs(pos_temp[0][1] - pos_temp[2][1]);
                    tem1 *= tem1; tem2 *= tem2;
                    d1 = (uint16_t)sqrt((double)(tem1 + tem2));

                    tem1 = abs(pos_temp[1][0] - pos_temp[3][0]);
                    tem2 = abs(pos_temp[1][1] - pos_temp[3][1]);
                    tem1 *= tem1; tem2 *= tem2;
                    d2 = (uint16_t)sqrt((double)(tem1 + tem2));
                    fac = (float)d1 / d2;

                    if(fac < 0.95 || fac > 1.05)
                    {
                        cnt = 0;
                        TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);
                        TP_Drow_Touch_Point(20, 20, RED);
                        continue;
                    }

                    tp_dev.xfac = (float)(lcddev.width - 40) / (pos_temp[1][0] - pos_temp[0][0]);
                    tp_dev.xoff = (int16_t)((lcddev.width - tp_dev.xfac * (pos_temp[1][0] + pos_temp[0][0])) / 2);
                    tp_dev.yfac = (float)(lcddev.height - 40) / (pos_temp[2][1] - pos_temp[0][1]);
                    tp_dev.yoff = (int16_t)((lcddev.height - tp_dev.yfac * (pos_temp[2][1] + pos_temp[0][1])) / 2);

                    if(fabs(tp_dev.xfac) > 2 || fabs(tp_dev.yfac) > 2)
                    {
                        cnt = 0;
                        TP_Drow_Touch_Point(lcddev.width - 20, lcddev.height - 20, WHITE);
                        TP_Drow_Touch_Point(20, 20, RED);
                        LCD_ShowString(40, 26, 16, (uint8_t *)"Recalibrating...", 1);
                        tp_dev.touchtype = !tp_dev.touchtype;
                        if(tp_dev.touchtype)
                        {
                            CMD_RDX = 0x90;
                            CMD_RDY = 0xD0;
                        }
                        else
                        {
                            CMD_RDX = 0xD0;
                            CMD_RDY = 0x90;
                        }
                        continue;
                    }

                    POINT_COLOR = BLUE;
                    LCD_Clear(WHITE);
                    LCD_ShowString(35, 110, 16, (uint8_t *)"Calibration OK!", 1);
                    Delay_Ms(1000);
                    LCD_Clear(WHITE);
                    return;
            }
        }
        Delay_Ms(10);
        outtime++;
        if(outtime > 1000) break;
    }
}

uint8_t TP_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

    /* T_CLK(PC0), T_DIN(PC3), T_CS(PC13) as push-pull outputs */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_0 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* T_IRQ(PC10), T_DO(PC2) as input pull-up */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    TP_Read_XY(&tp_dev.x, &tp_dev.y);

    return 0; /* No calibration stored, will recalibrate */
}

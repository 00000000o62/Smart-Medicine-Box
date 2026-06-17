#include "medbox_ui.h"
#include "medbox_schedule.h"
#include "lcd.h"
#include "gui.h"
#include <stdio.h>
#include <string.h>

static uint8_t screen_dirty = 1;

float    display_temp   = 0.0f;
float    display_humi   = 0.0f;
uint16_t display_tvoc   = 0;
uint16_t display_co2    = 0;
uint8_t  wifi_connected = 0;
char     wifi_ip[16]    = "---";
uint8_t  sensor_ok      = 0;
float    display_lux    = 0.0f;

void UI_Init(void) { screen_dirty = 1; LCD_Clear(WHITE); }

void UI_DrawAll(void)
{
    LCD_Clear(WHITE);

    /* ===== Top bar: 智能药箱 (GB2312: D6C7 C4DC D2A9 CFE4) ===== */
    LCD_Fill(0, 0, 239, 25, BLUE);
    Show_Str(5, 5, WHITE, BLUE, (uint8_t *)"\xD6\xC7\xC4\xDC\xD2\xA9\xCF\xE4", 16, 0);
    {
        char t[8]; sprintf(t, "%02d:%02d", current_time.hour, current_time.minute);
        POINT_COLOR = WHITE; BACK_COLOR = BLUE;
        LCD_ShowString(180, 5, 16, (uint8_t *)t, 0);
    }

    POINT_COLOR = BLACK; BACK_COLOR = WHITE;

    /* ===== Date + Time ===== */
    {
        char b[32];
        sprintf(b, "%04d-%02d-%02d  %02d:%02d:%02d",
                current_time.year, current_time.month, current_time.day,
                current_time.hour, current_time.minute, current_time.second);
        LCD_ShowString(20, 32, 16, (uint8_t *)b, 0);
    }

    /* ===== Separator ===== */
    POINT_COLOR = GRAY; LCD_DrawLine(5, 50, 234, 50);

    /* ===== Sensor section header: 温湿度测试 ===== */
    /* 温湿度监测 */
    Show_Str(5, 55, DARKBLUE, WHITE,
             (uint8_t *)"\xCE\xC2\xCA\xAA\xB6\xC8\xBC\xE0\xB2\xE2", 16, 0);

    UI_UpdateSensors();

    /* ===== WiFi ===== */
    {
        char b[32];
        if(wifi_connected) sprintf(b, "WiFi: %s", wifi_ip);
        else sprintf(b, "WiFi: OFF");
        POINT_COLOR = wifi_connected ? GREEN : GRAY;
        LCD_ShowString(5, 115, 16, (uint8_t *)b, 0);
    }

    /* ===== Alarm overlay (kept even though medication list removed) ===== */
    if(alarm_active)
    {
        LCD_Fill(20, 200, 220, 270, YELLOW);
        POINT_COLOR = RED; BACK_COLOR = YELLOW;
        LCD_ShowString(35, 210, 16, (uint8_t *)"TIME TO TAKE MED!", 0);
        char m[32]; sprintf(m, "%s  Box#%d",
            schedule[alarm_slot_index].name, schedule[alarm_slot_index].compartment);
        POINT_COLOR = BLACK;
        LCD_ShowString(30, 235, 16, (uint8_t *)m, 0);
    }

    screen_dirty = 0;
}

void UI_UpdateClock(void)
{
    LCD_Fill(20, 32, 220, 48, WHITE);
    POINT_COLOR = BLACK; BACK_COLOR = WHITE;
    char b[32];
    sprintf(b, "%04d-%02d-%02d  %02d:%02d:%02d",
            current_time.year, current_time.month, current_time.day,
            current_time.hour, current_time.minute, current_time.second);
    LCD_ShowString(20, 32, 16, (uint8_t *)b, 0);

    LCD_Fill(180, 5, 230, 22, BLUE);
    POINT_COLOR = WHITE; BACK_COLOR = BLUE;
    char t[8]; sprintf(t, "%02d:%02d", current_time.hour, current_time.minute);
    LCD_ShowString(180, 5, 16, (uint8_t *)t, 0);
}

void UI_UpdateSensors(void)
{
    LCD_Fill(5, 72, 234, 130, WHITE);

    /* 温度: xx.x C */
    {
        char b[24];
        if(sensor_ok)
            sprintf(b, "\xCE\xC2\xB6\xC8: %d.%d C",
                    (int)display_temp, abs((int)(display_temp * 10) % 10));
        else
            sprintf(b, "\xCE\xC2\xB6\xC8: --.- C");
        POINT_COLOR = BLACK;
        Show_Str(5, 75, BLACK, WHITE, (uint8_t *)b, 16, 0);
    }

    /* 湿度: xx % */
    {
        char b[20];
        if(sensor_ok)
            sprintf(b, "\xCA\xAA\xB6\xC8: %d %%", (int)display_humi);
        else
            sprintf(b, "\xCA\xAA\xB6\xC8: -- %%");
        POINT_COLOR = BLACK;
        Show_Str(130, 75, BLACK, WHITE, (uint8_t *)b, 16, 0);
    }

    /* Air quality — 2 rows to fit 240px */
    {
        char b[40];
        if(display_tvoc > 0)
        {
            const char *q; uint16_t qc;
            if(display_tvoc < 100)      { q = "Good";  qc = GREEN; }
            else if(display_tvoc < 300) { q = "Moderate"; qc = YELLOW; }
            else if(display_tvoc < 500) { q = "Poor";  qc = RED; }
            else                        { q = "Bad!";  qc = BRED; }
            POINT_COLOR = qc;
            sprintf(b, "Air: %s  VOC: %d ppb", q, display_tvoc);
            LCD_ShowString(5, 95, 16, (uint8_t *)b, 0);
            POINT_COLOR = BLACK;
            sprintf(b, "CO2: %d ppm", display_co2);
            LCD_ShowString(5, 112, 16, (uint8_t *)b, 0);
        }
        else
        {
            POINT_COLOR = GRAY;
            sprintf(b, "Air: waiting...");
            LCD_ShowString(5, 95, 16, (uint8_t *)b, 0);
        }

        /* Light sensor */
        {
            POINT_COLOR = BLACK;
            if(display_lux > 0.0f)
                sprintf(b, "Light: %d lx", (int)display_lux);
            else
                sprintf(b, "Light: --- lx");
            LCD_ShowString(5, 129, 16, (uint8_t *)b, 0);
        }
    }
}

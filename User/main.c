#include "debug.h"
#include "lcd.h"
#include "gui.h"
#include "spi.h"
#include "medbox_schedule.h"
#include "medbox_ui.h"
#include "dht11.h"
#include "sgp30.h"
#include "esp8266.h"
#include "bh1750.h"
#include "config.h"
#include <string.h>

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("SystemClk:%lu\r\n", (unsigned long)SystemCoreClock);
    printf("=== Smart Medicine Box ===\r\n");

    /* LCD */
    LCD_Init();
    LCD_Clear(BLUE);
    POINT_COLOR = WHITE; BACK_COLOR = BLUE;
    LCD_ShowString(40, 120, 16, (uint8_t *)"Smart Medicine Box", 0);
    LCD_ShowString(50, 150, 16, (uint8_t *)"Starting...", 0);
    Delay_Ms(1500);

    /* Core modules */
    MedSchedule_Init();
    MedTime_Init();
    Button_Init();

    /* Sensors */
    DHT11_Init();
    printf("DHT11 init\r\n");
    SGP30_Init();
    SGP30_Probe();
    BH1750_Init();

    /* WiFi */
    ESP8266_Init();
    if(ESP8266_ATTest())
    {
        printf("ESP8266 AT OK\r\n");
        if(ESP8266_ConnectWiFi(WIFI_SSID, WIFI_PASS))
        {
            char ip[16] = {0};
            if(ESP8266_GetIP(ip))
            {
                wifi_connected = 1;
                strncpy(wifi_ip, ip, 15);
                printf("WiFi: %s\r\n", ip);
                /* Establish persistent MQTT */
                ESP8266_OneNET_Connect(ONENET_BROKER, ONENET_PORT,
                    ONENET_CLIENT, ONENET_USER, ONENET_PASS);
            }
        }
    }

    /* Draw initial screen */
    UI_DrawAll();
    printf("Ready.\r\n");

    /* ==== Main loop ==== */
    uint16_t sensor_tick = 0;
    uint16_t upload_tick = 0;

    while(1)
    {
        MedTime_Update();
        MedSchedule_CheckAlarm();

        /* Clock update every 1s */
        if(current_time.ticks == 0)
            UI_UpdateClock();

        /* Micro switch: count pills silently (no display) */
        Button_Poll();

        /* Sensors every 2s */
        sensor_tick++;
        if(sensor_tick >= 20)
        {
            sensor_tick = 0;

            /* DHT11 */
            {
                DHT11_Data d;
                uint8_t result = DHT11_Read(&d);
                if(result && d.valid)
                {
                    display_temp = d.temperature;
                    display_humi = d.humidity;
                    sensor_ok = 1;
                    printf("DHT11 OK: T=%d.%d H=%d%%\r\n",
                           (int)display_temp, (int)(display_temp * 10) % 10,
                           (int)display_humi);
                    if(sgp30.ready)
                        SGP30_SetHumidity(display_temp, display_humi);
                }
                else
                {
                    printf("DHT11 FAIL: ret=%d rh=%d.%d tmp=%d.%d sum=%d ok=%d\r\n",
                           result,
                           d.humi_int, d.humi_dec,
                           d.temp_int, d.temp_dec,
                           d.checksum, d.valid);
                }
            }

            /* SGP30 */
            if(sgp30.ready)
            {
                SGP30_Data s;
                if(SGP30_Measure(&s))
                {
                    display_tvoc = s.tvoc_ppb;
                    display_co2  = s.co2_ppm;
                    printf("SGP30: TVOC=%d CO2=%d\r\n", display_tvoc, display_co2);
                }
            }

            /* BH1750 */
            {
                float lux;
                if(BH1750_Read(&lux))
                {
                    display_lux = lux;
                    printf("BH1750: Light=%d lx\r\n", (int)lux);
                }
            }

            /* Partial redraw of sensor area */
            UI_UpdateSensors();
        }

        /* WiFi status check every 30s */
        static uint16_t wifi_tick = 0;
        wifi_tick++;
        if(wifi_tick >= 300)
        {
            wifi_tick = 0;
            if(!wifi_connected)
            {
                /* Retry WiFi connection */
                if(ESP8266_ATTest())
                {
                    if(ESP8266_ConnectWiFi(WIFI_SSID, WIFI_PASS))
                    {
                        char ip[16] = {0};
                        if(ESP8266_GetIP(ip))
                        {
                            wifi_connected = 1;
                            strncpy(wifi_ip, ip, 15);
                            printf("WiFi OK: %s\r\n", ip);
                            ESP8266_OneNET_Connect(ONENET_BROKER, ONENET_PORT,
                                ONENET_CLIENT, ONENET_USER, ONENET_PASS);
                        }
                    }
                }
            }
            /* Update WiFi display line */
            {
                LCD_Fill(5, 115, 180, 130, WHITE);
                char b[32];
                uint16_t wcolor;
                if(wifi_connected) {
                    sprintf(b, "WiFi: %s", wifi_ip);
                    wcolor = GREEN;
                } else if(ESP8266_ATTest()) {
                    sprintf(b, "WiFi: NO IP");
                    wcolor = YELLOW;
                } else {
                    sprintf(b, "WiFi: NO HW");
                    wcolor = RED;
                }
                POINT_COLOR = wcolor;
                LCD_ShowString(5, 115, 16, (uint8_t *)b, 0);
            }
        }

        /* OneNET upload every 30s (TCP→CONNECT→PUBLISH→CLOSE stateless) */
        upload_tick++;
        if(upload_tick >= 300 && wifi_connected)
        {
            upload_tick = 0;
            static uint32_t msg_id = 0;
            msg_id++;

            char json[384];
            sprintf(json,
                "{"
                  "\"id\":%lu,"
                  "\"dp\":{"
                    "\"temperature\":[{\"v\":%d.%d}],"
                    "\"humidity\":[{\"v\":%d.%d}],"
                    "\"tvoc\":[{\"v\":%d}],"
                    "\"co2\":[{\"v\":%d}],"
                    "\"lux\":[{\"v\":%d}],"
                    "\"pill_count\":[{\"v\":%lu}]"
                  "}"
                "}",
                (unsigned long)(msg_id % 2147483647),
                (int)display_temp, abs((int)(display_temp * 10) % 10),
                (int)display_humi, abs((int)(display_humi * 10) % 10),
                display_tvoc,
                display_co2,
                (int)display_lux,
                (unsigned long)pill_count);

            if(!ESP8266_OneNET_Publish(ONENET_TOPIC, json))
            {
                /* Publish failed — reconnect and retry once */
                printf("OneNET: reconnect...\r\n");
                if(ESP8266_OneNET_Connect(ONENET_BROKER, ONENET_PORT,
                    ONENET_CLIENT, ONENET_USER, ONENET_PASS))
                {
                    if(ESP8266_OneNET_Publish(ONENET_TOPIC, json))
                        printf("OneNET: uploaded\r\n");
                }
            }
            else
                printf("OneNET: uploaded\r\n");
        }

        Delay_Ms(100);
    }
}

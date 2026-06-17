/* ESP8266-01S WiFi driver via USART2 AT commands
   MQTT via raw TCP (manual packet construction — bypasses AT MQTT commands) */
#include "esp8266.h"
#include "ch32v30x_usart.h"
#include "ch32v30x_gpio.h"
#include "ch32v30x_rcc.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

static char esp_rx_buf[512];
static uint16_t esp_rx_len = 0;

/* ============================================================
   Interrupt & low-level helpers
   ============================================================ */

void USART2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART2_IRQHandler(void)
{
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        if(esp_rx_len < sizeof(esp_rx_buf) - 1)
            esp_rx_buf[esp_rx_len++] = (uint8_t)USART_ReceiveData(USART2);
    }
    if(USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET)
        (void)USART_ReceiveData(USART2);
    if(USART_GetFlagStatus(USART2, USART_FLAG_FE) != RESET)
        (void)USART_ReceiveData(USART2);
    if(USART_GetFlagStatus(USART2, USART_FLAG_NE) != RESET)
        (void)USART_ReceiveData(USART2);
}

/* Send a null-terminated string via USART2 */
static void esp_send(const char *str)
{
    while(*str)
    {
        USART_SendData(USART2, *str++);
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    }
}

/* Classic AT command: send string, wait for expected response */
static uint8_t esp_at_cmd(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    esp_rx_len = 0;
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    esp_send(cmd);
    esp_send("\r\n");
    uint32_t waited = 0;
    while(waited < timeout_ms)
    {
        if(strstr(esp_rx_buf, expect)) return 1;
        Delay_Ms(50);
        waited += 50;
    }
    return 0;
}

/* ============================================================
   Public API
   ============================================================ */

void ESP8266_Init(void)
{
    GPIO_InitTypeDef g = {0};
    USART_InitTypeDef u = {0};
    NVIC_InitTypeDef  n = {0};

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    g.GPIO_Pin   = GPIO_Pin_2;
    g.GPIO_Mode  = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &g);

    g.GPIO_Pin  = GPIO_Pin_3;
    g.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(GPIOA, &g);

    u.USART_BaudRate            = 115200;
    u.USART_WordLength          = USART_WordLength_8b;
    u.USART_StopBits            = USART_StopBits_1;
    u.USART_Parity              = USART_Parity_No;
    u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    u.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2, &u);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    n.NVIC_IRQChannel                   = USART2_IRQn;
    n.NVIC_IRQChannelPreemptionPriority = 1;
    n.NVIC_IRQChannelSubPriority        = 1;
    n.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&n);

    USART_Cmd(USART2, ENABLE);
    printf("ESP8266: USART2 init 115200\r\n");
}

void ESP8266_Reset(void)
{
    esp_at_cmd("AT+RST", "ready", 3000);
    Delay_Ms(2000);
    printf("ESP8266: reset done\r\n");
}

uint8_t ESP8266_ATTest(void)
{
    return esp_at_cmd("AT", "OK", 2000);
}

uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *pwd)
{
    char cmd[128];
    esp_at_cmd("AT+CWMODE=1", "OK", 2000);
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    if(!esp_at_cmd(cmd, "OK", 15000))
    {
        printf("ESP8266: WiFi connect FAIL\r\n");
        return 0;
    }
    printf("ESP8266: WiFi connected\r\n");
    return 1;
}

uint8_t ESP8266_GetIP(char *ip_buf)
{
    esp_rx_len = 0;
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    esp_send("AT+CIFSR\r\n");
    Delay_Ms(1000);
    char *p = strstr(esp_rx_buf, "+CIFSR:STAIP,\"");
    if(p)
    {
        p += 14;
        uint8_t i = 0;
        while(*p && *p != '"' && i < 15) ip_buf[i++] = *p++;
        ip_buf[i] = '\0';
        return 1;
    }
    return 0;
}

/* ---- MQTT via raw TCP ---- */

/* Dump CONNACK reason from +IPD */
static void mqtt_dump_connack(void)
{
    char *p = strstr(esp_rx_buf, "+IPD,4");
    if(!p) { printf("  CONNACK: not found\r\n"); return; }
    p += 7;
    uint8_t b0 = (uint8_t)p[0];
    uint8_t b1 = (uint8_t)p[1];
    uint8_t b2 = (uint8_t)p[2];
    uint8_t b3 = (uint8_t)p[3];
    printf("  CONNACK: %02X %02X %02X %02X", b0, b1, b2, b3);
    if(b0 == 0x20 && b1 == 0x02 && b2 == 0x00)
    {
        if(b3 == 0x00)      printf(" (Accepted)\r\n");
        else if(b3 == 0x04) printf(" (Bad username/password!)\r\n");
        else if(b3 == 0x05) printf(" (Not authorized!)\r\n");
        else                printf(" (Rejected code=%d)\r\n", b3);
    }
    else printf(" (unknown)\r\n");
}

/* Open TCP connection via AT+CIPSTART.
   Returns 1 on CONNECT, 0 on failure. */
static uint8_t esp_tcp_open(const char *host, uint16_t port)
{
    char cmd[64];
    esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
    Delay_Ms(200);
    esp_at_cmd("AT+CIPMUX=0", "OK", 2000);
    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%d", host, port);
    if(!esp_at_cmd(cmd, "CONNECT", 10000))
    {
        printf("ESP8266: TCP %s:%d FAIL\r\n", host, port);
        return 0;
    }
    printf("ESP8266: TCP %s:%d OK\r\n", host, port);
    return 1;
}

/* Send raw bytes via AT+CIPSEND. */
static uint8_t esp_tcp_send(const uint8_t *data, int len)
{
    char cmd[32];
    sprintf(cmd, "AT+CIPSEND=%d", len);

    esp_rx_len = 0;
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    esp_send(cmd);
    esp_send("\r\n");

    uint32_t waited = 0;
    while(waited < 3000)
    {
        if(memchr(esp_rx_buf, '>', esp_rx_len)) break;
        Delay_Ms(50);
        waited += 50;
    }
    if(waited >= 3000)
    {
        /* Debug dump */
        esp_rx_buf[esp_rx_len] = '\0';
        printf("CIPSEND no '>' rx[%d]: ", (int)esp_rx_len);
        for(int i = 0; i < (int)esp_rx_len && i < 120; i++)
        {
            uint8_t c = (uint8_t)esp_rx_buf[i];
            if(c >= 32 && c < 127) printf("%c", c);
            else printf("[%02X]", c);
        }
        printf("\r\n");
        return 0;
    }

    /* Send raw bytes */
    for(int i = 0; i < len; i++)
    {
        USART_SendData(USART2, data[i]);
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    }

    waited = 0;
    while(waited < 10000)
    {
        if(strstr(esp_rx_buf, "SEND OK")) return 1;
        if(strstr(esp_rx_buf, "ERROR")) return 0;
        Delay_Ms(50);
        waited += 50;
    }
    printf("CIPSEND timeout\r\n");
    return 0;
}

/* MQTT remaining length encoder */
static int mqtt_encode_remlen(uint8_t *buf, int len)
{
    int i = 0;
    do {
        uint8_t b = len % 128;
        len /= 128;
        if(len > 0) b |= 0x80;
        buf[i++] = b;
    } while(len > 0);
    return i;
}

uint8_t ESP8266_MQTT_Connect(const char *broker, uint16_t port,
                              const char *client_id,
                              const char *username,
                              const char *password)
{
    int clen = (int)strlen(client_id);
    int ulen = (int)strlen(username);
    int plen = (int)strlen(password);

    if(!esp_tcp_open(broker, port))
        return 0;

    uint8_t pkt[512];
    int pos = 0;
    int remaining = 10 + (2 + clen) + (2 + ulen) + (2 + plen);

    pkt[pos++] = 0x10;
    pos += mqtt_encode_remlen(&pkt[pos], remaining);

    pkt[pos++] = 0x00; pkt[pos++] = 0x04;
    pkt[pos++] = 'M'; pkt[pos++] = 'Q'; pkt[pos++] = 'T'; pkt[pos++] = 'T';
    pkt[pos++] = 0x04;
    pkt[pos++] = 0xC2;
    pkt[pos++] = 0x00; pkt[pos++] = 0x78;

    pkt[pos++] = (uint8_t)(clen >> 8);
    pkt[pos++] = (uint8_t)(clen);
    memcpy(&pkt[pos], client_id, clen); pos += clen;

    pkt[pos++] = (uint8_t)(ulen >> 8);
    pkt[pos++] = (uint8_t)(ulen);
    memcpy(&pkt[pos], username, ulen); pos += ulen;

    pkt[pos++] = (uint8_t)(plen >> 8);
    pkt[pos++] = (uint8_t)(plen);
    memcpy(&pkt[pos], password, plen); pos += plen;

    if(!esp_tcp_send(pkt, pos))
    {
        printf("ESP8266: CONNECT send FAIL\r\n");
        return 0;
    }

    uint32_t waited = 0;
    while(waited < 5000)
    {
        if(strstr(esp_rx_buf, "+IPD,4"))
        {
            mqtt_dump_connack();
            printf("ESP8266: MQTT connected to %s:%d\r\n", broker, port);
            return 1;
        }
        Delay_Ms(50);
        waited += 50;
    }
    printf("ESP8266: no CONNACK\r\n");
    return 0;
}

uint8_t ESP8266_MQTT_Publish(const char *topic, const char *payload)
{
    uint8_t pkt[600];
    int pos = 0;

    int tlen = (int)strlen(topic);
    int plen = (int)strlen(payload);
    int remaining = 2 + tlen + 2 + plen;

    pkt[pos++] = 0x32;
    pos += mqtt_encode_remlen(&pkt[pos], remaining);

    pkt[pos++] = (uint8_t)(tlen >> 8);
    pkt[pos++] = (uint8_t)(tlen);
    memcpy(&pkt[pos], topic, tlen); pos += tlen;

    pkt[pos++] = 0x00;
    pkt[pos++] = 0x01;
    memcpy(&pkt[pos], payload, plen); pos += plen;

    if(!esp_tcp_send(pkt, pos))
    {
        printf("ESP8266: MQTT publish FAIL\r\n");
        return 0;
    }
    return 1;
}

uint8_t ESP8266_OneNET_Upload(
    const char *broker, uint16_t port,
    const char *client_id, const char *username, const char *password,
    const char *topic, const char *json)
{
    /* Build MQTT CONNECT packet */
    int clen = (int)strlen(client_id);
    int ulen = (int)strlen(username);
    int plen = (int)strlen(password);
    int tlen = (int)strlen(topic);
    int jlen = (int)strlen(json);

    if(!esp_tcp_open(broker, port))
        return 0;

    uint8_t pkt[512];
    int pos = 0;
    int remaining = 10 + (2 + clen) + (2 + ulen) + (2 + plen);

    pkt[pos++] = 0x10;
    pos += mqtt_encode_remlen(&pkt[pos], remaining);

    pkt[pos++] = 0x00; pkt[pos++] = 0x04;
    pkt[pos++] = 'M'; pkt[pos++] = 'Q'; pkt[pos++] = 'T'; pkt[pos++] = 'T';
    pkt[pos++] = 0x04;
    pkt[pos++] = 0xC2;
    pkt[pos++] = 0x00; pkt[pos++] = 0x78;

    pkt[pos++] = (uint8_t)(clen >> 8);
    pkt[pos++] = (uint8_t)(clen);
    memcpy(&pkt[pos], client_id, clen); pos += clen;

    pkt[pos++] = (uint8_t)(ulen >> 8);
    pkt[pos++] = (uint8_t)(ulen);
    memcpy(&pkt[pos], username, ulen); pos += ulen;

    pkt[pos++] = (uint8_t)(plen >> 8);
    pkt[pos++] = (uint8_t)(plen);
    memcpy(&pkt[pos], password, plen); pos += plen;

    if(!esp_tcp_send(pkt, pos))
    {
        esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
        printf("OneNET: CONNECT send FAIL\r\n");
        return 0;
    }

    uint32_t waited = 0;
    uint8_t conn_ok = 0;
    while(waited < 5000)
    {
        if(strstr(esp_rx_buf, "+IPD,4"))
        {
            mqtt_dump_connack();
            conn_ok = 1;
            break;
        }
        Delay_Ms(50);
        waited += 50;
    }
    if(!conn_ok)
    {
        esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
        printf("OneNET: no CONNACK\r\n");
        return 0;
    }

    /* Build PUBLISH */
    pos = 0;
    remaining = 2 + tlen + 2 + jlen;
    pkt[pos++] = 0x32;
    pos += mqtt_encode_remlen(&pkt[pos], remaining);
    pkt[pos++] = (uint8_t)(tlen >> 8);
    pkt[pos++] = (uint8_t)(tlen);
    memcpy(&pkt[pos], topic, tlen); pos += tlen;
    pkt[pos++] = 0x00;
    pkt[pos++] = 0x01;
    memcpy(&pkt[pos], json, jlen); pos += jlen;

    if(!esp_tcp_send(pkt, pos))
    {
        esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
        printf("OneNET: PUBLISH send FAIL\r\n");
        return 0;
    }

    esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
    return 1;
}

uint8_t ESP8266_OneNET_UploadBin(
    const char *broker, uint16_t port,
    const char *client_id, const char *username, const char *password,
    const char *topic, const uint8_t *data, int data_len)
{
    int clen = (int)strlen(client_id);
    int ulen = (int)strlen(username);
    int plen = (int)strlen(password);
    int tlen = (int)strlen(topic);

    if(!esp_tcp_open(broker, port))
        return 0;

    uint8_t pkt[512];
    int pos = 0;
    int remaining = 10 + (2 + clen) + (2 + ulen) + (2 + plen);

    pkt[pos++] = 0x10;
    pos += mqtt_encode_remlen(&pkt[pos], remaining);

    pkt[pos++] = 0x00; pkt[pos++] = 0x04;
    pkt[pos++] = 'M'; pkt[pos++] = 'Q'; pkt[pos++] = 'T'; pkt[pos++] = 'T';
    pkt[pos++] = 0x04;
    pkt[pos++] = 0xC2;
    pkt[pos++] = 0x00; pkt[pos++] = 0x78;

    pkt[pos++] = (uint8_t)(clen >> 8);
    pkt[pos++] = (uint8_t)(clen);
    memcpy(&pkt[pos], client_id, clen); pos += clen;

    pkt[pos++] = (uint8_t)(ulen >> 8);
    pkt[pos++] = (uint8_t)(ulen);
    memcpy(&pkt[pos], username, ulen); pos += ulen;

    pkt[pos++] = (uint8_t)(plen >> 8);
    pkt[pos++] = (uint8_t)(plen);
    memcpy(&pkt[pos], password, plen); pos += plen;

    if(!esp_tcp_send(pkt, pos))
    {
        esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
        printf("OneNET: CONNECT send FAIL\r\n");
        return 0;
    }

    uint32_t waited = 0;
    uint8_t conn_ok = 0;
    while(waited < 5000)
    {
        if(strstr(esp_rx_buf, "+IPD,4"))
        {
            mqtt_dump_connack();
            conn_ok = 1;
            break;
        }
        Delay_Ms(50);
        waited += 50;
    }
    if(!conn_ok)
    {
        esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
        printf("OneNET: no CONNACK\r\n");
        return 0;
    }

    pos = 0;
    remaining = 2 + tlen + 2 + data_len;
    pkt[pos++] = 0x32;
    pos += mqtt_encode_remlen(&pkt[pos], remaining);
    pkt[pos++] = (uint8_t)(tlen >> 8);
    pkt[pos++] = (uint8_t)(tlen);
    memcpy(&pkt[pos], topic, tlen); pos += tlen;
    pkt[pos++] = 0x00;
    pkt[pos++] = 0x01;
    memcpy(&pkt[pos], data, data_len); pos += data_len;

    if(!esp_tcp_send(pkt, pos))
    {
        esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
        printf("OneNET: PUBLISH send FAIL\r\n");
        return 0;
    }

    esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
    return 1;
}

/* ============================================================
   Persistent connection API (device stays "online")
   ============================================================ */

static uint8_t mqtt_persistent = 0;

/* TCP open → MQTT CONNECT, keep alive. No close. */
uint8_t ESP8266_OneNET_Connect(
    const char *broker, uint16_t port,
    const char *client_id, const char *username, const char *password)
{
    int clen = (int)strlen(client_id);
    int ulen = (int)strlen(username);
    int plen = (int)strlen(password);

    esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
    Delay_Ms(200);
    mqtt_persistent = 0;

    if(!esp_tcp_open(broker, port))
        return 0;

    uint8_t pkt[512];
    int pos = 0;
    int remaining = 10 + (2 + clen) + (2 + ulen) + (2 + plen);

    pkt[pos++] = 0x10;
    pos += mqtt_encode_remlen(&pkt[pos], remaining);

    pkt[pos++] = 0x00; pkt[pos++] = 0x04;
    pkt[pos++] = 'M'; pkt[pos++] = 'Q'; pkt[pos++] = 'T'; pkt[pos++] = 'T';
    pkt[pos++] = 0x04;
    pkt[pos++] = 0xC2;
    pkt[pos++] = 0x00; pkt[pos++] = 0x78;

    pkt[pos++] = (uint8_t)(clen >> 8);
    pkt[pos++] = (uint8_t)(clen);
    memcpy(&pkt[pos], client_id, clen); pos += clen;

    pkt[pos++] = (uint8_t)(ulen >> 8);
    pkt[pos++] = (uint8_t)(ulen);
    memcpy(&pkt[pos], username, ulen); pos += ulen;

    pkt[pos++] = (uint8_t)(plen >> 8);
    pkt[pos++] = (uint8_t)(plen);
    memcpy(&pkt[pos], password, plen); pos += plen;

    if(!esp_tcp_send(pkt, pos))
    {
        esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
        return 0;
    }

    uint32_t waited = 0;
    while(waited < 5000)
    {
        if(strstr(esp_rx_buf, "+IPD,4"))
        {
            mqtt_dump_connack();
            mqtt_persistent = 1;
            printf("OneNET: persistent MQTT online\r\n");
            return 1;
        }
        Delay_Ms(50);
        waited += 50;
    }
    printf("OneNET: no CONNACK\r\n");
    esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
    return 0;
}

/* Publish on persistent connection. */
uint8_t ESP8266_OneNET_Publish(const char *topic, const char *json)
{
    if(!mqtt_persistent)
        return 0;

    uint8_t pkt[600];
    int pos = 0;
    int tlen = (int)strlen(topic);
    int jlen = (int)strlen(json);
    int remaining = 2 + tlen + 2 + jlen;

    pkt[pos++] = 0x32;
    pos += mqtt_encode_remlen(&pkt[pos], remaining);

    pkt[pos++] = (uint8_t)(tlen >> 8);
    pkt[pos++] = (uint8_t)(tlen);
    memcpy(&pkt[pos], topic, tlen); pos += tlen;

    pkt[pos++] = 0x00;
    pkt[pos++] = 0x01;
    memcpy(&pkt[pos], json, jlen); pos += jlen;

    if(!esp_tcp_send(pkt, pos))
    {
        mqtt_persistent = 0;
        esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
        return 0;
    }
    return 1;
}

void ESP8266_OneNET_Close(void)
{
    if(mqtt_persistent)
    {
        esp_at_cmd("AT+CIPCLOSE", "OK", 1000);
        mqtt_persistent = 0;
    }
}

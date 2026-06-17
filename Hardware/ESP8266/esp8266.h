#ifndef __ESP8266_H
#define __ESP8266_H

#include "ch32v30x.h"

#define ESP8266_UART USART2

void ESP8266_Init(void);
void ESP8266_Reset(void);
uint8_t ESP8266_ATTest(void);
uint8_t ESP8266_ConnectWiFi(const char *ssid, const char *pwd);
uint8_t ESP8266_GetIP(char *ip_buf);

/* OneNET MQTT via ESP8266 */
uint8_t ESP8266_MQTT_Connect(const char *broker, uint16_t port,
                              const char *client_id,
                              const char *username,
                              const char *password);
uint8_t ESP8266_MQTT_Publish(const char *topic, const char *payload);

/* Persistent connection: TCP open → MQTT CONNECT (no close) */
uint8_t ESP8266_OneNET_Connect(
    const char *broker, uint16_t port,
    const char *client_id, const char *username, const char *password);

/* Publish on existing persistent TCP connection */
uint8_t ESP8266_OneNET_Publish(const char *topic, const char *json);

/* Close persistent connection */
void ESP8266_OneNET_Close(void);

/* Combined upload: TCP → CONNECT → PUBLISH → CLOSE (stateless) */
uint8_t ESP8266_OneNET_Upload(
    const char *broker, uint16_t port,
    const char *client_id, const char *username, const char *password,
    const char *topic, const char *json);

/* Binary variant: PUBLISH payload is raw bytes */
uint8_t ESP8266_OneNET_UploadBin(
    const char *broker, uint16_t port,
    const char *client_id, const char *username, const char *password,
    const char *topic, const uint8_t *data, int data_len);

#endif

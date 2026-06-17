/* config.example.h — 复制为 config.h 并填入真实值
   config.h 已在 .gitignore 中，不会被提交到 GitHub */

#ifndef __CONFIG_H
#define __CONFIG_H

/* WiFi */
#define WIFI_SSID       "你的WiFi名"
#define WIFI_PASS       "你的WiFi密码"

/* OneNET 多协议接入 MQTT (旧版) */
#define ONENET_BROKER   "183.230.40.39"
#define ONENET_PORT     6002
#define ONENET_CLIENT   "你的设备ID"
#define ONENET_USER     "你的产品ID"
#define ONENET_PASS     "你的鉴权信息(Auth_Code)"
#define ONENET_TOPIC    "$dp"

/* OneNET 新版平台 MQTT
#define ONENET_BROKER   "mqtts.heclouds.com"
#define ONENET_PORT     1883
#define ONENET_CLIENT   "你的设备名"
#define ONENET_USER     "你的产品ID"
#define ONENET_PASS     "鉴权Token(由设备密钥计算)"
#define ONENET_TOPIC    "$sys/产品ID/设备名/dp/post/json"
*/

#endif

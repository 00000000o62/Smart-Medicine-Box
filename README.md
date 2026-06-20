# 智能药箱 Smart Medicine Box

基于 **CH32V307VCT6 (RISC-V)** 的物联网智能药箱，集成环境监测、云端同步与微信小程序远程监控。

> GitHub: https://github.com/00000000o62/Smart-Medicine-Box

---

## 系统架构

```
┌─ 感知层 ─────────────────────────────────────────────┐
│  DHT11温湿度    SGP30空气质量   BH1750光照    微动开关  │
│  (单总线PA11)    (I2C1 PB6/7)   (软件I2C PA12/15)    │
└──────────────┬───────────────────────────────────────┘
               │
┌─ 控制层 ────┴───────────────────────────────────────┐
│  CH32V307VCT6  96MHz RISC-V  288K Flash  32K RAM    │
│  3.2" ILI9341 SPI LCD (240x320) 中文显示             │
│  XPT2046 触摸 · SU-03T语音(预留) · 7路药仓GPIO        │
└──────────────┬───────────────────────────────────────┘
               │
┌─ 云端层 ────┴──────┐    ┌─ 应用层 ──────────────────┐
│  ESP8266-01S WiFi   │    │  微信小程序                 │
│  TCP + 手工MQTT报文 │───→│  每5秒拉OneNET REST API    │
│  OneNET 物联网平台  │    │  卡片式传感器数据监控        │
└────────────────────┘    └───────────────────────────┘
```

---

## 硬件配置

### 主控与显示

| 模块 | 型号 | 接口 | 引脚 |
|---|---|---|---|
| 主控 | CH32V307VCT6 | — | — |
| 屏幕 | 3.2" ILI9341 SPI TFT | 硬件 SPI2 | PB9-BL PB10-DC PB11-CS PB12-RST PB13-SCK PB14-MISO PB15-MOSI |
| 触摸 | XPT2046 (电阻屏) | 软件 SPI | PC0-TCLK PC2-TDO PC3-TDIN PC10-TIRQ PC13-TCS |

### 传感器

| 模块 | 型号 | 接口 | 引脚 | 说明 |
|---|---|---|---|---|
| 温湿度 | DHT11 | 单线 GPIO | PA11 | NOP微秒延时 + 校验和 |
| 空气质量 | SGP30 | 硬件 I2C1 | PB6-SCL PB7-SDA | TVOC(ppb) + eCO₂(ppm) |
| 光照 | BH1750 (GY-302) | 软件 I2C | PA12-SCL PA15-SDA | 1~65535 lx |

### 通信与控制

| 模块 | 型号 | 接口 | 引脚 | 说明 |
|---|---|---|---|---|
| WiFi | ESP8266-01S | USART2 | PA2-TX PA3-RX | TCP+手工MQTT报文 |
| 语音 | SU-03T | USART3 | 待接线 | 离线语音识别 |
| 按键 | 微动开关 | GPIO 轮询 | PA0 | 3态消抖, 服药计数 |
| 药仓 | 7 路 GPIO | GPIO | PA1-PA7 | 输出控制 |
| 调试 | UART1 printf | USART1 | PA9-TX | 115200 8N1 |

### 完整引脚分配

```
PA0     微动开关          PA11    DHT11 DATA
PA1-7   药仓 1-7          PA12    BH1750 SCL (软件I2C)
PA9     USART1 TX (调试)   PA13    SWDIO
PA10    USART1 RX           PA14    SWCLK
PA15    BH1750 SDA        PB0-5   空闲
PB6     SGP30 SCL (I2C1)   PB7     SGP30 SDA (I2C1)
PB8     空闲               PB9     LCD 背光
PB10    LCD DC             PB11    LCD CS
PB12    LCD RST            PB13    LCD SCK (SPI2)
PB14    LCD MISO           PB15    LCD MOSI (SPI2)
PC0     触摸 T_CLK         PC2     触摸 T_DO
PC3     触摸 T_DIN         PC10    触摸 T_IRQ
PC13    触摸 T_CS
```

---

## 软件架构

```
medbox/
├── Hardware/              # 外设驱动层
│   ├── LCD/               # ILI9341驱动 + FONT.H (100+ GB2312中文字库)
│   ├── SPI/               # 硬件SPI2
│   ├── TOUCH/             # XPT2046触摸 (软件SPI)
│   ├── GUI/               # 绘图基元 (点线矩形圆三角形)
│   ├── DHT11/             # 单总线温湿度 (内联汇编NOP延时)
│   ├── SGP30/             # I2C空气质量 (TVOC+CO₂)
│   ├── BH1750/            # 软件I2C光照
│   └── ESP8266/           # WiFi + TCP/MPQTT直连
├── User/                  # 应用层
│   ├── main.c             # 主循环 (超级循环, 无RTOS)
│   ├── medbox_ui.c/h      # 中文LCD界面
│   ├── medbox_schedule.c/h # 服药时间表 + 按键处理
│   ├── config.h           # 敏感配置 (WiFi/OneNET凭据, gitignored)
│   └── config.example.h   # 配置模板
├── miniprogram/           # 微信小程序
│   ├── pages/index/       # 主页面 (卡片式布局)
│   └── utils/             # OneNET API调用
├── Peripheral/            # WCH SDK标准库
├── Startup/               # 启动汇编
└── Ld/                    # 链接脚本
```

---

## 主循环流程

```
初始化: LCD→时钟→按键→传感器→WiFi→MQTT持久连接
    ↓
┌─ while(1) ──────────────────────────────────┐
│  每1秒:   时钟更新                           │
│  每100ms: 按键轮询 (3态消抖)                 │
│  每2秒:   DHT11→SGP30→BH1750采集 + 刷新屏幕 │
│  每30秒:  WiFi检测重连 + OneNET MQTT上报     │
└─────────────────────────────────────────────┘
```

---

## 屏幕界面

```
┌─ 智能药箱 ────────── 14:30 ──┐
│ 2026-05-19  14:30:25          │
│───────────────────────────────│
│ 温湿度监测                     │
│ 温度: 25.5℃   湿度: 62%       │
│ Air: Good   VOC: 150 ppb     │
│ CO2: 450 ppm                  │
│ Light: 320 lx                 │
│ WiFi: 192.168.1.5             │
└───────────────────────────────┘
```

---

## 微信小程序

```
小程序 ←HTTPS← OneNET REST API ←(读库)← 设备MQTT上报
```

- 单页面卡片式布局，显示温度/湿度/TVOC/CO₂/光照/服药计数
- 每 5 秒自动刷新，空气质量自动分色 (Good🟢/Moderate🟡/Poor🟠)
- 下拉手动刷新，在线/离线状态指示

### 小程序运行

1. 微信开发者工具打开 `miniprogram/` 目录
2. 编辑 `utils/config.js` 填入 OneNET 产品ID/设备名/API Token
3. 配置 request 合法域名: `https://iot-api.heclouds.com`
4. 预览 → 真机扫码

---

## OneNET 云端

### MQTT 上报 (设备→云)

- Broker: `mqtts.heclouds.com:1883`
- 鉴权: Token (设备密钥 + HMAC-SHA1, `\n`分隔签名串)
- Topic: `$sys/{pid}/{device}/dp/post/json`
- 格式: `{"id":1,"dp":{"temperature":[{"v":29.5}],...}}`

### REST API (小程序→云)

- API: `https://iot-api.heclouds.com/datapoint/history-datapoints`
- 鉴权: Token (version=2022-05-01, `&`分隔签名串)

---

## 关键技术点

| 技术 | 方案 | 说明 |
|---|---|---|
| 浮点显示 | 整数运算 `%d.%d` | newlib-nano 不支持 `%f` |
| 中文显示 | GB2312 16×16 点阵 | Python Pillow + SimSun 生成 |
| DHT11 时序 | RISC-V 内联汇编 NOP | 96MHz, 48×NOP ≈ 1μs |
| I2C 复用 | 硬I2C(SGP30) + 软I2C(BH1750) | 仅一个I2C外设，软件模拟第二个 |
| MQTT | TCP + 手工构造二进制报文 | 绕过ESP8266 AT MQTT命令限制 |
| Token 鉴权 | Base64解码密钥 + HMAC-SHA1 | OneNET 官方算法 |
| 敏感信息 | config.h + .gitignore | 凭据不入Git仓库 |

---

## 构建与烧录

- **IDE**: MounRiver Studio
- **工具链**: riscv-none-embed-gcc (rv32imac + XW)
- **调试器**: WCH-LinkE, SWD (PA13/PA14)
- **固件大小**: ~45KB code, ~2.6KB RAM

```bash
# Clone
git clone https://github.com/00000000o62/Smart-Medicine-Box.git

# 配置凭据
cp User/config.example.h User/config.h
# → 编辑 config.h 填入 WiFi/OneNET 凭据

# MounRiver Studio: File → Import → Existing Project → medbox/
# Build → Flash
```

---

## License

MIT

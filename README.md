# 智能药箱 (Smart Medicine Box)

CH32V307VCT6 + ILI9341 智能药箱原型机。

## 硬件

| 模块 | 型号 | 接口 | 引脚 |
|---|---|---|---|
| 屏幕 | 3.2" ILI9341 SPI TFT | SPI2 | PB9-BL, PB10-DC, PB11-CS, PB12-RST, PB13-SCK, PB14-MISO, PB15-MOSI |
| 触摸 | XPT2046 (电阻屏) | 软件 SPI | PC0-TCLK, PC2-TDO, PC3-TDIN, PC10-TIRQ, PC13-TCS |
| 温湿度 | DHT11 | 单线 GPIO | PA11 |
| 空气质量 | SGP30 | 硬件 I2C1 | PB6-SCL, PB7-SDA |
| 光照 | BH1750 (GY-302) | 软件 I2C | PA12-SCL, PA15-SDA |
| WiFi | ESP8266-01S | USART2 | PA2-TX, PA3-RX |
| 语音 | SU-03T | USART3 | 待接线 |
| 按键 | 微动开关 | GPIO 轮询 | PA0 (内部上拉, 对 GND) |
| 药仓 | 7 路 GPIO 输出 | GPIO | PA1-PA7 |
| 调试 | USART1 printf | PA9-TX | 115200, 8N1 |

## 全引脚分配

```
PA0     微动开关 (GPIO 轮询)
PA1-6   药仓 1-6
PA7     药仓 7
PA8     空闲
PA9     USART1 TX (调试 printf)
PA10    USART1 RX (调试)
PA11    DHT11 DATA
PA12    BH1750 SCL (软件 I2C)
PA13    SWDIO (调试)
PA14    SWCLK (调试)
PA15    BH1750 SDA (软件 I2C)
PB0-5   空闲
PB6     SGP30 SCL (I2C1)
PB7     SGP30 SDA (I2C1)
PB8     空闲
PB9     LCD 背光
PB10    LCD DC/RS
PB11    LCD CS
PB12    LCD RST
PB13    LCD SCK (SPI2)
PB14    LCD MISO (SPI2)
PB15    LCD MOSI (SPI2)
PC0     触摸 T_CLK (软件 SPI)
PC1     空闲
PC2     触摸 T_DO (软件 SPI)
PC3     触摸 T_DIN (软件 SPI)
PC4-6   空闲
PC7     空闲
PC8     空闲
PC9     空闲
PC10    触摸 T_IRQ
PC11    空闲
PC12    空闲
PC13    触摸 T_CS (软件 SPI)
```

## 软件结构

```
medbox/
├── Hardware/
│   ├── BH1750/       # 光照传感器 (软件 I2C, PA12/PA15)
│   ├── DHT11/        # 温湿度传感器 (单线 GPIO, PA11)
│   ├── ESP8266/      # WiFi 模块 (USART2 AT 指令, PA2/PA3)
│   ├── GUI/          # 绘图基元 (线/矩形/圆/三角形/文字)
│   ├── LCD/          # ILI9341 驱动 + FONT.H 中英文字库
│   ├── SGP30/        # 空气质量传感器 (硬件 I2C1, PB6/PB7)
│   ├── SPI/          # SPI2 硬件驱动
│   └── TOUCH/        # XPT2046 触摸驱动 (软件 SPI)
├── Network/          # OneNET MQTT 上报 (预留)
├── User/
│   ├── main.c        # 主程序
│   ├── medbox_ui.c/h # 界面 (中文单屏, 传感器数据)
│   └── medbox_schedule.c/h  # 服药时间表 + 按键 + 时钟
└── Peripheral/       # WCH 标准库
```

## 屏幕界面

```
┌─ 智能药箱 ────────── 14:30 ──┐
│ 2026-05-19  14:30:25          │
│───────────────────────────────│
│ 温湿度监测                     │
│ 温度: 25.5 C   湿度: 62 %     │
│ Air: Good  VOC: 150 ppb      │
│ CO2: 450 ppm                  │
│ Light: 320 lx                 │
│ WiFi: OFF                     │
└───────────────────────────────┘
```

- 中文标题 "智能药箱"
- 每 2 秒刷新传感器数据
- 每秒刷新时钟
- 无触摸交互（触摸驱动保留未启用）

## 中文字库

FONT.H 包含约 100 个 16×16 中文字符 (GB2312 编码)：
原始 64 字 + 新增 36 字 (Pillow+SimSun 生成)。

新增常用字：智能药箱温湿度空气质量环境监测服药提醒时间开关已取确认关闭今日次数网络连接状态

## 按键功能

PA0 微动开关，GPIO 轮询 + 3 态消抖 (300ms)，每按一次 `pill_count++`，串口打印计数值。

## WiFi / OneNET

- ESP8266 通过 USART2 发 AT 指令连 WiFi
- MQTT 上报 OneNET Studio (`$sys/4QaO1ZOHK2/medbox_001/thing/property/post`)
- 物模型属性：pill_count, temperature, humidity, today_taken, today_missed, next_dose, next_medicine
- 当前 WiFi 凭据硬编码在 main.c，需按实际环境修改 SSID/PASS

## 语音模块 (待集成)

SU-03T，SU-03T-dev skill 已就绪，资料在 `D:/edge-download/SU-03T开发包；版本V2.1.2/`

## 构建

- IDE: MounRiver Studio
- 工具链: riscv-none-embed-gcc (rv32imac + XW)
- RAM: 32KB, Flash: 288KB
- 当前固件: ~45KB code + 2.6KB RAM
- 下载: WCH-LinkE, SWD (PA13/PA14)

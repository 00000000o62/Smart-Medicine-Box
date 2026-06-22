# 设备 → OneNET → 小程序 全链路实现指南

> 通用模板，将 `{...}` 替换为你的实际值即可。

---

## 一、总体数据流

```
┌─ 终端设备 ──────────────────────────────────────────────────┐
│  MCU ─UART── WiFi模块 ──WiFi── 路由器 ──Internet──┐        │
└─────────────────────────────────────────────────────┘        │
                                                          ↓
┌─ OneNET 云平台 ───────────────────────────────────────────────┤
│                                                               │
│  ① MQTT Broker   接收设备上报                                │
│         ↓                                                     │
│  ② 数据存储引擎  持久化所有数据点 (datapoints)                │
│         ↓                                                     │
│  ③ REST API      对外提供查询接口                             │
└──────────────────────────────────────────────────────────┘     │
                                                          ↓
┌─ 微信小程序 ──────────────────────────────────────────────────┐
│  wx.request() → GET API → 解析JSON → 渲染UI                   │
└───────────────────────────────────────────────────────────────┘
```

---

## 二、第一阶段：设备 → OneNET（设备上云）

### 2.1 平台准备

| 步骤 | 操作 | 获取的关键信息 |
|---|---|---|
| 创建产品 | OneNET 控制台 → 多协议接入 → MQTT → 数据流格式 | 产品ID: `{PRODUCT_ID}` |
| 创建设备 | 设备名自定义，如 `dev1` | 设备密钥: `{DEVICE_KEY}` |
| 获取 API 密钥 | 产品概况 → 查看 access_key (需短信验证) | access_key: `{ACCESS_KEY}` |

### 2.2 MQTT 鉴权密钥生成

**算法流程：**

```
输入:  设备密钥 (Base64) + 产品ID + 设备名 + 过期时间戳

Step 1: Base64 解码设备密钥 → 32字节二进制密钥
Step 2: 构造签名串 (用 \n 换行分隔，顺序固定)
        "{过期时间戳}\n{签名方法}\nproducts/{产品ID}/devices/{设备名}\n2018-10-31"
Step 3: HMAC-SHA1(解码后的密钥, 签名串) → 20字节签名
Step 4: Base64 编码签名 → URL 编码

输出: version=2018-10-31&res=products%2F{PRODUCT_ID}%2Fdevices%2F{DEVICE_NAME}
      &et={EXPIRY_TIMESTAMP}&method=sha1&sign={URL_ENCODED_SIGNATURE}
```

**Python 实现：**

```python
import hmac, hashlib, base64, urllib.parse, time

def mqtt_token(device_key, product_id, device_name):
    key = base64.b64decode(device_key)
    et = str(int(time.time()) + 365 * 24 * 3600)
    res = f"products/{product_id}/devices/{device_name}"
    sign_str = f"{et}\nsha1\n{res}\n2018-10-31"
    sign = base64.b64encode(
        hmac.new(key, sign_str.encode(), hashlib.sha1).digest()
    ).decode()
    return (f"version=2018-10-31"
            f"&res={urllib.parse.quote(res, safe='')}"
            f"&et={et}&method=sha1"
            f"&sign={urllib.parse.quote(sign, safe='')}")
```

### 2.3 TCP 直连 + 手工 MQTT 报文

**连接流程：**

```
Step 1: AT+CWJAP → 连接 WiFi
Step 2: AT+CIPSTART="TCP","mqtts.heclouds.com",1883 → TCP 连接
Step 3: 手工构造 MQTT CONNECT 报文 → AT+CIPSEND 发送
Step 4: 等待 CONNACK (0x20 0x02 0x00 0x00 = 通过)
Step 5: 手工构造 MQTT PUBLISH 报文 → AT+CIPSEND 发送数据
```

**MQTT CONNECT 报文结构：**

```
 0x10          固定头: CONNECT
 0xNN 0xNN     剩余长度 (varint)
 00 04         协议名长度
 M Q T T       协议名: "MQTT"
 04            协议等级: v3.1.1
 C2            连接标志: 用户名+密码+清除会话
 00 78         Keep Alive: 120秒
 00 XX         Client ID 长度
 ...           Client ID: {DEVICE_NAME}
 00 XX         Username 长度
 ...           Username: {PRODUCT_ID}
 00 XX         Password 长度
 ...           Password: Token 签名串
```

**MQTT PUBLISH 报文结构：**

```
 0x32          固定头: PUBLISH, QoS=1
 0xNN 0xNN     剩余长度 (varint)
 00 XX         Topic 长度
 $sys/...      Topic: $sys/{PRODUCT_ID}/{DEVICE_NAME}/dp/post/json
 00 01         Packet ID
 {...}         Payload: JSON 数据
```

**上报 JSON 格式：**

```json
{
  "id": 1,
  "dp": {
    "temperature": [{"v": 29.5}],
    "humidity":    [{"v": 62.0}],
    "tvoc":        [{"v": 150}],
    "co2":         [{"v": 450}],
    "lux":         [{"v": 320}]
  }
}
```

### 2.4 上报周期建议

```
主循环 100ms/tick
    │
    ├─ 每 1 秒:  UI 刷新
    ├─ 每 2 秒:  传感器采集
    ├─ 每 30 秒: MQTT Publish 上报
    └─ 每 30 秒: WiFi 断线检测 + 自动重连
```

---

## 三、第二阶段：OneNET 数据存储

### 3.1 数据接收

MQTT Broker 收到 PUBLISH 后：
1. 解析 Topic → 识别设备和数据流
2. 解析 Payload JSON → 提取 `dp` 对象
3. 写入时序数据库

### 3.2 数据流自动创建

首次上报时 OneNET 自动创建数据流，无需预先定义。

### 3.3 数据点格式

```json
{
  "at": "2026-06-21 18:22:23.748",
  "at_timestamp": 1782037343748,
  "value": 29.7
}
```

---

## 四、第三阶段：OneNET → 小程序

### 4.1 API 鉴权

API Token 与 MQTT Token 算法相同但参数不同：

| 参数 | MQTT Token | API Token |
|---|---|---|
| version | 2018-10-31 | 2018-10-31 |
| res | products/{pid}/devices/{dev} | **products/{pid}** |
| 密钥 | 设备密钥 | **产品 access_key** (短信验证获取) |

**Python 实现：**

```python
def api_token(access_key, product_id):
    """access_key: 产品概况页短信验证后获取的 Base64 密钥"""
    key = base64.b64decode(access_key)
    et = str(int(time.time()) + 365 * 24 * 3600)
    res = f"products/{product_id}"
    sign_str = f"{et}\nsha1\n{res}\n2018-10-31"
    sign = base64.b64encode(
        hmac.new(key, sign_str.encode(), hashlib.sha1).digest()
    ).decode()
    return (f"version=2018-10-31"
            f"&res={urllib.parse.quote(res, safe='')}"
            f"&et={et}&method=sha1"
            f"&sign={urllib.parse.quote(sign, safe='')}")
```

### 4.2 API 请求格式

```
GET https://iot-api.heclouds.com/datapoint/history-datapoints
    ?product_id={PRODUCT_ID}
    &device_name={DEVICE_NAME}
    &datastream_id=temperature
    &limit=1

Header:
    Authorization: {API_TOKEN}
```

### 4.3 小程序轮询机制

```
小程序启动
    │
    ├─ onLoad → fetchData()          首次拉取
    ├─ setInterval(fetchData, 5000)  每5秒轮询
    │      │
    │      ├─ wx.request → GET API × N 路数据流 (并行)
    │      ├─ 解析: datastreams[0].datapoints[0].value
    │      ├─ 离线判断: Date.now() - lastTimestamp > 60s
    │      └─ setData → 渲染 UI
    │
    └─ onUnload → clearInterval
```

### 4.4 API 返回结构与解析

```javascript
// API 返回
{
  "code": 0,
  "data": {
    "datastreams": [{
      "id": "temperature",
      "datapoints": [{
        "value": 29.7,                    // ← 数值
        "at_timestamp": 1782037343748     // ← 毫秒时间戳 (在线判断)
      }]
    }]
  }
}

// 解析
const val = res.data.data.datastreams[0].datapoints[0].value;
const ts  = res.data.data.datastreams[0].datapoints[0].at_timestamp;
```

---

## 五、在线状态判定

```
设备正常: 每30秒上报 → 时间戳 < 60秒前 → 🟢 在线
设备离线: 停止上报     → 时间戳 > 60秒前 → 🔴 离线
```

---

## 六、参数速查表

| 参数 | 获取位置 | 示例占位 |
|---|---|---|
| 产品ID | 产品概况 | `{PRODUCT_ID}` |
| 设备名 | 设备列表 | `{DEVICE_NAME}` |
| 设备密钥 | 设备详情 (用于MQTT) | `{DEVICE_KEY_Base64}` |
| access_key | 产品概况→短信验证 (用于API) | `{ACCESS_KEY_Base64}` |
| MQTT Broker | 固定 | `mqtts.heclouds.com:1883` |
| MQTT Topic | 固定格式 | `$sys/{PID}/{DEV}/dp/post/json` |
| API 端点 | 固定 | `iot-api.heclouds.com` |
| 上报周期 | 自定义 | 30秒 |
| 轮询周期 | 自定义 | 5秒 |
| 离线阈值 | 自定义 | 60秒 |

---

## 七、常见问题

| 问题 | 原因 | 解决 |
|---|---|---|
| CONNACK 0x04 (Bad username/password) | Token 签名算法错误 | 检查签名串是否用 `\n` 分隔，密钥是否正确 Base64 解码 |
| API 返回 10403 认证失败 | access_key 错误或 token 参数不对 | 确认用产品 access_key (非设备密钥)，res 为 `products/{PID}` |
| 小程序超时 timeout | 域名未加入白名单 | 开发者工具勾选"不校验域名"，正式上线配置 request 域名 |
| 设备始终离线 | 上报已停止或 TCP 连接断开 | 检查 WiFi 重连和 MQTT 持久连接逻辑 |
| 小程序显示旧数据 | 历史数据仍在数据库中 | 用 `at_timestamp` 字段判断数据新鲜度 |

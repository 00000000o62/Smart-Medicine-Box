// config.example.js — 复制为 config.js 并填入真实值
// config.js 已在 .gitignore 中，不会被提交到 GitHub

module.exports = {
  productId:  '你的产品ID',       // OneNET 产品ID
  deviceName: '你的设备名',       // OneNET 设备名
  // API Token: 使用官方工具生成，details:
  // https://open.iot.10086.cn/doc/mqtt/book/manual/auth/token.html
  apiToken:   '你的API_Token',
  apiBase:    'https://iot-api.heclouds.com',
  refreshMs:  5000
};

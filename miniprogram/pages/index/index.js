// 智能药箱 - 传感器数据监控
// 通过 OneNET REST API 拉取设备数据流

// 从外部配置文件加载敏感参数（config.js 已 gitignore）
const CONFIG = require('../../utils/config.js');

// 数据流名称
const STREAMS = ['temperature', 'humidity', 'tvoc', 'co2', 'lux', 'pill_count'];

Page({
  data: {
    temperature: '--',
    humidity: '--',
    tvoc: '--',
    co2: '--',
    lux: '--',
    pill_count: '--',
    airQuality: '--',
    airColor: '#999',
    updateTime: '',
    online: false,
    loading: true
  },

  timer: null,

  onLoad() {
    this.fetchData();
    this.timer = setInterval(() => this.fetchData(), CONFIG.refreshMs);
  },

  onUnload() {
    if (this.timer) clearInterval(this.timer);
  },

  onPullDownRefresh() {
    this.fetchData().then(() => wx.stopPullDownRefresh());
  },

  async fetchData() {
    try {
      // 并行查所有数据流
      const results = await Promise.all(
        STREAMS.map(stream => this.fetchStream(stream))
      );

      const data = {};
      let allOk = true;
      for (let i = 0; i < STREAMS.length; i++) {
        const val = results[i];
        if (val !== null) {
          data[STREAMS[i]] = this.formatValue(STREAMS[i], val);
        } else {
          allOk = false;
        }
      }

      // 空气质量评价
      const tvocVal = results[2]; // tvoc index
      let airQuality = '--';
      let airColor = '#999';
      if (tvocVal !== null) {
        if (tvocVal < 100) { airQuality = 'Air: Good'; airColor = '#4CAF50'; }
        else if (tvocVal < 300) { airQuality = 'Air: Moderate'; airColor = '#FF9800'; }
        else if (tvocVal < 500) { airQuality = 'Air: Poor'; airColor = '#F44336'; }
        else { airQuality = 'Air: Bad!'; airColor = '#D32F2F'; }
      }

      const now = new Date();
      this.setData({
        ...data,
        airQuality,
        airColor,
        online: allOk,
        loading: false,
        updateTime: `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`
      });
    } catch (e) {
      console.error('Fetch error:', e);
      this.setData({ loading: false, online: false });
    }
  },

  async fetchStream(streamId) {
    return new Promise((resolve) => {
      wx.request({
        url: `${CONFIG.apiBase}/datapoint/history-datapoints`,
        data: {
          product_id: CONFIG.productId,
          device_name: CONFIG.deviceName,
          datastream_id: streamId,
          limit: 1
        },
        header: {
          'Authorization': CONFIG.apiToken
        },
        timeout: 5000,
        success(res) {
          if (res.statusCode === 200 && res.data && res.data.code === 0) {
            const list = res.data.data?.list || res.data.data?.datapoints;
            if (list && list.length > 0) {
              const dp = list[0];
              const val = dp.value !== undefined ? dp.value : dp.datapoints?.[0]?.value;
              resolve(val !== undefined ? val : null);
              return;
            }
          }
          resolve(null);
        },
        fail() { resolve(null); }
      });
    });
  },

  formatValue(stream, val) {
    if (val === null || val === undefined) return '--';
    switch (stream) {
      case 'temperature': return Number(val).toFixed(1) + '℃';
      case 'humidity': return Math.round(Number(val)) + '%';
      case 'tvoc': return Math.round(Number(val)) + ' ppb';
      case 'co2': return Math.round(Number(val)) + ' ppm';
      case 'lux': return Math.round(Number(val)) + ' lx';
      case 'pill_count': return String(Math.round(Number(val))) + ' 次';
      default: return String(val);
    }
  }
});

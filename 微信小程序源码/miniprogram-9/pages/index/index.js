/**
 * @简要：微信小程序页面定义
 * @注意：此页面用于与OneNET物联网平台交互，获取设备数据和状态，并控制设备属性。
 */
Page({

  /**
   * @简要：页面数据初始化
   * @注意：定义页面的初始数据结构，包括设备属性值、设备状态、开关状态等。
   */
  data: {
    onenet_data: [], // 用来存储设备属性值的数组
    device_status: [], // 用来存储设备状态信息的数组
    switch1_checked: false, // 加湿器开关状态
    switch2_checked: false, // 窗帘开关状态
    switch3_checked: false, // 灯光亮度

  },

  /**
   * @简要：配置信息
   * @注意：定义与OneNET平台交互所需的配置信息，包括鉴权信息、产品ID、设备名称等。
   */
  config: {
    auth_info: "version=2022-05-01&res=userid%2F436368&et=1904559032&method=sha1&sign=hSjVWGygYBqHHCALFAfQiVW5Oy4%3D", // 鉴权信息
    product_id: "1bUie6VQTe", // 产品ID
    device_name: "d1", // 设备名称
    api_base_url: "https://iot-api.heclouds.com", // OneNET API基础URL
    start_time: 0, // 开始时间，用于请求数据时间戳区间
    end_time: 0, // 结束时间，用于请求数据时间戳区间
    limit: 1 // 获取最近的一个数据
  },

  /**
   * @简要：页面加载时执行
   * @参数：options - 页面加载时传递的参数
   * @注意：在页面加载时，初始化时间戳，获取设备数据和状态，并设置定时器定期更新。
   * @返回值：无
   */
  onLoad(options) {
    const { start_time, end_time } = this.get_timestamps(); // 获取时间戳
    this.config.start_time = start_time;
    this.config.end_time = end_time;
    this.onenet_fetch_data(); // 先获取一次设备数据，更新页面
    this.onenet_fetch_device_status(); // 获取设备在线情况
    setInterval(() => {
      const { start_time, end_time } = this.get_timestamps();
      this.config.start_time = start_time;
      this.config.end_time = end_time;
      this.onenet_fetch_device_status(); // 定期获取设备状态
      this.onenet_fetch_data(); // 定期获取设备数据
    }, 3000); // 推荐每3000毫秒更新一次，根据实际数据刷新情况调整
  },

  /**
   * @简要：获取时间戳
   * @参数：无
   * @注意：计算当前时间和一周前的时间戳，用于请求数据的时间区间。
   * @返回值：对象 { start_time, end_time }
   */
  get_timestamps() {
    const now = new Date().getTime(); // 当前时间的时间戳
    const one_week_ago = now - 7 * 24 * 60 * 60 * 1000; // 一周前的时间戳
    return {
      start_time: one_week_ago,
      end_time: now
    };
  },

  /**
   * @简要：获取设备属性状态
   * @参数：无
   * @注意：向OneNET平台发送请求，获取设备的属性状态，并更新页面数据。
   * @返回值：无
   */
  onenet_fetch_data() {
    const { api_base_url, product_id, device_name, auth_info } = this.config;
    wx.request({
      url: `${api_base_url}/thingmodel/query-device-property?product_id=${product_id}&device_name=${device_name}`,
      method: "GET",
      header: {
        'Authorization': auth_info
      },
      success: (res) => {
        console.log("OneNET数据请求成功，返回数据：", res.data);
        if (res.data.code === 0) {
          this.setData({
            onenet_data: res.data, // 获取的数据放入onenet_data中
            switch1_checked: res.data.data[3].value === 'true' ? true : false, // 布尔类型数据需要这样赋值
            switch2_checked: res.data.data[1].value === 'true' ? true : true,
            switch3_checked: res.data.data[6].value === 'true' ? true : false,
          });
        } else {
          // 请求成功但code不为0，视为错误，显示错误信息
          console.log("OneNET请求错误，错误信息：", res.data.msg);
          wx.showToast({
            title: res.data.msg || '请求出错', // 使用返回的错误信息，如果没有则显示默认信息
            icon: 'none', // 不显示图标
            duration: 2000 // 提示框自动隐藏的时间，单位是毫秒
          });
        }
      },
      fail: (err) => {
        // 请求失败，则在屏幕上显示错误信息
        console.log("OneNET数据请求失败");
        console.error(err); // 打印错误堆栈信息
        // 显示错误提示框
        wx.showToast({
          title: '请求失败', // 使用默认错误信息
          icon: 'none', // 不显示图标
          duration: 2000 // 提示框自动隐藏的时间，单位是毫秒
        });
      }
    });
  },

  /**
   * @简要：获取设备状态信息
   * @参数：无
   * @注意：向OneNET平台发送请求，获取设备的在线状态，并更新页面数据。
   * @返回值：无
   */
  onenet_fetch_device_status() {
    const { api_base_url, product_id, device_name, auth_info, start_time, end_time, limit } = this.config;
    wx.request({
      url: `${api_base_url}/device/status-history?product_id=${product_id}&device_name=${device_name}&start_time=${start_time}&end_time=${end_time}&limit=${limit}`,
      method: "GET",
      header: {
        'Authorization': auth_info
      },
      success: (res) => {
        this.setData({
          device_status: res.data // 更新设备状态数据
        });
      },
      fail: (err) => {
        console.log("设备状态信息请求失败");
        console.error(err); // 处理请求失败的情况
      }
    });
  },

  /**
   * @简要：设置设备属性
   * @参数：event - 触发事件的对象，包含自定义数据和开关状态
   * @注意：向OneNET平台发送请求，设置设备的属性值，并根据返回结果提示用户。
   * @返回值：无
   */
// 修改后的 onenet_set_device_property 方法
onenet_set_device_property(event) {
  const param_name = event.currentTarget.dataset.param; // 获取自定义数据
  const is_checked = event.detail.value; // 获取开关状态
  const { api_base_url, product_id, device_name, auth_info } = this.config;
  // 显示加载提示框
  wx.showLoading({
    title: '正在执行...', // 提示文字
    mask: true, // 是否显示透明蒙层，防止触摸穿透
  });
  wx.request({
    url: `${api_base_url}/thingmodel/set-device-property`,
    method: 'POST',
    header: {
      'Authorization': auth_info,
    },
    data: {
      "product_id": product_id,
      "device_name": device_name,
      "params": {
        [param_name]: is_checked
      }
    },
    success: (res) => {
      console.log('OneNET属性设置请求成功，返回数据', res.data); // 打印接收到的数据

      // 隐藏加载提示框
      wx.hideLoading();

      // 检查响应是否成功
      if (res.data && res.data.code === 0 && res.data.data && res.data.data.code === 200) {
        // 显示成功提示框
        wx.showToast({
          title: '操作成功', // 提示的文字内容
          icon: 'success', // 图标类型，使用成功图标
          duration: 1500 // 提示框自动隐藏的时间，单位是毫秒
        });
      } else {
        // 显示失败提示框
        wx.showToast({
          title: res.data.msg || '操作失败', // 提示的文字内容，使用服务器返回的msg信息
          icon: 'none', // 不显示图标
          duration: 1500 // 提示框自动隐藏的时间，单位是毫秒
        });
      }
    },
    fail: (err) => {
      console.log('OneNET属性设置请求失败，返回数据：', err); // 打印错误信息
      // 隐藏加载提示框
      wx.hideLoading();
      // 显示失败提示框
      wx.showToast({
        title: '操作失败', // 提示的文字内容
        icon: 'none', // 不显示图标
        duration: 1500 // 提示框自动隐藏的时间，单位是毫秒
      });
    }
  });
},
  onenet_set_enum_property(event) {
    const param_name = event.currentTarget.dataset.param;
    const value = event.currentTarget.dataset.value;
    const { api_base_url, product_id, device_name, auth_info } = this.config;
    
    wx.showLoading({ title: '执行中...', mask: true });
    
    wx.request({
      url: `${api_base_url}/thingmodel/set-device-property`,
      method: 'POST',
      header: { 'Authorization': auth_info },
      data: {
        "product_id": product_id,
        "device_name": device_name,
        "params": { [param_name]: value }
      },
      success: (res) => {
        wx.hideLoading();
        if (res.data?.code === 0) {
          wx.showToast({ title: '操作成功', icon: 'success' });
        } else {
          wx.showToast({ title: res.data?.msg || '操作失败', icon: 'none' });
        }
      },
      fail: (err) => {
        wx.hideLoading();
        wx.showToast({ title: '请求失败', icon: 'none' });
        console.error('枚举控制失败:', err);
      }
    });
  },
  
});

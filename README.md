# WeatherStation-epaper-remote-SHT40-WF42
基于ESP8266、WF42电子纸屏、SHT40温湿度传感器的远程气象站项目（代码来源：半糖duck），支持获取空气质量、实时/未来天气数据，并通过WF42电子纸屏展示，配套Python服务端动态生成天气JSON，支持WiFi配置及多语言切换。
![Uploading image.png…]()

> 注意：本项目仅适配WF42型号电子纸屏，其他型号电子纸屏需自行调整驱动引脚及时序逻辑。

## 项目特性
- **硬件适配**：仅兼容WF42电子纸屏，适配ESP8266系列主控、SHT40温湿度传感器；
- **天气数据获取**：通过配套Python服务端解析和风天气JSON数据，获取实时/7天预报（温度、湿度、风向、降水、空气质量等）；
- **WiFi管理**：集成WiFiManager库（v0.12），支持AP模式配网，自动连接已配置WiFi，支持静态IP配置；
- **多语言支持**：内置中文/英文双语切换，适配天气展示、配置页面文本；
- **轻量级JSON解析**：使用JsonStreamingParser库，低内存占用解析大JSON数据；
- **时间同步**：通过网络时间服务器同步本地时间，支持UTC偏移配置；
- **服务端增强**：配套server.py实现动态天气JSON生成，支持地名/经纬度解析、JWT认证降级、离线Mock回退、农历计算等。

## 硬件依赖
- 主控：ESP8266开发板（如NodeMCU）；
- 传感器：SHT40温湿度传感器；
- 显示：WF42电子纸屏（仅适配该型号）；
- 其他：WiFi天线、电源模块。

## 软件依赖
### 核心库
| 库名 | 版本 | 用途 | 仓库地址 |
|------|------|------|----------|
| WiFiManager | 0.12 | ESP8266 WiFi配网/连接管理 | [tzapu/WiFiManager](https://github.com/tzapu/WiFiManager.git) |
| JsonStreamingParser | 1.0.4 | 轻量级JSON流解析 | [squix78/json-streaming-parser](https://github.com/squix78/json-streaming-parser.git) |

### ESP8266官方库
- ESP8266WiFi
- ESP8266WiFiMulti
- ESP8266HTTPClient
- WiFiClientSecureBearSSL

### 服务端依赖（Python）
```bash
pip install requests pyjwt lunarcalendar
```

## 核心文件说明
| 文件/目录 | 功能 |
|-----------|------|
| `heweather.h/.cpp` | 和风天气客户端，解析天气JSON数据，封装天气/空气质量属性 |
| `lang.h` | 多语言配置（中文/英文），定义界面文本常量 |
| `TimeClient.h/.cpp` | 网络时间同步，解析服务器时间并格式化输出 |
| `EPD_drive_gpio.h/.cpp` | WF42电子纸屏GPIO驱动，SPI通信实现（仅适配WF42） |
| `server.py` | 动态天气服务端（核心），生成设备可解析的标准化天气JSON |
| `library/` | 第三方依赖库（WiFiManager、JsonStreamingParser） |

## 快速开始
### 1. 环境配置
#### 设备端（ESP8266）
- 安装Arduino IDE/PlatformIO，配置ESP8266开发环境；
- 安装依赖库：通过Arduino库管理器安装WiFiManager（v0.12）、JsonStreamingParser（v1.0.4），或手动克隆仓库到`library/`目录。

#### 服务端（Python）
- 安装Python 3.7+，执行`pip install requests pyjwt lunarcalendar`安装依赖；
- 修改`server.py`配置区，替换高德API Key、和风天气JWT私钥/KID/SUB（若无则保留默认，服务会自动降级）。

### 2. 代码配置
#### 设备端
- 修改`lang.h`：设置`LANG`为1（中文）或2（英文）；
- 配置服务端地址：在`heweather.h`中修改`server`为部署后的`server.py`地址；
- WF42屏配置：在`EPD_drive_gpio.h`中确认GPIO引脚定义（适配WF42硬件接线，不可直接复用其他屏引脚）；
- 时间同步：在`TimeClient.h`中配置UTC偏移量（如东八区设为8.0）。

#### 服务端
```python
# 替换server.py中配置区为自有密钥
GAODE_KEY = "你的高德地理编码API Key"
PRIVATE_KEY = """-----BEGIN PRIVATE KEY-----
你的和风天气EdDSA私钥
-----END PRIVATE KEY-----"""
KID = "你的和风天气KID"
SUB = "你的和风天气SUB"
```

### 3. 启动与运行
#### 步骤1：启动服务端
```bash
# 基础启动（默认端口8000，监听所有网卡）
python server.py

# 指定端口启动
python server.py --port 8080
```

#### 步骤2：设备端上传与配网
- 将ESP8266代码上传至开发板；
- 首次上电后，ESP8266启动AP模式（SSID：ESP+芯片ID）；
- 手机连接该AP，访问任意网页进入配网页面，输入WiFi名称、密码、城市（拼音/经纬度）；
- 配置完成后，设备自动连接WiFi，向`server.py`请求天气数据并在WF42屏展示。

## 核心功能详解
### 1. 设备端核心逻辑
#### WiFi配置与连接（WiFiManager v0.12）
```cpp
// 自动连接已保存WiFi，失败则启动AP配网（仅适配ESP8266）
WiFiManager wifiManager;
wifiManager.autoConnect("WeatherStation-AP"); // AP名称自定义
```

#### WF42电子纸屏驱动（仅适配WF42）
```cpp
// 向WF42屏写入字节数据（SPI通信）
void driver_delay_xms(unsigned long xms) {
    delay(xms); // WF42屏专用延时逻辑
}
SPI_Write(value); // 适配WF42的SPI时序
```

#### 天气数据解析
```cpp
// 对接server.py获取标准化天气数据
heweatherclient client("http://你的服务端IP:8000/weather.php", "ch");
client.update(); // 拉取并解析JSON
String nowTemp = client.now_tmp; // 获取实时温度
String todayMax = client.today_tmp_max; // 获取今日最高温
```

### 2. 服务端核心逻辑（server.py）
#### 地名/经纬度解析
支持`lut1`参数传入地名（如"北京"）或经纬度（如"116.31,39.95"），自动通过高德API转换：
```python
def geocode_place_to_coord(place_name):
    url = "https://restapi.amap.com/v3/geocode/geo"
    params = {"address": place_name, "key": GAODE_KEY}
    r = requests.get(url, params=params, timeout=8)
    loc = d['geocodes'][0].get('location')  # 转换为"lon,lat"格式
```

#### JWT认证降级
尝试生成EdDSA算法JWT令牌，失败则跳过认证，避免服务不可用：
```python
def try_generate_jwt(private_key, kid, sub):
    try:
        payload = {'iat': int(time.time()) - 30, 'exp': int(time.time()) + 3000, 'sub': sub}
        token = jwt.encode(payload, private_key, algorithm='EdDSA', headers={'kid': kid})
        return token
    except Exception as e:
        logv(f"JWT生成失败，降级为无认证模式：{e}")
        return None
```

#### 离线Mock回退
第三方API请求失败时，返回本地预定义的WF42屏可解析的Mock数据：
```python
MOCK_RESPONSE = {
  "now": {"cond":"晴","tmp":"10","hum":"50"},
  "daily": {"tmin":"0,1,2","tmax":"10,11,12"},
  "message":"离线模式：使用本地MOCK数据"
}
```

#### 农历计算（适配WF42屏展示）
```python
def get_lunar_date():
    # 计算当前日期对应的农历（格式：乙巳蛇年九月初五）
    solar = Solar(today.year, today.month, today.day)
    lunar = Converter.Solar2Lunar(solar)
    # 拼接农历字符串，适配WF42屏显示格式
    return f"{gz_year}{shengxiao}年{month_str}{day_str}"
```

### 3. 接口说明（设备←→服务端）
#### 请求地址
`POST http://<服务端IP>:<端口>/weather.php`

#### 请求参数（表单/JSON）
| 参数名 | 必选 | 类型 | 说明 |
|--------|------|------|------|
| lut1 | 是 | String | 地名（如"北京"）或经纬度（如"116.31,39.95"），用于定位天气数据 |
| lut4 | 否 | String | 设备自定义参数，仅用于日志记录 |
| bssid | 否 | String | 设备WiFi的BSSID，仅用于日志记录 |
| ssid | 否 | String | 设备连接的WiFi名称，仅用于日志记录 |

#### 测试示例
```bash
# curl测试（表单格式）
curl -X POST -d "lut1=北京&lut4=12345&bssid=AA:BB:CC:DD:EE:FF&ssid=MyWiFi" http://127.0.0.1:8000/weather.php
```

## 注意事项
1. **硬件适配**：本项目仅支持WF42电子纸屏，其他型号（如EPD 2.13/4.2）需重新编写`EPD_drive_gpio.h/.cpp`驱动逻辑；
2. **密钥配置**：高德API Key需申请「地理编码/逆地理编码」权限，否则地名转坐标功能失效；
3. **服务端部署**：需确保服务端能访问外网（对接高德/和风API），内网部署需配置网络代理；
4. **日志管理**：服务端运行时会生成`request_log.txt`，需定期清理避免占用过多磁盘空间；
5. **稳定性优化**：建议服务端配合`nohup`实现后台常驻：
   ```bash
   nohup python server.py --port 8000 > weather_server.log 2>&1 &
   ```

## 许可证
- 项目核心代码（设备端+服务端）：代码来源为半糖duck，未指定开源许可证；
- 第三方依赖库均采用MIT许可证：
  - WiFiManager (v0.12): [MIT License](library/WiFiManager/LICENSE)
  - JsonStreamingParser (v1.0.4): [MIT License](library/json-streaming-parser/LICENSE)
  - TimeClient: MIT License (Copyright © 2015 Daniel Eichhorn)

## 待优化项
- 设备端：增加SHT40传感器数据读取与天气数据融合展示；
- 服务端：支持多组WiFi凭证存储、优化WF42屏数据传输效率；
- 通用：增加本地配置持久化（SPIFFS存储WiFi/城市信息）、适配更多天气API。

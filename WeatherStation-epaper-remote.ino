/**
GPL v3.0
Copyright (c) 2017 by Hui Lu
*/

#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "JsonListener.h"
#include "Wire.h"
#include "TimeClient.h"
#include <ESP8266WebServer.h>
#include "heweather.h"
#include <EEPROM.h>
#include <SPI.h>
#include "webpage.h"
#include "EPD_drive.h"
#include "EPD_drive_gpio.h"
#include "bitmaps.h"
#include "lang.h"
#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#define ADC_ON digitalWrite(12, 1);
#define ADC_OFF digitalWrite(12, 0);
#define debug 0 ///< 调试模式，打开串口输出
#define SDA_PIN 13
#define SCL_PIN 14
// #define FAST_MODE
ADC_MODE(0);
/***************************
  全局设置
 **************************/
bool first_run = false;
bool exit_portal = 0;
String wifi_ssid;
String wifi_password;
int epd_time = 1200; // 局刷耗时ms
String epd_type;
byte epd_type_index;
byte contrast = 0x15;
String cdkey;
int crc;
String update_time_range;
byte showtime = 0;                                ///< 是否显示时间，不支持局刷的屏不要开
int sleeptime = 60;                               ///< 更新天气的间隔 单位为分钟,间隔是下面更新时间的间隔的整数倍如更新时间间隔为30分钟，更新天气间隔须为30分钟的倍数，不能是31，43分钟这样
int timeupdateinterval = 1 * 60;                  ///< 更新时间的间隔，单位为秒（显示时间时不可更改，只显示天气可改为60*30）
const float UTC_OFFSET = 8;                       ///< 时区，北京时间 UTC+8
byte end_time = 23;                               ///< 停止更新天气的时间 23：00
byte start_time = 0;                              ///< 开始更新天气的时间 7：00
const char *server = "http://192.168.31.16:8010"; ///< 提供天气信息的服务器
String client_name = "news";                      ///< 消息用户名设置，news为显示新闻，可以填写自定义用户名，通过duckweather.tk/client.php更新消息

/***************************
 **************************/
String city;
String lastUpdate = "--";
bool shouldsave = false;
bool updating = false; /// 是否在更新天气数据
TimeClient timeClient(UTC_OFFSET, server);
Duck_EPD EPD = Duck_EPD();
heweatherclient heweather(server, lang);
ESP8266WebServer web(80);
Ticker avoidstuck; /// 防止连接服务器过程中卡死
// 缩小 JsonDocument 大小以减少 RAM 占用。配置 JSON 内容较小，512 足够。
StaticJsonDocument<512> doc;
File uploadFile;

float accX=0,accY=0,accZ=0;

/****************/
// SHT4x I2C 地址
#define SHT4X_ADDR 0x44
// 测量模式命令
#define SHT4X_HIGH_PRECISION 0xFD
#define SHT4X_MED_PRECISION  0xF6
#define SHT4X_LOW_PRECISION  0xE0

// LIS2DW12 I2C 地址
#define LIS2DW12_ADDR 0x19  // 如果 SA0 接地则为0x18
// 寄存器定义
#define LIS2DW12_ADDR 0x19
#define LIS2DW12_CTRL1 0x20
#define LIS2DW12_CTRL3 0x22
#define LIS2DW12_CTRL6 0x25
#define LIS2DW12_STATUS 0x27
#define LIS2DW12_OUT_X_L 0x28
#define LIS2DW12_OUT_X_H 0x29
#define LIS2DW12_OUT_Y_L 0x2A
#define LIS2DW12_OUT_Y_H 0x2B
#define LIS2DW12_OUT_Z_L 0x2C
#define LIS2DW12_OUT_Z_H 0x2D

// CTRL1配置: ODR=0001(1.6Hz), MODE=10(单次转换模式), LP_MODE=00(低功耗模式1, 12位分辨率)
#define LIS2DW12_CTRL1_CONFIG 0x18

//传感器检测flag
uint8_t errorSHT40 = 0;

bool readSHT4xData(float &temperature, float &humidity, uint8_t cmd = SHT4X_HIGH_PRECISION) {
  Wire.beginTransmission(SHT4X_ADDR);
  Wire.write(cmd);
  if (Wire.endTransmission() != 0) {
    return false; // I2C 传输错误
  }

  // 根据 datasheet，高精度测量约 8.3ms
  delay(10);

  Wire.requestFrom(SHT4X_ADDR, 6);
  if (Wire.available() != 6) {
    return false; // 读取字节数不对
  }

  uint8_t rx_bytes[6];
  for (int i = 0; i < 6; i++) {
    rx_bytes[i] = Wire.read();
  }

  uint16_t temp_raw = ((uint16_t)rx_bytes[0] << 8) | rx_bytes[1];
  uint16_t humid_raw = ((uint16_t)rx_bytes[3] << 8) | rx_bytes[4];

  // 转换为物理值
  temperature = -45.0 + 175.0 * temp_raw / 65535.0;
  humidity = -6.0 + 125.0 * humid_raw / 65535.0;

  // 限制范围
  if (humidity > 100.0) humidity = 100.0;
  if (humidity < 0.0)   humidity = 0.0;

  pinMode(CLK, OUTPUT);
  pinMode(DIN, OUTPUT);
  
  return true;
}
// bool safeReadSHT4x(float &temperature, float &humidity) {
//   Wire.begin(SDA, SCL);               // 确保 I2C 激活（若已激活也没事）
//   bool ok = readSHT4xData(temperature, humidity);
//   // 等待 I2C 总线稳定并释放
//   delay(5);
//   // 释放 TwoWire（若实现提供 end()）
//   #if defined(TWOWIRE_H) || defined(WIRE_H) || defined(Wire)
//   // Some TwoWire implementations provide end(); use if available
//   Wire.end();
//   #endif
//   pinMode(CLK, OUTPUT);
//   pinMode(DIN, OUTPUT);
//   digitalWrite(CLK, LOW);
//   digitalWrite(DIN, LOW);
//   yield();
//   return ok;
// }

/**
 * @brief 初始化LIS2DW12传感器
 * 
 * 配置为:
 * - 低功耗模式1 (12位分辨率)
 * - 单次转换模式
 * - 软件触发 (SLP_MODE_SEL=1)
 * - ±2g量程
 */
bool initLIS2DW12() {
   Wire.begin(SDA_PIN, SCL_PIN);               // 确保 I2C 激活（若已激活也没事）
  // 配置CTRL1: 低功耗模式1 + 单次转换模式
  if (!writeRegister(LIS2DW12_CTRL1, LIS2DW12_CTRL1_CONFIG)) {
    return false;
  }
  
  // 配置CTRL3: 使能软件触发 (SLP_MODE_SEL=1)
  // 初始状态SLP_MODE_1=0
  if (!writeRegister(LIS2DW12_CTRL3, 0x02)) {
    return false;
  }
  
  // 配置CTRL6: ±2g满量程, 禁用低噪声模式
  if (!writeRegister(LIS2DW12_CTRL6, 0x00)) {
    return false;
  }
  pinMode(CLK, OUTPUT);
  pinMode(DIN, OUTPUT);
  digitalWrite(CLK, LOW);
  digitalWrite(DIN, LOW);
  yield();
  return true;
}

/**
 * @brief 触发单次转换
 * 
 * 设置SLP_MODE_1=1启动转换, 转换完成后硬件自动清零
 */
bool triggerSingleConversion() {
  Wire.begin(SDA_PIN, SCL_PIN);
  // 设置SLP_MODE_SEL=1 和 SLP_MODE_1=1
  return writeRegister(LIS2DW12_CTRL3, 0x03);
  // 关闭I2C并重置引脚状态
  pinMode(CLK, OUTPUT);
  pinMode(DIN, OUTPUT);
  digitalWrite(CLK, LOW);
  digitalWrite(DIN, LOW);
  yield();
}

/**
 * @brief 等待数据就绪标志
 * 
 * @return true 数据就绪
 * @return false 超时
 */
bool waitForDataReady() {
  Wire.begin(SDA_PIN, SCL_PIN);
  const unsigned long timeout = 10; // 超时时间10ms
  unsigned long startTime = millis();
  
  while (millis() - startTime < timeout) {
    uint8_t status = readRegister(LIS2DW12_STATUS);
    if (status & 0x01) { // 检查DRDY位(bit0)
      return true;
    }
    delayMicroseconds(100); // 短暂延时后重试
  }
    // 关闭I2C并重置引脚状态
  pinMode(CLK, OUTPUT);
  pinMode(DIN, OUTPUT);
  digitalWrite(CLK, LOW);
  digitalWrite(DIN, LOW);
  yield();
  return false;
}

/**
 * @brief 读取三轴加速度数据
 * @param x 用于存储X轴加速度的指针（单位：g）
 * @param y 用于存储Y轴加速度的指针（单位：g）
 * @param z 用于存储Z轴加速度的指针（单位：g）
 * @return true 读取成功，false 读取失败
 */
bool readAcceleration(float* x, float* y, float* z) {
    // 检查指针有效性
    if (x == nullptr || y == nullptr || z == nullptr) {
        return false;
    }

    Wire.begin(SDA_PIN, SCL_PIN);
    
    // 从OUT_X_L寄存器开始连续读取6个字节
    Wire.beginTransmission(LIS2DW12_ADDR);
    Wire.write(LIS2DW12_OUT_X_L);
    bool transmissionOk = (Wire.endTransmission(false) == 0);
    
    if (!transmissionOk || Wire.requestFrom((uint8_t)LIS2DW12_ADDR, (uint8_t)6) != 6) {
        // 读取失败时重置引脚状态
        pinMode(SCL_PIN, OUTPUT);
        pinMode(SDA_PIN, OUTPUT);
        digitalWrite(SCL_PIN, LOW);
        digitalWrite(SDA_PIN, LOW);
        yield();
        return false;
    }
    
    // 读取原始数据
    uint8_t xlo = Wire.read();
    uint8_t xhi = Wire.read();
    uint8_t ylo = Wire.read();
    uint8_t yhi = Wire.read();
    uint8_t zlo = Wire.read();
    uint8_t zhi = Wire.read();
    
    // 关闭I2C并重置引脚状态
  pinMode(CLK, OUTPUT);
  pinMode(DIN, OUTPUT);
  digitalWrite(CLK, LOW);
  digitalWrite(DIN, LOW);
  yield();
    
    // 组合为16位值 (12位数据左对齐, 低4位为0)
    int16_t x_raw = (int16_t)((xhi << 8) | xlo) >> 4;
    int16_t y_raw = (int16_t)((yhi << 8) | ylo) >> 4;
    int16_t z_raw = (int16_t)((zhi << 8) | zlo) >> 4;
    
    // 转换为g值 (灵敏度: 0.976 mg/digit = 0.000976 g/digit)
    const float sensitivity = 0.000976f;
    *x = x_raw * sensitivity;
    *y = y_raw * sensitivity;
    *z = z_raw * sensitivity;

  #ifdef debug
  // 打印数据（注意指针需要解引用*）
  Serial.print("X: "); Serial.print(*x, 3); Serial.print(" g\t");  // 保留3位小数更易读
  Serial.print("Y: "); Serial.print(*y, 3); Serial.print(" g\t");
  Serial.print("Z: "); Serial.print(*z, 3); Serial.println(" g");
  #endif

    
    return true;
}
/**
 * @brief 向寄存器写入数据
 */
bool writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LIS2DW12_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return (Wire.endTransmission() == 0);
}

/**
 * @brief 从寄存器读取数据
 */
uint8_t readRegister(uint8_t reg) {
  Wire.beginTransmission(LIS2DW12_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  
  if (Wire.requestFrom((uint8_t)LIS2DW12_ADDR, (uint8_t)1) != 1) {
    return 0xFF;
  }
  
  return Wire.read();
}

void setup()
{
#ifdef debug
  Serial.begin(115200);
  Serial.printf("Serial begins at %dms\n\n", millis());
#endif

  initLIS2DW12();
  triggerSingleConversion();
  waitForDataReady();
  readAcceleration(&accX,&accY,&accZ);

  read_time_from_rtc_mem(); // 读取时间
#ifdef debug
  Serial.printf("time read finish at %dms\n\n", millis());
#endif
  check_epd(); // 刷时间后二次唤醒使屏进入睡眠
#ifdef debug
  Serial.printf("check sleep command finish at %dms\n\n", millis());
#endif
  EEPROM.begin(20);
  read_config_from_eeprom(); // 从eeprom读取基本设置，节省时间
#ifdef debug
  Serial.printf("eeprom finish at %dms\n\n", millis());
#endif
  check_rtc_mem(0); // 检测是不是第一次上电
#ifdef debug
  Serial.printf("check_rtc at %dms\n\n", millis());
#endif

  if (read_config() == 126)
  {
#ifdef debug
    Serial.println("always sleep flag=1, go to sleep"); // 什么也不做直接睡眠，电池没电或者长时间没连接WIFI
#endif
    EPD.deepsleep();
    ESP.deepSleep(60 * 60 * 1000000UL);
  }

#ifdef debug
  Serial.printf("io init at %dms\n\n", millis());
#endif
  pinMode(5, INPUT_PULLUP);
  pinMode(CS, OUTPUT);
  pinMode(DC, OUTPUT);
  pinMode(RST, OUTPUT);
  pinMode(BUSY, INPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DIN, OUTPUT);
  pinMode(12, OUTPUT);

#ifdef debug
  Serial.printf("IO init finish at %dms\n\n", millis());
#endif

update_time();//刷新时间，返回则继续向下运行

#ifdef FAST_MODE
  crc = 1;
#else
  crc = ESP.checkFlashCRC();
#endif

  if (crc == false)
  {
#ifdef debug
    Serial.printf("crc false!\n\n");
#endif
    always_sleep();
  }

#ifdef debug
  Serial.printf("Change to sta mode\n\n");
#endif
  WiFi.mode(WIFI_STA);
  LittleFS.begin();
  /*************************************************
  wifimanager
  *************************************************/
  if (LittleFS.exists("/config.json"))
  {
#ifdef debug
    Serial.println("config.json exists");
#endif
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
#ifdef debug
      Serial.println("Failed to open config file");
#endif
    }
    size_t size = configFile.size();
    if (size > 1024)
    {
#ifdef debug
      Serial.println("Config file size is too large");
#endif
    }

    String temp = configFile.readString();
#ifdef debug
    Serial.println(temp);
#endif
    auto error = deserializeJson(doc, temp);
    if (error)
    {
#ifdef debug
      Serial.println("Failed to parse config file");
      Serial.println(error.c_str());
#endif
    }

    cdkey = String(doc["cdkey"].as<String>());
    city = String(doc["city"].as<String>());
    epd_type = doc["epd_type"].as<String>();
    client_name = doc["client_name"].as<String>();
    heweather.client_name = client_name;
    showtime = crc * doc["showtime"].as<byte>();
    sleeptime = crc * doc["sleeptime"].as<int>();
    start_time = crc * doc["start_time"].as<int>();
    end_time = crc * doc["end_time"].as<int>();
    update_time_range = doc["update_time"].as<String>();
    wifi_ssid = doc["wifi_ssid"].as<String>();
    wifi_password = doc["wifi_password"].as<String>();
    epd_type_index = doc["epd_type_index"].as<byte>();
    contrast = doc["contrast"].as<byte>();
#ifdef debug
    Serial.print("client_name:");
    Serial.println(client_name);
    Serial.print("city:");
    Serial.println(city);
    Serial.print("epd_type:");
    Serial.println(epd_type);
    Serial.print("showtime:");
    Serial.println(showtime);
    Serial.print("sleeptime:");
    Serial.println(sleeptime);
    Serial.print("start_time:");
    Serial.println(start_time);
    Serial.print("end_time:");
    Serial.println(end_time);
    Serial.print("wifi_ssid:");
    Serial.println(wifi_ssid);
    Serial.print("wifi_password:");
    Serial.println(wifi_password);
    Serial.print("epd_type_index:");
    Serial.println(epd_type_index);
    Serial.print("contrast:");
    Serial.println(contrast);
#endif
    configFile.close();
  }

  if (accZ > 0.8)
  {
    StartPortal();
  }

  if (!LittleFS.exists("/config.json"))
  {

#ifdef debug
    Serial.println("config.json dont exists");
#endif
    EPD.EPD_Set_Model(epd_type_index);
    EPD.EPD_init_Full();
    EPD.clearbuffer();
    EPD.fontscale = 2;
    EPD.SetFont(FONT12);
    EPD.DrawUTF(0, 0, "配置文件不存在");
    EPD.DrawUTF(32, 0, "请重新配置");
    EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer, 1);
    EPD.deepsleep();
    ESP.deepSleep(60 * 60 * 1000000UL);
  }
  
  if (accZ > 0.8)
  {
    StartPortal();
  }

    show_status(); // 显示进度

WiFi.mode(WIFI_OFF);  // 先强制关闭WiFi
delay(500);           // 等待状态切换
WiFi.mode(WIFI_STA);  // 再切换到STA模式

WiFi.persistent(false);
WiFi.disconnect(false);  // 清除之前的连接信息（避免残留配置干扰）
delay(2000);

while (WiFi.status() != WL_CONNECTED) {
  WiFi.begin(wifi_ssid, wifi_password);
  
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
#ifdef debug
    Serial.print(".");
#endif
  }

  if (WiFi.status() == WL_CONNECTED && crc) {
#ifdef debug
    Serial.println("WiFi connected!");
#endif
    break;
  }

#ifdef debug
  Serial.printf("WiFi connect failed at %dms\n", millis());
#endif

  EPD.EPD_Set_Model(epd_type_index);
  EPD.EPD_init_Full();
  EPD.clearbuffer();
  EPD.fontscale = 2;
  EPD.SetFont(FONT12);

  if (!crc) {
    EPD.DrawUTF(0, 0, "flash校验错误");
    EPD.DrawUTF(32, 0, "程序不完整或已更改");
  } else {
    EPD.DrawUTF(0, 0, "WIFI连接失败");
    EPD.DrawUTF(32, 0, "请重新配置或尝试重新启动");
  }

  EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer, 1);
  ESP.deepSleep(2* 1000000UL);
}


#ifdef debug
  Serial.printf("Wifi connected at %dms \n", millis());
#endif
  WiFi.persistent(true);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  /*************************************************
  EPPROM
  *************************************************/
  heweather.city = city;

  /*************************************************
     update weather
  *************************************************/
  // heweather.city="huangdao";
  heweather.EPDbuffer = &EPD.EPDbuffer[0];
  avoidstuck.attach(30, check);
  heweather.bssid = WiFi.BSSIDstr();
  heweather.ssid = WiFi.SSID();
  heweather.epd_type = String(epd_type_index);
  updating = true;
  updateData();
  updating = false;

// WiFi.persistent(false);
// WiFi.disconnect(true);  // 清除之前的连接信息
// delay(300);
// WiFi.mode(WIFI_OFF);    // 关闭WiFi
// delay(300);



  EPD.EPD_Set_Model(epd_type_index);
  EPD.EPD_init_Full();
  EPD.EPD_Set_Contrast(contrast);
  updatedisplay();
}
void show_status()
{
    if (first_run == true)
    {
      EPD.EPD_init_Full();
      EPD.clearbuffer();
      EPD.fontscale = 2;
      EPD.SetFont(FONT12);
      EPD.EPD_Set_Contrast(contrast);
      EPD.DrawUTF(140, 50, heweather.city + " 连接服务器中....");
      EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer, 1);
    }
}
void check()
{
  if (updating == true)
  {
    avoidstuck.detach();
#ifdef debug
    Serial.printf("Time out while connecting server\n\n");
#endif
      EPD.EPD_Set_Model(epd_type_index);
      EPD.clearbuffer();
      EPD.fontscale = 1;
    write_time_to_rtc_mem();
    ESP.deepSleep(timeupdateinterval * 1 * 1000000UL, WAKE_RF_DEFAULT);
  }
  avoidstuck.detach();
  return;
}

void loop()
{

  EPD.deepsleep();
  if (showtime == 1)
  {
    byte seconds = timeClient.getSeconds_byte();
    if (seconds > 58)
      timeupdateinterval = 60 - seconds + 60;
    else
      timeupdateinterval = 60 - seconds;
    // timeupdateinterval=1;//暴力测试
  }
  timeClient.localEpoc += timeupdateinterval * 1000;
  // timeClient.localEpoc+=60*1000;//暴力测试
  write_time_to_rtc_mem(); // save time before sleeping}

#ifdef debug
  Serial.printf("Finish at %dms\n", millis());
#endif

  ESP.deepSleep(timeupdateinterval * 1 * 1000000, WAKE_RF_DISABLED);
  // ESP.deepSleep(1 * 1 * 1000000,WAKE_RF_DISABLED); //暴力测试
}

bool safeReadSHT4x(float &temperature, float &humidity) {
  Wire.begin(SDA_PIN, SCL_PIN);               // 确保 I2C 激活（若已激活也没事）
  bool ok = readSHT4xData(temperature, humidity);
  // 等待 I2C 总线稳定并释放
  delay(5);
  // 释放 TwoWire（若实现提供 end()）
  #if defined(TWOWIRE_H) || defined(WIRE_H) || defined(Wire)
  // Some TwoWire implementations provide end(); use if available
  Wire.end();
  #endif
  pinMode(CLK, OUTPUT);
  pinMode(DIN, OUTPUT);
  digitalWrite(CLK, LOW);
  digitalWrite(DIN, LOW);
  yield();
  return ok;
}

void updatedisplay()
{
  float Hum, Temp;
   if (safeReadSHT4x(Temp,Hum)) {
    #ifdef debug
    Serial.print("Temperature: ");
    Serial.print(Temp, 2);
    Serial.print(" °C, Humidity: ");
    Serial.print(Hum, 2);
    Serial.println(" %");
    #endif
  }
  else
  {
    errorSHT40 = 1;
  }
 

  EPD.clearbuffer();
  if (heweather.citystr == "null")
    heweather.citystr = "未知城市,请重新设置";
  if (showtime == 1)
  {
    if (heweather.timeout == false)
    {
      EPD.fontscale = 1;
      EPD.SetFont(FONT12);
      EPD.DrawXbm_P(2, 2, 12, 12, (unsigned char *)city_icon);
      EPD.DrawUTF(2, 14, heweather.citystr + " 更新时间：" + lastUpdate);

      // 日历部分
      EPD.fontscale = 2;
      EPD.DrawUTF(20, 2, heweather.year.substring(7, 17)); // 日期
      EPD.DrawUTF(47, 2, heweather.year.substring(18));    // 星期
      EPD.fontscale = 1;
      EPD.DrawUTF(76, 2, heweather.nongli); // 农历

      // 天气信息栏2
      int b = 105;
      EPD.DrawXbm_P(24, 132 + b, 35, 72, digi_num[timeClient.getHours()[0] - 0x30]);
      EPD.DrawXbm_P(24, 169 + b, 35, 72, digi_num[timeClient.getHours()[1] - 0x30]);
      EPD.DrawXbm_P(24, 206 + b, 35, 72, digi_num[10]);
      EPD.DrawXbm_P(24, 220 + b, 35, 72, digi_num[timeClient.getMinutes()[0] - 0x30]);
      EPD.DrawXbm_P(24, 255 + b, 35, 72, digi_num[timeClient.getMinutes()[1] - 0x30]);
      write_last_time(timeClient.getHours()[0], timeClient.getHours()[1], timeClient.getMinutes()[0], timeClient.getMinutes()[1]);
      int x = 20;
      int y = 0;
      EPD.SetFont(ICON80);
      unsigned char code[] = {0x00, heweather.getMeteoconIcon(heweather.now_cond_index.toInt())};
      EPD.DrawUnicodeStr(16, 95, 80, 80, 1, code);
      // 天气信息栏1
      x = 20;
      y = 180;
      EPD.SetFont(FONT12);
      EPD.fontscale = 2;
      EPD.DrawUTF(x, y, heweather.now_tmp + "°");
      EPD.DrawUTF(x += 27, y, heweather.now_cond);
      EPD.fontscale = 1;
            if(errorSHT40 == 0)
      {
      EPD.DrawUTF(x=76,y,"室温"+String(Temp,1)+"°");
      EPD.DrawUTF(x+=14,y,"湿度"+String(Hum,0)+"%");
      }
      else
      {
      EPD.DrawUTF(x+=14,y,"室外湿度"+heweather.now_hum+"%");
      }   
      EPD.fontscale = 1;

      int t_data[] = {33, 33, 30, 32, 34, 33,
                      24, 24, 23, 24, 25, 26};

      EPD.DrawWeatherChart(190, 216, 25, 375, 6, 6, heweather.tmax_array, heweather.tmin_array, heweather.code_d_array, heweather.code_n_array, heweather.text_d_array, heweather.text_n_array, heweather.date_array, heweather.week_array);

      EPD.DrawXbm_P(285, 1, 12, 12, (unsigned char *)message);
      EPD.DrawUTF(285, 14, heweather.message); // EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer,3);
      dis_batt(3, 377);
      EPD.Inverse(0, 16, 0, 400);
    }
    else
    {
      EPD.EPD_Set_Model(epd_type_index);
      if (first_run == true)
      {
        EPD.EPD_init_Full();
        EPD.clearbuffer();
        EPD.fontscale = 2;
        EPD.SetFont(FONT12);
        EPD.EPD_Set_Contrast(contrast);
        EPD.DrawUTF(16, 0, "连接服务器超时");
        EPD.DrawUTF(50, 0, "请检查网络连接或按左侧EN键重试");
        EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer, 1);
        always_sleep();
      }
      else
      {
        EPD.EPD_init_Part();
        EPD.clearbuffer();
        EPD.fontscale = 1;
        EPD.SetFont(FONT12);
        EPD.EPD_Set_Contrast(contrast);
        EPD.DrawXbm_P(2, 2, 12, 12, (unsigned char *)city_icon);
        EPD.DrawUTF(2, 14, heweather.citystr + " 连接服务器超时于：" + lastUpdate);
        int b = 105;

        EPD.DrawXbm_P(24, 132 + b, 35, 72, digi_num[timeClient.getHours()[0] - 0x30]);
        EPD.DrawXbm_P(24, 169 + b, 35, 72, digi_num[timeClient.getHours()[1] - 0x30]);
        EPD.DrawXbm_P(24, 206 + b, 35, 72, digi_num[10]);
        EPD.DrawXbm_P(24, 220 + b, 35, 72, digi_num[timeClient.getMinutes()[0] - 0x30]);
        EPD.DrawXbm_P(24, 255 + b, 35, 72, digi_num[timeClient.getMinutes()[1] - 0x30]);
        EPD.Inverse(0, 16, 0, 400);
        EPD.EPD_Transfer_Part(0, 15, 0, 319, (unsigned char *)EPD.EPDbuffer, 1);
        EPD.EPD_Transfer_Part(16, 87, 237, 399, (unsigned char *)EPD.EPDbuffer, 1);
        EPD.EPD_Update_Part();
        EPD.ReadBusy_long();
        // EPD.EPD_init_Part();
        EPD.EPD_Transfer_Part(0, 15, 0, 319, (unsigned char *)EPD.EPDbuffer, 1);
        // EPD.EPD_Transfer_Part(0,15,0,319,(unsigned char *)EPD.EPDbuffer,1);
        EPD.EPD_Transfer_Part(16, 87, 237, 399, (unsigned char *)EPD.EPDbuffer, 1);
        EPD.EPD_Update_Part();
      }
    }
  }
  else // showtime==0
  {
    if (heweather.timeout == false)
    {
      EPD.fontscale = 1;
      EPD.SetFont(FONT12);
      EPD.DrawXbm_P(2, 2, 12, 12, (unsigned char *)city_icon);

      EPD.DrawUTF(2, 14, heweather.citystr + " 更新时间：" + lastUpdate);
       #ifdef debug
      Serial.println(String(heweather.citystr));
      #endif
      // 日历部分
      EPD.fontscale = 2;
      EPD.DrawUTF(20, 2, heweather.year.substring(7, 17)); // 日期
      EPD.DrawUTF(47, 2, heweather.year.substring(18));    // 星期
      EPD.fontscale = 1;
      EPD.DrawUTF(76, 2, heweather.nongli); // 农历

      // 天气信息栏2
      int x = 20;
      int y = 0;
      EPD.DrawXbm_P(x, 280, 12, 12, (unsigned char *)fl);
      EPD.DrawUTF(x, 294, "体感温度" + heweather.now_fl + "°");
      EPD.DrawXbm_P(x += 14, 280, 12, 12, (unsigned char *)dir);
      EPD.DrawUTF(x, 294, "风向" + heweather.now_dir);
      EPD.DrawXbm_P(x += 14, 280, 12, 12, (unsigned char *)sc);
      EPD.DrawUTF(x, 294, "风力" + heweather.now_sc + "级");
      EPD.DrawXbm_P(x += 14, 280, 12, 12, (unsigned char *)pcpn);
      EPD.DrawUTF(x, 294, "降水量" + heweather.now_pcpn + "mm");
      EPD.DrawXbm_P(x += 14, 280, 12, 12, (unsigned char *)vis);
      EPD.DrawUTF(x, 294, "能见度" + heweather.now_vis + "km");
      EPD.DrawXbm_P(x += 14, 280, 12, 12, (unsigned char *)pres);
      EPD.DrawUTF(x, 294, "气压" + heweather.now_pres + "hPa");

      EPD.SetFont(ICON80);
      unsigned char code[] = {0x00, heweather.getMeteoconIcon(heweather.now_cond_index.toInt())};
      EPD.DrawUnicodeStr(12, 120, 80, 80, 1, code);
      // 天气信息栏1
      x = 20;
      y = 210;
      EPD.SetFont(FONT12);
      EPD.fontscale = 2;
      EPD.DrawUTF(x, y, heweather.now_tmp + "°");
      EPD.DrawUTF(x += 27, y, heweather.now_cond);
      EPD.fontscale = 1;
            if(errorSHT40 == 0)
      {
      EPD.DrawUTF(x=76,y,"室温"+String(Temp,1)+"°");
      EPD.DrawUTF(x+=14,y,"湿度"+String(Hum,0)+"%");
      }
      else
      {
      EPD.DrawUTF(x+=14,y,"室外湿度"+heweather.now_hum+"%");
      }   
      EPD.fontscale = 1;

      EPD.DrawYline(20, 100, 114);
      EPD.DrawYline(20, 100, 275);
      int t_data[] = {33, 33, 30, 32, 34, 33,
                      24, 24, 23, 24, 25, 26};

      EPD.DrawWeatherChart(190, 216, 25, 375, 6, 6, heweather.tmax_array, heweather.tmin_array, heweather.code_d_array, heweather.code_n_array, heweather.text_d_array, heweather.text_n_array, heweather.date_array, heweather.week_array);

      EPD.DrawXbm_P(285, 1, 12, 12, (unsigned char *)message);
      EPD.DrawUTF(285, 14, heweather.message);
      dis_batt(3, 377);
      EPD.Inverse(0, 16, 0, 400);
    }
    else
    {
      EPD.EPD_Set_Model(epd_type_index);

      if (first_run == true)
      {
        EPD.EPD_init_Full();
        EPD.clearbuffer();
        EPD.fontscale = 2;
        EPD.SetFont(FONT12);
        EPD.EPD_Set_Contrast(contrast);
        EPD.DrawUTF(16, 0, "连接服务器超时");
        EPD.DrawUTF(50, 0, "请检查网络连接或按左侧EN键重试");
        EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer, 1);
        always_sleep();
      }
      else
      {
        EPD.EPD_init_Part();
        EPD.clearbuffer();
        EPD.fontscale = 1;
        EPD.SetFont(FONT12);
        EPD.EPD_Set_Contrast(contrast);
        EPD.DrawXbm_P(2, 2, 12, 12, (unsigned char *)city_icon);
        EPD.DrawUTF(2, 14, heweather.citystr + " 连接服务器超时于：" + lastUpdate);

        EPD.Inverse(0, 16, 0, 400);
        EPD.EPD_Transfer_Part(0, 15, 0, 319, (unsigned char *)EPD.EPDbuffer, 1);
        EPD.EPD_Update_Part();
        EPD.ReadBusy_long();
        EPD.EPD_Transfer_Part(0, 15, 0, 319, (unsigned char *)EPD.EPDbuffer, 1);
        EPD.EPD_Update_Part();
      }
    }
  }

  if (heweather.city != "" && heweather.timeout == false)
  {
    EPD.EPD_init_Full();
    EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer, 1);
  }
}
/**
 * @brief 显示电池电量
 * @param x,y 显示位置
 */

void dis_batt(int16_t x, int16_t y)
{
  /*attention! calibrate it yourself */
  float voltage;
  float batt_voltage;
  long sum = 0;

  ADC_ON;
  for (int i = 0; i < 100; i++)
    sum += analogRead(A0);
  voltage = (float)sum * 5.7 / 102400;
  batt_voltage = voltage;
#ifdef debug
  Serial.println(String(batt_voltage) + "V");
#endif
  ADC_OFF;
  sum = 0;

  if (batt_voltage <= 3.6)
  {
    EPD.clearbuffer();
    EPD.DrawXbm_P(39, 98, 100, 50, (unsigned char *)needcharge);
    always_sleep();
#ifdef debug
    Serial.println("baterry low");
#endif
  }
  if (batt_voltage > 3.6 && batt_voltage <= 3.7)
    EPD.DrawXbm_P(x, y, 20, 10, (unsigned char *)batt_1);
  if (batt_voltage > 3.7 && batt_voltage <= 3.8)
    EPD.DrawXbm_P(x, y, 20, 10, (unsigned char *)batt_2);
  if (batt_voltage > 3.8 && batt_voltage <= 3.9)
    EPD.DrawXbm_P(x, y, 20, 10, (unsigned char *)batt_3);
  if (batt_voltage > 3.9 && batt_voltage <= 4.0)
    EPD.DrawXbm_P(x, y, 20, 10, (unsigned char *)batt_4);
  if (batt_voltage > 4.0)
    EPD.DrawXbm_P(x, y, 20, 10, (unsigned char *)batt_5);
}
/*
 * @brief 显示时间
 */
void dis_time(int16_t x, int16_t y)
{
  x = 24;
  y = 128;

  EPD.fontscale = 1;
  EPD.clearbuffer();
  int b = 0;
  b = 105;
  char hour[2]; // 当前时间
  hour[0] = timeClient.getHours()[0];
  hour[1] = timeClient.getHours()[1];
  char minute[2];
  minute[0] = timeClient.getMinutes()[0];
  minute[1] = timeClient.getMinutes()[1];
  EPD.EPD_init_Part();
  EPD.DrawXbm_P(24, 132 + b, 35, 72, digi_num[hour[0] - 0x30]);
  EPD.DrawXbm_P(24, 169 + b, 35, 72, digi_num[hour[1] - 0x30]);
  EPD.DrawXbm_P(24, 206 + b, 35, 72, digi_num[10]);
  EPD.DrawXbm_P(24, 220 + b, 35, 72, digi_num[minute[0] - 0x30]);
  EPD.DrawXbm_P(24, 255 + b, 35, 72, digi_num[minute[1] - 0x30]);
  EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer, 1);

  EPD.clearbuffer();
  byte rtc_mem[4];
  ESP.rtcUserMemoryRead(12, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
  EPD.DrawXbm_P(24, 132 + b, 35, 72, digi_num[rtc_mem[0] - 0x30]);
  EPD.DrawXbm_P(24, 169 + b, 35, 72, digi_num[rtc_mem[1] - 0x30]);
  EPD.DrawXbm_P(24, 206 + b, 35, 72, digi_num[10]);
  EPD.DrawXbm_P(24, 220 + b, 35, 72, digi_num[rtc_mem[2] - 0x30]);
  EPD.DrawXbm_P(24, 255 + b, 35, 72, digi_num[rtc_mem[3] - 0x30]);
  EPD.EPD_Transfer_Full_BW((unsigned char *)EPD.EPDbuffer, 4);
  EPD.EPD_Update();
  write_last_time(hour[0], hour[1], minute[0], minute[1]);
}
/*
 * @brief 更新天气，校准时间
 */
void updateData()
{
  heweather.cdkey = cdkey;
  heweather.update();
  timeClient.updateTime(heweather.t);
  lastUpdate = timeClient.getHours() + ":" + timeClient.getMinutes();
  #ifdef debug
  Serial.print("lastUpdate:");
  Serial.println(lastUpdate);

    Serial.print("H:");
  Serial.println(timeClient.getHours());

    Serial.print("M:");
  Serial.println(timeClient.getMinutes());

  #endif
  byte rtc_mem[4];
  rtc_mem[0] = 126;
  byte Hours = timeClient.getHours().toInt();
}

/*
 * @brief 读取rtc mem中存储的睡眠之前的时间
 */
void read_time_from_rtc_mem()
{
#ifdef debug
  Serial.println("Reading time from rtc mem\n\n");
#endif
  byte rtc_mem[4];
  ESP.rtcUserMemoryRead(8, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
  timeClient.localEpoc = ((unsigned long)rtc_mem[3] << 24) | ((unsigned long)rtc_mem[2] << 16) | ((unsigned long)rtc_mem[1] << 8) | (unsigned long)rtc_mem[0];
}
/*
 * @brief 将当前时间写入rtc mem
 */
void write_time_to_rtc_mem()
{
  // write time to rtc before sleep
  long time_before_sleep;
  time_before_sleep = timeClient.getCurrentEpoch();
  byte rtc_mem[4];
  rtc_mem[0] = byte(time_before_sleep);
  rtc_mem[1] = byte(time_before_sleep >> 8);
  rtc_mem[2] = byte(time_before_sleep >> 16);
  rtc_mem[3] = byte(time_before_sleep >> 24);
  ESP.rtcUserMemoryWrite(8, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
}
void write_last_time(byte hour0, byte hour1, byte minute0, byte minute1) // 上次更新时的时间是多少
{
  byte rtc_mem[4];
  rtc_mem[0] = byte(hour0);
  rtc_mem[1] = byte(hour1);
  rtc_mem[2] = minute0;
  rtc_mem[3] = minute1;
  ESP.rtcUserMemoryWrite(12, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
}
/*
 * @brief 读取rtcmem中第六个byte
 * @detail rtcmem中第六个byte,若等于126，则代表连接WIFI超时，或者电池没电
 */
byte read_config()
{
  byte rtc_mem[4];
  ESP.rtcUserMemoryRead(4, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
  ////check if sleeping forever is needed
  return rtc_mem[2];
}
/*
 * @brief 将标志位写入126,不再更新时间和天气，也不刷新屏幕
 */
void always_sleep()
{
  byte rtc_mem[4];
  ESP.rtcUserMemoryRead(4, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
  if (rtc_mem[2] != 126)
  {
    rtc_mem[2] = 126;
    ESP.rtcUserMemoryWrite(4, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
  }
}
/*
 * @brief 检测是不是第一次上电运行此程序
 */
void check_rtc_mem(bool force_init)
{
  /*
  rtc_mem[0] sign for first run
  rtc_mem[1] how many hours left
  */

  byte rtc_mem[4];
  ESP.rtcUserMemoryRead(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
  if (rtc_mem[0] != 126 || force_init == 1)
  {
#ifdef debug
    Serial.println("first time to run or en pressed");
#endif
    first_run = true;
    byte times = byte(sleeptime * 60 / timeupdateinterval);
    // Serial.println("times");Serial.println(times);
    rtc_mem[0] = 126;
    rtc_mem[1] = 0;
    rtc_mem[2] = times; // time
    if (force_init == 1)
      rtc_mem[2] = times = 0;
#ifdef debug
    Serial.println("rctmemblock0-2");
    Serial.println(rtc_mem[2]);
#endif
    // Serial.println("rctmemblock0-2");Serial.println(rtc_mem[2]);
    ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
  }
  else
  {
    first_run = false;
  }
}
/*
 * @brief 检测更新时间还是天气
 *
 */
void update_time()
{

  byte rtc_mem[4];
  byte times = byte(sleeptime * 60 / timeupdateinterval);
  ESP.rtcUserMemoryRead(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
#ifdef debug
  Serial.println(timeClient.getFormattedTime());
  Serial.printf("Update weather after %d times of time update\r\n", times);
  Serial.printf("Time updated %d times\r\n", rtc_mem[2]);
#endif

  if (rtc_mem[2] > times - 2)

  {
    rtc_mem[2] = 0;
#ifdef debug
    Serial.println("Need to update weather");
#endif
    ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
    EPD.EPD_Set_Model(epd_type_index);
    EPD.EPD_init_Full();
    EPD.EPD_Set_Contrast(contrast);
  }
  else
  {
    if (showtime == 1)
    {
      HARDWARE_SPI = 1;
      SPI.begin();
      SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
      EPD.EPD_Set_Model(epd_type_index);
#ifdef debug
      Serial.printf("Epd init begin at %dms\n", millis());
#endif
      EPD.EPD_init_Part();
#ifdef debug
      Serial.printf("Epd init finish at %dms\n", millis());
#endif
      EPD.EPD_Set_Contrast(contrast);
    }

#ifdef debug
    Serial.println("Don't need to update weather, need time");
#endif
    if (end_time < start_time)
    {
      byte temp = start_time; // 6
      start_time = end_time;  // 23
      end_time = temp;
    }
    if (timeClient.getHours_byte() < end_time && timeClient.getHours_byte() >= start_time)
    {
#ifdef debug
      Serial.println("Update at daytime");
#endif
      byte rct_temp = byte(rtc_mem[2] + 1);
      rtc_mem[2] = rct_temp;
      ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
    }
    else
    {
#ifdef debug
      Serial.println("Update at night");
#endif
      byte rct_temp = byte(times - 2);
      rtc_mem[2] = rct_temp;
      ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
    }
#ifdef debug
    Serial.printf("Epd begin at %dms\n", millis());
#endif
    if (showtime == 1)
    {
      dis_time(1, 240);
#ifdef debug
      Serial.printf("Epd data transfer end at %dms\n", millis());
      Serial.printf("rtcmem[2]=%dtimes-1=%d\n", rtc_mem[2], times - 1);
#endif
      // 判断下次是否要开启射频
      if ((rtc_mem[2] == times - 1))
      {
#ifdef debug
        Serial.printf("RF ON\n", millis());
#endif
        rtc_mem[3] = 1; // 要开启
      }
      else
      {
#ifdef debug
        Serial.printf("RFCLOSE\n", millis());
#endif
        rtc_mem[3] = 2; // 不开启
      }
      ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));

      timeClient.localEpoc += epd_time;
      write_time_to_rtc_mem();
      ESP.deepSleepInstant(epd_time * 1 * 1000, WAKE_RF_DISABLED);

#ifdef debug
      Serial.printf("Epd finish at %dms\n", millis());
#endif
      timeupdateinterval = 60;
    }

    timeClient.localEpoc += timeupdateinterval * 1000;
    write_time_to_rtc_mem();

#ifdef debug
    Serial.printf("Esp finish at %dms\n", millis());
#endif
    Serial.printf("rtcmem[2]=%dtimes-1=%d\n", rtc_mem[2], times - 1);
    if ((rtc_mem[2] == times - 1))
    {
#ifdef debug
      Serial.printf("RF ON\n", millis());
#endif

      ESP.deepSleepInstant(timeupdateinterval * 1 * 1000000, WAKE_RF_DEFAULT);
    }
    else
    {
#ifdef debug
      Serial.printf("RFCLOSE\n", millis());
#endif

      ESP.deepSleepInstant(timeupdateinterval * 1 * 1000000, WAKE_RF_DISABLED);
    }
  }
}
void check_epd()
{
#ifdef debug
  Serial.println("Checking if sleep command is needed\n");
#endif
  byte rtc_mem[4];
  ESP.rtcUserMemoryRead(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
  if (rtc_mem[3] == 1)
  {
    EPD.deepsleep();
    timeupdateinterval = 60;
    timeupdateinterval = 60 - timeClient.getSeconds_byte();
    timeClient.localEpoc += timeupdateinterval * 1000;
    // timeClient.localEpoc+=60*1000;
    write_time_to_rtc_mem();
    rtc_mem[3] = 0;
    ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
#ifdef debug
    Serial.println("send sleep command to epd-WAKE_RF_DEFAULT\n\n");
#endif
    ESP.deepSleepInstant(timeupdateinterval * 1 * 1000000, WAKE_RF_DEFAULT);
  }
  if (rtc_mem[3] == 2)
  {
    EPD.deepsleep();
    timeupdateinterval = 60;
    timeupdateinterval = 60 - timeClient.getSeconds_byte();
    timeClient.localEpoc += timeupdateinterval * 1000;
    // timeClient.localEpoc+=60*1000;
    write_time_to_rtc_mem();
    rtc_mem[3] = 0;
    ESP.rtcUserMemoryWrite(0, (uint32_t *)&rtc_mem, sizeof(rtc_mem));
#ifdef debug
    Serial.println("send sleep command to epd-WAKE_RF_DISABLED\n\n");
#endif
    ESP.deepSleepInstant(timeupdateinterval * 1 * 1000000, WAKE_RF_DISABLED);
  }
#ifdef debug
  Serial.println("sleep command is not needed\n\n");
#endif
}
void write_config_to_eeprom()
{
  /*eeprom map
   *[0]epd_type
   *[1]showtime
   *[2][3]sleeptime
   *[4][5]timeupdateinterval
   *[6]end_time
   *[7]start_time
   *[8]contrast
   */

#ifdef debug
  Serial.println("EEPROM write:");
  Serial.printf("epd_type_index=%d\n", epd_type_index);
  Serial.printf("showtime=%d\n", showtime);
  Serial.printf("sleeptime=%d\n", sleeptime);
  Serial.printf("timeupdateinterval=%d\n", timeupdateinterval);
  Serial.printf("end_time=%d\n", end_time);
  Serial.printf("start_time=%d\n", start_time);
  Serial.printf("contrast=%d\n", contrast);
#endif
  EEPROM.write(0, epd_type_index);
  EEPROM.write(1, showtime);
  EEPROM.write(2, sleeptime >> 8);
  EEPROM.write(3, byte(sleeptime));
  EEPROM.write(4, timeupdateinterval >> 8);
  EEPROM.write(5, byte(timeupdateinterval));
  EEPROM.write(6, end_time);
  EEPROM.write(7, start_time);
  EEPROM.write(8, contrast);
  EEPROM.commit();
}

void read_config_from_eeprom()
{
  /*eeprom map
   *[0]epd_type
   *[1]showtime
   *[2][3]sleeptime
   *[4][5]timeupdateinterval
   *[6]end_time
   *[7]start_time
   */

  epd_type_index = EEPROM.read(0);
  showtime = EEPROM.read(1);
  sleeptime = EEPROM.read(2) << 8 | EEPROM.read(3);
  // sleeptime=5;
  timeupdateinterval = EEPROM.read(4) << 8 | EEPROM.read(5);
  end_time = EEPROM.read(6);
  start_time = (byte)EEPROM.read(7);
  contrast = (byte)EEPROM.read(8);
  /*
 #ifdef debug
  Serial.println("EEPROM read:");
  Serial.printf("epd_type_index=%d\n",epd_type_index);
  Serial.printf("showtime=%d\n",showtime);
  Serial.printf("sleeptime=%d\n",sleeptime);
  Serial.printf("timeupdateinterval=%d\n",timeupdateinterval);
  Serial.printf("end_time=%d\n",end_time);
  Serial.printf("start_time=%d\n",start_time);
  Serial.printf("contrast=%d\n\n",contrast);
 #endif*/
}
void clear_wifi()
{
  // Serial.println("clear wifi");
}
void handleJs()
{
  String page;
  page += FPSTR(INDEX_JS);
  web.send(200, "text/html", page);
}
void handleRoot()
{
#ifdef debug
  Serial.println("root page\n\n");
#endif
  String page;
  page += FPSTR(INDEX);

  web.send(200, "text/html", page);

  page += FPSTR(INDEX_JS);
  web.send(200, "text/html", page);
}
void handlePhoto()
{
  String page;
  page += FPSTR(PHOTO);
  web.send(200, "text/html", page);
}

void handleSaveConfig()
{
  String wifi_ssid = "ssid";
  String wifi_password = "pass";
  city = web.arg("city");
  wifi_ssid = web.arg("wifi_ssid");
  wifi_password = web.arg("wifi_password");
  client_name = web.arg("client_name");
  epd_type = web.arg("epd");
  showtime = web.arg("showtime").toInt();
  sleeptime = 60 * web.arg("update_interval").toInt();
  update_time_range = web.arg("update_time");
  contrast = web.arg("contrast").toInt();
  cdkey = web.arg("cdkey");
#ifdef debug
  Serial.print("arg.city:");
  Serial.println(city);
  Serial.print("arg.client_name:");
  Serial.println(client_name);
#endif
  doc.clear();
  doc["client_name"] = client_name;
  doc["city"] = city;
  doc["epd_type"] = epd_type;
  doc["showtime"] = showtime;
  doc["sleeptime"] = sleeptime;
  doc["start_time"] = 0;
  doc["end_time"] = 0;
  doc["update_time"] = update_time_range;
  doc["wifi_ssid"] = wifi_ssid;
  doc["wifi_password"] = wifi_password;
  doc["contrast"] = contrast;
  doc["cdkey"] = cdkey;

  if (epd_type == "wx29")
    epd_type_index = 0;
  if (epd_type == "wf29")
    epd_type_index = 1;
  if (epd_type == "opm42")
  {
    epd_type_index = 2; /*doc["showtime"]=0;*/
  }
  if (epd_type == "wf58")
    epd_type_index = 3;
  if (epd_type == "wft29bz03")
    epd_type_index = 4;
  if (epd_type == "dke42")
    epd_type_index = 6;
  if (epd_type == "dke29")
    epd_type_index = 7;
  if (epd_type == "wf42")
    epd_type_index = 8;
  if (epd_type == "wf32")
    epd_type_index = 9;
  doc["epd_type_index"] = epd_type_index;

  // doc.clear();
  if (update_time_range == "2")
  {
    start_time = 6;
    end_time = 23;
    doc["start_time"] = 6;
    doc["end_time"] = 23;
  }
  if (update_time_range == "1")
  {
    start_time = 0;
    end_time = 23;
    doc["start_time"] = 0;
    doc["end_time"] = 23;
  }

  if (showtime == 1)
    timeupdateinterval = 60;
  else
    timeupdateinterval = 60 * 30;
  // if(LittleFS.exists("/config.json")) LittleFS.remove("/config.json");
  File configFile = LittleFS.open("/config.json", "w+");
  if (!configFile)
  {
#ifdef debug
    Serial.println("Failed to open config file for writing");
#endif
  }
  serializeJson(doc, configFile);
  configFile.close();

#ifdef debug
  Serial.println("Trying to read config.json");
  if (LittleFS.exists("/config.json"))
  {
    configFile = LittleFS.open("/config.json", "r");
    String temp = configFile.readString();
    Serial.println(temp);
    configFile.close();
  }
  else
  {
    Serial.println("config json not exist");
  }
#endif

  write_config_to_eeprom(); // 往eeprom中拷贝一份
#ifdef debug
  Serial.println(wifi_ssid);
  Serial.println(wifi_password);
#endif
  if (wifi_password != "")
  {

    WiFi.begin(wifi_ssid, wifi_password);
  }
  else
  {
    WiFi.begin(wifi_ssid);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
#ifdef debug
    Serial.println("Wifi connect faild");
#endif debug
  }

  String page = "设置已保存，稍等在此页面上方查看是否连接成功";
  web.send(200, "text/html", page);
}
void handleReadSetting()
{
  // 用固定缓冲区与 snprintf 构建 JSON，减少 String 临时对象和堆碎片。
  char out[512];
  // 注意：不要把敏感信息（wifi 密码）打印到串口日志中。
  snprintf(out, sizeof(out), "{\"cdkey\":\"%s\",\"city\":\"%s\",\"client_name\":\"%s\",\"contrast\":\"%u\",\"epd\":\"%s\",\"showtime\":\"%u\",\"update_interval\":\"%d\",\"update_time\":\"%s\",\"wifi_ssid\":\"%s\",\"wifi_password\":\"%s\"}",
           cdkey.c_str(), city.c_str(), client_name.c_str(), contrast, epd_type.c_str(), showtime, sleeptime / 60, update_time_range.c_str(), WiFi.SSID().c_str(), WiFi.psk().c_str());

  web.send(200, "text/html", String(out));
}
void handleReset()
{
  WiFi.mode(WIFI_STA);
  exit_portal = 1;
}
void handleReadStatus()
{
  float voltage;
  long sum = 0;
  ADC_ON;
  for (int i = 0; i < 1000; i++)
    sum += analogRead(A0);
  ADC_OFF;
  voltage = (float)sum * 5.7 / 1024000;
  String sta_ssid;
  if (WiFi.SSID() != "")
    sta_ssid = WiFi.SSID();
  else
    sta_ssid = "未连接";
#ifdef debug
// Serial.println(sta_ssid);
#endif
  String page = "{\"sta_ssid\":\"" + sta_ssid + "\",\"onenet_id\":\"" + showtime + "\",\"batt_vol\":\"" + String(voltage) + "V\"}";
  web.send(200, "text/html", page);
}
void handleFile()
{
  String page;
  page += FPSTR(FILE_WEBPAGE);
  web.send(200, "text/html", page);
}

void handleFileList()
{
  String page = "";
  FSInfo fs_info;
  LittleFS.info(fs_info);
  page += "{\"used\":\"" + String(fs_info.usedBytes) + "/" + String(fs_info.totalBytes) + "\",\"percentage\":\"" + String(fs_info.usedBytes * 100 / fs_info.totalBytes) + "\",";
  page += "\"file_list\":[";
  Dir dir = LittleFS.openDir("/");
  byte i = 0;
  while (dir.next())
  {
    if (i != 0)
      page += ",";
    page += "{\"name\":\"" + dir.fileName() + "\",\"size\":\"" + dir.fileSize() + "\"}";
    i++;
  }
  page += "]}";
  web.send(200, "text/html", page);
#ifdef debug
  Serial.println(page);
#endif
}
void handleFileUpload()
{
  Serial.println("uploadpage");

  HTTPUpload &upload = web.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    // Make sure paths always start with "/"
    if (!filename.startsWith("/"))
    {
      filename = "/" + filename;
    }
    Serial.println(String("handleFileUpload Name: ") + filename);
    uploadFile = LittleFS.open(filename, "w+");
    if (!uploadFile)
    {
      return web.send(500, "CREATE FAILED");
    }
    Serial.println(String("Upload: START, filename: ") + filename);
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (uploadFile)
    {
      size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
      Serial.printf("currentsize=%d", upload.currentSize);
      /*for(int i=0;i<upload.currentSize;i++)
      {
        uploadFile.write(upload.buf[i]);
        Serial.print(upload.buf[i]+"  ");
        }*/
      if (bytesWritten != upload.currentSize)
      {
        return web.send(500, "WRITE FAILED");
      }
    }
    Serial.println(String("Upload: WRITE, Bytes: ") + upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (uploadFile)
    {
      /*uploadFile.seek(0,SeekSet);
      while(uploadFile.available()){
     Serial.println(uploadFile.read());
         }*/
      uploadFile.close();
      Serial.println("closed1");
    }

    Serial.println(String("Upload: END, Size: ") + upload.totalSize);
    showpic();
  }
}
void showpic()
{
  EPD.EPD_init_Full();
  EPD.ReadBusy();
  EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer, 3);

  EPD.clearbuffer();
  EPD.EPD_Write((unsigned char *)LUT_gray_opm42, sizeof(LUT_gray_opm42));
  EPD.ReadBusy();
  EPD.EPD_WriteCMD(0x22);
  EPD.EPD_WriteData(0xc5);
  EPD.EPD_WriteCMD(0x04);
  EPD.EPD_WriteData(0x32);
  EPD.EPD_WriteData(0xa8);
  EPD.EPD_WriteData(0x32);
  for (int i = 16; i > 0; i--)
  {
    EPD.DrawXbm_spiff_gray(0, 0, 400, 300, i);
    EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer, 1);
  }
  EPD.EPD_WriteCMD(0x04);
  EPD.EPD_WriteData(0x41);
  EPD.EPD_WriteData(0xa8);
  EPD.EPD_WriteData(0x32);
}
void StartPortal()
{

#ifdef debug
  Serial.println("Changing to AP Mode");
#endif
  WiFi.setOutputPower(10.5);
  delay(10);
  WiFi.mode(WIFI_AP_STA);
  delay(10);
  WiFi.softAP("Epaper Weather Station", "", 11, 0, 4);
  delay(10);
  web.on("/", handleRoot);
  web.on("/index", handleRoot);
  web.on("/js", handleJs);
  web.on("/saveconfig", handleSaveConfig);
  web.on("/readsettings", handleReadSetting);
  web.on("/reset", handleReset);
  web.on("/readstatus", handleReadStatus);
  web.on("/photo", handlePhoto);
  web.on("/file", handleFile);
  web.on("/get_file_list", handleFileList);
  web.on("/upload", HTTP_POST, []()
         { web.send(200); }, handleFileUpload);
  web.begin();
// web.setNoDelay(true);
#ifdef debug
// Serial.println("Init EPD\n");
#endif
  EPD.EPD_Set_Model(epd_type_index);
  EPD.EPD_init_Full();

  EPD.clearbuffer();
  EPD.fontscale = 2;
  EPD.SetFont(FONT12);
  if (crc == false)
  {
    EPD.DrawUTF(0, 0, "FLASH校验失败！！");
    EPD.DrawUTF(36, 0, "程序不完整或已更改");
    EPD.DrawUTF(72, 0, "无法正常运行");
    EPD.DrawUTF(104, 0, "请重新烧录");
  }
  else
  {
    EPD.DrawUTF(0, 0, "接入点已启动");
    EPD.DrawUTF(30, 0, "请用手机连接 WIFI :");
    EPD.DrawUTF(60, 0, "Epaper Weather Station");
    EPD.DrawUTF(90, 0, "浏览器打开192.168.4.1");
    EPD.fontscale = 1;
    long espid = ESP.getChipId();
    espid = (espid >> (32 - 6)) | (espid << 6);
    espid ^= 114987395;
    EPD.DrawUTF(128 - 12, 0, "设备ID" + String(espid));
  }
  EPD.EPD_Dis_Full((unsigned char *)EPD.EPDbuffer, 1);
#ifdef debug
// Serial.println("Web server running");
#endif
  while (1)
  {
    web.handleClient();
    if (exit_portal == 1)
    {
      check_rtc_mem(1);
      break;
    }
  }
}
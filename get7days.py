# 每日天气预报
# 平台: API iOS Android
# 每日天气预报API，提供全球城市未来3-30天天气预报，包括：日出日落、月升月落、最高最低温度、天气白天和夜间状况、风力、风速、风向、相对湿度、大气压强、降水量、露点温度、紫外线强度、能见度等。

# 请求路径 
# /v7/weather/{days}
# 参数 
# 路径参数
# days(必选)预报天数，支持最多30天预报，可选值：
# 3d 3天预报。
# 7d 7天预报。
# 10d 10天预报。
# 15d 15天预报。
# 30d 30天预报。
# 查询参数
# location(必选)需要查询地区的LocationID或以英文逗号分隔的经度,纬度坐标（十进制，最多支持小数点后两位），LocationID可通过GeoAPI获取。例如 location=101010100 或 location=116.41,39.92
# lang多语言设置，请阅读多语言文档，了解我们的多语言是如何工作、如何设置以及数据是否支持多语言。
# unit数据单位设置，可选值包括unit=m（公制单位，默认）和unit=i（英制单位）。更多选项和说明参考度量衡单位。
# 请求示例 
# curl -X GET --compressed \
# -H 'AuthorizationJWT: Bearer your_token' \
# 'https://your_api_host/v7/weather/3d?location=101010100'
# 请将your_token替换为你的JWT身份认证，将your_api_host替换为你的API Host

# 返回数据 
# 返回数据是JSON格式并进行了Gzip压缩。

# {
#   "code": "200",
#   "updateTime": "2021-11-15T16:35+08:00",
#   "fxLink": "http://hfx.link/2ax1",
#   "daily": [
#     {
#       "fxDate": "2021-11-15",
#       "sunrise": "06:58",
#       "sunset": "16:59",
#       "moonrise": "15:16",
#       "moonset": "03:40",
#       "moonPhase": "盈凸月",
#       "moonPhaseIcon": "803",
#       "tempMax": "12",
#       "tempMin": "-1",
#       "iconDay": "101",
#       "textDay": "多云",
#       "iconNight": "150",
#       "textNight": "晴",
#       "wind360Day": "45",
#       "windDirDay": "东北风",
#       "windScaleDay": "1-2",
#       "windSpeedDay": "3",
#       "wind360Night": "0",
#       "windDirNight": "北风",
#       "windScaleNight": "1-2",
#       "windSpeedNight": "3",
#       "humidity": "65",
#       "precip": "0.0",
#       "pressure": "1020",
#       "vis": "25",
#       "cloud": "4",
#       "uvIndex": "3"
#     },
#     {
#       "fxDate": "2021-11-16",
#       "sunrise": "07:00",
#       "sunset": "16:58",
#       "moonrise": "15:38",
#       "moonset": "04:40",
#       "moonPhase": "盈凸月",
#       "moonPhaseIcon": "803",
#       "tempMax": "13",
#       "tempMin": "0",
#       "iconDay": "100",
#       "textDay": "晴",
#       "iconNight": "101",
#       "textNight": "多云",
#       "wind360Day": "225",
#       "windDirDay": "西南风",
#       "windScaleDay": "1-2",
#       "windSpeedDay": "3",
#       "wind360Night": "225",
#       "windDirNight": "西南风",
#       "windScaleNight": "1-2",
#       "windSpeedNight": "3",
#       "humidity": "74",
#       "precip": "0.0",
#       "pressure": "1016",
#       "vis": "25",
#       "cloud": "1",
#       "uvIndex": "3"
#     },
#     {
#       "fxDate": "2021-11-17",
#       "sunrise": "07:01",
#       "sunset": "16:57",
#       "moonrise": "16:01",
#       "moonset": "05:41",
#       "moonPhase": "盈凸月",
#       "moonPhaseIcon": "803",
#       "tempMax": "13",
#       "tempMin": "0",
#       "iconDay": "100",
#       "textDay": "晴",
#       "iconNight": "150",
#       "textNight": "晴",
#       "wind360Day": "225",
#       "windDirDay": "西南风",
#       "windScaleDay": "1-2",
#       "windSpeedDay": "3",
#       "wind360Night": "225",
#       "windDirNight": "西南风",
#       "windScaleNight": "1-2",
#       "windSpeedNight": "3",
#       "humidity": "56",
#       "precip": "0.0",
#       "pressure": "1009",
#       "vis": "25",
#       "cloud": "0",
#       "uvIndex": "3"
#     }
#   ],
#   "refer": {
#     "sources": [
#       "QWeather",
#       "NMC",
#       "ECMWF"
#     ],
#     "license": [
#       "QWeather Developers License"
#     ]
#   }
# }
# code 请参考状态码
# updateTime 当前API的最近更新时间
# fxLink 当前数据的响应式页面，便于嵌入网站或应用
# daily.fxDate 预报日期
# daily.sunrise 日出时间，在高纬度地区可能为空
# daily.sunset 日落时间，在高纬度地区可能为空
# daily.moonrise 当天月升时间，可能为空
# daily.moonset 当天月落时间，可能为空
# daily.moonPhase 月相名称
# daily.moonPhaseIcon 月相图标代码，另请参考天气图标项目
# daily.tempMax 预报当天最高温度
# daily.tempMin 预报当天最低温度
# daily.iconDay 预报白天天气状况的图标代码，另请参考天气图标项目
# daily.textDay 预报白天天气状况文字描述，包括阴晴雨雪等天气状态的描述
# daily.iconNight 预报夜间天气状况的图标代码，另请参考天气图标项目
# daily.textNight 预报晚间天气状况文字描述，包括阴晴雨雪等天气状态的描述
# daily.wind360Day 预报白天风向360角度
# daily.windDirDay 预报白天风向
# daily.windScaleDay 预报白天风力等级
# daily.windSpeedDay 预报白天风速，公里/小时
# daily.wind360Night 预报夜间风向360角度
# daily.windDirNight 预报夜间当天风向
# daily.windScaleNight 预报夜间风力等级
# daily.windSpeedNight 预报夜间风速，公里/小时
# daily.precip 预报当天总降水量，默认单位：毫米
# daily.uvIndex 紫外线强度指数
# daily.humidity 相对湿度，百分比数值
# daily.pressure 大气压强，默认单位：百帕
# daily.vis 能见度，默认单位：公里
# daily.cloud 云量，百分比数值。可能为空
# refer.sources 原始数据来源，或数据源说明，可能为空
# refer.license 数据许可或版权声明，可能为空

import requests
import json  # 新增：导入json模块

# 请求配置（替换为你的实际参数）
url = "https://k552q9ct44.re.qweatherapi.com/v7/weather/7d"
params = {
    "location": "116.38,39.91"  # 城市LocationID（北京）
}
headers = {
    # Bearer令牌（替换为你的有效令牌）
    "AuthorizationJWT": "Bearer eyJhbGciOiJFZERTQSIsImtpZCI6IkNDNUJURkhZUDkiLCJ0eXAiOiJKV1QifQ.eyJpYXQiOjE3NjE0ODU4OTksImV4cCI6MTc2MTQ4ODkyOSwic3ViIjoiNEEyRkpNREZBQiJ9.2vfwf5Iz9rO9l-hz5HyypJ-LYbCk0AFc7sThrovnGoAyb5l8yYqeBm2eO8pp_iJ2a5X-9AP-IJPhMIYAKC-zDA",
    "Accept-Encoding": "gzip, deflate"  # 支持压缩（对应curl的--compressed）
}

try:
    # 发送GET请求（启用压缩支持）
    response = requests.get(
        url,
        params=params,
        headers=headers,
        timeout=10,
        stream=True  # 支持流式处理压缩数据
    )
    
    # 打印响应状态码
    print(f"响应状态码: {response.status_code}")
    
    # 打印响应内容（已自动解压）
    print("响应内容:")
    print(json.dumps(response.json(), ensure_ascii=False, indent=2))  # 现在可以正常使用json模块了

except requests.exceptions.RequestException as e:
    # 捕获请求异常（网络错误、超时等）
    print(f"请求失败: {str(e)}")

# 响应状态码: 200
# 响应内容:
# {
#   "code": "200",
#   "updateTime": "2025-10-26T21:42+08:00",
#   "fxLink": "https://www.qweather.com/weather/xicheng-101011700.html",
#   "daily": [
#     {
#       "fxDate": "2025-10-26",
#       "sunrise": "06:37",
#       "sunset": "17:21",
#       "moonrise": "11:13",
#       "moonset": "20:01",
#       "moonPhase": "蛾眉月",
#       "moonPhaseIcon": "801",
#       "tempMax": "13",
#       "tempMin": "0",
#       "iconDay": "100",
#       "textDay": "晴",
#       "iconNight": "150",
#       "textNight": "晴",
#       "wind360Day": "315",
#       "windDirDay": "西北风",
#       "windScaleDay": "1-3",
#       "windSpeedDay": "3",
#       "wind360Night": "315",
#       "windDirNight": "西北风",
#       "windScaleNight": "1-3",
#       "windSpeedNight": "3",
#       "humidity": "26",
#       "precip": "0.0",
#       "pressure": "1019",
#       "vis": "25",
#       "cloud": "0",
#       "uvIndex": "3"
#     },
#     {
#       "fxDate": "2025-10-27",
#       "sunrise": "06:38",
#       "sunset": "17:20",
#       "moonrise": "12:03",
#       "moonset": "20:58",
#       "moonPhase": "蛾眉月",
#       "moonPhaseIcon": "801",
#       "tempMax": "14",
#       "tempMin": "1",
#       "iconDay": "100",
#       "textDay": "晴",
#       "iconNight": "150",
#       "textNight": "晴",
#       "wind360Day": "225",
#       "windDirDay": "西南风",
#       "windScaleDay": "1-3",
#       "windSpeedDay": "3",
#       "wind360Night": "0",
#       "windDirNight": "北风",
#       "windScaleNight": "1-3",
#       "windSpeedNight": "3",
#       "humidity": "41",
#       "precip": "0.0",
#       "pressure": "1017",
#       "vis": "25",
#       "cloud": "0",
#       "uvIndex": "3"
#     },
#     {
#       "fxDate": "2025-10-28",
#       "sunrise": "06:39",
#       "sunset": "17:19",
#       "moonrise": "12:48",
#       "moonset": "22:02",
#       "moonPhase": "蛾眉月",
#       "moonPhaseIcon": "801",
#       "tempMax": "14",
#       "tempMin": "4",
#       "iconDay": "100",
#       "textDay": "晴",
#       "iconNight": "305",
#       "textNight": "小雨",
#       "wind360Day": "90",
#       "windDirDay": "东风",
#       "windScaleDay": "1-3",
#       "windSpeedDay": "3",
#       "wind360Night": "0",
#       "windDirNight": "北风",
#       "windScaleNight": "1-3",
#       "windSpeedNight": "3",
#       "humidity": "58",
#       "precip": "0.0",
#       "pressure": "1014",
#       "vis": "25",
#       "cloud": "1",
#       "uvIndex": "3"
#     },
#     {
#       "fxDate": "2025-10-29",
#       "sunrise": "06:40",
#       "sunset": "17:18",
#       "moonrise": "13:25",
#       "moonset": "23:09",
#       "moonPhase": "上弦月",
#       "moonPhaseIcon": "802",
#       "tempMax": "15",
#       "tempMin": "6",
#       "iconDay": "101",
#       "textDay": "多云",
#       "iconNight": "151",
#       "textNight": "多云",
#       "wind360Day": "225",
#       "windDirDay": "西南风",
#       "windScaleDay": "1-3",
#       "windSpeedDay": "3",
#       "wind360Night": "0",
#       "windDirNight": "北风",
#       "windScaleNight": "1-3",
#       "windSpeedNight": "3",
#       "humidity": "38",
#       "precip": "0.0",
#       "pressure": "1019",
#       "vis": "25",
#       "cloud": "17",
#       "uvIndex": "3"
#     },
#     {
#       "fxDate": "2025-10-30",
#       "sunrise": "06:41",
#       "sunset": "17:16",
#       "moonrise": "13:56",
#       "moonset": "",
#       "moonPhase": "盈凸月",
#       "moonPhaseIcon": "803",
#       "tempMax": "15",
#       "tempMin": "7",
#       "iconDay": "101",
#       "textDay": "多云",
#       "iconNight": "305",
#       "textNight": "小雨",
#       "wind360Day": "45",
#       "windDirDay": "东北风",
#       "windScaleDay": "1-3",
#       "windSpeedDay": "3",
#       "wind360Night": "315",
#       "windDirNight": "西北风",
#       "windScaleNight": "1-3",
#       "windSpeedNight": "3",
#       "humidity": "57",
#       "precip": "0.0",
#       "pressure": "1016",
#       "vis": "25",
#       "cloud": "0",
#       "uvIndex": "3"
#     },
#     {
#       "fxDate": "2025-10-31",
#       "sunrise": "06:43",
#       "sunset": "17:15",
#       "moonrise": "14:24",
#       "moonset": "00:18",
#       "moonPhase": "盈凸月",
#       "moonPhaseIcon": "803",
#       "tempMax": "15",
#       "tempMin": "6",
#       "iconDay": "104",
#       "textDay": "阴",
#       "iconNight": "150",
#       "textNight": "晴",
#       "wind360Day": "225",
#       "windDirDay": "西南风",
#       "windScaleDay": "1-3",
#       "windSpeedDay": "3",
#       "wind360Night": "315",
#       "windDirNight": "西北风",
#       "windScaleNight": "1-3",
#       "windSpeedNight": "3",
#       "humidity": "32",
#       "precip": "0.0",
#       "pressure": "1014",
#       "vis": "25",
#       "cloud": "1",
#       "uvIndex": "3"
#     },
#     {
#       "fxDate": "2025-11-01",
#       "sunrise": "06:44",
#       "sunset": "17:14",
#       "moonrise": "14:49",
#       "moonset": "01:28",
#       "moonPhase": "盈凸月",
#       "moonPhaseIcon": "803",
#       "tempMax": "14",
#       "tempMin": "5",
#       "iconDay": "100",
#       "textDay": "晴",
#       "iconNight": "150",
#       "textNight": "晴",
#       "wind360Day": "315",
#       "windDirDay": "西北风",
#       "windScaleDay": "1-3",
#       "windSpeedDay": "3",
#       "wind360Night": "315",
#       "windDirNight": "西北风",
#       "windScaleNight": "1-3",
#       "windSpeedNight": "3",
#       "humidity": "34",
#       "precip": "0.0",
#       "pressure": "1021",
#       "vis": "25",
#       "cloud": "0",
#       "uvIndex": "3"
#     }
#   ],
#   "refer": {
#     "sources": [
#       "QWeather"
#     ],
#     "license": [
#       "QWeather Developers License"
#     ]
#   }
# }
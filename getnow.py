# 实时天气
# 平台: API iOS Android
# 获取中国3000+市县区和海外20万个城市实时天气数据，包括实时温度、体感温度、风力风向、相对湿度、大气压强、降水量、能见度、露点温度、云量等。

# 注意：实况数据均为近实时数据，相比真实的物理世界有5-20分钟的延迟，请根据实况数据中的obsTime确定数据对应的准确时间。

# 请求路径 
# /v7/weather/now
# 参数 
# 查询参数
# location(必选)需要查询地区的LocationID或以英文逗号分隔的经度,纬度坐标（十进制，最多支持小数点后两位），LocationID可通过GeoAPI获取。例如 location=101010100 或 location=116.41,39.92
# lang多语言设置，请阅读多语言文档，了解我们的多语言是如何工作、如何设置以及数据是否支持多语言。
# unit数据单位设置，可选值包括unit=m（公制单位，默认）和unit=i（英制单位）。更多选项和说明参考度量衡单位。
# 请求示例 
# curl -X GET --compressed \
# -H 'AuthorizationJWT: Bearer your_token' \
# 'https://your_api_host/v7/weather/now?location=101010100'
# 请将your_token替换为你的JWT身份认证，将your_api_host替换为你的API Host

# 返回数据 
# 返回数据是JSON格式并进行了Gzip压缩。

# {
#   "code": "200",
#   "updateTime": "2020-06-30T22:00+08:00",
#   "fxLink": "http://hfx.link/2ax1",
#   "now": {
#     "obsTime": "2020-06-30T21:40+08:00",
#     "temp": "24",
#     "feelsLike": "26",
#     "icon": "101",
#     "text": "多云",
#     "wind360": "123",
#     "windDir": "东南风",
#     "windScale": "1",
#     "windSpeed": "3",
#     "humidity": "72",
#     "precip": "0.0",
#     "pressure": "1003",
#     "vis": "16",
#     "cloud": "10",
#     "dew": "21"
#   },
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
# now.obsTime 数据观测时间
# now.temp 温度，默认单位：摄氏度
# now.feelsLike 体感温度，默认单位：摄氏度
# now.icon 天气状况的图标代码，另请参考天气图标项目
# now.text 天气状况的文字描述，包括阴晴雨雪等天气状态的描述
# now.wind360 风向360角度
# now.windDir 风向
# now.windScale 风力等级
# now.windSpeed 风速，公里/小时
# now.humidity 相对湿度，百分比数值
# now.precip 过去1小时降水量，默认单位：毫米
# now.pressure 大气压强，默认单位：百帕
# now.vis 能见度，默认单位：公里
# now.cloud 云量，百分比数值。可能为空
# now.dew 露点温度。可能为空
# refer.sources 原始数据来源，或数据源说明，可能为空
# refer.license 数据许可或版权声明，可能为空

import requests
import json  # 新增：导入json模块

# 请求配置（替换为你的实际参数）
url = "https://k552q9ct44.re.qweatherapi.com/v7/weather/now"
params = {
    "location": "101010100"  # 城市LocationID（北京）
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
#   "updateTime": "2025-10-26T21:36+08:00",
#   "fxLink": "https://www.qweather.com/weather/beijing-101010100.html",
#   "now": {
#     "obsTime": "2025-10-26T21:32+08:00",
#     "temp": "6",
#     "feelsLike": "4",
#     "icon": "150",
#     "text": "晴",
#     "wind360": "225",
#     "windDir": "西南风",
#     "windScale": "0",
#     "windSpeed": "1",
#     "humidity": "36",
#     "precip": "0.0",
#     "pressure": "1023",
#     "vis": "30",
#     "cloud": "0",
#     "dew": "-13"
#   },
#   "refer": {
#     "sources": [
#       "QWeather"
#     ],
#     "license": [
#       "QWeather Developers License"
#     ]
#   }
# }
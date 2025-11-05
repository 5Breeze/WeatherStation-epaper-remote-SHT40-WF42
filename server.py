#!/usr/bin/env python3
"""
dynamic_weather_server.py

每次 POST /weather.php 都动态生成天气 JSON。
改进点：
 - 支持 lut1 为经纬度("lon,lat") 或地名(如 "Beijing")（会尝试用高德地理编码转经纬度）
 - JWT 生成失败时自动降级为无认证调用（避免因秘钥问题直接失败）
 - 请求失败时使用本地 MOCK_RESPONSE 作为离线回退
 - 更详细的日志，记录返回的字节数和完整响应（便于排查 "101 bytes" 问题）
 - 将 lut4、bssid、ssid 等写入 request_log.txt

用法:
  python dynamic_weather_server.py --port 8000

测试示例:
  curl -X POST -d "lut1=116.31,39.95&lut4=12345&bssid=AA:BB:CC&ssid=MyWiFi" http://127.0.0.1:8000/weather.php
"""

from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import parse_qs, urlparse
import json
import logging
import time
import argparse
from datetime import datetime
import requests
import jwt
import lunarcalendar
from operator import itemgetter

# === 配置区（请按需替换 API KEY / 私钥） ===
GAODE_KEY = "1c038d4300你的api93a821239b888"
PRIVATE_KEY = """-----BEGIN PRIVATE KEY-----
MC4CAQA55DK2VwBCI"你的私钥KEY"zq4112ao+RAKA7YDu7b24PADTxiUzCmm
-----END PRIVATE KEY-----"""
KID = "CC5 你的KID FHYP9"
SUB = "4A2F 你的KID MDFAB"

# 当外部 API 失败时的本地 mock（确保响应结构一致）
MOCK_RESPONSE = {
  "now": {"cond":"晴","cond_index":"0","hum":"50","tmp":"10","dir":"东","sc":"1","fl":"10","pcpn":"0.0","vis":"10","pres":"1020"},
  "city":"北京市朝阳区望京街道",
  "daily":{
    "week":"今天,明天,周二,周三,周四,周五,周六",
    "tmin":"0,1,2,3,4,5,6",
    "tmax":"10,11,12,13,14,15,16",
    "code_d_index":"0,0,2,2,4,4,0",
    "code_n_index":"0,25,25,2,2,0,0",
    "text_d":"晴,晴,多云,多云,阴,小雨,晴",
    "text_n":"晴,小雨,小雨,多云,阴,晴,晴",
    "day":"2025-10-26,2025-10-27,2025-10-28,2025-10-29,2025-10-30,2025-10-31,2025-11-01"
  },
  "message":"离线模式：使用本地 MOCK 数据",
  "year":datetime.now().strftime("%Y年%m月%d日 %A"),
  "nongli":"乙巳蛇年九月初五",
  "t":datetime.now().strftime("DATE: %a,%d %b %Y %H:%M:%S"),
  "status":"ok"
}

# ======= 工具函数 =======

def logv(msg):
    ts = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
    logging.info(f"[{ts}] {msg}")


def get_weather_index(weather_text):
    text = (weather_text or "").lower()
    if "晴" in text: return "0"
    if "多云" in text: return "2"
    if "阴" in text: return "4"
    if "霾" in text or "污染" in text: return "10"
    if "雨" in text: return "25"
    if "雪" in text: return "36"
    if "雾" in text: return "42"
    return ""


def try_generate_jwt(private_key, kid, sub, exp_seconds=3000):
    try:
        payload = {'iat': int(time.time()) - 30, 'exp': int(time.time()) + exp_seconds, 'sub': sub}
        headers = {'kid': kid}
        token = jwt.encode(payload, private_key, algorithm='EdDSA', headers=headers)
        return token
    except Exception as e:
        logv(f"JWT 生成失败，降级为无认证模式：{e}")
        return None


def geocode_place_to_coord(place_name):
    """当 lut1 不是 lon,lat 时，尝试使用高德地理编码把地名 -> 坐标(lon,lat)"""
    if not place_name:
        return None
    try:
        url = "https://restapi.amap.com/v3/geocode/geo"
        params = {"address": place_name, "key": GAODE_KEY, "city": ""}
        r = requests.get(url, params=params, timeout=8)
        r.raise_for_status()
        d = r.json()
        if d.get('status') == '1' and d.get('geocodes'):
            loc = d['geocodes'][0].get('location')  # 格式 "116.31,39.95"
            return loc
    except Exception as e:
        logv(f"高德地名转坐标失败: {e}")
    return None


def get_gaode_regeo_from_coord(location):
    """coord -> province+district+township（用于 city 字段）"""
    try:
        url = "https://restapi.amap.com/v3/geocode/regeo"
        params = {"location": location, "key": GAODE_KEY, "extensions": "all", "output": "json"}
        r = requests.get(url, params=params, timeout=8)
        r.raise_for_status()
        d = r.json()
        if d.get('status') == '1':
            ac = d.get('regeocode', {}).get('addressComponent', {})
            parts = [ac.get('province',''), ac.get('district',''), ac.get('township','')]
            return ''.join([p for p in parts if p])
    except Exception as e:
        logv(f"高德逆编码失败: {e}")
    return ""


def get_lunar_date():
    """计算当前日期对应的农历（格式：乙巳蛇年九月初五）"""
    try:
        from lunarcalendar import Converter, Solar
        from datetime import datetime

        # 当前公历日期
        today = datetime.now()
        solar = Solar(today.year, today.month, today.day)

        # 转换为农历
        lunar = Converter.Solar2Lunar(solar)

        # 天干地支与生肖映射
        gan = "甲乙丙丁戊己庚辛壬癸"
        zhi = "子丑寅卯辰巳午未申酉戌亥"
        zodiac = "鼠牛虎兔龙蛇马羊猴鸡狗猪"

        # 农历天干地支年份
        gz_year = gan[(lunar.year - 4) % 10] + zhi[(lunar.year - 4) % 12]
        shengxiao = zodiac[(lunar.year - 4) % 12]  # 生肖

        # 农历月份和日期中文
        month_names = ["正", "二", "三", "四", "五", "六", "七", "八", "九", "十", "冬", "腊"]
        day_prefix = ["初", "十", "廿", "三"]
        day_names = [""] + [p + "十" if i == 10 else p + "一二三四五六七八九"[i - 1]
                            for p in day_prefix for i in range(1, 11)]

        # 是否闰月
        month_str = ("闰" if lunar.isleap else "") + month_names[lunar.month - 1] + "月"
        day_str = day_names[lunar.day]

        return f"{gz_year}{shengxiao}年{month_str}{day_str}"
    except Exception as e:
        print(f"农历计算失败：{str(e)}")
        return "乙巳蛇年九月初五"





def safe_get(url, params=None, headers=None):
    try:
        r = requests.get(url, params=params, headers=headers, timeout=8)
        r.raise_for_status()
        return {"status_code": r.status_code, "data": r.json()}
    except Exception as e:
        return {"status_code": None, "error": str(e)}


# ======= 构建天气 JSON（主逻辑） =======

def build_weather_json(lut1_coord):
    """lut1_coord: 确保为 'lon,lat' 格式。如果 None 或者请求失败，会返回 MOCK_RESPONSE 作为回退。"""
    # API endpoints（和风示例）
    now_url = "https://k552q9ct44.re.qweatherapi.com/v7/weather/now"
    daily_url = "https://k552q9ct44.re.qweatherapi.com/v7/weather/7d"
    minutely_url = "https://k552q9ct44.re.qweatherapi.com/v7/minutely/5m"
    # alert 要求 lat/lon 顺序 -> 我们保留 lat,lon 的拼接
    try:
        lon, lat = [s.strip() for s in lut1_coord.split(',')]
    except Exception:
        logv("构建天气 JSON 失败：lut1_coord 非法")
        return {**MOCK_RESPONSE, "message": "输入坐标非法，返回离线 MOCK"}

    alert_url = f"https://k552q9ct44.re.qweatherapi.com/weatheralert/v1/current/{lat}/{lon}"

    # 先尝试生成 JWT（失败则降级为无认证）
    jwt_token = try_generate_jwt(PRIVATE_KEY, KID, SUB)
    headers = {"Accept-Encoding": "gzip, deflate"}
    if jwt_token:
        headers["Authorization"] = f"Bearer {jwt_token}"

    params = {"location": f"{lon},{lat}"}

    # 1) now
    now_result = safe_get(now_url, params=params, headers=headers)
    if not now_result.get('data'):
        logv(f"获取 now 失败: {now_result.get('error')}")
        return {**MOCK_RESPONSE, "message": "实时接口失败，使用 MOCK"}
    now_data = now_result['data'].get('now', {})
    now_text = now_data.get('text', '')

    # 2) daily
    daily_result = safe_get(daily_url, params=params, headers=headers)
    if not daily_result.get('data'):
        logv(f"获取 daily 失败: {daily_result.get('error')}")
        return {**MOCK_RESPONSE, "message": "预报接口失败，使用 MOCK"}

    # 3) alert
    alert_result = safe_get(alert_url, headers=headers)
    alerts = alert_result.get('data', {}).get('alerts', []) if alert_result.get('data') else []

    # 4) minutely
    minutely_result = safe_get(minutely_url, params=params, headers=headers)

    # 填充返回结构
    resp = {}
    resp['now'] = {
        'cond': now_text,
        'cond_index': get_weather_index(now_text),
        'hum': now_data.get('humidity',''),
        'tmp': now_data.get('temp',''),
        'dir': now_data.get('windDir',''),
        'sc': now_data.get('windScale',''),
        'fl': now_data.get('feelsLike',''),
        'pcpn': now_data.get('precip',''),
        'vis': now_data.get('vis',''),
        'pres': now_data.get('pressure','')
    }

    # daily 列表
    daily_list = daily_result['data'].get('daily', [])
    weeks, tmins, tmaxs, code_ds, code_ns, text_ds, text_ns, days = ([] for _ in range(8))
    for day in daily_list:
        date_str = day.get('fxDate','')
        if not date_str: continue
        date = datetime.strptime(date_str, "%Y-%m-%d")
        delta = (date.date() - datetime.now().date()).days
        if delta == 0: weeks.append('今天')
        elif delta == 1: weeks.append('明天')
        else:
            week_map = {1:'周一',2:'周二',3:'周三',4:'周四',5:'周五',6:'周六',7:'周日'}
            weeks.append(week_map.get(date.isoweekday(), ''))
        tmins.append(str(day.get('tempMin','')))
        tmaxs.append(str(day.get('tempMax','')))
        text_d = day.get('textDay','')
        text_n = day.get('textNight','')
        text_ds.append(text_d)
        text_ns.append(text_n)
        code_ds.append(get_weather_index(text_d))
        code_ns.append(get_weather_index(text_n))
        days.append(date_str)

    resp['daily'] = {
        'week': ','.join(weeks),
        'tmin': ','.join(tmins),
        'tmax': ','.join(tmaxs),
        'code_d_index': ','.join(code_ds),
        'code_n_index': ','.join(code_ns),
        'text_d': ','.join(text_ds),
        'text_n': ','.join(text_ns),
        'day': ','.join(days)
    }

    # city 字段：用逆地理编码获取可读地点（降级可用 lut1 原始值）
    city_name = get_gaode_regeo_from_coord(f"{lon},{lat}") or ''
    resp['city'] = city_name

    # message 优先预警 -> 降水摘要 -> 默认
    message = ''
    if alerts:
        try:
            latest = sorted(alerts, key=itemgetter('issuedTime'), reverse=True)[0]
            message = latest.get('headline') or latest.get('description','')
        except Exception:
            message = ''
    if not message:
        message = minutely_result.get('data', {}).get('summary','') if minutely_result.get('data') else ''
    if not message:
        message = f"今天{now_text}，适合出行"
    resp['message'] = message

    resp['year'] = datetime.now().strftime('%Y年%m月%d日 %A').replace('Monday','星期一').replace('Tuesday','星期二').replace('Wednesday','星期三').replace('Thursday','星期四').replace('Friday','星期五').replace('Saturday','星期六').replace('Sunday','星期日')
    resp['nongli'] = get_lunar_date()
    resp['t'] = datetime.now().strftime('DATE: %a,%d %b %Y %H:%M:%S')
    resp['status'] = 'ok'
    return resp


# ======= HTTP Handler =======
class DynamicWeatherHandler(BaseHTTPRequestHandler):
    server_version = "DynamicWeather/1.1"

    def do_POST(self):
        parsed = urlparse(self.path)
        logv(f"接收到 {self.command} {self.path} 来自 {self.client_address}")
        if parsed.path != '/weather.php':
            self.send_error(404, 'Not Found')
            return

        content_length = int(self.headers.get('Content-Length', 0))
        content_type = self.headers.get('Content-Type','')
        raw_body = self.rfile.read(content_length) if content_length>0 else b''
        body_text = raw_body.decode('utf-8', errors='replace')
        logv('---- Raw body ----')
        logv(body_text)

        form = {}
        if 'application/x-www-form-urlencoded' in content_type or '=' in body_text:
            qs = parse_qs(body_text, keep_blank_values=True)
            form = {k: v[0] if isinstance(v, list) and v else v for k,v in qs.items()}
            logv('---- Parsed form fields ----')
            for k,v in form.items(): logv(f"{k} = {v}")
        else:
            # 尝试解析 JSON
            if body_text.strip().startswith(('{','[')):
                try:
                    form = json.loads(body_text)
                    logv('---- Parsed JSON body ----')
                    logv(json.dumps(form, ensure_ascii=False, indent=2))
                except Exception as e:
                    logv(f"解析请求体为 JSON 失败: {e}")

        # 从 form 取出关键字段
        lut1_raw = form.get('lut1','').strip()
        # lut1 有可能是地名（如 "Beijing"）或经纬度 "116.31,39.95"
        lut1_coord = None
        if ',' in lut1_raw:
            parts = [p.strip() for p in lut1_raw.split(',')]
            if len(parts) == 2:
                try:
                    float(parts[0]); float(parts[1])
                    lut1_coord = f"{parts[0]},{parts[1]}"
                except Exception:
                    lut1_coord = None
        if not lut1_coord and lut1_raw:
            # 尝试把地名转为坐标
            loc = geocode_place_to_coord(lut1_raw)
            if loc:
                lut1_coord = loc
            else:
                logv(f"无法把 lut1 ({lut1_raw}) 转为坐标，使用默认坐标。")
                lut1_coord = '116.38,39.91'

        lut4 = form.get('lut4','')
        bssid = form.get('bssid','')
        ssid = form.get('ssid','')

        # 追加写入 request_log.txt
        try:
            with open('request_log.txt','a', encoding='utf-8') as f:
                f.write(f"[{datetime.now().isoformat()}] lut4={lut4}, bssid={bssid}, ssid={ssid}, raw_lut1={lut1_raw}\n")
        except Exception as e:
            logv(f"写入 request_log.txt 失败: {e}")

        # 生成天气 JSON
        try:
            weather_json = build_weather_json(lut1_coord)
        except Exception as e:
            logv(f"生成天气 JSON 异常: {e}")
            weather_json = {**MOCK_RESPONSE, 'message': f'生成天气失败: {e}'}

        resp_text = json.dumps(weather_json, ensure_ascii=False)
        resp_bytes = resp_text.encode('utf-8')

        self.send_response(200)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(resp_bytes)))
        self.end_headers()
        self.wfile.write(resp_bytes)

        logv(f"Returned {len(resp_bytes)} bytes; preview: {resp_text[:200]}")


# ======= run server =======

def run(host='0.0.0.0', port=8000):
    logging.basicConfig(level=logging.INFO, format='%(message)s')
    httpd = HTTPServer((host, port), DynamicWeatherHandler)
    logging.info(f"🌤 动态天气服务器已启动：http://{host}:{port} (POST /weather.php)")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        logging.info('Shutting down...')
        httpd.server_close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='0.0.0.0')
    parser.add_argument('--port', type=int, default=8000)
    args = parser.parse_args()
    run(args.host, args.port)

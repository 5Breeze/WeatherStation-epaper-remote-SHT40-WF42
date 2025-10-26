#!/usr/bin/env python3
"""
mock_server.py

轻量的 HTTP 服务器（无外部依赖），用于模拟 `https://epdweather.thinkermaker.xyz/weather.php` 接口。
接收 POST /weather.php（application/x-www-form-urlencoded），打印详细日志并返回示例 JSON 响应。

用法（PowerShell）:
 python .\mock_server.py --port 8000

测试（PowerShell）:
 Invoke-RestMethod -Method Post -Uri http://localhost:8000/weather.php -Body @{lut1="Beijing"; lut2="zh-cn"; lut3="news"; lut4="12345"; lut5="MYKEY"} -Verbose

或者使用 requests:
 pip install requests
 python -c "import requests; print(requests.post('http://localhost:8000/weather.php', data={'lut1':'Beijing','lut2':'zh-cn','lut3':'news','lut4':'12345','lut5':'MYKEY'}).text)"

"""

from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import parse_qs, urlparse
import json
import argparse
import logging
import time

MOCK_RESPONSE = {
    "now": {
        "cond": "晴",#天气
        "cond_index": "4",#天气代码
        #cond_index说明
# 0: 晴天
# 1-3: 多云
# 4: 阴天
# 5-18: 霾/污染天气
# 19-32: 各种降雨天气（在代码中当遇到这个范围时会设置 rain=1）
# 33-40: 雪天气
# 41-43: 雾天气
        "hum": "45",#湿度notused
        "tmp": "20",#温度notused
        "dir": "NE",#风向
        "sc": "3",#风力
        "fl": "20",#体感温度
        "pcpn": "0.0",#降水量cm
        "vis": "10",#能见度km
        "pres": "1013"#大气压hpa
    },
    "city": "北京",
      "daily": {
    "week": "今天,明天,周三,周四,周五,周六",#used
    "tmin": "12,10,8,5,7,9",#used
    "tmax": "20,18,15,12,14,16",#used
    "code_d_index": "0,1,4,19,2,3",#used
    "code_n_index": "1,2,19,20,4,0",#used
    "text_d": "晴,多云,阴,小雨,阴,多云",#used
    "text_n": "多云,阴,小雨,中雨,阴,晴",#used
    "day": "10-25,10-26,10-27,10-28,10-29,10-30"
  },
    "city": "北京",
    "message": "今天空气质量良好，适合出行",#消息
    "year": "2025年10月25日 星期五",#日期right
    "nongli": "乙巳蛇年九月初五",#农历
    "t": "DATE: Sat,25 Oct 2025 23:28:30",#系统时间base
    "status": "ok"
}

class MockHandler(BaseHTTPRequestHandler):
    server_version = "MockWeather/0.1"

    def log_verbose(self, msg):
        # 标准化日志输出
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
        logging.info(f"[{timestamp}] {msg}")

    def do_GET(self):
        self.log_verbose(f"GET {self.path} from {self.client_address}")
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.end_headers()
        self.wfile.write(b"Mock weather server. POST /weather.php to get JSON response.\n")

    def do_POST(self):
        parsed = urlparse(self.path)
        self.log_verbose(f"POST {self.path} from {self.client_address}")

        content_length = int(self.headers.get('Content-Length', 0))
        content_type = self.headers.get('Content-Type', '')

        self.log_verbose("---- Request headers ----")
        for k, v in self.headers.items():
            self.log_verbose(f"{k}: {v}")

        raw_body = self.rfile.read(content_length) if content_length > 0 else b''
        try:
            body_text = raw_body.decode('utf-8', errors='replace')
        except Exception:
            body_text = str(raw_body)
        self.log_verbose("---- Raw body ----")
        self.log_verbose(body_text)

        # Parse form data if urlencoded
        form = {}
        if 'application/x-www-form-urlencoded' in content_type:
            qs = parse_qs(body_text, keep_blank_values=True)
            # parse_qs -> values are lists, keep simple
            form = {k: v[0] if isinstance(v, list) and len(v) > 0 else v for k, v in qs.items()}
            self.log_verbose("---- Parsed form fields ----")
            for k, v in form.items():
                self.log_verbose(f"{k} = {v}")
        else:
            # try JSON
            if 'application/json' in content_type or body_text.startswith(('{', '[')):
                try:
                    form = json.loads(body_text)
                    self.log_verbose("---- Parsed JSON body ----")
                    self.log_verbose(json.dumps(form, ensure_ascii=False, indent=2))
                except Exception as e:
                    self.log_verbose(f"Failed to parse JSON body: {e}")

        # Very verbose: full client address and connection info
        self.log_verbose(f"Client address: {self.client_address}")

        # Only respond for /weather.php to mimic original server
        if parsed.path == '/weather.php':
            # Optionally you could check parameters here (lut5 cdkey) and return 401 if missing
            # For now always return MOCK_RESPONSE
            resp_text = json.dumps(MOCK_RESPONSE, ensure_ascii=False)
            resp_bytes = resp_text.encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'application/json; charset=utf-8')
            self.send_header('Content-Length', str(len(resp_bytes)))
            self.end_headers()
            self.wfile.write(resp_bytes)
            self.log_verbose(f"Returned mock JSON ({len(resp_bytes)} bytes)")
        else:
            # Unknown endpoint
            self.send_response(404)
            self.send_header('Content-Type', 'text/plain; charset=utf-8')
            self.end_headers()
            self.wfile.write(b"Not Found\n")
            self.log_verbose("Returned 404")

    def log_message(self, format, *args):
        # override default to route through logging
        self.log_verbose(format % args)


def run(host='0.0.0.0', port=8000):
    logging.basicConfig(level=logging.INFO, format='%(message)s')
    httpd = HTTPServer((host, port), MockHandler)
    logging.info(f"Mock server listening on http://{host}:{port} (POST /weather.php)")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        logging.info('Shutting down...')
        httpd.server_close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run mock weather PHP server')
    parser.add_argument('--host', default='0.0.0.0', help='listen host')
    parser.add_argument('--port', type=int, default=8000, help='listen port')
    args = parser.parse_args()
    run(host=args.host, port=args.port)

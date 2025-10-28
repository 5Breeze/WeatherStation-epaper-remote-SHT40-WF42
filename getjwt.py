#!/usr/bin/env python3
import sys
import time
import jwt

# Open PEM
private_key = """-----BEGIN PRIVATE KEY-----
MC4CAQAwBQYDK2VwBCI你的KEYsQnyjAao+RAKA7YDu7b24PADTxiUzCmm
-----END PRIVATE KEY-----"""

payload = {
    'iat': int(time.time()) - 30,
    'exp': int(time.time()) + 3000,
    'sub': '4A2你的SUBDFAB'
}
headers = {
    'kid': 'CC5B你的KIDHYP9'
}

# Generate JWT
encoded_jwt = jwt.encode(payload, private_key, algorithm='EdDSA', headers = headers)

print(f"JWT:  {encoded_jwt}")

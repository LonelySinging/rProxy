import os 
import requests

UA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 Edg/91.0.864.67"
headers = {
    "Charset":"UTF-8",
    "Accept":"*/*",
    "User-Agent":UA,
    "Connection":"Keep-Alive",
    "Accept-Encoding": "gzip"
}

def open_html(url):
    proxies = {'http':'http://39.106.164.33:7201'}
    # proxies = {'https':'https://127.0.0.1:7201'}
    # proxies = {'https':'https://39.106.164.33:7201'}
    # proxies = {'http':'http://127.0.0.1:7201'}
    r = requests.get(url, proxies=proxies, headers=headers, timeout=10)
    return r.text
    
print(open_html("http://xint.top:9999"))
# print(open_html("https://baidu.com"))
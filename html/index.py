#!/usr/bin/env python3
import os
import http.cookies

cookie_header = os.environ.get('HTTP_COOKIE', '')
cookies = http.cookies.SimpleCookie(cookie_header)

count = 0
if 'visit_count' in cookies:
    try:
        count = int(cookies['visit_count'].value)
    except ValueError:
        count = 0

count += 1
new_cookie = http.cookies.SimpleCookie()
new_cookie['visit_count'] = str(count)

print("Status: 200 OK")
print("Content-Type: text/plain")
print(new_cookie.output())
print("")
print(f"// welcome to webserv ({count})")

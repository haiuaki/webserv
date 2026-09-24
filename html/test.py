import sys
import os

print("Status: 200 OK")
print("Content-Type: text/plain")
print("")

print("Hello from Python CGI!")
print("Method used: " + os.environ.get("REQUEST_METHOD", "UNKNOWN"))

if os.environ.get("REQUEST_METHOD") == "POST":
    body = sys.stdin.read()
    print("Body received: " + body)

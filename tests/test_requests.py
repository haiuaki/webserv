import socket
import time
import sys

# Color codes for output
GREEN = '\033[92m'
CYAN = '\033[96m'
MAGENTA = '\033[95m'
RED = '\033[91m'
RESET = '\033[0m'

PORT = 8080
HOST = '127.0.0.1'

def print_request(req, title="Sending Request"):
    # Remove trailing newlines and carriage returns for a tighter display
    clean_req = req.replace('\r', '').strip()
    padded = "\n".join([f"    {line}" for line in clean_req.split("\n")])
    print(f"{CYAN}[*] {title}:{RESET}\n{CYAN}{padded}{RESET}")

def send_evil_fragmented_request():
    print(f"\n{MAGENTA}=== Testing: Fragmented Request ==={RESET}")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((HOST, PORT))
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")
        return

    # 1. Send half the request line and wait 1 second
    req1 = "GET /in"
    print_request(req1, "Sending Fragment 1")
    s.sendall(req1.encode())
    time.sleep(1)
    
    # 2. Send the rest of the line and half the headers
    req2 = "dex.html HTTP/1.1\r\nHost: local"
    print_request(req2, "Sending Fragment 2")
    s.sendall(req2.encode())
    time.sleep(1)
    
    # 3. Finish the request
    req3 = "host:8080\r\n\r\n"
    print_request(req3, "Sending Fragment 3")
    s.sendall(req3.encode())
    
    time.sleep(1)
    s.close()
    print(f"{GREEN}[+] SUCCESS: Fragmented Request Sent!{RESET}")

def send_chunked_request():
    print(f"\n{MAGENTA}=== Testing: Chunked Encoding ==={RESET}")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((HOST, PORT))
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")
        return
        
    headers = "POST /upload HTTP/1.1\r\nHost: localhost:8080\r\nTransfer-Encoding: chunked\r\n\r\n"
    print_request(headers, "Sending Headers")
    s.sendall(headers.encode())
    time.sleep(1)
    
    chunk1 = "4\r\nWiki\r\n"
    print_request(chunk1, "Sending Chunk 1")
    s.sendall(chunk1.encode())
    time.sleep(1)
    
    chunk2 = "5\r\npedia\r\n"
    print_request(chunk2, "Sending Chunk 2")
    s.sendall(chunk2.encode())
    time.sleep(1)
    
    chunk3 = "0\r\n\r\n"
    print_request(chunk3, "Sending Final Chunk (0)")
    s.sendall(chunk3.encode())
    
    time.sleep(1)
    s.close()
    print(f"{GREEN}[+] SUCCESS: Chunked Request Sent!{RESET}")

def send_standard_get():
    print(f"\n{MAGENTA}=== Testing: Standard GET Request ==={RESET}")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((HOST, PORT))
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")
        return
        
    request = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
    print_request(request)
    s.sendall(request.encode())
    time.sleep(0.5)
    s.close()
    print(f"{GREEN}[+] SUCCESS: Standard GET Sent!{RESET}")

def send_standard_post():
    print(f"\n{MAGENTA}=== Testing: Standard POST Request ==={RESET}")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((HOST, PORT))
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")
        return
        
    request = "POST /api/login HTTP/1.1\r\nHost: localhost\r\nContent-Length: 21\r\n\r\n"
    body = '{"username": "admin"}'
    print_request(request)
    s.sendall(request.encode())
    time.sleep(0.5)
    
    print_request(body)
    s.sendall(body.encode())
    time.sleep(0.5)
    s.close()
    print(f"{GREEN}[+] SUCCESS: Standard POST Sent!{RESET}")

def send_invalid_transfer_encoding():
    print(f"\n{MAGENTA}=== Testing: Invalid Transfer-Encoding (gzip) ==={RESET}")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((HOST, PORT))
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")
        return
        
    request = "POST /upload HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: gzip\r\n\r\n"
    print_request(request)
    s.sendall(request.encode())
    time.sleep(1)
    s.close()
    print(f"{GREEN}[+] SUCCESS: (Check server console for 'Unsupported Transfer-Encoding' error){RESET}")

def send_invalid_http_version():
    print(f"\n{MAGENTA}=== Testing: HTTP/2.0 Request (Should Fail) ==={RESET}")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        
        request = "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n"
        print_request(request)
        s.sendall(request.encode())
        
        s.close()
        print(f"{GREEN}[+] SUCCESS: (Check server console for 'Unsupported HTTP Version' error){RESET}")
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")

def send_invalid_method():
    print(f"\n{MAGENTA}=== Testing: Invalid Method (Should Fail) ==={RESET}")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        
        request = "GIBBERISH / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        print_request(request)
        s.sendall(request.encode())
        
        s.close()
        print(f"{GREEN}[+] SUCCESS: (Check server console for 'Unsupported Method' error){RESET}")
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")

def send_pipelined_requests():
    print(f"\n{MAGENTA}=== Testing: Pipelined Requests (2 requests in 1 packet) ==={RESET}")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        
        # Send two completely separate HTTP requests jammed into a single string/packet!
        request1 = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        request2 = "GET /api HTTP/1.1\r\nHost: localhost\r\n\r\n"
        
        combined = request1 + request2
        print_request(combined, "Sending Pipelined Packet")
        s.sendall(combined.encode())
        
        # We wait 1 second to give the server time to process BOTH requests!
        time.sleep(1)
        s.close()
        print(f"{GREEN}[+] SUCCESS: Pipelined requests sent! Check server logs to verify BOTH were parsed.{RESET}")
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")

if __name__ == "__main__":
    print(f"\n{MAGENTA}--- STARTING HTTP REQUEST TESTS ---{RESET}")
    send_standard_get()
    send_standard_post()
    send_evil_fragmented_request()
    send_chunked_request()
    send_pipelined_requests()
    send_invalid_transfer_encoding()
    send_invalid_http_version()
    send_invalid_method()
    print(f"\n{GREEN}--- ALL HTTP REQUEST TESTS COMPLETED ---{RESET}\n")

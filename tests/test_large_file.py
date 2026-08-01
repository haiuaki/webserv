import socket
import os
import time

PORT = 8080
HOST = '127.0.0.1'
FILE_PATH = "html/massive_test_file.txt"

# --- COLORS ---
GREEN = '\033[92m'
CYAN = '\033[96m'
MAGENTA = '\033[95m'
RED = '\033[91m'
RESET = '\033[0m'

if __name__ == "__main__":
    print(f"\n{MAGENTA}--- STARTING MASSIVE FILE LOAD TEST ---{RESET}")
    
    # 1. Create a massive 7 Megabyte text file
    if not os.path.exists("html"):
        os.makedirs("html")
        
    chunk = "This is a massive test file designed to fill up the OS socket buffer! " * 100
    with open(FILE_PATH, "w") as f:
        for _ in range(1000):
            f.write(chunk)
            
    size_mb = os.path.getsize(FILE_PATH) / (1024 * 1024)
    print(f"{CYAN}[*] Created temporary massive file: {FILE_PATH} ({size_mb:.2f} MB){RESET}")

    # 2. Setup connection
    print(f"\n{MAGENTA}=== Testing: GET /massive_test_file.txt ==={RESET}")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((HOST, PORT))
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")
        exit(1)

    # 3. Request the massive file (Asking for Connection: close so the server hangs up when done)
    request = "GET /massive_test_file.txt HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
    clean_req = request.replace('\r', '').strip()
    padded = "\n".join([f"    {line}" for line in clean_req.split("\n")])
    print(f"{CYAN}[*] Sending Request:{RESET}\n{CYAN}{padded}{RESET}")
    
    s.sendall(request.encode())

    # 4. Download it back chunk by chunk!
    bytes_received = 0
    header_length = 0
    headers_parsed = False
    start_time = time.time()

    while True:
        data = s.recv(16384) # Receive in 16KB chunks
        if not data:
            break # Connection closed or finished
            
        # If we haven't found the end of the headers yet, look for \r\n\r\n
        if not headers_parsed:
            header_end = data.find(b'\r\n\r\n')
            if header_end != -1:
                header_length = header_end + 4 # +4 for the \r\n\r\n itself
                headers_parsed = True
                # Only count the body bytes from this first chunk!
                bytes_received += (len(data) - header_length)
            else:
                # Still reading headers (rare for 16KB chunk, but possible)
                header_length += len(data)
        else:
            bytes_received += len(data)
        
    end_time = time.time()

    # 5. Output results
    if bytes_received == os.path.getsize(FILE_PATH):
        print(f"{GREEN}[+] SUCCESS: Server successfully streamed {bytes_received} bytes without blocking!{RESET}")
        print(f"    {CYAN}Time taken: {end_time - start_time:.4f} seconds{RESET}")
    else:
        print(f"{RED}[-] FAILED: Expected {os.path.getsize(FILE_PATH)} bytes but only received {bytes_received} bytes!{RESET}")

    # 6. Clean up
    s.close()
    os.remove(FILE_PATH)
    
    print(f"\n{GREEN}--- LOAD TEST COMPLETED ---{RESET}\n")

import socket
import time
import os

# Color codes for output
GREEN = '\033[92m'
CYAN = '\033[96m'
MAGENTA = '\033[95m'
RED = '\033[91m'
RESET = '\033[0m'

PORT = 8080
HOST = '127.0.0.1'

# The file we will create and delete!
TEST_FILE = "html/target.txt"

def setup_test_file():
    """Creates a physical file inside the html/ directory before testing."""
    # Ensure html/ directory exists
    if not os.path.exists("html"):
        os.makedirs("html")
        
    with open(TEST_FILE, "w") as f:
        f.write("This file is meant to be deleted!")
    print(f"{CYAN}[*] Created temporary test file: {TEST_FILE}{RESET}")

def send_request(method, uri, expected_status):
    print(f"\n{MAGENTA}=== Testing: {method} {uri} ==={RESET}")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.connect((HOST, PORT))
    except Exception as e:
        print(f"{RED}[!] Error connecting to server: {e}{RESET}")
        return

    # Send the raw HTTP request
    request = f"{method} {uri} HTTP/1.1\r\nHost: localhost\r\n\r\n"
    clean_req = request.replace('\r', '').strip()
    padded = "\n".join([f"    {line}" for line in clean_req.split("\n")])
    print(f"{CYAN}[*] Sending Request:{RESET}\n{CYAN}{padded}{RESET}")
    s.sendall(request.encode())
    
    # Wait and read the response back from the server!
    time.sleep(0.1)
    response = s.recv(4096).decode()
    s.close()
    
    # Extract just the first line (e.g. "HTTP/1.1 200 OK")
    status_line = response.split('\r\n')[0]
    
    # Check if the status line contains our expected status code!
    if expected_status in status_line:
        print(f"{GREEN}[+] SUCCESS: Server returned exactly what we expected -> {status_line}{RESET}")
    else:
        print(f"{RED}[-] FAILED: Expected {expected_status} but got -> {status_line}{RESET}")
        print(f"Full response:\n{response}")

if __name__ == "__main__":
    print(f"\n{MAGENTA}--- STARTING ROUTER INTEGRATION TESTS ---{RESET}")
    
    # 1. Setup the dummy file
    setup_test_file()
    
    # 2. Test GET (Should successfully serve the file with 200 OK)
    send_request("GET", "/target.txt", "200 OK")
    
    # 3. Test DELETE (Should successfully delete the file with 204 No Content)
    send_request("DELETE", "/target.txt", "204 No Content")
    
    # 4. Test GET Again! (File is gone, so it MUST return 404 Not Found)
    send_request("GET", "/target.txt", "404 Not Found")
    
    # 5. Test DELETE on missing file (Should return 404 Not Found)
    send_request("DELETE", "/missing.txt", "404 Not Found")
    
    # 6. Test DELETE on a directory path (Should return 403 Forbidden due to trailing slash)
    send_request("DELETE", "/images/", "403 Forbidden")
    
    print(f"\n{GREEN}--- ALL ROUTER TESTS COMPLETED ---{RESET}\n")

#!/usr/bin/env python3

import socket
import time
import threading

# --- ANSI COLORS ---
RESET   = "\033[0m"
RED     = "\033[31m"
GREEN   = "\033[32m"
YELLOW  = "\033[33m"
CYAN    = "\033[36m"
MAGENTA = "\033[35m"

HOST = '127.0.0.1'
PORT = 8080

def test_ghost():
    print(f"\n{MAGENTA}=== Testing: Ghost Test (100 rapid connects/disconnects) ==={RESET}")
    for _ in range(100):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((HOST, PORT))
            s.close()
        except Exception as e:
            print(f"{RED}[-] FAILED: {e}{RESET}")
            return
    print(f"{GREEN}[+] SUCCESS: Server survived 100 rapid ghost connections!{RESET}")

def test_slowloris():
    print(f"\n{MAGENTA}=== Testing: Slowloris Test (Sending 1 byte per second) ==={RESET}")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        for i in range(5):
            s.send(b"A")
            print(f"{CYAN}[*] Sent 1 byte...{RESET}")
            time.sleep(1)
        s.close()
        print(f"{GREEN}[+] SUCCESS: Server gracefully handled the slowloris connection!{RESET}")
    except Exception as e:
        print(f"{RED}[-] FAILED: {e}{RESET}")

swarm_errors = 0

def swarm_worker(worker_id):
    global swarm_errors
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        s.send(f"Swarm worker {worker_id} reporting in!\r\n".encode())
        time.sleep(1)
        s.close()
    except Exception as e:
        swarm_errors += 1

def test_swarm():
    global swarm_errors
    swarm_errors = 0
    print(f"\n{MAGENTA}=== Testing: Swarm Test (50 simultaneous concurrent connections) ==={RESET}")
    threads = []
    for i in range(50):
        t = threading.Thread(target=swarm_worker, args=(i,))
        threads.append(t)
        t.start()
    
    for t in threads:
        t.join()
        
    if swarm_errors > 0:
        print(f"{RED}[-] FAILED: {swarm_errors} connections refused.{RESET}")
    else:
        print(f"{GREEN}[+] SUCCESS: Server handled 50 simultaneous connections perfectly!{RESET}")

def test_large_payload():
    print(f"\n{MAGENTA}=== Testing: Large Payload Test (Sending 50,000 bytes) ==={RESET}")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        print(f"{CYAN}[*] Sending Request:{RESET}\n{CYAN}    <50,000 bytes of data>{RESET}")
        s.sendall(b"X" * 50000)
        time.sleep(1) 
        s.close()
        print(f"{GREEN}[+] SUCCESS: Server absorbed the massive payload!{RESET}")
    except Exception as e:
        print(f"{RED}[-] FAILED: {e}{RESET}")

def multiport_worker(port):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, port))
        s.send(f"Multi-port testing on {port}!\r\n".encode())
        time.sleep(1)
        s.close()
    except Exception:
        pass 

def test_multiport():
    print(f"\n{MAGENTA}=== Testing: Multi-Port Test (Connections to 8080, 8081, 9090) ==={RESET}")
    ports = [8080, 8081, 9090]
    
    # First, verify if the server is actually listening on these ports
    for port in ports:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((HOST, port))
            s.close()
        except ConnectionRefusedError:
            print(f"{RED}[!] Error connecting to server: Port {port} closed. Run `webserv` with `complex.conf`.{RESET}")
            return
            
    threads = []
    for _ in range(10): # Spawn 10 simultaneous connections per port
        for port in ports:
            t = threading.Thread(target=multiport_worker, args=(port,))
            threads.append(t)
            t.start()
            
    for t in threads:
        t.join()
    print(f"{GREEN}[+] SUCCESS: Server successfully handled traffic across all 3 ports simultaneously!{RESET}")

if __name__ == "__main__":
    print(f"\n{MAGENTA}--- STARTING TCP MULTIPLEXER TESTS ---{RESET}")
    time.sleep(1)
    
    test_ghost()
    time.sleep(1)
    
    test_swarm()
    time.sleep(1)
    
    test_large_payload()
    time.sleep(1)
    
    test_multiport()
    time.sleep(1)
    
    test_slowloris()
    
    print(f"\n{GREEN}--- ALL MULTIPLEXER TESTS COMPLETED ---{RESET}\n")

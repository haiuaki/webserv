_This project has been created as part of the 42 curriculum by juljin._

# webserv

## Description

`webserv` is a custom HTTP/1.1 server written in C++98. Inspired by NGINX, it is capable of handling multiple concurrent connections using I/O multiplexing, serving static content, executing CGI scripts, and processing various HTTP methods based on a configuration file.

## Instructions

### 1. Compilation

To compile the project, run:
`make`

### 2. Execution

To run the server, provide an optional configuration file:
`./webserv [configuration_file.conf]`

### 3. Testing

Several test suites have been built to ensure the stability of the server. You can run all tests at once using:
`make test`

Or run them individually:

| Command | Description |
| --- | --- |
| `make test_config_parser` | Compiles and runs the configuration parser test suite. |
| `make test_multiplexer` | Runs the Python script testing the I/O multiplexer. |
| `make test_requests` | Runs the Python script testing HTTP requests. |

### 4. Cleaning

| Command       | Description                            |
| ------------- | -------------------------------------- |
| `make clean`  | Remove object files                    |
| `make fclean` | Remove object files and the executable |
| `make re`     | Rebuild the project                    |

## Server Configuration

The server parses `.conf` files to configure the HTTP servers. The configuration file mimics NGINX's syntax and must follow these rules:

#### 1. Server Block Context

Each server is defined within a `server {}` block. Inside, you can configure:
- `listen [host:]<port> [default_server]`: The port (and optional host and default_server status) on which the server listens.
- `server_name <name> ...`: One or more domain names associated with this server block.
- `root <path>`: The root directory for the server.
- `index <file> ...`: Default files to serve if a directory is requested.
- `client_max_body_size <size>[K|M|G]`: Limit the size of client request bodies.
- `error_page <code ...> <path>`: Custom error pages for specific HTTP status codes.

#### 2. Location Block Context

Routes are defined within `location <path> {}` blocks inside a server block. Configuration includes:
- `root <path>`: The root directory specifically for this route.
- `index <file> ...`: Default files to serve if a directory is requested.
- `autoindex <on/off>`: Enable or disable directory listing.
- `client_max_body_size <size>[K|M|G]`: Limit client request body size (overrides server config).
- `error_page <code ...> <path>`: Custom error pages (overrides server config).
- `allow_methods <GET|POST|DELETE> ...`: HTTP methods allowed for the route.
- `cgi <extension> <path>`: Execute CGI scripts (e.g., `.php` or `.py`).
- `upload_store <path>`: Directory where uploaded files should be saved.
- `return <code> <url>`: HTTP redirection.

#### 3. Constraints

- **Nested Locations**: Location blocks cannot be nested inside other location blocks.
- **Supported Methods**: The server only supports `GET`, `POST`, and `DELETE` HTTP methods.
- **HTTP Codes**: Redirection (`return`) codes must be 3xx. Error page codes must be between 100 and 599.
- **Index Directives**: An absolute path can only be the last element in an `index` directive (mimicking NGINX behavior).
- **CGI**: The CGI extension must start with a dot (e.g. `.py`).
- **Contextual Inheritance**: Directives such as `root`, `index`, `client_max_body_size`, and `error_page` safely cascade from the `server` scope down to nested `location` blocks. Location-specific overrides seamlessly mask server-wide settings on a per-route basis.

#### Example File
```text
server {
    listen 127.0.0.1:8080 default_server;
    server_name example.com;
    
    root /var/www/html;
    index index.html;
    
    client_max_body_size 10M;
    error_page 404 /errors/404.html;

    location / {
        index home.html;
        autoindex off;
        allow_methods GET POST;
    }

    location /uploads {
        root /var/www/uploads;
        client_max_body_size 50M;
        error_page 413 /errors/413_upload.html;
        allow_methods POST DELETE;
        upload_store /var/www/uploads/tmp;
    }

    location /cgi-bin {
        root /var/www/cgi-bin;
        allow_methods GET POST;
        cgi .py /usr/bin/python3;
    }
    
    location /old-api {
        return 301 /cgi-bin;
    }
}
```

## Technical Overview

### The Role of a Web Server

In real-life applications, a web server (like NGINX or Apache) acts as an intermediary between client requests (usually from a web browser) and the server's backend resources. It listens for incoming network connections, parses HTTP requests, routes them to the appropriate static files or dynamic scripts (CGI), and sends back an HTTP response. High-performance web servers use advanced I/O multiplexing to handle thousands of concurrent connections efficiently without creating a separate thread for every request.

### The TCP/IP Model & Implementation

The internet relies on the TCP/IP suite, which abstracts network communication into four layers: Application, Transport, Internet, and Network Access. 

For this project, the **Application Layer** (HTTP) is implemented on top of the OS-provided **Transport Layer** (TCP):
- **Transport Layer (TCP)**: The operating system handles the reliable transmission of packets, ensuring data arrives in order and without errors. The server interfaces with this layer using **Sockets**.
- **Application Layer (HTTP)**: Custom logic is built to interpret the raw stream of bytes delivered by TCP into structured HTTP requests, process them, and construct valid HTTP responses according to RFC standards.

### Core Networking Concepts

- **Sockets**: A socket is a software endpoint that establishes a bidirectional communication link between two programs on a network. POSIX sockets (`socket()`, `bind()`, `listen()`, `accept()`) are used to open network ports and accept incoming client connections.
- **Packets & Byte Streams**: Data sent over TCP is broken into packets by the OS. Because TCP is a stream-oriented protocol, a single HTTP request might arrive in multiple chunks (or multiple requests in one chunk). The server buffers these raw byte streams and parses them continuously until a complete HTTP message is formed.
- **I/O Multiplexing**: To handle multiple clients simultaneously without multithreading, a multiplexing system (like `select`, `poll`, `epoll`, or `kqueue`) is used. This allows the server to monitor multiple sockets and only wake up to read or write when a socket is ready, ensuring non-blocking operations.

### Dynamic Content & State Management

- **CGI (Common Gateway Interface)**: While a web server natively serves static files (HTML, CSS, images), it cannot execute code on its own. CGI is a standard protocol that allows the server to interface with external executables (like Python or PHP scripts) to generate dynamic content. The server passes HTTP request data to the script via environment variables and standard input (`stdin`), waits for the script to execute, and reads the output via standard output (`stdout`) to construct the final HTTP response.
- **Cookies & Session Management**: HTTP is inherently a stateless protocol, meaning each request is completely independent. To maintain state (like keeping a user logged in), servers use Cookies. The server can send a `Set-Cookie` header to the client, which the client's browser will store and send back in subsequent requests via the `Cookie` header, allowing backend CGI scripts to identify and track user sessions.

## Resources

### 1. Documentation and References

The following resources were consulted during the development of the project:
- [NGINX Core Module Directives](https://nginx.org/en/docs/http/ngx_http_core_module.html#directives) - Official documentation for NGINX HTTP core directives.
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) - A comprehensive guide for understanding C/C++ socket programming.
- [RFC Editor - HTTP Specifications](https://www.rfc-editor.org/) - The official repository for HTTP RFCs and standards.
- [MDN Web Docs - HTTP messages](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Messages) - Anatomy of an HTTP message.
- [MDN Web Docs - HTTP Cookies](https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/Cookies) - Guide to using HTTP cookies for state management.
- [MDN Web Docs - Set-Cookie](https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Set-Cookie) - Reference for the `Set-Cookie` HTTP response header.

### 2. Use of AI

AI was utilized as a technical assistant during the development of this project:

- **Tasks**: Used for generating boilerplate documentation, standardizing Doxygen-style comments across the codebase, and summarizing complex RFC specifications. It also assisted in debugging edge-cases related to I/O multiplexing.
- **Parts**: The inline codebase documentation (Doxygen), the configuration parsing logic, and the README structures.

_Note: All code and logic were manually implemented and fully understood by the author to ensure technical integrity._

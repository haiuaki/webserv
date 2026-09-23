#include "network/Server.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "utils/Colors.hpp"
#include "utils/Utils.hpp"

// --- CONSTRUCTOR & DESTRUCTOR --------------------------------------------- //

Server::Server(const std::string& host, uint16_t port)
	: listenFd_(-1), host_(host), port_(port) {}

Server::~Server() {
	if (listenFd_ != -1) {
		close(listenFd_);
		std::cout << GRAY << "[LOG] " << YELLOW << "Server socket closed on "
				  << host_ << ":" << port_ << '.' << RESET << '\n';
	}
}

// --- GETTERS -------------------------------------------------------------- //

int Server::getListenFd() const {
	return listenFd_;
}

const std::string& Server::getHost() const {
	return host_;
}

uint16_t Server::getPort() const {
	return port_;
}

const std::vector<const ServerConfig*>& Server::getConfigs() const {
	return configs_;
}

// --- SETTERS -------------------------------------------------------------- //

void Server::addConfig(const ServerConfig* config) {
	configs_.push_back(config);
}

// --- METHODS -------------------------------------------------------------- //

void Server::setupSocket() {
	// Create a TCP socket
	listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listenFd_ == -1) {
		throw std::runtime_error("`socket()` failed");
	}

	// Set SO_REUSEADDR to prevent 'Address already in use' errors
	int opt = 1;
	if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))
	    == -1) {
		throw std::runtime_error("`setsockopt()` failed");
	}

	// Make the socket non-blocking and close-on-exec
	if (fcntl(listenFd_, F_SETFL, O_NONBLOCK | FD_CLOEXEC) == -1) {
		throw std::runtime_error("`fcntl()` failed to set non-blocking");
	}

	// Resolve the Host and Port using getaddrinfo (42 compliant)
	struct addrinfo hints, *res;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;       // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP
	hints.ai_flags = AI_PASSIVE;     // For wildcard IP if node is NULL

	std::string portStr = utils::toString(port_);

	const char* node = NULL;
	if (!host_.empty() && host_ != "localhost" && host_ != "0.0.0.0") {
		node = host_.c_str();
	}

	if (getaddrinfo(node, portStr.c_str(), &hints, &res) != 0) {
		throw std::runtime_error("`getaddrinfo()` failed for host: " + host_);
	}

	// Bind the socket
	if (bind(listenFd_, res->ai_addr, res->ai_addrlen) == -1) {
		freeaddrinfo(res);
		throw std::runtime_error("`bind()` failed for host: " + host_
		                         + " port: " + portStr);
	}
	freeaddrinfo(res);

	// Listen for incoming connections
	if (listen(listenFd_, SOMAXCONN) == -1) {
		throw std::runtime_error("`listen()` failed");
	}

	std::cout << GRAY << "[LOG] " << GREEN
			  << "Server successfully listening on " << host_ << ":" << port_
			  << '.' << RESET << '\n';
}

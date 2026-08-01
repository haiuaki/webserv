#include "network/Client.hpp"

#include <unistd.h>

#include <string>

// --- CONSTRUCTOR ---------------------------------------------------------- //

Client::Client(int fd, int serverFd, const std::string& ip)
	: fd_(fd), serverFd_(serverFd), ip_(ip) {}

// --- METHOD --------------------------------------------------------------- //

void Client::appendRequestData(const char* data, int length) {
	// Immediately feed the data to the state machine
	request.parse(std::string(data, length));
}

// --- GETTERS -------------------------------------------------------------- //

int Client::getFd() const {
	return fd_;
}

int Client::getServerFd() const {
	return serverFd_;
}

const std::string& Client::getIp() const {
	return ip_;
}

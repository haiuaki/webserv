#include "network/Client.hpp"

#include <unistd.h>

#include <string>

// --- CONSTRUCTOR ---------------------------------------------------------- //

Client::Client(int fd, const std::string& ip) : fd_(fd), ip_(ip) {}

// --- METHOD --------------------------------------------------------------- //

void Client::appendRequestData(const char* data, int length) {
	// Immediately feed the data to the state machine
	request.parse(std::string(data, length));
}

// --- GETTERS -------------------------------------------------------------- //

int Client::getFd() const {
	return fd_;
}

const std::string& Client::getIp() const {
	return ip_;
}

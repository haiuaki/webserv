#include "network/Client.hpp"

#include <unistd.h>

#include <string>

// --- CONSTRUCTOR ---------------------------------------------------------- //

Client::Client(int fd, const std::string& ip) : fd_(fd), ip_(ip) {}

// --- METHOD --------------------------------------------------------------- //

void Client::appendRequestData(const char* data, int length) {
	// By specifying the length, std::string can safely hold binary data
	// (like image uploads) even if the data contains a null-terminator '\0'
	requestBuffer_.append(data, length);
}

// --- GETTERS -------------------------------------------------------------- //

int Client::getFd() const {
	return fd_;
}

const std::string& Client::getIp() const {
	return ip_;
}

const std::string& Client::getRequestBuffer() const {
	return requestBuffer_;
}

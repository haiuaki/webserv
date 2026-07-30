#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "http/HttpRequest.hpp"

class Client {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		Client(int fd, const std::string& ip);

		// --- METHOD ------------------------------------------------------- //
		void appendRequestData(const char* data, int length);

		// --- GETTERS ------------------------------------------------------ //
		int getFd() const;
		const std::string& getIp() const;

		// --- ATTRIBUTE ---------------------------------------------------- //
		HttpRequest request;

	private:
		// --- ATTRIBUTES --------------------------------------------------- //
		int fd_;
		std::string ip_;
};

#endif

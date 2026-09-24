#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

/**
 * @brief Represents a single persistent client connection.
 *
 * Stores the file descriptor, the client's IP address, and holds
 * the HttpRequest and HttpResponse objects for the duration of the
 * connection. This allows the server to handle fragmented data asynchronously.
 */
class Client {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		Client(int fd, int serverFd, const std::string& ip);

		// --- METHOD ------------------------------------------------------- //
		void appendRequestData(const char* data, int length);

		// --- GETTERS ------------------------------------------------------ //
		int getFd() const;
		int getServerFd() const;
		const std::string& getIp() const;

		// --- ATTRIBUTES --------------------------------------------------- //
		HttpRequest request;
		HttpResponse response;
		std::string responseQueue;

		pid_t cgiPid;
		int cgiReadFd;
		int cgiWriteFd;
		std::string cgiOutput;
		std::string cgiBodyRemaining;
		bool isCgiRunning;

	private:
		// --- ATTRIBUTES --------------------------------------------------- //
		int fd_;
		int serverFd_;
		std::string ip_;
};

#endif

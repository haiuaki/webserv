#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		Client(int fd, const std::string& ip);

		// --- METHOD ------------------------------------------------------- //
		void appendRequestData(const char* data, int length);

		// --- GETTERS ------------------------------------------------------ //
		int getFd() const;
		const std::string& getIp() const;
		const std::string& getRequestBuffer() const;

	private:
		// --- ATTRIBUTES --------------------------------------------------- //
		int fd_;
		std::string ip_;
		std::string requestBuffer_;
};

#endif

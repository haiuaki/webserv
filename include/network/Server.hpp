#ifndef SERVER_HPP
#define SERVER_HPP

#include <stdint.h>

#include <string>
#include <vector>

#include "config/ServerConfig.hpp"

class Server {
	public:
		// --- CONSTRUCTOR & DESTRUCTOR ------------------------------------- //
		Server(const std::string& host, uint16_t port);
		~Server();

		// --- GETTERS ------------------------------------------------------ //
		int getListenFd() const;
		const std::string& getHost() const;
		uint16_t getPort() const;
		const std::vector<const ServerConfig*>& getConfigs() const;

		// --- SETTERS ------------------------------------------------------ //
		void addConfig(const ServerConfig* config);

		// --- METHODS ------------------------------------------------------ //
		void setupSocket();

	private:
		// --- ATTRIBUTES --------------------------------------------------- //
		int listenFd_;
		std::string host_;
		uint16_t port_;
		std::vector<const ServerConfig*> configs_;
};

#endif

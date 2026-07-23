#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <map>
#include <vector>

#include "config/ServerConfig.hpp"

class ServerManager {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		ServerManager(
			const std::map< std::string, std::vector<ServerConfig> >& cluster);

	private:
		// --- METHOD ------------------------------------------------------- //
		void validateServers();

		// --- ATTRIBUTE ---------------------------------------------------- //
		std::map< std::string, std::vector<ServerConfig> > cluster_;
};

#endif

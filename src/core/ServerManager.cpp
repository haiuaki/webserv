#include "core/ServerManager.hpp"

#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "config/ServerConfig.hpp"

// --- CONSTRUCTOR ---------------------------------------------------------- //

ServerManager::ServerManager(
	const std::map< std::string, std::vector<ServerConfig> >& cluster)
	: cluster_(cluster) {
	validateServers();
}

// --- METHOD --------------------------------------------------------------- //

void ServerManager::validateServers() {
	// Loop through every Host:Port combination we need to listen on
	std::map< std::string, std::vector<ServerConfig> >::iterator it;
	for (it = cluster_.begin(); it != cluster_.end(); ++it) {
		std::string hostPort = it->first;
		std::vector<ServerConfig>& servers = it->second;

		std::set<std::string> seenNames;
		bool hasDefault = false;

		// Loop through all the virtual servers sharing this socket
		for (size_t i = 0; i < servers.size(); ++i) {
			// Check if multiple server blocks claim to be the `default_server`
			// on this socket
			if (servers[i].isDefaultServer()) {
				if (hasDefault) {
					throw std::runtime_error("Conflicting `default_server` "
					                         "directives found on `"
					                         + hostPort + "`.");
				}
				hasDefault = true;
			}

			// Check if the same domain name is claimed by multiple server
			// blocks on this socket
			const std::vector<std::string>& names = servers[i].getServerNames();
			for (size_t j = 0; j < names.size(); ++j) {
				if (seenNames.count(names[j])) {
					throw std::runtime_error("Conflicting `server_name` value `"
					                         + names[j] + "` found on `"
					                         + hostPort + "`.");
				}
				seenNames.insert(names[j]);
			}
		}

		// If no `default_server` was explicitly set by the user,
		// the first server on that socket is default
		if (!hasDefault && !servers.empty()) {
			servers[0].setDefaultServer(true);
		}
	}
}

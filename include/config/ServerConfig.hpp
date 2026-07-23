#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <stdint.h>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "config/LocationConfig.hpp"

/**
 * @brief Represents a single virtual server configuration.
 *
 * Stores all directives parses from a `server { ... }` block,
 * including host, port, error pages, and nested location routes.
 */
class ServerConfig {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		ServerConfig();

		// --- GETTERS ------------------------------------------------------ //
		const std::string& getHost() const;
		uint16_t getPort() const;
		bool isDefaultServer() const;
		const std::vector<std::string>& getServerNames() const;
		const std::string& getRootPath() const;
		const std::vector<std::string>& getIndexFiles() const;
		size_t getClientMaxBodySize() const;
		const std::map<int, std::string>& getErrorPages() const;
		const std::map<std::string, LocationConfig>& getLocations() const;

		// --- SETTERS ------------------------------------------------------ //
		void setHost(const std::string& host);
		void setPort(uint16_t port);
		void setDefaultServer(bool isDefaultServer);
		void setServerNames(const std::vector<std::string>& serverName);
		void setRootPath(const std::string& rootPath);
		void setIndexFiles(const std::vector<std::string>& indexFile);
		void setClientMaxBodySize(size_t clientMaxBodySize);
		void addErrorPage(int errorCode, const std::string& errorPath);
		void addLocation(const std::string& path, const LocationConfig& config);

	private:
		// --- ATTRIBUTES --------------------------------------------------- //
		std::string host_;
		uint16_t port_;
		bool isDefaultServer_;
		std::vector<std::string> serverNames_;
		std::string rootPath_;
		std::vector<std::string> indexFiles_;
		size_t clientMaxBodySize_;
		std::map<int, std::string> errorPages_;
		std::map<std::string, LocationConfig> locations_;
};

#endif

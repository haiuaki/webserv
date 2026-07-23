#include "config/ServerConfig.hpp"

#include <stdint.h>

#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// --- CONSTRUCTOR ---------------------------------------------------------- //

ServerConfig::ServerConfig()
	: host_("0.0.0.0")
	, port_(8080)
	, isDefaultServer_(false)
	, clientMaxBodySize_(1048576) // Default to 1MB
{}

// --- GETTERS -------------------------------------------------------------- //

const std::string& ServerConfig::getHost() const {
	return host_;
}

uint16_t ServerConfig::getPort() const {
	return port_;
}

bool ServerConfig::isDefaultServer() const {
	return isDefaultServer_;
}

const std::vector<std::string>& ServerConfig::getServerNames() const {
	return serverNames_;
}

const std::string& ServerConfig::getRootPath() const {
	return rootPath_;
}

const std::vector<std::string>& ServerConfig::getIndexFiles() const {
	return indexFiles_;
}

size_t ServerConfig::getClientMaxBodySize() const {
	return clientMaxBodySize_;
}

const std::map<int, std::string>& ServerConfig::getErrorPages() const {
	return errorPages_;
}

const std::map<std::string, LocationConfig>&
ServerConfig::getLocations() const {
	return locations_;
}

// --- SETTERS -------------------------------------------------------------- //

void ServerConfig::setHost(const std::string& host) {
	host_ = host;
}

void ServerConfig::setPort(uint16_t port) {
	port_ = port;
}

void ServerConfig::setDefaultServer(bool isDefaultServer) {
	isDefaultServer_ = isDefaultServer;
}

void ServerConfig::setServerNames(const std::vector<std::string>& serverName) {
	serverNames_ = serverName;
}

void ServerConfig::setRootPath(const std::string& rootPath) {
	rootPath_ = rootPath;
}

void ServerConfig::setIndexFiles(const std::vector<std::string>& indexFile) {
	indexFiles_ = indexFile;
}

void ServerConfig::setClientMaxBodySize(size_t clientMaxBodySize) {
	clientMaxBodySize_ = clientMaxBodySize;
}

void ServerConfig::addErrorPage(int errorCode, const std::string& errorPath) {
	errorPages_[errorCode] = errorPath;
}

void ServerConfig::addLocation(const std::string& path,
                               const LocationConfig& config) {
	if (locations_.count(path) > 0) {
		throw std::runtime_error("Duplicate location block: `" + path + "`");
	}
	locations_[path] = config;
}

#include "config/LocationConfig.hpp"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

// --- CONSTRUCTOR ---------------------------------------------------------- //

LocationConfig::LocationConfig() : clientMaxBodySize_(0), isAutoindex_(false) {}

// --- GETTERS -------------------------------------------------------------- //
const std::string& LocationConfig::getPath() const {
	return path_;
}

const std::string& LocationConfig::getRootPath() const {
	return rootPath_;
}

const std::vector<std::string>& LocationConfig::getIndexFiles() const {
	return indexFiles_;
}

bool LocationConfig::isAutoindex() const {
	return isAutoindex_;
}

const std::map<int, std::string>& LocationConfig::getErrorPages() const {
	return errorPages_;
}

size_t LocationConfig::getClientMaxBodySize() const {
	return clientMaxBodySize_;
}

const std::vector<std::string>& LocationConfig::getAllowedMethods() const {
	return allowedMethods_;
}

const std::map<std::string, std::string>&
LocationConfig::getCgiHandlers() const {
	return cgiHandlers_;
}

const std::string& LocationConfig::getUploadDir() const {
	return uploadDir_;
}

const std::pair<int, std::string>& LocationConfig::getReturn() const {
	return return_;
}

// --- SETTERS -------------------------------------------------------------- //
void LocationConfig::setPath(const std::string& path) {
	path_ = path;
}

void LocationConfig::setRootPath(const std::string& rootPath) {
	rootPath_ = rootPath;
}

void LocationConfig::setIndexFiles(const std::vector<std::string>& indexFiles) {
	indexFiles_ = indexFiles;
}

void LocationConfig::setAutoindex(bool isAutoIndex) {
	isAutoindex_ = isAutoIndex;
}

void LocationConfig::setClientMaxBodySize(size_t size) {
	clientMaxBodySize_ = size;
}

void LocationConfig::addErrorPage(int errorCode, const std::string& errorPath) {
	errorPages_[errorCode] = errorPath;
}

void LocationConfig::setErrorPages(
	const std::map<int, std::string>& errorPages) {
	errorPages_ = errorPages;
}

void LocationConfig::setAllowedMethods(
	const std::vector<std::string>& methods) {
	allowedMethods_ = methods;
}

void LocationConfig::addCgiHandler(const std::string& ext,
                                   const std::string& path) {
	cgiHandlers_[ext] = path;
}

void LocationConfig::setUploadDir(const std::string& uploadDir) {
	uploadDir_ = uploadDir;
}

void LocationConfig::setReturn(const std::pair<int, std::string>& redirect) {
	return_ = redirect;
}

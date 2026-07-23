#include "config/ConfigParser.hpp"

#include <stdint.h>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "utils/Colors.hpp"
#include "utils/Utils.hpp"

// --- CONSTRUCTOR ---------------------------------------------------------- //

ConfigParser::ConfigParser(const std::string& filename) : currentPos_(0) {
	// Enforce configuration file standardization
	if (filename.length() <= 5
	    || filename.substr(filename.length() - 5) != ".conf") {
		throw std::runtime_error(
			"Configuration file must have a `.conf` extension.");
	}

	// Register Server Directive Parsers
	serverDirectiveParsers_["listen"] = &ConfigParser::parseListen;
	serverDirectiveParsers_["server_name"] = &ConfigParser::parseServerName;
	serverDirectiveParsers_["root"] = &ConfigParser::parseServerRoot;
	serverDirectiveParsers_["index"] = &ConfigParser::parseServerIndex;
	serverDirectiveParsers_["client_max_body_size"] =
		&ConfigParser::parseServerClientMaxBodySize;
	serverDirectiveParsers_["error_page"] = &ConfigParser::parseServerErrorPage;

	// Register Location Directive Parsers
	locationDirectiveParsers_["root"] = &ConfigParser::parseLocationRoot;
	locationDirectiveParsers_["index"] = &ConfigParser::parseLocationIndex;
	locationDirectiveParsers_["autoindex"] = &ConfigParser::parseAutoindex;
	locationDirectiveParsers_["client_max_body_size"] =
		&ConfigParser::parseLocationClientMaxBodySize;
	locationDirectiveParsers_["error_page"] =
		&ConfigParser::parseLocationErrorPage;
	locationDirectiveParsers_["allow_methods"] =
		&ConfigParser::parseAllowedMethods;
	locationDirectiveParsers_["cgi"] = &ConfigParser::parseCgi;
	locationDirectiveParsers_["upload_store"] = &ConfigParser::parseUploadDir;
	locationDirectiveParsers_["return"] = &ConfigParser::parseReturn;

	tokenizeConfig(filename);

	if (tokens_.empty()) {
		throw std::runtime_error("Configuration file is empty or missing.");
	}

	parseTokens();
}

// --- GETTER --------------------------------------------------------------- //

const std::map< std::string, std::vector<ServerConfig> >&
ConfigParser::getServers() const {
	return servers_;
}

// --- DEBUGGING ------------------------------------------------------------ //

void ConfigParser::printTokens() const {
	std::cout << BLUE << "=== TOKEN DUMP ===" << RESET << '\n';
	for (size_t i = 0; i < tokens_.size(); ++i) {
		std::cout << GRAY << "[" << i << "] -> '" << CYAN << tokens_[i] << GRAY
				  << "'" << RESET << '\n';
	}
	std::cout << BLUE << "==================" << RESET << '\n';
}

// --- TOKEN UTILITIES ------------------------------------------------------ //

void ConfigParser::advance() {
	++currentPos_;

	if (currentPos_ > tokens_.size()) {
		throw std::runtime_error("Unexpected end of configuration file.");
	}
}

void ConfigParser::expect(const std::string& expectedToken) {
	advance();

	if (tokens_[currentPos_] != expectedToken) {
		throw std::runtime_error("Syntax error: expected `" + expectedToken
		                         + "` but found `" + tokens_[currentPos_]
		                         + "`.");
	}
}

// --- CORE PARSING LOGIC --------------------------------------------------- //

void ConfigParser::tokenizeConfig(const std::string& filename) {
	// Read the entire file into a single string
	std::ifstream file(filename.c_str());
	if (!file.is_open()) {
		throw std::runtime_error("Could not open configuration file.");
	}

	// Feed the file's raw underlying buffer into the stringstream
	std::stringstream buffer;
	buffer << file.rdbuf();
	std::string content = buffer.str();

	// Define delimiters
	const std::string whitespace = " \t\n\r\f\v";
	const std::string special = "{};";

	for (size_t i = 0; i < content.length(); ++i) {
		// Skip whitespace
		i = content.find_first_not_of(whitespace, i);
		if (i == std::string::npos) {
			break;
		}
		// Handle comments: if '#' found, skip to next line
		if (content[i] == '#') {
			i = content.find_first_of("\n", i);
			if (i == std::string::npos) {
				break; // If comment is at EOF
			}
			continue;
		}
		// Handle special chars: '{', '}' & ';' are tokens
		if (special.find(content[i]) != std::string::npos) {
			tokens_.push_back(std::string(1, content[i]));
			continue;
		}
		// Handle words: find the end of the current word
		size_t end = content.find_first_of(whitespace + special + "#", i);
		if (end == std::string::npos) {
			tokens_.push_back(content.substr(i)); // Push the last word
			break;
		}

		tokens_.push_back(content.substr(i, end - i));
		i = end - 1; // Move index to the end of the found word (-1 because the
		             // loop does ++i)
	}
}

void ConfigParser::parseTokens() {
	while (currentPos_ < tokens_.size()) {
		if (tokens_[currentPos_] == "server") {
			parseServerBlock();
		} else {
			throw std::runtime_error("Unknown directive in global context.");
		}
	}
}

void ConfigParser::parseServerBlock() {
	expect("{");

	advance();

	ServerConfig newServer;

	while (currentPos_ < tokens_.size() && tokens_[currentPos_] != "}") {
		std::string directive = tokens_[currentPos_];

		if (directive == "location") {
			parseLocationBlock(newServer);
		} else if (serverDirectiveParsers_.count(directive) > 0) {
			// Check if the directive exists in our map and calls the correct
			// function via the pointer
			(this->*serverDirectiveParsers_[directive])(newServer);
		} else {
			throw std::runtime_error("Unknown directive in server block: `"
			                         + directive + "`");
		}
		advance();
	}

	if (currentPos_ >= tokens_.size()) {
		throw std::runtime_error(
			"Unexpected end of configuration file, missing '}'");
	}

	// Insert the new server into the `ServerConfig` vector for that host:port
	std::string hostPort = newServer.getHost() + ":"
	                       + utils::toString(newServer.getPort());
	servers_[hostPort].push_back(newServer);

	advance();
}

void ConfigParser::parseLocationBlock(ServerConfig& server) {
	advance();

	std::string path = tokens_[currentPos_];
	if (path == "{") {
		throw std::runtime_error("Location block requires a path.");
	}
	if (path[0] != '/') {
		throw std::runtime_error("Location path must begin with a '/': `" + path
		                         + "`");
	}
	expect("{");

	advance();

	LocationConfig newLocation;
	// Inheriting from its parent server's configuration for certain directives
	newLocation.setRootPath(server.getRootPath());
	newLocation.setIndexFiles(server.getIndexFiles());
	newLocation.setErrorPages(server.getErrorPages());
	newLocation.setClientMaxBodySize(server.getClientMaxBodySize());

	while (currentPos_ < tokens_.size() && tokens_[currentPos_] != "}") {
		std::string directive = tokens_[currentPos_];

		if (directive == "location") {
			throw std::runtime_error(
				"Nested location blocks are not supported.");
		} else if (locationDirectiveParsers_.count(directive) > 0) {
			(this->*locationDirectiveParsers_[directive])(newLocation);
		} else {
			throw std::runtime_error("Unknown directive in location block: `"
			                         + directive + "`");
		}
		advance();
	}

	if (currentPos_ >= tokens_.size()) {
		throw std::runtime_error(
			"Unexpected end of configuration file, missing '}'");
	}

	// Save the path inside the object, becoming self-aware
	newLocation.setPath(path);

	// Insert the new location into the `LocationConfig` map for the given path
	server.addLocation(path, newLocation);
}

// --- SERVER DIRECTIVE HELPERS --------------------------------------------- //

void ConfigParser::parseListen(ServerConfig& server) {
	advance();

	std::string arg = tokens_[currentPos_];

	std::string host = server.getHost(); // Default Host
	int port = server.getPort();         // Default Port

	size_t colonPos = arg.find(':');
	if (colonPos != std::string::npos) {
		// If we find an IP address and a Port, overwrite both
		host = arg.substr(0, colonPos);
		port = utils::stringToInt(arg.substr(colonPos + 1));
	} else if (arg.find('.') != std::string::npos) {
		// If we only find an IP address, overwrite host
		host = arg;
	} else {
		// If we only find a Port, overwrite port
		port = utils::stringToInt(arg);
	}

	if (port < 0 || port > 65535) {
		throw std::runtime_error("Port out of range: `" + arg + "`");
	}

	server.setHost(host);
	server.setPort(static_cast<uint16_t>(port));

	// Check for `default_server`
	advance();
	while (tokens_[currentPos_] != ";") {
		if (tokens_[currentPos_] == "default_server") {
			server.setDefaultServer(true);
		} else {
			throw std::runtime_error("Unknown argument in listen directive: `"
			                         + tokens_[currentPos_] + "`");
		}
		advance();
	}
}

void ConfigParser::parseServerName(ServerConfig& server) {
	advance();

	std::vector<std::string> serverNames;
	while (tokens_[currentPos_] != ";") {
		serverNames.push_back(tokens_[currentPos_]);
		advance();
	}

	if (serverNames.empty()) {
		throw std::runtime_error(
			"`server_name` directive requires at least one name.");
	}

	server.setServerNames(serverNames);
}

void ConfigParser::parseServerRoot(ServerConfig& server) {
	advance();

	std::string arg = tokens_[currentPos_];
	if (arg == ";") {
		throw std::runtime_error("Empty `root` directive.");
	}

	server.setRootPath(arg);

	expect(";");
}

void ConfigParser::parseServerIndex(ServerConfig& server) {
	advance();

	std::vector<std::string> indexFiles;
	while (tokens_[currentPos_] != ";") {
		std::string currentFile = tokens_[currentPos_];

		// If the file starts with '/', it MUST be the last element
		if (!currentFile.empty() && currentFile[0] == '/') {
			// Check if the next token is the semicolon ';'
			if (currentPos_ + 1 >= tokens_.size()
			    || tokens_[currentPos_ + 1] != ";") {
				throw std::runtime_error(
					"Syntax error: an absolute path (`" + currentFile
					+ "`) can only be the last element in an index directive.");
			}
		}

		indexFiles.push_back(currentFile);
		advance();
	}

	if (indexFiles.empty()) {
		throw std::runtime_error(
			"`index` directive requires at least one file.");
	}

	server.setIndexFiles(indexFiles);
}

void ConfigParser::parseServerClientMaxBodySize(ServerConfig& server) {
	advance();

	std::string arg = tokens_[currentPos_];
	std::string originalArg = arg;
	if (arg == ";") {
		throw std::runtime_error("Empty `client_max_body_size`.");
	}

	// Check if the value is valid (positive)
	if (!arg.empty() && arg[0] == '-') {
		throw std::runtime_error("Invalid value `" + originalArg
		                         + "` in `client_max_body_size` directive.");
	}

	size_t multiplier = 1;
	char lastChar = arg[arg.length() - 1];

	// Check if the last character is a letter
	if (lastChar == 'K' || lastChar == 'k') {
		multiplier = 1024;
		arg = arg.substr(0, arg.length() - 1); // Remove the 'K'
	} else if (lastChar == 'M' || lastChar == 'm') {
		multiplier = 1024 * 1024;
		arg = arg.substr(0, arg.length() - 1); // Remove the 'M'
	} else if (lastChar == 'G' || lastChar == 'g') {
		multiplier = 1024 * 1024 * 1024;
		arg = arg.substr(0, arg.length() - 1); // Remove the 'G'
	}

	// Convert the remaining numbers
	size_t rawSize;
	try {
		rawSize = static_cast<size_t>(utils::stringToInt(arg));
	} catch (const std::exception& e) {
		throw std::runtime_error("Invalid value `" + originalArg
		                         + "` in `client_max_body_size` directive.");
	}
	size_t finalSize = rawSize * multiplier;

	server.setClientMaxBodySize(finalSize);

	expect(";");
}

void ConfigParser::parseServerErrorPage(ServerConfig& server) {
	advance();

	std::vector<int> errorCodes;
	std::string errorPath;

	while (tokens_[currentPos_] != ";") {
		// If the next token is a semicolon ';', the current token is the path
		if (currentPos_ + 1 < tokens_.size()
		    && tokens_[currentPos_ + 1] == ";") {
			errorPath = tokens_[currentPos_];
		} else {
			// Otherwise it's an error code
			int code = utils::stringToInt(tokens_[currentPos_]);
			if (code < 100 || code > 599) {
				throw std::runtime_error("Invalid HTTP Error Code: `"
				                         + tokens_[currentPos_] + "`");
			}
			errorCodes.push_back(code);
		}
		advance();
	}

	if (errorCodes.empty()) {
		throw std::runtime_error(
			"`error_page` directive requires at least one error code.");
	}

	if (errorPath.empty()) {
		throw std::runtime_error(
			"`error_page` directive requires a file path.");
	}

	// Add every code to the map, pointing to the same path
	for (size_t i = 0; i < errorCodes.size(); ++i) {
		server.addErrorPage(errorCodes[i], errorPath);
	}
}

// --- LOCATION DIRECTIVE HELPERS ------------------------------------------- //

void ConfigParser::parseLocationRoot(LocationConfig& location) {
	advance();

	std::string arg = tokens_[currentPos_];
	if (arg == ";") {
		throw std::runtime_error("Empty `root` directive in location block.");
	}

	location.setRootPath(arg);

	expect(";");
}

void ConfigParser::parseLocationIndex(LocationConfig& location) {
	advance();

	std::vector<std::string> indexFiles;
	while (tokens_[currentPos_] != ";") {
		std::string currentFile = tokens_[currentPos_];

		// If the file starts with '/', it MUST be the last element
		if (!currentFile.empty() && currentFile[0] == '/') {
			// Check if the next token is the semicolon ';'
			if (currentPos_ + 1 >= tokens_.size()
			    || tokens_[currentPos_ + 1] != ";") {
				throw std::runtime_error(
					"Syntax error: an absolute path (`" + currentFile
					+ "`) can only be the last element in an index directive.");
			}
		}

		indexFiles.push_back(currentFile);
		advance();
	}

	if (indexFiles.empty()) {
		throw std::runtime_error(
			"`index` directive in location block requires at least one file.");
	}

	location.setIndexFiles(indexFiles);
}

void ConfigParser::parseAutoindex(LocationConfig& location) {
	advance();

	std::string arg = tokens_[currentPos_];
	if (arg == "on") {
		location.setAutoindex(true);
	} else if (arg == "off") {
		location.setAutoindex(false);
	} else {
		throw std::runtime_error("Invalid argument for `autoindex`: `" + arg
		                         + "`. Expected `on` or `off`.");
	}

	expect(";");
}

void ConfigParser::parseLocationClientMaxBodySize(LocationConfig& location) {
	advance();

	std::string arg = tokens_[currentPos_];
	std::string originalArg = arg;
	if (arg == ";") {
		throw std::runtime_error(
			"Empty `client_max_body_size` in location block.");
	}
	if (!arg.empty() && arg[0] == '-') {
		throw std::runtime_error("Invalid value `" + arg
		                         + "` in `client_max_body_size` directive.");
	}

	size_t multiplier = 1;
	char lastChar = arg[arg.length() - 1];

	// Check if the last character is a letter
	if (lastChar == 'K' || lastChar == 'k') {
		multiplier = 1024;
		arg = arg.substr(0, arg.length() - 1); // Remove the 'K'
	} else if (lastChar == 'M' || lastChar == 'm') {
		multiplier = 1024 * 1024;
		arg = arg.substr(0, arg.length() - 1); // Remove the 'M'
	} else if (lastChar == 'G' || lastChar == 'g') {
		multiplier = 1024 * 1024 * 1024;
		arg = arg.substr(0, arg.length() - 1); // Remove the 'G'
	}

	// Convert the remaining numbers
	size_t rawSize;
	try {
		rawSize = static_cast<size_t>(utils::stringToInt(arg));
	} catch (const std::exception& e) {
		throw std::runtime_error("Invalid value `" + originalArg
		                         + "` in `client_max_body_size` directive.");
	}
	size_t finalSize = rawSize * multiplier;

	location.setClientMaxBodySize(finalSize);

	expect(";");
}

void ConfigParser::parseLocationErrorPage(LocationConfig& location) {
	advance();

	std::vector<int> errorCodes;
	std::string errorPath;

	while (tokens_[currentPos_] != ";") {
		// If the next token is a semicolon ';', the current token is the path
		if (currentPos_ + 1 < tokens_.size()
		    && tokens_[currentPos_ + 1] == ";") {
			errorPath = tokens_[currentPos_];
		} else {
			// Otherwise it's an error code
			int code = utils::stringToInt(tokens_[currentPos_]);
			if (code < 100 || code > 599) {
				throw std::runtime_error("Invalid HTTP Error Code: `"
				                         + tokens_[currentPos_] + "`");
			}
			errorCodes.push_back(code);
		}
		advance();
	}

	if (errorCodes.empty()) {
		throw std::runtime_error(
			"`error_page` directive requires at least one error code.");
	}

	if (errorPath.empty()) {
		throw std::runtime_error(
			"`error_page` directive requires a file path.");
	}

	// Add every code to the map, pointing to the same path
	for (size_t i = 0; i < errorCodes.size(); ++i) {
		location.addErrorPage(errorCodes[i], errorPath);
	}
}

void ConfigParser::parseAllowedMethods(LocationConfig& location) {
	advance();

	std::vector<std::string> methods;
	while (tokens_[currentPos_] != ";") {
		std::string method = tokens_[currentPos_];

		// Strictly limiting HTTP methods to GET, POST, and DELETE
		if (method != "GET" && method != "POST" && method != "DELETE") {
			throw std::runtime_error("Invalid or unsupported HTTP method: `"
			                         + method + "`");
		}

		methods.push_back(method);
		advance();
	}

	if (methods.empty()) {
		throw std::runtime_error(
			"`allow_methods` directive requires at least one method.");
	}

	location.setAllowedMethods(methods);
}

void ConfigParser::parseCgi(LocationConfig& location) {
	advance();

	std::string ext = tokens_[currentPos_];
	if (ext == ";") {
		throw std::runtime_error(
			"`cgi` directive requires an extension and a path.");
	}
	if (ext[0] != '.') {
		throw std::runtime_error("CGI extension must start with a dot '.': `"
		                         + ext + "`");
	}

	advance();

	std::string path = tokens_[currentPos_];
	if (path == ";") {
		throw std::runtime_error(
			"`cgi` directive requires an executable path after the extension.");
	}

	location.addCgiHandler(ext, path);

	expect(";");
}

void ConfigParser::parseUploadDir(LocationConfig& location) {
	advance();

	std::string arg = tokens_[currentPos_];
	if (arg == ";") {
		throw std::runtime_error("Empty `upload_store` directive.");
	}

	location.setUploadDir(arg);

	expect(";");
}

void ConfigParser::parseReturn(LocationConfig& location) {
	advance();

	std::string codeStr = tokens_[currentPos_];
	int code = utils::stringToInt(codeStr);

	if (code < 300 || code > 399) {
		throw std::runtime_error("Invalid redirect code: `" + codeStr
		                         + "`. Expected 3xx.");
	}
	advance();

	std::string url = tokens_[currentPos_];
	if (url == ";") {
		throw std::runtime_error(
			"`return` directive requires a destination URL.");
	}

	location.setReturn(std::make_pair(code, url));

	expect(";");
}

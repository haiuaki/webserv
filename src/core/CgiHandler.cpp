#include "core/CgiHandler.hpp"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "utils/Utils.hpp"

// --- CONSTRUCTOR ---------------------------------------------------------- //

CgiHandler::CgiHandler(const HttpRequest& request, const ServerConfig& server,
                       const LocationConfig* location,
                       const std::string& scriptPath,
                       const std::string& clientIp)
	: request_(request)
	, server_(server)
	, location_(location)
	, scriptPath_(scriptPath)
	, clientIp_(clientIp) {}

// --- PRIVATE HELPERS ------------------------------------------------------ //

void CgiHandler::buildEnvironment() {
	envStrings_.clear();
	envp_.clear();

	// Standard CGI Environment Variables
	envStrings_.push_back("GATEWAY_INTERFACE=CGI/1.1");
	envStrings_.push_back("SERVER_PROTOCOL=HTTP/1.1");
	envStrings_.push_back("SERVER_SOFTWARE=webserv/1.0");

	envStrings_.push_back("REQUEST_METHOD=" + request_.getMethod());
	envStrings_.push_back("SERVER_PORT=" + utils::toString(server_.getPort()));

	// Remote IP
	envStrings_.push_back("REMOTE_ADDR=" + clientIp_);

	// Separate URI from Query String
	std::string uri = request_.getUri();
	size_t questionMark = uri.find('?');
	std::string scriptName = uri;
	std::string queryString = "";
	if (questionMark != std::string::npos) {
		scriptName = uri.substr(0, questionMark);
		queryString = uri.substr(questionMark + 1);
	}

	envStrings_.push_back("SCRIPT_NAME=" + scriptName);
	envStrings_.push_back("PATH_INFO="
	                      + scriptName); // Simplification for 42 subject
	envStrings_.push_back("PATH_TRANSLATED=" + scriptPath_);
	envStrings_.push_back("QUERY_STRING=" + queryString);

	// Request specific headers
	if (request_.getHeaders().count("Content-Type")) {
		envStrings_.push_back("CONTENT_TYPE="
		                      + request_.getHeader("Content-Type"));
	}
	if (request_.getHeaders().count("Content-Length")) {
		envStrings_.push_back("CONTENT_LENGTH="
		                      + request_.getHeader("Content-Length"));
	}

	// Add all client headers prefixed with HTTP_
	std::map<std::string, std::string>::const_iterator it;
	const std::map<std::string, std::string>& headers = request_.getHeaders();
	for (it = headers.begin(); it != headers.end(); ++it) {
		std::string headerName = it->first;
		for (size_t i = 0; i < headerName.length(); ++i) {
			if (headerName[i] == '-')
				headerName[i] = '_';
			else
				headerName[i] = std::toupper(headerName[i]);
		}
		envStrings_.push_back("HTTP_" + headerName + "=" + it->second);
	}

	// Convert std::string to char* array for execve
	for (size_t i = 0; i < envStrings_.size(); ++i) {
		envp_.push_back(const_cast<char*>(envStrings_[i].c_str()));
	}
	envp_.push_back(NULL);
}

// --- METHOD --------------------------------------------------------------- //

void CgiHandler::execute(int& outReadFd, int& outWriteFd, pid_t& outPid) {
	buildEnvironment();

	int pipeIn[2];  // Server writes to [1], CGI reads from [0]
	int pipeOut[2]; // CGI writes to [1], Server reads from [0]

	if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1) {
		throw std::runtime_error("CGI pipe creation failed.");
	}

	pid_t pid = fork();
	if (pid == -1) {
		throw std::runtime_error("CGI fork failed.");
	}

	if (pid == 0) {
		// --- CHILD PROCESS ---
		// Redirect STDIN to read end of pipeIn
		dup2(pipeIn[0], STDIN_FILENO);
		// Redirect STDOUT to write end of pipeOut
		dup2(pipeOut[1], STDOUT_FILENO);

		// Close unused pipe ends
		close(pipeIn[0]);
		close(pipeIn[1]);
		close(pipeOut[0]);
		close(pipeOut[1]);

		// Extract correct CGI executable from location block
		std::string executable = "/usr/bin/php-cgi"; // Default fallback
		if (location_) {
			const std::map<std::string, std::string>& cgis =
				location_->getCgiHandlers();
			std::string ext = scriptPath_.substr(scriptPath_.find_last_of('.'));
			if (cgis.count(ext) > 0) {
				executable = cgis.at(ext);
			}
		}

		char* argv[] = {const_cast<char*>(executable.c_str()),
		                const_cast<char*>(scriptPath_.c_str()), NULL};

		execve(argv[0], argv, &envp_[0]);

		// If execve fails
		std::cerr << "execve failed for CGI\n";
		exit(1);
	} else {
		// --- PARENT PROCESS ---
		// Close the ends we don't need
		close(pipeIn[0]);
		close(pipeOut[1]);

		// Provide the relevant FDs and PID to the caller
		outWriteFd = pipeIn[1];
		outReadFd = pipeOut[0];
		outPid = pid;

		// Make the server's ends non-blocking
		fcntl(outWriteFd, F_SETFL, O_NONBLOCK | FD_CLOEXEC);
		fcntl(outReadFd, F_SETFL, O_NONBLOCK | FD_CLOEXEC);
	}
}

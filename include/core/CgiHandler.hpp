#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <map>
#include <string>
#include <vector>

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "network/Client.hpp"

/**
 * @brief Abstracts the POSIX complexity of launching a CGI process.
 *
 * This class translates HTTP request headers into RFC 3875 compliant
 * environment variables, safely creates non-blocking pipes, and forks
 * the server to execute the CGI script, returning the file descriptors
 * back to the caller for asynchronous tracking.
 */
class CgiHandler {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		CgiHandler(const HttpRequest& request, const ServerConfig& server,
		           const LocationConfig* location,
		           const std::string& scriptPath, const std::string& clientIp);

		// --- METHOD ------------------------------------------------------- //
		/**
		 * @brief Executes the CGI and populates the given FDs.
		 *
		 * @param outReadFd  Reference to store the read-end of the pipe.
		 * @param outWriteFd Reference to store the write-end of the pipe.
		 * @param outPid     Reference to store the child process PID.
		 * @throws std::runtime_error if pipe() or fork() fails.
		 */
		void execute(int& outReadFd, int& outWriteFd, pid_t& outPid);

	private:
		// --- HELPERS ------------------------------------------------------ //
		void buildEnvironment();

		// --- ATTRIBUTES --------------------------------------------------- //
		const HttpRequest& request_;
		const ServerConfig& server_;
		const LocationConfig* location_;
		std::string scriptPath_;
		std::string clientIp_;

		std::vector<std::string> envStrings_;
		std::vector<char*> envp_;
};

#endif

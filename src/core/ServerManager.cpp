#include "core/ServerManager.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "config/ServerConfig.hpp"
#include "core/Router.hpp"
#include "utils/Colors.hpp"

extern bool g_isRunning;

// --- CONSTRUCTOR & DESTRUCTOR --------------------------------------------- //

ServerManager::ServerManager(
	const std::map< std::string, std::vector<ServerConfig> >& cluster)
	: cluster_(cluster) {
	validateServers();
}

ServerManager::~ServerManager() {
	// Delete all Server objects (automatically closes their sockets)
	for (size_t i = 0; i < servers_.size(); ++i) {
		delete servers_[i];
	}
	servers_.clear();

	// Delete any active Client objects that were still connected
	std::map<int, Client*>::iterator it;
	for (it = clients_.begin(); it != clients_.end(); ++it) {
		delete it->second;
	}
	clients_.clear();
}

// --- METHODS -------------------------------------------------------------- //

void ServerManager::initServers() {
	std::map< std::string, std::vector<ServerConfig> >::iterator it;

	for (it = cluster_.begin(); it != cluster_.end(); ++it) {
		// The first config of this port determines the host IP
		const ServerConfig& firstConfig = it->second[0];

		// Dynamically allocate the Server to prevent vector resizing bugs
		Server* newServer = new Server(firstConfig.getHost(),
		                               firstConfig.getPort());

		// Open the socket and `listen()`
		newServer->setupSocket();

		// Attach all the virtual server configs to this socket
		for (size_t i = 0; i < it->second.size(); ++i) {
			newServer->addConfig(&(it->second[i]));
		}

		servers_.push_back(newServer);
	}
}

void ServerManager::run() {
	for (size_t i = 0; i < servers_.size(); ++i) {
		struct pollfd spfd;
		spfd.fd = servers_[i]->getListenFd(); // Grab the listening socket
		spfd.events = POLLIN; // Alert when data is ready to `recv()`
		spfd.revents = 0;
		pollFds_.push_back(spfd);
	}

	std::cout << '\n'
			  << GRAY << "[LOG] " << GREEN
			  << "Server is running. Waiting for connections.." << RESET
			  << '\n';

	while (g_isRunning) {
		int readyCount = poll(&pollFds_[0], pollFds_.size(), -1);

		if (readyCount < 0) {
			if (!g_isRunning) {
				break;
			}
			std::cerr << RED << "[ERROR] `poll()` failed.\n" << RESET;
			continue;
		}

		std::vector<struct pollfd>::iterator it;
		for (it = pollFds_.begin(); it != pollFds_.end();) {
			// Skip dormant socket
			if (it->revents == 0) {
				++it;
				continue;
			}
			// Is it a new browser trying to connect?
			if (it->revents & POLLIN && isServerFd(it->fd)) {
				acceptNewConnection(it->fd);
				++it;
			}
			// Is it a client sending us an HTTP Request?
			else if (it->revents & POLLIN) {
				if (!handleClientRequest(it->fd)) {
					it = closeConnection(it);
				} else {
					++it;
				}
			}
			// Are we ready to send an HTTP Response back?
			else if (it->revents & POLLOUT) {
				if (!handleClientResponse(it->fd)) {
					it = closeConnection(it);
				} else {
					// Switch back to reading for Keep-Alive if fully sent
					if (clients_[it->fd]->responseQueue.empty()) {
						it->events = POLLIN;
					}
					++it;
				}
			}
			// Did the client brutally disconnect?
			else if (it->revents & POLLHUP) {
				it = closeConnection(it);
			} else {
				++it;
			}
		}

		// Safely append new connections after the iterator loop finishes
		if (!newPollFds_.empty()) {
			pollFds_.insert(pollFds_.end(), newPollFds_.begin(),
			                newPollFds_.end());
			newPollFds_.clear();
		}
	}
}

void ServerManager::validateServers() {
	// Loop through every Host:Port combination we need to listen on
	std::map< std::string, std::vector<ServerConfig> >::iterator it;
	for (it = cluster_.begin(); it != cluster_.end(); ++it) {
		const std::string& hostPort = it->first;
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

// --- PRIVATE HELPERS ------------------------------------------------------ //

bool ServerManager::isServerFd(int fd) {
	for (size_t i = 0; i < servers_.size(); ++i) {
		if (fd == servers_[i]->getListenFd()) {
			return true;
		}
	}
	return false;
}

void ServerManager::acceptNewConnection(int serverFd) {
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);

	// Accept the incoming connection
	int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
	if (clientFd == -1) {
		std::cerr << RED << "[ERROR] `accept()` failed for a new connection."
				  << RESET << '\n';
		return;
	}

	// Extract Client IP for future CGI usage
	std::string clientIP = inet_ntoa(clientAddr.sin_addr);

	// Make the socket non-blocking and close-on-exec
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK | FD_CLOEXEC) == -1) {
		std::cerr << RED << "[ERROR] `fcntl()` failed on client FD " << clientFd
				  << RESET << '\n';
		close(clientFd);
		return;
	}

	// Safely construct the poll structure
	struct pollfd spfd;
	std::memset(&spfd, 0, sizeof(spfd));
	spfd.fd = clientFd;
	spfd.events = POLLIN;
	spfd.revents = 0;

	// Append to a safe temporary queue (prevents Iterator Segfaults)
	newPollFds_.push_back(spfd);

	// Create the persistent Client object
	clients_[clientFd] = new Client(clientFd, serverFd, clientIP);

	std::cout << GRAY << "[LOG] " << CYAN << "New connection from " << clientIP
			  << " established on FD: " << clientFd << RESET << '\n';
}

bool ServerManager::handleClientRequest(int clientFd) {
	char buffer[4096];
	int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead > 0) {
		// Feed the raw binary data into the HttpRequest state machine
		clients_[clientFd]->appendRequestData(buffer, bytesRead);
		return processParsedRequest(clientFd);
	} else if (bytesRead == 0) {
		// Browser closed the tab or connection gracefully
		return false;
	} else {
		// A network error occurred
		std::cerr << RED << "[ERROR] `recv()` failed on client FD " << clientFd
				  << RESET << '\n';
		return false;
	}
}

bool ServerManager::processParsedRequest(int clientFd) {
	if (clients_[clientFd]->request.isComplete()) {
		std::cout << GRAY << "[LOG] " << GREEN
				  << "Successfully Parsed Request from FD " << clientFd << ":\n"
				  << "      Method: " << clients_[clientFd]->request.getMethod()
				  << "\n"
				  << "      URI:    " << clients_[clientFd]->request.getUri()
				  << "\n"
				  << RESET;

		// Route the request
		const ServerConfig* targetConfig = NULL;
		int serverFd = clients_[clientFd]->getServerFd();
		for (size_t i = 0; i < servers_.size(); ++i) {
			if (servers_[i]->getListenFd() == serverFd) {
				std::string host = clients_[clientFd]->request.getHeader(
					"Host");
				const std::vector<const ServerConfig*>& configs =
					servers_[i]->getConfigs();

				targetConfig = configs[0]; // Fallback
				for (size_t j = 0; j < configs.size(); ++j) {
					const std::vector<std::string>& names =
						configs[j]->getServerNames();
					for (size_t k = 0; k < names.size(); ++k) {
						if (names[k] == host) {
							targetConfig = configs[j];
							break;
						}
					}
				}
				break;
			}
		}

		if (targetConfig) {
			Router::route(clients_[clientFd]->request,
			              clients_[clientFd]->response, *targetConfig);

			// Serialize exactly ONCE to the outgoing queue buffer
			clients_[clientFd]->responseQueue =
				clients_[clientFd]->response.serialize();

			// Find this client in pollFds_ to set POLLOUT
			for (size_t i = 0; i < pollFds_.size(); ++i) {
				if (pollFds_[i].fd == clientFd) {
					pollFds_[i].events = POLLOUT;
					break;
				}
			}
		}
		return true;
	} else if (clients_[clientFd]->request.hasError()) {
		std::cout << GRAY << "[LOG] " << RED << "Bad Request from FD "
				  << clientFd << ": "
				  << clients_[clientFd]->request.getErrorMessage() << '\n'
				  << RESET;
		return false;
	} else {
		// Request's still parsing, wait for more data from `poll()`
		return true;
	}
}

bool ServerManager::handleClientResponse(int clientFd) {
	Client* client = clients_[clientFd];

	ssize_t bytesSent = send(clientFd, client->responseQueue.c_str(),
	                         client->responseQueue.length(), 0);

	if (bytesSent > 0) {
		// Slice off what was successfully sent
		client->responseQueue.erase(0, bytesSent);

		// Is there more to send?
		if (!client->responseQueue.empty()) {
			std::cout << GRAY << "[LOG] " << MAGENTA
					  << "Partial response sent to FD " << clientFd << " ("
					  << bytesSent << " bytes). Waiting to send more..."
					  << RESET << '\n';
			return true; // Keep POLLOUT active!
		}

		std::cout << GRAY << "[LOG] " << MAGENTA << "Response fully sent to FD "
				  << clientFd << "!" << RESET << '\n';

		// Keep-Alive check! HTTP/1.1 defaults to keep-alive unless 'Connection:
		// close' is explicitly sent by client.
		if (client->request.getHeader("Connection") == "close") {
			return false; // Close the connection
		}

		// Keep-Alive! Clear state for the next request.
		client->request.clear();
		client->response.clear();

		// Immediately trigger parsing to catch any pipelined requests sitting
		// in the buffer
		client->request.parse("");
		if (client->request.isComplete() || client->request.hasError()) {
			return processParsedRequest(clientFd);
		}

		return true;
	} else if (bytesSent == 0) {
		return false;
	} else {
		std::cerr << RED << "[ERROR] `send()` failed on client FD " << clientFd
				  << RESET << '\n';
		return false;
	}
}

std::vector<struct pollfd>::iterator
ServerManager::closeConnection(std::vector<struct pollfd>::iterator it) {
	std::cout << GRAY << "[LOG] " << YELLOW << "Client FD " << it->fd
			  << " disconnected." << RESET << '\n';

	// Delete the persistent client object if it exists
	if (clients_.find(it->fd) != clients_.end()) {
		delete clients_[it->fd];
		clients_.erase(it->fd);
	}

	// Physically close the network socket
	close(it->fd);

	// Erase from the poll array and return safe iterator
	return pollFds_.erase(it);
}

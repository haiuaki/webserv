#include "core/ServerManager.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "config/ServerConfig.hpp"
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
				handleClientResponse(it->fd);
				++it;
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

	// Make the socket non-blocking
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1) {
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
	clients_[clientFd] = new Client(clientFd, clientIP);

	std::cout << GRAY << "[LOG] " << CYAN << "New connection from " << clientIP
			  << " established on FD: " << clientFd << RESET << '\n';
}

bool ServerManager::handleClientRequest(int clientFd) {
	char buffer[4096];
	int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead > 0) {
		// Append the raw binary data to the client's persistent buffer
		clients_[clientFd]->appendRequestData(buffer, bytesRead);

		std::cout << GRAY << "[LOG] " << BLUE << "Read " << bytesRead
				  << " bytes from client FD " << clientFd
				  << " (Total Buffer Size: "
				  << clients_[clientFd]->getRequestBuffer().size()
				  << " bytes)\n"
				  << RESET;
		return true; // Keep connection open
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

void ServerManager::handleClientResponse(int clientFd) {
	std::cout << GRAY << "[LOG] " << MAGENTA
			  << "Sending response to client FD: " << clientFd << '.' << RESET
			  << '\n';
	// TODO: send()
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

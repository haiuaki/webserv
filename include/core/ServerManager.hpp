#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP

#include <map>
#include <vector>

#include "config/ServerConfig.hpp"
#include "network/Client.hpp"
#include "network/Server.hpp"

/**
 * @brief Core network multiplexer and event loop manager.
 *
 * ServerManager is responsible for opening all server sockets, monitoring them
 * for incoming events using `poll()`, and managing the lifecycle of all
 * active Client connections without blocking the main thread.
 */
class ServerManager {
	public:
		// --- CONSTRUCTOR & DESTRUCTOR ------------------------------------- //
		/**
		 * @brief Constructs the ServerManager and validates the parsed cluster.
		 *
		 * @param cluster Map grouping server configs by their "host:port" keys.
		 */
		ServerManager(
			const std::map< std::string, std::vector<ServerConfig> >& cluster);

		/// Destroys the ServerManager, safely tearing down all sockets and
		/// cleaning up clients.
		~ServerManager();

		// --- METHODS ------------------------------------------------------ //
		/// Dynamically allocates and initializes all physical server sockets.
		void initServers();

		/// Triggers the infinite `poll()` loop, intercepting TCP events.
		void run();

	private:
		// --- METHOD ------------------------------------------------------- //
		/// Validates that no multiple servers share the same name on a given
		/// port.
		void validateServers();

		// --- HELPERS ------------------------------------------------------ //
		/**
		 * @brief Checks if a given file descriptor belongs to a listening
		 *        server socket.
		 *
		 * @param fd The file descriptor to check.
		 *
		 * @return true if it is a server socket, false if it is a client
		 *         connection.
		 */
		bool isServerFd(int fd);
		/**
		 * @brief Accepts a new client connection on the specified server
		 *        socket.
		 *
		 * @param serverFd The file descriptor of the listening server.
		 */
		void acceptNewConnection(int serverFd);
		/**
		 * @brief Reads incoming HTTP request data from an active client socket.
		 *
		 * @param clientFd The file descriptor of the client sending data.
		 *
		 * @return true if data was successfully read, false if the connection
		 *         dropped.
		 */
		bool handleClientRequest(int clientFd);
		/**
		 * @brief Generates and transmits the HTTP response back to the client.
		 *
		 * @param clientFd The file descriptor of the client expecting a
		 *                 response.
		 */
		void handleClientResponse(int clientFd);
		/**
		 * @brief Safely shuts down a client socket and purges its memory.
		 *
		 * @param it Iterator pointing to the active `pollfd` struct.
		 *
		 * @return A safe iterator pointing to the next active element in the
		 *         `pollFds_` vector.
		 */
		std::vector<struct pollfd>::iterator
		closeConnection(std::vector<struct pollfd>::iterator it);

		// --- ATTRIBUTES --------------------------------------------------- //
		std::map< std::string, std::vector<ServerConfig> > cluster_;
		std::vector<Server*> servers_;
		std::vector<struct pollfd> pollFds_;
		std::vector<struct pollfd> newPollFds_;
		std::map<int, Client*> clients_;
};

#endif

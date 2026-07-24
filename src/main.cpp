#include <csignal>
#include <cstring>
#include <exception>
#include <iostream>

#include "config/ConfigParser.hpp"
#include "core/ServerManager.hpp"
#include "utils/Colors.hpp"

bool g_isRunning = true;

static void handleSigInt(int signal) {
	(void)signal;
	std::cout << "\n"
			  << GRAY << "[LOG] " << YELLOW << "Shutting down web server.."
			  << RESET << '\n';
	g_isRunning = false;
}

static void setupSignals() {
	// Shut down on Ctrl+C
	struct sigaction saInt;
	std::memset(&saInt, 0, sizeof(saInt));
	saInt.sa_handler = handleSigInt;
	sigemptyset(&saInt.sa_mask);
	sigaction(SIGINT, &saInt, NULL);

	// Ignore broken pipes
	struct sigaction saPipe;
	std::memset(&saPipe, 0, sizeof(saPipe));
	saPipe.sa_handler = SIG_IGN;
	sigemptyset(&saPipe.sa_mask);
	sigaction(SIGPIPE, &saPipe, NULL);
}

int main(int ac, char** av) {
	setupSignals();

	if (ac > 2) {
		std::cerr << RED << "Usage: " << av[0] << " [configuration_file]"
				  << RESET << '\n';
		return 1;
	}

	// Default configuration file if none is provided
	std::string configFile = "conf/default.conf";

	// Override default configuration file if one is provided
	if (ac == 2) {
		configFile = av[1];
	}

	try {
		// Parse the syntax of the configuration file
		ConfigParser parser(configFile);
		std::cout << GRAY << "[LOG] " << GREEN
				  << "Configuration loaded successfully." << RESET << '\n';

		// Validate routing logic and initialize `ServerManager`
		ServerManager manager(parser.getServers());
		std::cout << GRAY << "[LOG] " << GREEN
				  << "Server clusters validated and ready." << RESET << '\n';

		manager.initServers();
		manager.run();
	} catch (const std::exception& e) {
		std::cerr << RED << "[ERROR] " << e.what() << RESET << '\n';
		return 1;
	}

	return 0;
}

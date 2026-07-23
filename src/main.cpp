#include <exception>
#include <iostream>

#include "config/ConfigParser.hpp"
#include "core/ServerManager.hpp"
#include "utils/Colors.hpp"

int main(int ac, char** av) {
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
		std::cout << GREEN << "[ SUCCESS ] Configuration loaded successfully."
				  << RESET << '\n';

		// Validate routing logic and initialize `ServerManager`
		ServerManager manager(parser.getServers());
		std::cout << GREEN << "[ SUCCESS ] Server clusters validated and ready."
				  << RESET << '\n';

		// TODO: manager.start()
	} catch (const std::exception& e) {
		std::cerr << RED << "[ FATAL ERROR ] " << e.what() << RESET << '\n';
		return 1;
	}

	return 0;
}

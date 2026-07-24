#include <exception>
#include <iostream>
#include <string>

#include "config/ConfigParser.hpp"
#include "core/ServerManager.hpp"
#include "utils/Colors.hpp"

bool g_isRunning = true;

int main(int ac, char** av) {
	if (ac != 2) {
		std::cerr << RED << "Usage: " << av[0] << " <configuration file>"
				  << RESET << '\n';
		return 1;
	}

	std::string file = av[1];
	std::cout << CYAN << "Testing: " << file << RESET << " ... ";

	try {
		ConfigParser parser(file);
		ServerManager manager(parser.getServers());
		std::cout << GREEN << "[ SUCCESS ]" << RESET << '\n';
		parser.printTokens();
	} catch (const std::exception& e) {
		std::cout << YELLOW << "[ CAUGHT ERROR ] -> " << RED << e.what()
				  << RESET << '\n';
	}

	return 0;
}

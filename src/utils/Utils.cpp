#include "utils/Utils.hpp"

#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>

namespace utils {

int stringToInt(const std::string& s) {
	std::istringstream ss(s);
	int result;

	if (!(ss >> result) || !ss.eof()) {
		throw std::runtime_error("Invalid number format: `" + s + "`");
	}

	return result;
}

std::string trim(const std::string& s) {
	const std::string ws = " \t\n\r\f\v";

	size_t first = s.find_first_not_of(ws);
	if (first == std::string::npos) {
		return "";
	}
	size_t last = s.find_last_not_of(ws);

	return (s.substr(first, (last - first + 1)));
}

} // namespace utils

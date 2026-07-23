#include "utils/Utils.hpp"

#include <sstream>
#include <stdexcept>

namespace utils {

int stringToInt(const std::string& s) {
	std::istringstream ss(s);
	int result;

	if (!(ss >> result) || !ss.eof()) {
		throw std::runtime_error("Invalid number format: `" + s + "`");
	}

	return result;
}

} // namespace utils

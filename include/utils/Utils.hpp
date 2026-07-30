#ifndef UTILS_HPP
#define UTILS_HPP

#include <sstream>
#include <string>

namespace utils {

/**
 * @brief Safely converts a string to an integer
 *
 * Uses a `stringstream` to extract an integer from the provided string.
 * It ensures strict type safety and guarantees that the entire string is a
 * valid number.
 *
 * @param s The string to be converted (e.g., "8080").
 *
 * @return int The converted integer value.
 *
 * @throws std::runtime_error If the string contains non-numeric characters,
 *                            or if the number overflows a standard integer.
 */
int stringToInt(const std::string& s);

/**
 * @brief Trims leading and trailing whitespace from a string.
 *
 * @param s The string to trim.
 *
 * @return std::string The trimmed string.
 */
std::string trim(const std::string& s);

/**
 * @brief Converts any streamable data type to `std::string`.
 *
 * This template acts as a C++98-compliant alternative to C++11's
 * `std::to_string()`. It uses a `std::stringstream` to safely format standard
 * data types (e.g., int, size_t, float) into their string representations.
 *
 * @tparam T    The data type of the value to be converted.
 * @param value The value to convert into a string (e.g., 8080).
 *
 * @return std::string The string representation of the value.
 */
template <typename T> std::string toString(const T& value) {
	std::ostringstream ss;
	ss << value;
	return ss.str();
}

} // namespace utils

#endif

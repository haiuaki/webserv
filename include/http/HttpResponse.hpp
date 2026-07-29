#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <map>
#include <string>

/**
 * @brief Represents an outgoing HTTP response.
 *
 * Provides an interface to construct HTTP responses safely and easily
 * by setting status codes, headers, and the message body. Can be safely
 * serialized into a raw network string to send back to the client.
 */
class HttpResponse {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		HttpResponse();

		// --- GETTERS ------------------------------------------------------ //
		int getStatusCode() const;
		const std::string& getHeader(const std::string& key) const;
		const std::string& getBody() const;

		// --- SETTERS ------------------------------------------------------ //
		void setStatusCode(int statusCode);
		void setHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);

		// --- METHODS ------------------------------------------------------ //
		/// Generates the raw HTTP string to send over the TCP socket
		std::string serialize() const;
		static std::string getReasonPhrase(int statusCode);

		/// Clears the response to be reused for the next request on a
		/// Keep-Alive connection
		void clear();

	private:
		// --- HELPER ------------------------------------------------------- //
		/// Returns the human-readable text for a status code
		/// (e.g., 404 -> "Not Found")
		

		// --- ATTRIBUTES --------------------------------------------------- //
		int statusCode_;
		std::map<std::string, std::string> headers_;
		std::string body_;
};

#endif

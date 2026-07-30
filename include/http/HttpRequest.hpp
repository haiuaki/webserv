#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cstddef>
#include <map>
#include <string>

// --- ENUM ----------------------------------------------------------------- //

/// Represents the current phase of the HTTP parsing state machine.
enum ParseState {
	STATE_REQUEST_LINE, // Waiting to parse "GET / HTTP/1.1\r\n"
	STATE_HEADERS,      // Waiting to parse all "Key: Value\r\n" pairs
	STATE_BODY,         // Waiting to download the payload
	STATE_COMPLETE,     // Request is fully downloaded and parsed
	STATE_ERROR         // Invalid syntax or bad request
};

// --- CLASS ---------------------------------------------------------------- //

/**
 * @brief Incrementally parses raw TCP stream data into a structured HTTP
 *        Request.
 *
 * Uses a `Finite State Machine` to safely handle fragmented network packets
 * without blocking the main ServerManager poll() loop.
 */
class HttpRequest {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		HttpRequest();

		// --- GETTERS ------------------------------------------------------ //

		/// Returns true if the request is 100% downloaded and ready for routing
		bool isComplete() const;

		/// Returns true if a syntax error or bad request was detected
		bool hasError() const;

		/// Returns the HTTP status code representing the error
		int getErrorCode() const;

		/// Returns the specific error message if `hasError()` is true
		const std::string& getErrorMessage() const;

		const std::string& getMethod() const;
		const std::string& getUri() const;
		const std::string& getVersion() const;
		const std::map<std::string, std::string>& getHeaders() const;
		const std::string& getHeader(const std::string& key) const;
		const std::string& getBody() const;

		// --- METHOD ------------------------------------------------------- //
		/**
		 * @brief Feeds raw TCP bytes into the parser state machine.
		 *
		 * @param rawData The string chunk captured by `recv()`.
		 */
		void parse(const std::string& rawData);
		void clear();

	private:
		// --- HELPERS ------------------------------------------------------ //

		/// Parses "GET /index.html HTTP/1.1\r\n" */
		void parseRequestLine();

		/// Parses Key-Value headers until an empty "\r\n" is found */
		void parseHeaders();

		/// Downloads the payload based on the `Content-Length` header */
		void parseBody();

		/// Iteratively decodes a `Transfer-Encoding:` chunked payload */
		void parseChunkedBody();

		// --- ATTRIBUTES --------------------------------------------------- //
		ParseState state_;
		std::string rawBuffer_;
		int errorCode_;
		std::string errorMessage_;

		std::string method_;
		std::string uri_;
		std::string version_;
		std::map<std::string, std::string> headers_;
		std::string body_;

		bool isParsingChunkSize_;
		size_t expectedChunkSize_;
};

#endif

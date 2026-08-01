#include "http/HttpRequest.hpp"

#include <cstddef>
#include <map>
#include <sstream>
#include <string>
#include <utils/Utils.hpp>

// --- CONSTRUCTOR ---------------------------------------------------------- //

HttpRequest::HttpRequest()
	: state_(STATE_REQUEST_LINE)
	, errorCode_(0)
	, isParsingChunkSize_(true)
	, expectedChunkSize_(0) {}

// --- GETTERS -------------------------------------------------------------- //

bool HttpRequest::isComplete() const {
	return state_ == STATE_COMPLETE;
}

bool HttpRequest::hasError() const {
	return state_ == STATE_ERROR;
}

int HttpRequest::getErrorCode() const {
	return errorCode_;
}

const std::string& HttpRequest::getErrorMessage() const {
	return errorMessage_;
}

const std::string& HttpRequest::getMethod() const {
	return method_;
}

const std::string& HttpRequest::getUri() const {
	return uri_;
}

const std::string& HttpRequest::getVersion() const {
	return version_;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
	return headers_;
}

const std::string& HttpRequest::getHeader(const std::string& key) const {
	std::map<std::string, std::string>::const_iterator it = headers_.find(key);
	if (it != headers_.end()) {
		return it->second;
	}

	// 'static' keeps the memory alive permanently, preventing segfaults
	static const std::string empty = "";
	return empty;
}

const std::string& HttpRequest::getBody() const {
	return body_;
}

// --- METHOD --------------------------------------------------------------- //

void HttpRequest::parse(const std::string& rawData) {
	if (state_ == STATE_COMPLETE || state_ == STATE_ERROR) {
		return;
	}

	rawBuffer_ += rawData;

	ParseState previousState;
	do {
		previousState = state_;

		if (state_ == STATE_REQUEST_LINE) {
			parseRequestLine();
		} else if (state_ == STATE_HEADERS) {
			parseHeaders();
		} else if (state_ == STATE_BODY) {
			if (getHeader("Transfer-Encoding") == "chunked") {
				parseChunkedBody();
			} else {
				parseBody();
			}
		}
	} while (state_ != STATE_COMPLETE && state_ != STATE_ERROR
	         && state_ != previousState);
}

void HttpRequest::clear() {
	state_ = STATE_REQUEST_LINE;
	// We MUST NOT clear rawBuffer_ here, so that pipelined requests survive!
	errorCode_ = 0;
	errorMessage_.clear();
	
	method_.clear();
	uri_.clear();
	version_.clear();
	headers_.clear();
	body_.clear();
	
	isParsingChunkSize_ = true;
	expectedChunkSize_ = 0;
}

// --- HELPERS -------------------------------------------------------------- //

void HttpRequest::parseRequestLine() {
	// Check if a full line has been received
	size_t pos = rawBuffer_.find("\r\n");
	if (pos == std::string::npos) {
		return; // Wait for more data from `poll()`
	}

	// Extract full line from buffer (e.g., "GET /index.html HTTP/1.1")
	std::string line = rawBuffer_.substr(0, pos);

	// Delete the parsed line from the buffer
	rawBuffer_.erase(0, pos + 2); // +2 to remove "\r\n"

	// Split the line into Method, URI and Version
	std::istringstream stream(line);
	stream >> method_ >> uri_ >> version_;

	if (method_ != "GET" && method_ != "POST" && method_ != "DELETE") {
		state_ = STATE_ERROR;
		errorCode_ = 501; // Not Implemented
		errorMessage_ = "Unsupported Method";
		return;
	}

	if (version_ != "HTTP/1.1" && version_ != "HTTP/1.0") {
		state_ = STATE_ERROR;
		errorCode_ = 505;
		errorMessage_ = "Unsupported HTTP Version";
		return;
	}

	state_ = STATE_HEADERS;
}

void HttpRequest::parseHeaders() {
	while (true) {
		size_t pos = rawBuffer_.find("\r\n");
		if (pos == std::string::npos) {
			return; // Wait for more data from `poll()`
		}

		// If line is just "\r\n", it's the end of the headers
		if (pos == 0) {
			rawBuffer_.erase(0, 2);
			state_ = STATE_BODY;
			return;
		}

		// Extract header (e.g., "Host: localhost")
		std::string line = rawBuffer_.substr(0, pos);
		rawBuffer_.erase(0, pos + 2);

		// Split by the colon to get the Key and Value
		size_t colon = line.find(':');
		if (colon != std::string::npos) {
			std::string key = line.substr(0, colon);
			std::string value = utils::trim(line.substr(colon + 1));
			headers_[key] = value;
		}
	}
}

void HttpRequest::parseBody() {
	if (headers_.count("Content-Length") > 0) {
		size_t expectedSize = utils::stringToInt(headers_["Content-Length"]);

		// Only extract the exact number of bytes we still need
		size_t bytesNeeded = expectedSize - body_.length();
		size_t bytesToTake = std::min(bytesNeeded, rawBuffer_.length());

		body_ += rawBuffer_.substr(0, bytesToTake);
		rawBuffer_.erase(0, bytesToTake);

		if (body_.length() >= expectedSize) {
			// If we downaloaded all bytes, the request is finished
			state_ = STATE_COMPLETE;
		}
	} else if (headers_.count("Transfer-Encoding") > 0) {
		if (headers_["Transfer-Encoding"] == "chunked") {
			parseChunkedBody();
		} else {
			// HTTP/1.1 allows other encodings (e.g. gzip, deflate)
			state_ = STATE_ERROR;
			errorCode_ = 501; // Not Implemented
			errorMessage_ = "Unsupported Transfer-Encoding";
		}
	} else {
		// No `Content-Length` and no `Transfer-Encoding` means there is no body
		state_ = STATE_COMPLETE;
	}
}

void HttpRequest::parseChunkedBody() {
	while (true) {
		if (isParsingChunkSize_) {
			// Look for "\r\n" to extract the chunk size
			size_t pos = rawBuffer_.find("\r\n");
			if (pos == std::string::npos) {
				return; // Wait for more data from `poll()`
			}

			std::string hexStr = rawBuffer_.substr(0, pos);
			rawBuffer_.erase(0, pos + 2); // Remove the size line

			// Convert hexadecimal string to integer
			std::stringstream ss;
			ss << std::hex << hexStr;
			ss >> expectedChunkSize_;

			// If chunk size is 0, the body is completely finished
			if (expectedChunkSize_ == 0) {
				state_ = STATE_COMPLETE;
				return;
			}
			isParsingChunkSize_ = false;
		} else {
			// Wait until we've downloaded the entire chunk and the "\r\n"
			if (rawBuffer_.length() >= expectedChunkSize_ + 2) {
				body_ += rawBuffer_.substr(0, expectedChunkSize_);
				rawBuffer_.erase(0, expectedChunkSize_ + 2);

				// Loop back and start looking for the next chunk size
				isParsingChunkSize_ = true;
			} else {
				return; // Wait for more data from `poll()`
			}
		}
	}
}

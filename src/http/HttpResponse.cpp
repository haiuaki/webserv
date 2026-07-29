#include "http/HttpResponse.hpp"

#include <map>
#include <sstream>
#include <string>

#include "utils/Utils.hpp"

// --- CONSTRUCTOR ---------------------------------------------------------- //

HttpResponse::HttpResponse() : statusCode_(200) {}

// --- GETTERS -------------------------------------------------------------- //

int HttpResponse::getStatusCode() const {
	return statusCode_;
}

const std::string& HttpResponse::getHeader(const std::string& key) const {
	std::map<std::string, std::string>::const_iterator it = headers_.find(key);
	if (it != headers_.end()) {
		return it->second;
	}

	// 'static' keeps it alive, preventing segfaults
	static const std::string empty = "";
	return empty;
}

const std::string& HttpResponse::getBody() const {
	return body_;
}

// --- SETTERS -------------------------------------------------------------- //

void HttpResponse::setStatusCode(int statusCode) {
	statusCode_ = statusCode;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
	headers_[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
	body_ = body;

	setHeader("Content-Length", utils::toString(body_.length()));
}

// --- METHODS -------------------------------------------------------------- //

std::string HttpResponse::serialize() const {
	std::ostringstream response;

	// Status Line: HTTP/1.1 200 OK
	response << "HTTP/1.1 " << statusCode_ << " "
			 << getReasonPhrase(statusCode_) << "\r\n";

	// Headers
	std::map<std::string, std::string>::const_iterator it = headers_.begin();
	for (; it != headers_.end(); ++it) {
		response << it->first << ": " << it->second << "\r\n";
	}

	// Empty line indicating end of headers
	response << "\r\n";

	// Body
	response << body_;

	return response.str();
}

void HttpResponse::clear() {
	statusCode_ = 200;
	headers_.clear();
	body_.clear();
}

// --- HELPER --------------------------------------------------------------- //

std::string HttpResponse::getReasonPhrase(int statusCode) {
	switch (statusCode) {
		case 200:
			return "OK";
		case 201:
			return "Created";
		case 204:
			return "No Content";
		case 301:
			return "Moved Permanently";
		case 302:
			return "Found";
		case 400:
			return "Bad Request";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 413:
			return "Payload Too Large";
		case 500:
			return "Internal Server Error";
		case 501:
			return "Not Implemented";
		case 505:
			return "HTTP Version Not Supported";
		default:
			return "Unknown Status";
	}
}

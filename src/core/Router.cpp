#include "core/Router.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

// --- MAIN ENTRY ----------------------------------------------------------- //

void Router::route(const HttpRequest& request, HttpResponse& response,
                   const ServerConfig& server) {
	if (request.hasError()) {
		generateErrorResponse(request.getErrorCode(), response, server, NULL);
		return;
	}

	const LocationConfig* location = matchLocation(request.getUri(), server);

	const std::string& method = request.getMethod();

	if (location) {
		const std::vector<std::string>& allowed = location->getAllowedMethods();
		// If the config has listen methods, check for them
		// (if it's empty, all standard methods are allowed by default)
		if (!allowed.empty()) {
			if (std::find(allowed.begin(), allowed.end(), method)
			    == allowed.end()) {
				// The method was not found in the list of allowed methods
				// (405 Method not allowed)
				generateErrorResponse(405, response, server, location);
				return;
			}
		}
	}

	typedef void (*MethodHandler)(const HttpRequest&, HttpResponse&,
	                              const ServerConfig&, const LocationConfig*);
	static std::map<std::string, MethodHandler> handlers;
	if (handlers.empty()) {
		handlers["GET"] = &Router::handleGet;
		// handlers["POST"] = &Router::handlePost;
		handlers["DELETE"] = &Router::handleDelete;
	}

	if (handlers.count(method) > 0) {
		handlers[method](request, response, server, location);
	} else {
		// (501 Not Implemented)
		generateErrorResponse(501, response, server, location);
	}
}

// --- MATCHER -------------------------------------------------------------- //

const LocationConfig* Router::matchLocation(const std::string& uri,
                                            const ServerConfig& server) {
	const std::map<std::string, LocationConfig>& locations =
		server.getLocations();
	std::map<std::string, LocationConfig>::const_iterator it;

	const LocationConfig* bestMatch = NULL;
	size_t longestMatchLength = 0;
	for (it = locations.begin(); it != locations.end(); ++it) {
		const std::string& locationPath = it->first;

		// Check if `.find()` returns index 0, if URI starts with this path
		if (uri.find(locationPath) == 0) {
			// If it matches, is it the longest match ?
			if (locationPath.length() > longestMatchLength) {
				longestMatchLength = locationPath.length();
				bestMatch = &(it->second); // Save a pointer to the config
			}
		}
	}

	return bestMatch;
}

// --- METHOD HANDLERS ------------------------------------------------------ //

void Router::handleGet(const HttpRequest& request, HttpResponse& response,
                       const ServerConfig& server,
                       const LocationConfig* location) {
	// Determine the root, fallbacks to server root if no location matched
	std::string root = (location != NULL) ? location->getRootPath()
	                                      : server.getRootPath();

	// Safety fallback in case the config didn't specify a root
	if (root.empty()) {
		root = "./html";
	}

	// Build the file path (e.g., "./html" + "/index.html")
	std::string filepath = root + request.getUri();

	// If they request a directory, assume "index.html"
	if (filepath[filepath.length() - 1] == '/') {
		filepath += "index.html";
	}

	// Try to open the file
	std::ifstream file(filepath.c_str());
	if (!file.is_open()) {
		// 404 Not Found
		generateErrorResponse(404, response, server, location);
		return;
	}

	// Read the entire file into a string buffer
	std::ostringstream oss;
	oss << file.rdbuf();

	// Build the successful respone (200 OK)
	response.setStatusCode(200);
	response.setHeader("Content-Type", "text/html");
	response.setBody(oss.str());
}

void Router::handlePost(const HttpRequest& request, HttpResponse& response,
                        const ServerConfig& server,
                        const LocationConfig* location) {
	// TODO: Build the CGI execution engine first
	(void)request;
	(void)response;
	(void)server;
	(void)location;
}

void Router::handleDelete(const HttpRequest& request, HttpResponse& response,
                          const ServerConfig& server,
                          const LocationConfig* location) {
	// Determine the root, fallbacks to server root if no location matched
	std::string root = (location != NULL) ? location->getRootPath()
	                                      : server.getRootPath();
	if (root.empty()) {
		root = "./html";
	}

	// Build the target file path
	std::string filepath = root + request.getUri();

	// Prevent directory deletion for safety
	if (filepath[filepath.length() - 1] == '/') {
		// Not allowed to delete a directory (403 Forbidden)
		generateErrorResponse(403, response, server, location);
		return;
	}

	// Attempt to physically delete the file
	if (std::remove(filepath.c_str()) == 0) {
		// 204 No Content (Standard REST response for successful deletion)
		response.setStatusCode(204);
	} else {
		// Check the standard error number to see exactly why it failed
		if (errno == EACCES) {
			// Permission Denied (403 Forbidden)
			generateErrorResponse(403, response, server, location);
		} else {
			// File doesn't exist or other error (404 Not Found)
			generateErrorResponse(404, response, server, location);
		}
	}
}

// --- ERROR HANDLER -------------------------------------------------------- //

void Router::generateErrorResponse(int statusCode, HttpResponse& response,
                                   const ServerConfig& server,
                                   const LocationConfig* location) {
	std::string customPagePath;

	// Check if a custom error page is defined in the location block
	if (location != NULL) {
		const std::map<int, std::string>& locErrors = location->getErrorPages();
		if (locErrors.find(statusCode) != locErrors.end()) {
			customPagePath = locErrors.find(statusCode)->second;
		}
	}

	// Fallback to server block if not found in location
	if (customPagePath.empty()) {
		const std::map<int, std::string>& srvErrors = server.getErrorPages();
		if (srvErrors.find(statusCode) != srvErrors.end()) {
			customPagePath = srvErrors.find(statusCode)->second;
		}
	}

	// Attempt to load the custom error page if one was specified
	if (!customPagePath.empty()) {
		std::string root = (location != NULL) ? location->getRootPath()
		                                      : server.getRootPath();
		if (root.empty()) {
			root = "./html";
		}

		std::string fullPath = root + customPagePath;
		std::ifstream file(fullPath.c_str());
		if (file.is_open()) {
			std::ostringstream oss;
			oss << file.rdbuf();
			response.setStatusCode(statusCode);
			response.setHeader("Content-Type", "text/html");
			response.setBody(oss.str());
			return;
		}
	}

	// Generate a simple default fallback error page
	std::string reason = HttpResponse::getReasonPhrase(statusCode);
	std::ostringstream html;
	html << "<!DOCTYPE html>\n"
		 << "<html>\n"
		 << "\t<head>\n"
		 << "\t\t<title>" << statusCode << " " << reason << "</title>\n"
		 << "\t</head>\n"
		 << "\t<body style='text-align:center; padding: 50px;'>\n"
		 << "\t\t<h1>" << statusCode << " " << reason << "</h1>\n"
		 << "\t\t<hr/>\n"
		 << "\t\t<p>webserv / 1.0</p>\n"
		 << "\t</body>\n"
		 << "</html>\n";

	response.setStatusCode(statusCode);
	response.setHeader("Content-Type", "text/html");
	response.setBody(html.str());
}

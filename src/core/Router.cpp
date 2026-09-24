#include "core/Router.hpp"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "core/CgiHandler.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
#include "utils/Utils.hpp"

// --- MAIN ENTRY ----------------------------------------------------------- //

void Router::route(Client& client, const ServerConfig& server) {
	const HttpRequest& request = client.request;
	HttpResponse& response = client.response;

	if (request.hasError()) {
		generateErrorResponse(request.getErrorCode(), response, server, NULL);
		return;
	}

	const LocationConfig* location = matchLocation(request.getUri(), server);

	// Check max body size
	size_t maxBodySize = (location != NULL) ? location->getClientMaxBodySize()
	                                        : server.getClientMaxBodySize();
	if (maxBodySize > 0 && request.getBody().length() > maxBodySize) {
		generateErrorResponse(413, response, server, location);
		return;
	}

	// Check HTTP Redirection
	if (location != NULL) {
		const std::pair<int, std::string>& ret = location->getReturn();
		if (ret.first != 0) { // Assuming 0 means not set
			response.setStatusCode(ret.first);
			response.setHeader("Location", ret.second);
			response.setHeader("Content-Type", "text/html");
			response.setBody("<html><body><h1>"
			                 + HttpResponse::getReasonPhrase(ret.first)
			                 + "</h1><a href=\"" + ret.second
			                 + "\">Moved</a></body></html>");
			return;
		}
	}

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

	typedef void (*MethodHandler)(Client&, const ServerConfig&,
	                              const LocationConfig*);
	static std::map<std::string, MethodHandler> handlers;
	if (handlers.empty()) {
		handlers["GET"] = &Router::handleGet;
		handlers["POST"] = &Router::handlePost;
		handlers["DELETE"] = &Router::handleDelete;
	}

	if (handlers.count(method) > 0) {
		handlers[method](client, server, location);
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

static std::string getMimeType(const std::string& path) {
	size_t dotPos = path.find_last_of('.');
	if (dotPos == std::string::npos)
		return "text/plain";
	std::string ext = path.substr(dotPos);
	if (ext == ".html" || ext == ".htm")
		return "text/html";
	if (ext == ".css")
		return "text/css";
	if (ext == ".js")
		return "application/javascript";
	if (ext == ".png")
		return "image/png";
	if (ext == ".jpg" || ext == ".jpeg")
		return "image/jpeg";
	if (ext == ".gif")
		return "image/gif";
	if (ext == ".ico")
		return "image/x-icon";
	if (ext == ".json")
		return "application/json";
	if (ext == ".pdf")
		return "application/pdf";
	if (ext == ".txt")
		return "text/plain";
	return "application/octet-stream";
}

void Router::handleGet(Client& client, const ServerConfig& server,
                       const LocationConfig* location) {
	const HttpRequest& request = client.request;
	HttpResponse& response = client.response;

	std::string root = (location != NULL) ? location->getRootPath()
	                                      : server.getRootPath();
	if (root.empty()) {
		root = "./html";
	}

	std::string filepath = root + request.getUri();
	struct stat path_stat;

	if (stat(filepath.c_str(), &path_stat) != 0) {
		generateErrorResponse(404, response, server, location);
		return;
	}

	if (S_ISDIR(path_stat.st_mode)) {
		if (filepath[filepath.length() - 1] != '/') {
			response.setStatusCode(301);
			response.setHeader("Location", request.getUri() + "/");
			return;
		}

		bool indexFound = false;
		const std::vector<std::string>& indexFiles =
			(location != NULL) ? location->getIndexFiles()
							   : server.getIndexFiles();
		std::vector<std::string> defaultIndexes = indexFiles;
		if (defaultIndexes.empty())
			defaultIndexes.push_back("index.html");

		for (size_t i = 0; i < defaultIndexes.size(); ++i) {
			std::string testPath = filepath + defaultIndexes[i];
			if (stat(testPath.c_str(), &path_stat) == 0
			    && S_ISREG(path_stat.st_mode)) {
				filepath = testPath;
				indexFound = true;
				break;
			}
		}

		if (!indexFound) {
			bool autoindex = (location != NULL) ? location->isAutoindex()
			                                    : false;
			if (autoindex) {
				DIR* dir = opendir(filepath.c_str());
				if (dir) {
					std::ostringstream html;
					html << "<html><head><title>Index of " << request.getUri()
						 << "</title></head><body>";
					html << "<h1>Index of " << request.getUri()
						 << "</h1><hr><pre>";
					struct dirent* ent;
					while ((ent = readdir(dir)) != NULL) {
						std::string name = ent->d_name;
						if (name == ".")
							continue;
						html << "<a href=\"" << name
							 << (ent->d_type == DT_DIR ? "/" : "") << "\">"
							 << name << (ent->d_type == DT_DIR ? "/" : "")
							 << "</a>\n";
					}
					html << "</pre><hr></body></html>";
					closedir(dir);

					response.setStatusCode(200);
					response.setHeader("Content-Type", "text/html");
					response.setBody(html.str());
					return;
				} else {
					generateErrorResponse(403, response, server, location);
					return;
				}
			} else {
				generateErrorResponse(403, response, server, location);
				return;
			}
		}
	}

	if (location) {
		const std::map<std::string, std::string>& cgis =
			location->getCgiHandlers();
		std::string ext = "";
		size_t dotPos = filepath.find_last_of('.');
		if (dotPos != std::string::npos)
			ext = filepath.substr(dotPos);

		if (cgis.count(ext) > 0) {
			try {
				CgiHandler cgi(request, server, location, filepath,
				               client.getIp());
				cgi.execute(client.cgiReadFd, client.cgiWriteFd, client.cgiPid);
				client.isCgiRunning = true;
				client.cgiOutput.clear();
				client.cgiBodyRemaining = request.getBody();
			} catch (const std::exception& e) {
				std::cerr << "[CGI ERROR] " << e.what() << "\n";
				generateErrorResponse(500, response, server, location);
			}
			return; // Response is handled asynchronously via CGI pipes
		}
	}

	std::ifstream file(filepath.c_str());
	if (!file.is_open()) {
		generateErrorResponse(403, response, server, location);
		return;
	}

	std::ostringstream oss;
	oss << file.rdbuf();

	response.setStatusCode(200);
	response.setHeader("Content-Type", getMimeType(filepath));
	response.setBody(oss.str());
}

void Router::handlePost(Client& client, const ServerConfig& server,
                        const LocationConfig* location) {
	const HttpRequest& request = client.request;
	HttpResponse& response = client.response;

	std::string root = (location != NULL) ? location->getRootPath()
	                                      : server.getRootPath();
	if (root.empty())
		root = "./html";
	std::string filepath = root + request.getUri();

	if (location) {
		const std::map<std::string, std::string>& cgis =
			location->getCgiHandlers();
		std::string ext = "";
		size_t dotPos = filepath.find_last_of('.');
		if (dotPos != std::string::npos)
			ext = filepath.substr(dotPos);

		if (cgis.count(ext) > 0) {
			try {
				CgiHandler cgi(request, server, location, filepath,
				               client.getIp());
				cgi.execute(client.cgiReadFd, client.cgiWriteFd, client.cgiPid);
				client.isCgiRunning = true;
				client.cgiOutput.clear();
				client.cgiBodyRemaining = request.getBody();
			} catch (const std::exception& e) {
				std::cerr << "[CGI ERROR] " << e.what() << "\n";
				generateErrorResponse(500, response, server, location);
			}
			return; // Response is handled asynchronously via CGI pipes
		}
	}

	std::string uploadDir = (location != NULL) ? location->getUploadDir() : "";

	if (!uploadDir.empty()) {
		std::string root = (location != NULL) ? location->getRootPath()
		                                      : server.getRootPath();
		if (root.empty())
			root = "./html";

		std::string uploadPath = root + uploadDir;
		if (uploadPath[uploadPath.length() - 1] != '/')
			uploadPath += "/";

		// Very basic extraction of a filename. In a real app, parse
		// multipart/form-data. Here, we just use a default name or extract from
		// URI for simple tests.
		std::string filename = "upload_" + utils::toString(time(NULL));
		size_t lastSlash = request.getUri().find_last_of('/');
		if (lastSlash != std::string::npos
		    && lastSlash != request.getUri().length() - 1) {
			filename = request.getUri().substr(lastSlash + 1);
		}

		std::string fullPath = uploadPath + filename;
		std::ofstream outFile(fullPath.c_str(), std::ios::binary);

		if (outFile.is_open()) {
			outFile.write(request.getBody().data(), request.getBody().length());
			outFile.close();
			response.setStatusCode(201); // 201 Created
			response.setHeader("Content-Type", "text/plain");
			response.setBody("File uploaded successfully.");
			return;
		} else {
			generateErrorResponse(500, response, server, location);
			return;
		}
	}

	// If not upload and not CGI, 501 Not Implemented
	generateErrorResponse(501, response, server, location);
}

void Router::handleDelete(Client& client, const ServerConfig& server,
                          const LocationConfig* location) {
	const HttpRequest& request = client.request;
	HttpResponse& response = client.response;
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

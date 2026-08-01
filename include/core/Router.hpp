#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <string>

#include "config/LocationConfig.hpp"
#include "config/ServerConfig.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

/**
 * @brief The central routing engine of the Web Server.
 *
 * Takes a parsed `HttpRequest`, matches it against the `ServerConfig`,
 * and generates the appropriate `HttpResponse` (e.g., serving static files,
 * executing CGI, generating autoindexes, or producing error pages.).
 */
class Router {
	public:
		// --- MAIN ENTRY --------------------------------------------------- //
		static void route(const HttpRequest& request, HttpResponse& response,
		                  const ServerConfig& server);

	private:
		// --- CONSTRUCTOR (Hidden) ----------------------------------------- //
		Router() {}

		// --- MATCHER ------------------------------------------------------ //
		static const LocationConfig* matchLocation(const std::string& uri,
		                                           const ServerConfig& server);

		// --- METHOD HANDLERS ---------------------------------------------- //
		static void handleGet(const HttpRequest& request,
		                      HttpResponse& response,
		                      const ServerConfig& server,
		                      const LocationConfig* location);

		static void handlePost(const HttpRequest& request,
		                       HttpResponse& response,
		                       const ServerConfig& server,
		                       const LocationConfig* location);

		static void handleDelete(const HttpRequest& request,
		                         HttpResponse& response,
		                         const ServerConfig& server,
		                         const LocationConfig* location);

		// --- ERROR HANDLER ------------------------------------------------ //
		static void generateErrorResponse(int statusCode,
		                                  HttpResponse& response,
		                                  const ServerConfig& server,
		                                  const LocationConfig* location);
};

#endif

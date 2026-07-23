#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "config/ServerConfig.hpp"

/**
 * @brief Lexical analyzer and parser for Nginx-style configuration files.
 *
 * The `ConfigParser` reads a raw `.conf` file, tokenizes the contents, and
 * executes a recursive-descent syntax analysis. It heavily utilizes maps of
 * member function pointers to dynamically route global, server and location
 * directives to their specific parsers. The resulting data is bundled into
 * `ServerConfig` and `LocationConfig` objects for use by the core web server.
 */
class ConfigParser {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		/**
		 * @brief Constructs a `ConfigParser` and parses the given file.
		 *
		 * This constructor handles the entire parsing lifecycle.
		 * It opens the file, tokenizes the contents, and runs the syntax
		 * analyzer to populate the internal map of `ServerConfig` objects.
		 * If this constructor finishes without throwing,
		 * the configuration is guaranteed to be valid.
		 *
		 * @param filename The path to the `.conf` file to be parsed.
		 *
		 * @throws std::runtime_error If the file cannot be opened, or if a
		 *                            syntax error is encountered during
		 *                            parsing.
		 */
		ConfigParser(const std::string& filename);

		// --- GETTER ------------------------------------------------------- //
		/**
		 * @brief Retrieves the parsed server configurations.
		 *
		 * The servers are grouped by their exact Host:Port combination.
		 * This allows the `ServerManager` to easily find all virtual server
		 * blocks that share a single network socket.
		 *
		 * @return const std::map< std::string, std::vector<ServerConfig> >&
		 *         A read-only reference to the map where the key is "Host:Port"
		 *         and the value is a vector of servers sharing that socket.
		 */
		const std::map< std::string, std::vector<ServerConfig> >&
		getServers() const;

		// --- DEBUGGING ---------------------------------------------------- //
		/**
		 * @brief Prints the raw tokenized configuration file.
		 *
		 * This is a debugging utility designed to verify the behavior of the
		 * lexer (`tokenizeConfig`). It loops through the internal `tokens_`
		 * vector and prints each token on a new line, enclosed in brackets, to
		 * easily spot trailing whitespace or incorrect token splits.
		 */
		void printTokens() const;

	private:
		// --- TYPES -------------------------------------------------------- //
		typedef void (ConfigParser::*DirectiveParser)(ServerConfig&);
		typedef void (ConfigParser::*LocationDirectiveParser)(LocationConfig&);

		// --- TOKEN UTILITIES ---------------------------------------------- //
		/// Moves to the next token, throws an error on unexpected EOF
		void advance();
		/// Moves to the next token, ensures it matches `expectedToken`
		void expect(const std::string& expectedToken);

		// --- CORE PARSING LOGIC ------------------------------------------- //
		/**
		 * @brief Reads and tokenizes an Nginx-style configuration file.
		 *
		 * This function reads the entire contents of the specified file into
		 * memory and splits it into a vector of tokens. It automatically
		 * ignores whitespace and comments (anything following a '#' until the
		 * end of the line). Special characters like '{', '}', and ';' are
		 * treated as independent tokens regardless of spacing.
		 *
		 * @param filename The path to the configuration file that needs to be
		 *                 parsed.
		 *
		 * @throws std::runtime_error If the file does not exist or cannot be
		 *                            opened.
		 */
		void tokenizeConfig(const std::string& filename);

		/**
		 * @brief Parses the tokenized configuration file and builds
		 *               ServerConfig objects
		 *
		 * This function iterates through the internal `tokens_` vector,
		 * starting at the global context. It acts as the main entry point for
		 * the syntax analyzer, searching for `server` blocks and using a series
		 * of function pointers to dynamically route directives to their
		 * specific helper methods. Once finished, the internal `servers_` map
		 * is fully populated.
		 *
		 * @throws std::runtime_error If an invalid directive is found in the
		 *                            global context, or if there is a syntax
		 *                            error in the file.
		 */
		void parseTokens();

		/**
		 * @brief Parses the content of a `server { ... }` block.
		 *
		 * Expects the current position to be right before the opening '{'.
		 * It loops through the internal tokens, delegating standard directives
		 * to their respective helper functions, and delegates nested `location`
		 * directives to `parseLocationBlock()`. When finished, the populated
		 * `ServerConfig` is added to the `servers_` map based on its port.
		 *
		 * @throws std::runtime_error On syntax error, missing brackets,
		 *                            or invalid directives.
		 */
		void parseServerBlock();

		/**
		 * @brief Parses the contents of a `location { ... }` block.
		 *
		 * Operates similarly to `parseServerBlock()`, but creates a
		 * `LocationConfig` object. It handles port extraction and
		 * location-specific directives (like CGI settings and allowed methods).
		 * The finished location is then attached to the parent `ServerConfig`.
		 *
		 * @throws std::runtime_error On syntax error or missing brackets.
		 */
		void parseLocationBlock(ServerConfig& server);

		// --- DIRECTIVE PARSERS -------------------------------------------- //
		/// Extracts the port, optional IP address and optional `default_server`
		/// (e.g., `listen 8080 default_server;`)
		void parseListen(ServerConfig& server);

		/// Extracts one or more virtual host names (e.g., `server_name a.com`)
		void parseServerName(ServerConfig& server);

		/// Sets the default directory for serving files (e.g., `root /dir`)
		void parseServerRoot(ServerConfig& server);

		/// Sets default files to serve for directories
		/// (e.g., `index index.html`)
		void parseServerIndex(ServerConfig& server);

		/// Limits the maximum size of a client request body in bytes
		/// (e.g., `client_max_body_size 10G`)
		void parseServerClientMaxBodySize(ServerConfig& server);

		/// Maps an HTTP error code to an HTML file path (e.g., `404 /404.html`)
		void parseServerErrorPage(ServerConfig& server);

		// --- LOCATION DIRECTIVE PARSERS ----------------------------------- //
		/// Sets the root directory specifically for this route
		/// (e.g., `root /var/www/api`)
		void parseLocationRoot(LocationConfig& location);

		/// Sets default files to serve if the route is a directory
		/// (e.g., `index index.php`)
		void parseLocationIndex(LocationConfig& location);

		/// Enables or disables directory listing (e.g., `autoindex on`)
		void parseAutoindex(LocationConfig& location);

		/// Limits the maximum size of a client request body in bytes
		void parseLocationClientMaxBodySize(LocationConfig& location);

		/// Extracts a list of permitted HTTP methods
		/// (e.g., `allow_methods GET POST`)
		void parseLocationErrorPage(LocationConfig& location);

		/// Extracts a list of permitted HTTP methods
		/// (e.g., `allow_methods GET POST`)
		void parseAllowedMethods(LocationConfig& location);

		/// Maps a file extension to a CGI executable path
		/// (e.g., `cgi .php /usr/bin/php-cgi`)
		void parseCgi(LocationConfig& location);

		/// Defines the directory where client uploads should be saved
		/// (e.g., `upload_store /tmp`)
		void parseUploadDir(LocationConfig& location);

		/// Configures an HTTP redirect code and destination URL
		/// (e.g., `return 301 /new`)
		void parseReturn(LocationConfig& location);

		// --- ATTRIBUTES --------------------------------------------------- //
		size_t currentPos_;
		std::vector<std::string> tokens_;
		std::map< std::string, std::vector<ServerConfig> > servers_;
		std::map<std::string, DirectiveParser> serverDirectiveParsers_;
		std::map<std::string, LocationDirectiveParser>
			locationDirectiveParsers_;
};

#endif

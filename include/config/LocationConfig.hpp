#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Represents a single route configuration.
 *
 * Stores all directives parsed from a `location /path { ... }` block,
 * including CGI handlers, allowed HTTP methods, and directory settings.
 */
class LocationConfig {
	public:
		// --- CONSTRUCTOR -------------------------------------------------- //
		LocationConfig();

		// --- GETTERS ------------------------------------------------------ //
		const std::string& getPath() const;
		const std::string& getRootPath() const;
		const std::vector<std::string>& getIndexFiles() const;
		bool isAutoindex() const;
		size_t getClientMaxBodySize() const;
		const std::map<int, std::string>& getErrorPages() const;
		const std::vector<std::string>& getAllowedMethods() const;
		const std::map<std::string, std::string>& getCgiHandlers() const;
		const std::string& getUploadDir() const;
		const std::pair<int, std::string>& getReturn() const;

		// --- SETTERS ------------------------------------------------------ //
		void setPath(const std::string& path);
		void setRootPath(const std::string& rootPath);
		void setIndexFiles(const std::vector<std::string>& indexFiles);
		void setAutoindex(bool isAutoindex);
		void setClientMaxBodySize(size_t size);
		void addErrorPage(int errorCode, const std::string& errorPath);
		void setErrorPages(const std::map<int, std::string>& errorPages);
		void setAllowedMethods(const std::vector<std::string>& methods);
		void addCgiHandler(const std::string& ext, const std::string& path);
		void setUploadDir(const std::string& uploadDir);
		void setReturn(const std::pair<int, std::string>& redirect);

	private:
		// --- ATTRIBUTES --------------------------------------------------- //
		std::string path_;
		std::string rootPath_;
		size_t clientMaxBodySize_;
		std::vector<std::string> indexFiles_;
		bool isAutoindex_;
		std::map<int, std::string> errorPages_;
		std::vector<std::string> allowedMethods_;
		std::map<std::string, std::string> cgiHandlers_;
		std::string uploadDir_;
		std::pair<int, std::string> return_;
};

#endif

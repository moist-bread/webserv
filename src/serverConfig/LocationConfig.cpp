#include "../../inc/serverConfig/LocationConfig.hpp"
#include "../../inc/http/HTTP.hpp" // t_method, t_status_code

#include <algorithm> // std::find

/**
 * @brief Default constructor.
 *
 * Initializes configuration members to safe defaults:
 * - `autoindex` to -1 (unset/undefined state)
 * - `returnCode` to INVALID_CODE (no redirect)
 * - `CgiPass` to false (not a CGI location by default)
 */
LocationConfig::LocationConfig(void) : autoindex(-1), returnCode(INVALID_CODE), CgiPass(false) {}

/**
 * @brief Copy constructor.
 * @param source Source LocationConfig to copy.
 */
LocationConfig::LocationConfig(LocationConfig const &source) { *this = source; }

/**
 * @brief Destructor (trivial).
 */
LocationConfig::~LocationConfig(void) {}

/**
 * @brief Copy assignment operator.
 * @param source Source LocationConfig to assign from.
 * @return Reference to this instance.
 */
LocationConfig &LocationConfig::operator=(LocationConfig const &source)
{
	if (this != &source)
	{
		this->path = source.path;
		this->root = source.root;
		this->index = source.index;
		this->autoindex = source.autoindex;
		this->allowedMethods = source.allowedMethods;
		this->uploadStore = source.uploadStore;
		this->returnCode = source.returnCode;
		this->returnUrl = source.returnUrl;
		this->cgi = source.cgi;
		this->CgiPass = source.CgiPass;
	}
	return (*this);
}

/**
 * @brief Return the configured path directory for this location.
 * @return Reference to the path directory string.
 */
const std::string &LocationConfig::getPath(void) const { return (this->path); }

/**
 * @brief Return the configured root directory for this location.
 * @return Reference to the root directory string.
 */
const std::string &LocationConfig::getRoot(void) const { return (this->root); }

/**
 * @brief Return the list of index files for this location.
 * @return Reference to the vector of index file names.
 */
const std::vector<std::string> &LocationConfig::getIndexes(void) const { return (this->index); }

/**
 * @brief Whether directory autoindex is enabled for this location and path matches.
 * 
 * Checks if autoindex is enabled (set to 1) and if the requested URI path
 * exactly matches this location's configured path.
 * 
 * @param path_uri The URI path to match against this location's path.
 * @return true if autoindex is enabled AND the path matches this location, false otherwise.
 */
bool LocationConfig::isAutoIndex(const std::string &path_uri) const { return (this->autoindex == 1 && getPath() == path_uri); }

/**
 * @brief Check whether an HTTP method is allowed for this location.
 * @param method Method to check (type `t_method`).
 * @return true if the method is permitted, false otherwise.
 */
bool LocationConfig::isMethodAllowed(t_method method) const
{
	return (std::find(this->allowedMethods.begin(), this->allowedMethods.end(), method) != this->allowedMethods.end());
}

/**
 * @brief Get the configured upload storage path for this location.
 * @return Reference to the upload store path string.
 */
const std::string &LocationConfig::getUploadStore(void) const { return (this->uploadStore); }

/**
 * @brief Check whether this location performs an HTTP redirect.
 * @return true when a return code is configured, false otherwise.
 */
bool LocationConfig::isRedirect(void) const { return (this->returnCode != INVALID_CODE); }

/**
 * @brief Return the configured HTTP status code for redirects.
 * @return Redirect status code (e.g., 301) or `INVALID_CODE` when not set.
 */
t_status_code LocationConfig::getReturnCode(void) const { return (this->returnCode); }

/**
 * @brief Get the redirect target URL for this location.
 * @return Reference to the return URL string.
 */
const std::string &LocationConfig::getReturnUrl(void) const { return (this->returnUrl); }

/**
 * @brief Lookup CGI executable for a file extension.
 * @param extension File extension (including the dot), e.g. ".php".
 * @return Path to the CGI executable or empty string if none configured.
 */
std::string LocationConfig::getCgiExecutable(const std::string &extension) const
{
	std::map<std::string, std::string>::const_iterator it = this->cgi.find(extension);
	if (it != this->cgi.end())
		return (it->second);
	return ("");
}

/**
 * @brief Check whether this location is configured with CGI pass.
 * 
 * CGI pass indicates that the location should forward requests to a CGI handler
 * rather than serving files directly from the filesystem.
 * 
 * @return true if CGI pass is enabled, false otherwise.
 */
bool LocationConfig::isCgiPass() const { return (CgiPass); }

/**
 * @brief Pretty-print a `LocationConfig` for debugging.
 * @param out Output stream to write to.
 * @param source LocationConfig to print.
 * @return Reference to the output stream.
 */
std::ostream &operator<<(std::ostream &out, LocationConfig const &source)
{
	out << "    [Location] Path: " << source.path << "\n";
	out << "      Root: " << source.root << "\n";
	out << "      Index: ";
	for (size_t i = 0; i < source.index.size(); ++i)
		out << source.index[i] << " ";
	out << "\n";
	out << "      Autoindex: " << (source.autoindex == 1 ? "on" : "off") << "\n";
	out << "      Upload Store: " << source.uploadStore << "\n";

	if (source.returnCode != 0)
		out << "      Return: " << source.returnCode << " -> " << source.returnUrl << "\n";

	out << "      Allowed Methods: ";
	for (size_t i = 0; i < source.allowedMethods.size(); ++i)
	{
		out << HTTP::stringMethod(source.allowedMethods[i]) << " ";
	}
	out << "\n";

	out << "      CGI:\n";
	for (std::map<std::string, std::string>::const_iterator it = source.cgi.begin(); it != source.cgi.end(); ++it)
	{
		out << "        " << it->first << " -> " << it->second << "\n";
	}
	out << "      CGI Pass: " << (source.CgiPass ? "yes" : "no") << std::endl;
	return out;
}

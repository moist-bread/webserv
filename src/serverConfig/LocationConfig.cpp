#include "../../inc/serverConfig/LocationConfig.hpp"
#include "../../inc/requests/HTTP.hpp"

LocationConfig::LocationConfig(void) : autoindex(-1), returnCode(UNITIALIZED) {}

LocationConfig::LocationConfig(LocationConfig const &source) { *this = source; }

LocationConfig::~LocationConfig(void) {}

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
	}
	return (*this);
}

const std::string& LocationConfig::getRoot(void) const { return (this->root); }

const std::vector<std::string>& LocationConfig::getIndex(void) const { return (this->index); }

bool LocationConfig::isAutoIndexOn(void) const { return (this->autoindex == 1); }

bool LocationConfig::isMethodAllowed(t_method method) const
{
	return (std::find(this->allowedMethods.begin(), this->allowedMethods.end(), method) != this->allowedMethods.end());
}

const std::string& LocationConfig::getUploadStore(void) const { return (this->uploadStore); }

bool LocationConfig::isRedirect(void) const { return (this->returnCode != NO_STATUS); }

t_status_code LocationConfig::getReturnCode(void) const { return (this->returnCode); }

const std::string& LocationConfig::getReturnUrl(void) const { return (this->returnUrl); }

std::string LocationConfig::getCgiExecutable(const std::string &extension) const
{
	std::map<std::string, std::string>::const_iterator it = this->cgi.find(extension);
	if (it != this->cgi.end())
		return (it->second);
	return ("");
}

std::ostream &operator<<(std::ostream &out, LocationConfig const &source)
{
	out << "    [Location] Path: " << source.path << "\n";
	out << "      Root: " << source.root << "\n";
	out << "      Index: ";
	for (size_t i = 0; i < source.index.size(); ++i)
		out << source.index[i] << " ";
	out << "\n";
	out << "      Autoindex: " << (source.autoindex ? "on" : "off") << "\n";
	out << "      Upload Store: " << source.uploadStore << "\n";
	
	if (source.returnCode != 0)
		out << "      Return: " << source.returnCode << " -> " << source.returnUrl << "\n";

	out << "      Allowed Methods: ";
	for (size_t i = 0; i < source.allowedMethods.size(); ++i) {
		out << HTTP::stringMethod(source.allowedMethods[i]) << " ";
	}
	out << "\n";

	out << "      CGI:\n";
	for (std::map<std::string, std::string>::const_iterator it = source.cgi.begin(); it != source.cgi.end(); ++it) {
		out << "        " << it->first << " -> " << it->second << "\n";
	}
	return out;
}

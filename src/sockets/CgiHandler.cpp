#include "../../inc/sockets/CgiHandler.hpp"
#include "../../inc/http/Request.hpp"
#include "../../inc/serverConfig/ServerConfig.hpp"
#include "../../inc/serverConfig/LocationConfig.hpp"
#include "../../inc/string_utils.tpp"

#include <unistd.h>	 // close, fork, pipe
#include <ctime>	 // time
#include <fstream>	 // fstream
#include <algorithm> // transform
#include <csignal>	 // signal
#include <fcntl.h>	 // fcntl

/// @brief Converts any char into screaming snake case
/// @return for '-' or ' ' it returns '_', others return their uppercase version
static int screaming_snake_case(int i)
{
	if (i == '-' || i == ' ')
		return '_';
	return toupper(i);
}

CgiHandler::CgiHandler(void) { clear(); }

CgiHandler::CgiHandler(CgiHandler const &src) { *this = src; }

CgiHandler &CgiHandler::operator=(CgiHandler const &src)
{
	if (this != &src)
		setCgiActivityStart(src.getCgiActivityStart());
	return (*this);
}

CgiHandler::~CgiHandler(void) {}

/// @brief Sends to execution whatever is asked by the Request
void CgiHandler::process(Request &req)
{
	clear();
	update_info(req);
	executeCgi();
}

/// @brief Resets all variable values
void CgiHandler::clear()
{
	_pipeIn[0] = 0;
	_pipeIn[1] = 0;
	_pipeOut[0] = 0;
	_pipeOut[1] = 0;
	_pid = 0;

	_body.clear();
	_compiler.clear();
	_scriptPath.clear();
	_env.clear();

	setCgiActivityStart(VALUE_NOT_SET);
}

/// @brief Sets the Path for the script and creates the Cgi Environment Variables 
void CgiHandler::update_info(Request &req)
{
	_body = req.body;
	if (!req.loc->isCgiPass())
		_scriptPath = req.loc->getRoot() + req.path_uri.substr(req.loc->getPath().length());
	else
	{
		_scriptPath = req.loc->getRoot();
		size_t pos = req.path_uri.rfind("." + req.file_extension);
		if (pos == std::string::npos)
			_scriptPath += req.path_uri;
		else
		{
			pos = req.path_uri.rfind("/", pos);
			if (pos == std::string::npos)
				_scriptPath += req.path_uri;
			else
				_scriptPath += req.path_uri.substr(pos + 1);
		}
	}

	std::fstream fs(_scriptPath.c_str(), std::fstream::in);
	if (!fs.is_open())
		throw(CgiHandler::CgiExecutionFail("open failure"));

	_compiler = req.loc->getCgiExecutable(req.file_extension);

	_env.push_back("SERVER_SOFTWARE=" + to_str("server/1.0.0"));
	_env.push_back("SERVER_NAME=" + req.conf->getServerNames()[0]); // -------------------------
	_env.push_back("SERVER_PROTOCOL=" + HTTP::stringProtocol(req.protocol));
	_env.push_back("SERVER_PORT=" + to_str(req.conf->getListenPort()));
	_env.push_back("GATEWAY_INTERFACE=" + to_str("CGI/1.1"));

	_env.push_back("REQUEST_METHOD=" + HTTP::stringMethod(req.method));
	// -- path info is whatever comes after the program name in the url
	_env.push_back("PATH_INFO=" + req.path_uri);
	_env.push_back("QUERY_STRING=" + req.query);
	_env.push_back("SCRIPT_NAME=" + _scriptPath);
	_env.push_back("SCRIPT_FILENAME=" + extract_script_filename(req.path_uri, req.file_extension));

	if (req.headers.find("x-forwarded-for") != req.headers.end())
		_env.push_back("REMOTE_ADDR=" + req.headers["x-forwarded-for"]);
	_env.push_back("REQUEST_URI=" + req.path_uri + req.query);
	_env.push_back("CONTENT_LENGTH=" + to_str(_body.size()));
	if (req.headers.find("content-type") != req.headers.end())
		_env.push_back("CONTENT_TYPE=" + req.headers["content-type"]);

	for (map_strings::const_iterator it = req.headers.begin(); it != req.headers.end(); it++)
	{
		// -- add "HTTP_" before all keys, the "-" become "_" and all uppercase
		std::string key = (*it).first;
		std::transform(key.begin(), key.end(), key.begin(), ::screaming_snake_case);
		_env.push_back("HTTP_" + key + "=" + (*it).second);
	}
}

std::string CgiHandler::extract_script_filename(const std::string full_path, const std::string ext)
{
	// -- script filename is everything until the end of the script extention
	size_t pos = full_path.rfind("." + ext);
	if (pos == std::string::npos)
		return (full_path);
	return (full_path.substr(0, pos + ext.length() + 1));
}

/**
 * @brief Forks and executes the CGI script as a child process.
 *
 * Execution sequence:
 *  1. InitPipes()            — creates _pipeIn and _pipeOut fd pairs.
 *  2. writeBodyToCgiInput()  — streams the request body into _pipeIn[1],
 *                              then closes it so the child gets EOF on stdin.
 *  3. fork()                 — splits into parent and child.
 *
 * Parent path (pid >= 1):
 *  - Closes the fds it no longer needs: _pipeIn[0] (child's stdin read end)
 *    and _pipeOut[1] (child's stdout write end).
 *  - Retains _pipeOut[0] to later read the CGI output.
 *
 * Child path (pid == 0):
 *  - Redirects stdin  → _pipeIn[0]  via dup2, so the child reads the body
 *    that was already written into the pipe.
 *  - Redirects stdout → _pipeOut[1] via dup2, so CGI output flows back to
 *    the parent through _pipeOut[0].
 *  - Closes the remaining pipe ends it no longer needs after dup2.
 *  - Builds argv = { compiler, scriptPath, NULL } and a null-terminated
 *    env vector, then calls execve(2).
 *  - If execve fails, logs the error and throws CgiExecutionFail.
 *    Note: the throw after execve only fires if execve itself returns,
 *    which only happens on error — further testing may be needed.
 *
 * On success, records the fork time via setCgiActivityStart for timeout
 * tracking.
 *
 * @throws CgiHandler::CgiExecutionFail on pipe, write, fork, or execve failure.
 */
void CgiHandler::executeCgi()
{
	if (InitPipes() == -1)
		throw(CgiHandler::CgiExecutionFail("pipe failure"));

	// Body must be written before fork: after fork the parent no longer owns
	// _pipeIn[1] exclusively, and writing from two processes would corrupt the stream.
	if (writeBodyToCgiInput() == -1)
		throw(CgiHandler::CgiExecutionFail("write to CGI stdin failed"));

	this->_pid = fork();
	if (this->_pid == -1)
		throw(CgiHandler::CgiExecutionFail("fork failure"));
	else if (this->_pid >= 1) // Parent
	{
		// Close the child-side ends; parent only needs to read CGI output.
		close(this->_pipeIn[0]);
		close(this->_pipeOut[1]);
	}
	else if (this->_pid == 0) // Child
	{
		// Wire the pipe ends to standard file descriptors so the CGI script
		// transparently reads from stdin and writes to stdout.
		dup2(this->_pipeIn[0], STDIN_FILENO);
		dup2(this->_pipeOut[1], STDOUT_FILENO);

		// Original pipe fds are no longer needed after dup2.
		close(this->_pipeIn[1]);
		close(this->_pipeOut[0]);

		char *argv[3];
		argv[0] = const_cast<char *>(_compiler.c_str());
		argv[1] = const_cast<char *>(this->_scriptPath.c_str());
		argv[2] = NULL;

		// Build a null-terminated env array required by execve.
		std::vector<char *> exec_env;
		for (size_t i = 0; i < _env.size(); i++)
			exec_env.push_back(const_cast<char *>(_env[i].c_str()));
		exec_env.push_back(NULL);

		execve(argv[0], argv, &exec_env[0]);

		// Reached only if execve fails (invalid path, bad permissions, etc.).
		std::cerr << "FAILED TO EXECUTE" << std::endl;
		setCgiActivityStart(VALUE_NOT_SET);
		throw(CgiHandler::CgiExecutionFail("execve failure")); // TODO: verify throw behaviour in child context
	}
	// Record start time for CGI timeout tracking.
	setCgiActivityStart(std::time(NULL));
}

int CgiHandler::InitPipes()
{
	if (pipe(this->_pipeIn) == -1)
		return -1;

	if (pipe(this->_pipeOut) == -1)
		return -1;
	return 0;
}
/** 
 * @brief Writes the HTTP request body to the CGI child's stdin pipe.
 *
 * Sets the write end of the pipe to non-blocking mode, then sends the body
 * in 4 KB chunks. This avoids deadlocking when the body exceeds the kernel
 * pipe buffer (~64 KB): a blocking write of a large body would stall waiting
 * for the child to drain the buffer, while the child may itself be waiting
 * for the parent to read its output — a classic pipe deadlock.
 *
 * Write outcomes per chunk:
 *  - n > 0 : bytes accepted, advance cursor.
 *  - n == 0 : pipe buffer temporarily full, yield 1 ms and retry.
 *  - n < 0  : child closed its read end (EPIPE) or pipe unavailable (EAGAIN);
 *             stop writing and treat as a clean early exit, not a fatal error.
 *             (Note: errno is not inspected directly — 42 norm compliance.)
 *
 * Always closes the write end of _pipeIn before returning so the child
 * receives EOF on its stdin.
 *
 * @return  0 on success or empty body.
 *         -1 if a write error was encountered (child stopped reading early).
 */
int CgiHandler::writeBodyToCgiInput() const
{
	if (!_body.empty())
	{
		fcntl(this->_pipeIn[1], F_SETFL, O_NONBLOCK);

		const size_t CHUNK = 4096;
		size_t total = 0;
		bool writeError = false;

		while (total < _body.size())
		{
			size_t remaining = _body.size() - total;
			size_t toWrite = std::min(CHUNK, remaining);

			ssize_t n = write(this->_pipeIn[1], _body.c_str() + total, toWrite);

			if (n > 0)
			{
				total += n;
			}
			else if (n == 0)
			{
				// Pipe buffer temporarily full — yield and retry.
				usleep(1000);
			}
			else // n < 0
			{
				// Child closed its read end (EPIPE) or pipe unavailable (EAGAIN)
				// Either way: stop writing, not a fatal error
				std::cerr << "CGI stopped reading stdin after " << total << " bytes\n";
				writeError = true;
				break;
			}
		}
		close(this->_pipeIn[1]);
		if (writeError)
			return -1;
	}
	else
	{
		// No body to send; close the write end so the child gets EOF immediately.
		close(this->_pipeIn[1]);
	}

	return 0;
}

int CgiHandler::getPipeOutReadFd() const { return this->_pipeOut[0]; }

const time_t &CgiHandler::getCgiActivityStart(void) const { return (time_started); }

void CgiHandler::setCgiActivityStart(const time_t &t) { time_started = t; }

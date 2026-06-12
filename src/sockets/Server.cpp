#include "../../inc/sockets/Server.hpp"
#include "../../inc/sockets/Client.hpp"

#include "../../inc/ansi_color_codes.h"

#include <unistd.h> // close, read
#include <ctime>	// time, difftime
#include <string.h>	// strerror
#include <errno.h>	// errno
#include <fcntl.h>	// fcntl

#define TIMEOUT_TIME 120
#define READ_BUFFER_SIZE 65536 // 64kb per second

extern bool running;

// ─── Constructors / Destructor ───────────────────────────────────────────────
/**
 * @brief Constructs the server from a Config object.
 *
 * Initialises the base ASimpleServer with the first configured port, then
 * calls SetupPorts() to bind any additional ports declared in the config,
 * and launch() to start the event loop.
 *
 * @param config Parsed configuration containing one or more ServerConfig entries.
 */
Server::Server(Config config) : ASimpleServer(AF_INET, SOCK_STREAM, 0, config.getallServers()[0].getListenPort(), INADDR_ANY, 10), _config(config)
{	
	SetupPorts();
	launch();
}

Server::Server(Server const &source) : ASimpleServer(source), _config(source._config) { *this = source; }

Server &Server::operator=(Server const &source)
{
	if (this != &source)
	{
		_fdToServerConfig.clear();
		const std::vector<ServerConfig>& servers = _config.getallServers();
		for (size_t i = 0; i < _listeningFds.size() && i < servers.size(); i++)
			_fdToServerConfig[_listeningFds[i]] = &servers[i];
	}
	return (*this);
}

Server::~Server(void)
{
	for (size_t i = 0; i < _extraListeners.size(); i++)
		delete _extraListeners[i];
}

// ─── Setup ───────────────────────────────────────────────────────────────────

/**
 * @brief Binds all configured ports and populates internal fd mappings.
 *
 * Iterates over every ServerConfig entry. The first port reuses the socket
 * already created by ASimpleServer. Subsequent entries either share an
 * existing fd (same port) or create a new ListeningSocket. Each unique fd
 * is registered with poll via PopulatePollInfo().
 *
 * After the loop, _listeningFds, _serverPorts, and _fdToServerConfig are
 * fully populated.
 */
void Server::SetupPorts()
{
    const std::vector<ServerConfig>& servers = _config.getallServers();
    std::map<int, int> portToFd;   // Maps port number → already-created fd

    for (size_t i = 0; i < servers.size(); i++)
    {
        int port = servers[i].getListenPort();
        int fd;

        if (i == 0)
        {
			// First port: reuse the fd opened by the base class constructor.
            fd = this->_sock;
            _listeningFds.push_back(fd);
            PopulatePollInfo(fd);
            portToFd[port] = fd;
        }
        else if (portToFd.count(port))
        {
			// Port already bound — reuse its fd (multiple vhosts on same port).
            fd = portToFd[port]; 
            _listeningFds.push_back(fd);
        }
        else
        {
			// New port — create a dedicated ListeningSocket and track it for cleanup.
            ListeningSocket *listener = new ListeningSocket(AF_INET, SOCK_STREAM, 0, port, INADDR_ANY, 10);
            _extraListeners.push_back(listener);
            fd = listener->getSocketfd();
            _listeningFds.push_back(fd);
            PopulatePollInfo(fd);
            portToFd[port] = fd;
        }

        _serverPorts.push_back(port);
        _fdToServerConfig[fd] = &servers[i];
    }


	Inspect::inspect_server_activity("started up", *this);
}

/**
 * @brief Registers a file descriptor with the poll set.
 *
 * Sets the fd to non-blocking, then appends a pollfd entry listening for
 * POLLIN events. Called for both listening sockets and accepted client fds.
 *
 * @param fd The file descriptor to add.
 */
void Server::PopulatePollInfo(int fd)
{

	SetNonblocking(fd);

	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	_pollfds.push_back(pfd);
}

/**
 * @brief Sets a file descriptor to non-blocking mode via fcntl.
 *
 * Required for all fds registered with poll to prevent any single
 * read/write from stalling the event loop.
 *
 * @param fd The file descriptor to configure.
 */
void Server::SetNonblocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		std::cout << "Something went wrong while passing to NONBLOCKING" << std::endl;
	}
}

// ─── Client Management ───────────────────────────────────────────────────────

/**
 * @brief Removes a client, cleans up its CGI mapping, and closes its fd.
 *
 * Also erases the corresponding pollfd entry and decrements the loop index
 * so the caller's iteration remains valid after the removal.
 *
 * @param fd     The client file descriptor to remove.
 * @param index  Reference to the current poll loop index (decremented in place).
 * @param reason Reason code logged via Inspect (e.g. TIMEOUT, RECV_FAIL).
 */
void Server::removeClient(int fd, size_t &index, const t_remove_reason &reason)
{
	Inspect::inspect_removed_cl(reason, fd);
	_clients.erase(fd);
	// Remove any CGI pipe → client mapping that references this client
	for (std::map<int, int>::iterator it = _cgiMap.begin(); it != _cgiMap.end(); it++)
	{
		if (it->second == fd)
		{
			_cgiMap.erase(it);
			break;
		}
	}
	close(fd);
	_pollfds.erase(_pollfds.begin() + index);
	index--;
}

/**
 * @brief Closes a CGI pipe fd and removes it from the poll set and CGI map.
 *
 * Called when CGI output has been fully consumed (EOF) or on error.
 * Decrements the poll index so the caller's loop remains valid.
 *
 * @param fd          The CGI pipe read fd to close.
 * @param pollfds_idx Pointer to the current poll loop index (decremented in place).
 */
void Server::closeCgiConnection(const int &fd, size_t *pollfds_idx)
{
	close(fd);
	_cgiMap.erase(fd);
	_pollfds.erase(_pollfds.begin() + *pollfds_idx);
	(*pollfds_idx)--;
}

/**
 * @brief Switches a client fd's poll interest from POLLIN to POLLOUT.
 *
 * Called once a response is ready to send, transitioning the client from
 * read-mode to write-mode in the event loop.
 *
 * @param fd The client file descriptor to update.
 */
void Server::switchToPollout(const int &fd)
{
	for (size_t i = 0; i < _pollfds.size(); i++)
	{
		if (_pollfds[i].fd == fd)
		{
			_pollfds[i].events = POLLOUT;
			break;
		}
	}
}

// ─── Event Loop ──────────────────────────────────────────────────────────────

/**
 * @brief Main event loop. Drives all I/O via poll.
 *
 * Iterates over all registered fds on each poll wakeup and dispatches to
 * the appropriate handler based on the fd type and ready events:
 *
 *  - Listening socket + POLLIN  → accepter()           (new connection)
 *  - CGI pipe fd + POLLIN/HUP   → recieveCgiOutput()   (CGI output ready)
 *  - Client fd + POLLIN/HUP     → recieveClientRequest()
 *  - Any fd + POLLOUT           → sendClientResponse()
 *  - Non-server client fd       → inactivityTimeout()  (checked every iteration)
 *
 * The loop runs until the global `running` flag is cleared (e.g. by SIGINT).
 */
void Server::launch()
{
	if(_pollfds.empty())
		return (Inspect::inspect_server_activity("no ports configured, shuting down", *this));

	running = true;
	while (running)
	{
		int pollCount = poll(&_pollfds[0], _pollfds.size(), -1);
		if (pollCount <= 0)
		{
			std::cout << std::endl;
			if (pollCount == 0)
				// poll returned 0: no fds became ready before timeout
				Inspect::inspect_server_activity("timed out", *this);
			else 
				// poll returned -1: system error (e.g. EINTR on signal).
				Inspect::inspect_server_activity( "been stopped in poll by: " + std::string(strerror(errno)), *this);
		}

		for (size_t i = 0; i < _pollfds.size(); i++)
		{
			int fd = _pollfds[i].fd;

			// Drain remaining clients on graceful shutdown.
			if (!running && _clients.find(fd) != _clients.end())
			{
				removeClient(fd, i, SERVER_CLOSE);
				continue;
			}

			// --- New Connection ---
			if (isServerSocket(fd) && (_pollfds[i].revents & POLLIN))
				accepter(fd);

			// --- CGI ---
			else if (_cgiMap.find(fd) != _cgiMap.end() && (_pollfds[i].revents & (POLLIN | POLLHUP | POLLERR)))
				recieveCgiOutput(fd, &i);

			// --- Read (CLIENTE Sends REQUEST) ---
			else if (_pollfds[i].revents & (POLLIN | POLLHUP | POLLERR))
				recieveClientRequest(fd, &i);

			// --- Write (Server sends response) ---
			if (_pollfds[i].revents & POLLOUT)
				sendClientResponse(fd, &i);

			// Timeout check runs every iteration for non-server fds.
			if (!isServerSocket(fd) && _clients.find(fd) != _clients.end())
				inactivityTimeout(fd, &i);
		}
	}
	Inspect::inspect_server_activity("shut down", *this);
}

// ─── I/O Handlers ────────────────────────────────────────────────────────────
/**
 * @brief Accepts a new incoming connection on a listening socket.
 *
 * Calls accept, sets the new fd to non-blocking via PopulatePollInfo(),
 * and creates a Client object associated with the matching ServerConfig.
 *
 * @param listenFd The listening fd that became readable.
 * @throws std::runtime_error if listenFd has no associated ServerConfig.
 */
void Server::accepter(int listenFd)
{
    struct sockaddr_in clientAddr;
    int addrlen = sizeof(clientAddr);

    int newFd = accept(listenFd, (struct sockaddr *)&clientAddr, (socklen_t *)&addrlen);
    if (newFd < 0)
		return (Inspect::inspect_client_activity("failed to be accepted", newFd, 0));

	if (!_fdToServerConfig.count(listenFd))
		throw (std::runtime_error("cliente sem server config"));
		
	PopulatePollInfo(newFd);
	Client newClient(newFd, listenFd, _fdToServerConfig[listenFd]);
    _clients.insert(std::make_pair(newFd, newClient));
	Inspect::inspect_client_activity("been accepted", newFd, newClient.serverConfig->getListenPort());

	if (Inspect::debug)
	{
		std::cout << RED "this is the config\n" DEF;
		std::cout << *_fdToServerConfig[listenFd] << std::endl;
	}
}

/**
 * @brief Reads available output from a CGI pipe and forwards it to the client.
 *
 * Called when the CGI pipe fd has POLLIN, POLLHUP, or POLLERR set.
 *
 * Outcomes:
 *  - POLLERR           → close CGI, set 500 on the client, switch to POLLOUT.
 *  - bytesRead > 0     → buffer chunk in cl.response.cgi_reply, stay in read mode.
 *  - bytesRead == 0    → EOF; CGI finished. If no chunked data was received,
 *                        treat as an execution error (500). Otherwise success.
 *  - bytesRead < 0     → read error; close CGI and set 500.
 *
 * Always switches the client fd to POLLOUT so the response can be sent.
 *
 * @param fd          The CGI pipe read fd.
 * @param pollfds_idx Pointer to the current poll loop index.
 */
void Server::recieveCgiOutput(int fd, size_t *pollfds_idx)
{
	int clientFd = _cgiMap[fd];
	try
	{
		Client &cl = get_corresponding_client(clientFd);
		cl.cgi.setCgiActivityStart(std::time(NULL));

		if (_pollfds[*pollfds_idx].revents & POLLERR)
		{
			Inspect::inspect_cgi_activity("failed to execute due to a POLLERR error", clientFd);
			closeCgiConnection(fd, pollfds_idx);
			cl.response.status_code = INTERNAL_SERVER_ERROR;
			switchToPollout(clientFd);
			return;
		}

		char buffer[READ_BUFFER_SIZE];
		int bytesRead = read(fd, buffer, READ_BUFFER_SIZE);

		if (bytesRead > 0) // -- recieved content -> send it and wait for more
		{
			// Partial CGI output received — buffer it; more may follow.
			cl.response.cgi_reply = std::string(buffer, bytesRead);
		}
		else if (bytesRead == 0) // -- recieved EOF -> send to signal that the cgi ended
		{
			// EOF: CGI process finished writing. Clean up the pipe.
			cl.cgi.setCgiActivityStart(VALUE_NOT_SET);
			cl.response.cgi_reply.clear();
			closeCgiConnection(fd, pollfds_idx);

			if (!cl.response.is_chunked)
			{
				// EOF with no prior chunked output means the CGI produced nothing usable.
				Inspect::inspect_cgi_activity("failed to execute due to an error", clientFd);
				cl.response.status_code = INTERNAL_SERVER_ERROR;
			}
			else
				Inspect::inspect_cgi_activity("finished execution", clientFd);
		}
		else // -- read failed -> send error response
		{
			cl.cgi.setCgiActivityStart(VALUE_NOT_SET);
			Inspect::inspect_cgi_activity("failed to execute due to a read error", clientFd);
			closeCgiConnection(fd, pollfds_idx);
			cl.response.status_code = INTERNAL_SERVER_ERROR;
		}
		
	}
	catch (const std::runtime_error& e)
	{
		closeCgiConnection(fd, pollfds_idx);
		Inspect::inspect_client_activity(e.what(), clientFd, 0);
	}
	
	switchToPollout(clientFd);
}

/**
 * @brief Receives and processes an incoming HTTP request from a client.
 *
 * Reads up to READ_BUFFER_SIZE bytes from the client socket. On a clean read,
 * feeds the data into the client's Request parser. Handles three outcomes:
 *
 *  - IO_ERROR / IO_CLOSED → remove the client immediately.
 *  - ParseError           → record the error status and switch to POLLOUT to
 *                           send an error response.
 *  - Request incomplete   → return; wait for the next POLLIN.
 *  - Request complete     → if a CGI handler applies, launch it and put the
 *                           client fd to sleep (events = 0) until CGI finishes.
 *                           Otherwise switch to POLLOUT for a direct response.
 *
 * @param fd          The client file descriptor to read from.
 * @param pollfds_idx Pointer to the current poll loop index.
 */
void Server::recieveClientRequest(int fd, size_t *pollfds_idx)
{
	char tmp[READ_BUFFER_SIZE];
	int ret = recv(fd, tmp, READ_BUFFER_SIZE, 0);

	ConnectionStatus status = getStatus(ret);
	if (status == IO_ERROR || status == IO_CLOSED)
		return (removeClient(fd, *pollfds_idx, RECV_FAIL));

	std::string rec = std::string(tmp, ret);
	if (Inspect::debug)
	{
		std::cout << "Bytes recebidos: " << ret << std::endl;
		// std::cout << std::endl << "Raw Request:" << std::endl;
		// std::cout << rec << std::endl;
	}

	try
	{
		Client &cl = get_corresponding_client(fd);
		cl.response.status_code = OK;
		try
		{
			cl.request.process(rec);
		}
		catch (const Request::ParseError &e)
		{
			Inspect::inspect_request_activity(e.what(), cl.request);
			cl.request.set_state(END);
			cl.response.status_code = e.request_status;
			_pollfds[*pollfds_idx].events = POLLOUT;
			return;
		}

		cl.updateLastActivity();
		if (cl.request.get_state() != END)
			return;  // Request still being assembled (chunked / large body)

		Inspect::inspect_request_activity("", cl.request);
		
		if (!cl.request.loc->getCgiExecutable(cl.request.file_extension).empty()) // -- start the cgi execution right away
		{
			// CGI route: launch the script and suspend this client in poll until output arrives.
			try
			{
				Inspect::inspect_cgi_activity("started execution", fd);
				cl.cgi.process(cl.request);
				int contentOfCgiFd = cl.cgi.getPipeOutReadFd();
				PopulatePollInfo(contentOfCgiFd);
				_cgiMap.insert(std::make_pair(contentOfCgiFd, cl.getClientFd()));
				_pollfds[*pollfds_idx].events = 0;  // Client sleeps until CGI produces output
				return;
			}
			catch (const CgiHandler::CgiExecutionFail &e)
			{
				Inspect::inspect_cgi_activity(e.what(), fd);
				cl.response.status_code = INTERNAL_SERVER_ERROR;
			}
		}
	}
	catch(const std::runtime_error& e)
	{
		Inspect::inspect_client_activity(e.what(), fd, 0);
	}

	switchToPollout(fd); // switch to sending
}

/**
 * @brief Sends a pending HTTP response to a client.
 *
 * Calls response.process() to build the next chunk of full_response, then
 * writes it to the socket. Partial sends are handled by erasing the written
 * prefix; once the buffer is empty the response is fully delivered and the
 * client is switched back to POLLIN to accept the next request.
 *
 * On write failure the client is removed.
 *
 * @param fd          The client file descriptor to write to.
 * @param pollfds_idx Pointer to the current poll loop index.
 */
void Server::sendClientResponse(int fd, size_t *pollfds_idx)
{
	try
	{
		Client &cl = get_corresponding_client(fd);
		cl.response.process();
		
		int bytesWritten = responder(fd, cl.response.full_response);
		if (bytesWritten > 0)
		{
			// std::cout << BLU "response sent..." DEF << std::endl;
			cl.updateLastActivity();
			// Consume the bytes that were actually sent; leave any remainder for the next POLLOUT.
			cl.response.full_response.erase(0, bytesWritten);
			if (cl.response.full_response.empty())
			{
				// only continue onto the next request after being able to
				// send everything from the previously processed request
				Inspect::inspect_response_activity("", cl.response);
				cl.response.set_state(PREP);
				_pollfds[*pollfds_idx].events = POLLIN;
			}
		}
		else if (bytesWritten < 0)
		{
			removeClient(fd, *pollfds_idx, WRITE_FAIL);
		}
	}
	catch(const std::runtime_error& e)
	{
		Inspect::inspect_client_activity(e.what(), fd, 0);
	}
}

/**
 * @brief Enforces inactivity and CGI timeouts for a client.
 *
 * Checks two independent timers every event loop iteration:
 *  1. Client inactivity: if the client has been idle for more than
 *     TIMEOUT_TIME seconds, it is removed.
 *  2. CGI timeout: if an active CGI script has been running for more than
 *     TIMEOUT_TIME seconds without producing output, the client is removed.
 *
 * Also handles the "Connection: close" header by removing the client
 * immediately when that header is present in the response.
 *
 * @param fd          The client file descriptor to check.
 * @param pollfds_idx Pointer to the current poll loop index.
 */
void Server::inactivityTimeout(int fd, size_t *pollfds_idx)
{
	try
	{
		Client &cl = get_corresponding_client(fd);

		if (cl.response.headers.find("Connection") != cl.response.headers.end())
			if (cl.response.headers["Connection"] == "close")
				return (removeClient(fd, *pollfds_idx, CLOSE_CONNECTION));

		time_t now = std::time(NULL);
		time_t seconds_idle = std::difftime(now, cl.getLastActivity());

		if (seconds_idle > TIMEOUT_TIME)
			return (removeClient(fd, *pollfds_idx, TIMEOUT));

		// Check CGI-specific timeout only if a CGI is actively running.
		if (cl.cgi.getCgiActivityStart() == VALUE_NOT_SET)
			return;
		seconds_idle = std::difftime(now, cl.cgi.getCgiActivityStart());
		if (seconds_idle > TIMEOUT_TIME)
			return (removeClient(fd, *pollfds_idx, TIMEOUT_CGI));
	}
	catch(const std::runtime_error& e)
	{
		Inspect::inspect_client_activity(e.what(), fd, 0);
	}
}

// ─── Helpers / Accessors ─────────────────────────────────────────────────────

/**
 * @brief Returns true if the given fd is one of the server's listening sockets.
 *
 * Used to distinguish listening fds from client fds in the poll loop.
 *
 * @param fd The file descriptor to check.
 * @return true if fd is in _listeningFds, false otherwise.
 */
bool Server::isServerSocket(int fd)
{
	for (size_t j = 0; j < _listeningFds.size(); j++)
	{
		if (_listeningFds[j] == fd)
			return true;
	}
	return false;
}

/**
 * @brief Maps a raw I/O return value to a ConnectionStatus enum.
 *
 * Centralises the interpretation of read(2)/recv(2)/write(2) return values:
 *  - n < 0 → IO_ERROR   (syscall failed)
 *  - n == 0 → IO_CLOSED  (peer closed the connection)
 *  - n > 0 → IO_DATA_READY
 *
 * @param ret Return value from a read/recv/write call.
 * @return The corresponding ConnectionStatus.
 */
ConnectionStatus Server::getStatus(int ret)
{
	if (ret < 0)
		return IO_ERROR;
	if (ret == 0)
		return IO_CLOSED;
	return IO_DATA_READY;
}

/**
 * @brief Writes a response string to a client socket.
 *
 * Thin wrapper around write. The caller is responsible for interpreting
 * the return value (bytes written, 0, or -1).
 *
 * @param clientFd The client file descriptor to write to.
 * @param data     The response data to send.
 * @return Number of bytes written, or -1 on error.
 */
int Server::responder(int clientFd, const std::string &data)
{
	return write(clientFd, data.c_str(), data.size());
}

/// @brief This function throws!!
/// @return The client that is paired with FD in the _clients map
Client &Server::get_corresponding_client(const int &fd)
{
	std::map<int,Client>::iterator it = _clients.find(fd);

	if (it == _clients.end())
		throw(std::runtime_error("not been found in Clients map"));
	else
		return(it->second);
}

/**
 * @brief Returns the fd → ServerConfig pointer map (read-only).
 * @return Const reference to _fdToServerConfig.
 */
const std::map<int, const ServerConfig*> &Server::get_fdToServerConfig() const { return (_fdToServerConfig); };

/**
 * @brief Stream insertion operator. Prints server configuration summary.
 *
 * Lists each listening fd alongside its port and primary server name.
 *
 * @param out Output stream to write to.
 * @param src Server instance to describe.
 * @return Reference to the output stream.
 */
std::ostream &operator<<(std::ostream &out, const Server &src)
{
	out << MAG "-- Server Information --" DEF << std::endl;

	out << MAG "Fds → Server Configurations..." DEF << std::endl;
	for (std::map<int, const ServerConfig*>::const_iterator it = src.get_fdToServerConfig().begin(); it != src.get_fdToServerConfig().end(); it++)
	{
		out << MAG "    [" << (*it).first << "]" DEF;
		out << " | Port=" << (*it).second->getListenPort();
		out << " | Server Name=" << ((*it).second->serverName.empty() ? "none" : (*it).second->serverName) << " |" << std::endl;

	}
	return (out);
}

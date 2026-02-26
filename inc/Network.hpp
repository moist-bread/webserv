#pragma once

#include <netinet/in.h>
#include <sys/types.h>       
#include <sys/socket.h>
#include <iostream>
#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <cstring>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cstring>
#include <ctime>
#include "fcntl.h"
#include "poll.h"
#include "sockets/SocketController.hpp"
#include "sockets/ConnectingSocket.hpp"
#include "sockets/BindingSocket.hpp"
#include "sockets/ListeningSocket.hpp"
#include "sockets/ASimpleServer.hpp"
#include "sockets/Client.hpp"
#include "ansi_color_codes.h"

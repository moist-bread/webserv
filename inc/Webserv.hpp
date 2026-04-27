#pragma once

// ==┊ all headers
#include "serverConfig/Config.hpp"
#include "sockets/Server.hpp"

// ==┊ libs for main
#include <stdlib.h>	// srand
#include <csignal>	// signal
#include <ctime>	// time

// ==┊ aux funcs
void sigint_handler(int sig);

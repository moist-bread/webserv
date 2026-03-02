
bool running = false;

void sigint_handler(int sig)
{
	running = false;
	(void)sig;
}

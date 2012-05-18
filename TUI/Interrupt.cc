#include <signal.h>
#include <stdlib.h>
#include "TUI/Interrupt"

namespace GUI {

static volatile bool B_SIGINT_happened = false;
static void handle_SIGQUIT(int s) { /* ctrl-backslash */
	exit(1);
}
bool interrupted_P(void) { // mutable
	bool o;
	o = B_SIGINT_happened;
	B_SIGINT_happened = false;
	return(o);
}
static void handle_SIGINT(int s) {
	signal(SIGINT, handle_SIGINT);
	B_SIGINT_happened = true;
}
void install_SIGINT_handler(void) {
	struct sigaction act_INT;
	act_INT.sa_flags = 0;
	sigemptyset(&act_INT.sa_mask);
	sigaddset(&act_INT.sa_mask, SIGINT);
	act_INT.sa_handler = handle_SIGINT;
	sigaction(SIGINT, &act_INT, 0);
}
void install_SIGQUIT_handler(void) {
	signal(SIGQUIT, handle_SIGQUIT);
}

}; /* TUI */

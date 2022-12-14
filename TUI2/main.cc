#include <locale.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <5D/Allocators>
#include <5D/Values>
#include <5D/ModuleSystem>
#include <5D/FFIs>
#include <stack>
#include "REPL/Symbols"
#include "TUI2/REPL"
#include "Config/Config"
#include "Version"
/* TODO after making callbacks into 5D code work, just move this entire module into 5D code. */
using namespace Values;
using namespace Symbols;
static NodeT parseParenStr;
static NodeT parseMathStr;
static NodeT printMath;
static NodeT OPL;

namespace Formatters {
namespace Math {
void print(Values::NodeT OPL, FILE* output_file, int position, int indentation, Values::NodeT node);
}
}

void initImports(void) {
	parseParenStr = Evaluators::getModuleEntryAccessor("Parsers", symbolFromStr("parseParenStr"));
	parseMathStr = Evaluators::getModuleEntryAccessor("Parsers", symbolFromStr("parseMathStr"));
	printMath = Evaluators::getModuleEntryAccessor("Formatters", symbolFromStr("printMath!"));
	OPL = Evaluators::getModuleEntryAccessor("OPLs/Math.5D", symbolFromStr("table"));
	//NodeT OPL2 = Evaluators::eval(OPL, NULL);
	//Formatters::Math::print(NULL, stdout, 0, 0, OPL2);
	//printf("END OPL\n");
}
// static NodeT eval = Evaluators::getModuleEntryAccessor("Evaluators", symbolFromStr("eval"))
/* Note: use eval from the runtime instead of faking it? Doesn't work that nicely because of basis recursion property. */


static char** completion_matches(const char* text, rl_compentry_func_t* callback) {
	return(rl_completion_matches(text, callback));
}
/* will match the "wrong" one on purpose. */
static int lexForMatchingParen(const char* command, int commandLen, int position) {
	int originalParen = command[position];
	int direction = 1;
	int result = -1;
	if(originalParen == '(' || originalParen == '{' || originalParen == '[')
		direction = 1;
	else {
		direction = (-1);
	}
	/* TODO @name: <stdin> as keyword parameter. */
	NodeT call1 = makeCall(parseParenStr, makeStrRaw((char*) command, commandLen, true), direction, position);
	try {
		NodeT maybeResult = Evaluators::eval(call1, NULL);
		// FIXME proper Maybe handling.
		//Formatters::Math::print(NULL, stdout, 0, 0, maybeResult);
		//fflush(stdout);
		return(intFromNode(maybeResult));
	} catch(...) {
		return(-1);
	}
        return(result);
}
static int handle_readline_paren(int x, int key) {
	int otherParen;
	int oldPoint = rl_point;
	rl_insert(x, key);
	otherParen = lexForMatchingParen(rl_line_buffer, strlen(rl_line_buffer), oldPoint);
	if(otherParen != -1) {
		struct timeval timeout;
		fd_set readfds;
		int oldPoint;
		FD_ZERO(&readfds);
		FD_SET(0, &readfds);
		timeout.tv_sec = 0;
		timeout.tv_usec = 500000; /* 0.5 sec */
		oldPoint = rl_point;
		rl_point = otherParen;
		rl_redisplay();
		select(0 + 1, &readfds, NULL, NULL, &timeout); /* sleep */
		rl_point = oldPoint;
	}
	return(0);
}
static NodeT inputNode;
/* TODO "expected <$-operands> but got <nothing> near position 46 in line 2 in file \"<stdin>\"" if the ($) is at the end. */
static int handle_readline_crlf(int x, int key) {
	/* check rl_point and rl_line_buffer[rl_point - 1] */
	int point;
	//system("ogg123 -q /usr/share/sounds/gnome/default/alerts/glass.ogg");
	point = rl_point;
	while(rl_line_buffer[point] == ' ' || rl_line_buffer[point] == '\t')
		++point;
	/* TODO move across spaces */
	if((rl_line_buffer[point] == '\n' || rl_line_buffer[point] == 0)) {
		NodeT maybeResult = rl_line_buffer[0] ? Evaluators::eval(makeCall(parseMathStr, OPL, rl_line_buffer), NULL) : NULL;
		inputNode = maybeResult;
		//printf("PRRRR\n");
		//Formatters::Math::print(NULL, stdout, 0, 0, inputNode);
		//printf("END PRRR\n");
		//Formatters::Math::print(NULL, stdout, 0, 0, inputNode);
		// FIXME proper Maybe handling.
		/* if OK, do:
		if(strstr(e.what(), "expected <(-operands>") || strstr(e.what(), "expected ]") || strstr(e.what(), "expected <body>") || strstr(e.what(), "expected <[let") || strstr(e.what(), "expected <[import") || strstr(e.what(), "expected in")) {
			rl_insert(x, '\n');
			return(0);
		}
		*/
	}
	/* make sure we print our other stuff after the (possibly multi-line) statement */
	rl_point = strlen(rl_line_buffer);
	rl_redisplay();
	rl_done = 1;
	printf("\n");
	return(0); /* FIXME */
}
static int startup_readline(void) {
	rl_bind_key('\n', handle_readline_crlf);
	rl_bind_key('\r', handle_readline_crlf);
	if(isatty(fileno(stdin))) {
		rl_bind_key('(', handle_readline_paren);
		rl_bind_key('{', handle_readline_paren);
		rl_bind_key(')', handle_readline_paren);
		rl_bind_key('}', handle_readline_paren);
		rl_bind_key('[', handle_readline_paren);
		rl_bind_key(']', handle_readline_paren);
	}
	return(0); /* FIXME */
}
static char* command_generator(const char* text, int state) {
	return(NULL);
}
static char** complete(const char* text, int start, int end) {
	char** matches = NULL;
	//if(start == 0) // or after a brace.
		matches = completion_matches(text, command_generator);
	return(matches);
}

static void initReadline(void) {
	rl_readline_name = "5D";
	rl_attempted_completion_function = complete;
	rl_startup_hook = startup_readline;
	rl_sort_completion_matches = 1;
}
void REPL_run(const char* line, NodeT inputValue) {
	//Evaluators::execute(makeCall(printMath, OPL, stdout, 0, 0, inputNode), NULL);
	//printf("\n");
	try {
		NodeT result = Evaluators::eval(ModuleSystem::prepareModule(inputNode, "<stdin>"), NULL);
		Evaluators::execute(makeCall(printMath, OPL, stdout, 0, 0, result), NULL);
		printf("\n");
	} catch (std::exception& e) {
		std::string s = e.what();
		fprintf(stderr, "error: %s\n", s.c_str()); /* FIXME print classname */
	}
	//std::string resultStr = Evaluators::str(result);
	//printf("ok %s\n", resultStr.c_str());
}
int run(const char* prompt) {
	const char* line;
	//operator_precedence_list = new Scanners::OperatorPrecedenceList();
	while((line = readline(prompt))) {
		if(!line)
			break;
		if(!line[0])
			continue;
		add_history(line);
		// note that handle_readline_crlf remembered the result in #inputNode, so be sure to use it.
		REPL_run(line, inputNode);
		inputNode = NULL;
	}
	return(0);
}

int main(int argc, char* argv[]) {
	const char* prompt = "eval $ ";
	int status;
	setlocale(LC_ALL, "");
	initImports();
	initAllocators();  
	/*if(argc >= 1)
		Evaluators::set_shared_dir_by_executable(argv[0]);*/
	Evaluators::initLogic();
	initReadline();
	status = run(prompt);
	printf("\n");
	fflush(stdout);
	return(status);
}

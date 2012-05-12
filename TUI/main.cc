#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <list>
#include <set>
#include <stack>
#include <map>
#include <string>
#include <getopt.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <limits.h>
#include <locale.h>
#include "Scanners/MathParser"
#include "Formatters/Math"
#include "TUI/Interrupt"
#include "REPL/REPL"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Version"
#include "AST/HashTable"
#include "AST/AST"
//#include "Config/Config"
#include "FFIs/Allocators"
#include "Evaluators/ModuleLoader"

namespace REPLX {

struct REPL : AST::Node {
	AST::NodeT fTailEnvironment;
	AST::NodeT fTailUserEnvironment /* =fTailBuiltinEnvironmentFrontier */;
	AST::NodeT fTailUserEnvironmentFrontier;
	int fEnvironmentCount;
	bool fFileModified;
	char* fEnvironmentName;
	//struct Config* fConfig;
	AST::HashTable fEnvironmentTable;
	AST::NodeT fEnvironmentNames; // Cons
	AST::HashTable* fModules;
	int fCursorPosition;
	bool fBRunIO;
};
void REPL_set_IO(REPL* self, bool value) {
	self->fBRunIO = value;
}
int REPL_add_to_environment_simple_GUI(REPL* self, AST::NodeT sym, AST::NodeT value) {
	const char* name = AST::get_symbol1_name(sym);
	if(self->fEnvironmentTable.find(name) == self->fEnvironmentTable.end()) {
		self->fEnvironmentTable[name] = value;
		self->fEnvironmentNames = AST::makeCons(sym, self->fEnvironmentNames);
	}
	return(self->fEnvironmentCount++); // FIXME
}
// TODO delete etc.
void REPL_queue_scroll_down(REPL* self) {
        // TODO
}

int REPL_insert_into_output_buffer(struct REPL* self, int destination, const char* text) {
	printf("%s\n", text);
	return(destination);
}
Scanners::OperatorPrecedenceList* REPL_ensure_operator_precedence_list(struct REPL* self);

static void REPL_enqueue_LATEX(struct REPL* self, AST::NodeT result, int destination) {
	// TODO LATEX
	int position = 0; // TODO use actual horizontal position at the destination.
	std::stringstream buffer;
	std::string v;
	Formatters::Math::print_CXX(REPL_ensure_operator_precedence_list(self), buffer, position, result, 0, false);
	v = buffer.str();
	REPL_insert_into_output_buffer(self, destination, v.c_str());
}
        
};

namespace GUI {
void REPL_set_file_modified(struct REPL* self, bool value);
};
using namespace GUI;
#define FILL_END_ITER 
#define END_ITER 0
#include "REPL/REPLEnvironment"

namespace GUI {
using namespace Evaluators;

void REPL_clear(struct REPL* self) {
	self->fTailEnvironment = self->fTailUserEnvironment = self->fTailUserEnvironmentFrontier = NULL;
	self->fEnvironmentCount = 0;
	self->fEnvironmentNames = NULL;
	self->fEnvironmentTable.clear();
	// TODO modules
	REPL_init_builtins(self);
}
bool REPL_get_file_modified(struct REPL* self) {
	return(self->fFileModified);
}
void REPL_set_file_modified(struct REPL* self, bool value) {
	if(self->fFileModified == value)
		return;
	self->fFileModified = value;
	{
		// FIXME const char* absolute_name;
		// FIXME absolute_name = Config_get_environment_name(self->fConfig);
		// FIXME gtk_window_set_title(self->fWidget, g_strdup_printf("%s%s", absolute_name ? absolute_name : "(5D)", self->fFileModified ? " *" : ""));
	}
}
static char* REPL_ensure_default_environment_name(struct REPL* self) {
	char* result;
	char config_dir_name[PATH_MAX + 1];
	char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
	const char* HOME = getenv("HOME");
	if(!HOME || !HOME[0]) { 
		HOME = "/root";
	}
	if(XDG_CONFIG_HOME) {
		if(snprintf(config_dir_name, NAME_MAX, "%s", XDG_CONFIG_HOME) == -1)
			abort();
		if(mkdir(config_dir_name, 0750) == -1 && errno != EEXIST) {
			perror(config_dir_name);
			exit(1);
		}
		if(snprintf(config_dir_name, NAME_MAX, "%s/5D", XDG_CONFIG_HOME) == -1)
			abort();
		if(mkdir(config_dir_name, 0750) == -1 && errno != EEXIST) {
			perror(config_dir_name);
			exit(1);
		}
		if(snprintf(config_dir_name, NAME_MAX, "%s/5D/TUI_environment", XDG_CONFIG_HOME) == -1)
			abort();
	} else {
		if(snprintf(config_dir_name, NAME_MAX, "%s/.config", HOME) == -1)
			abort();
		if(mkdir(config_dir_name, 0750) == -1 && errno != EEXIST) {
			perror(config_dir_name);
			exit(1);
		}
		if(snprintf(config_dir_name, NAME_MAX, "%s/.config/5D", HOME) == -1)
			abort();
		if(mkdir(config_dir_name, 0750) == -1 && errno != EEXIST) {
			perror(config_dir_name);
			exit(1);
		}
		if(snprintf(config_dir_name, NAME_MAX, "%s/.config/5D/TUI_environment", HOME) == -1)
			abort();
	}
	result = GCx_strdup(config_dir_name);
	self->fEnvironmentName = result;
	return(result);
}
void REPL_init(struct REPL* self) {
	self->fTailEnvironment = NULL;
	self->fTailUserEnvironment = NULL;
	self->fTailUserEnvironmentFrontier = NULL;
	self->fCursorPosition = 0;

	self->fFileModified = false;
	self->fModules = NULL;
	self->fEnvironmentName = NULL;
	self->fEnvironmentCount = 0;
	//self->fConfig = load_Config();
	self->fBRunIO = true;
	REPL_clear(self);
}
char* REPL_get_output_buffer_text(struct REPL* self) {
	// TODO get history text (get_history_event etc)
	// TODO get_history_item
	HIST_ENTRY **history;
	char* buffer;
	char* pos;
	int count = 0;
	history = history_list();
	for(int i = 0; i < history_length; ++i) {
		// TODO timestamp?
		count += 2 + HISTENT_BYTES(history[i]);
	}
	++count;
	pos = buffer = (char*) GC_MALLOC_ATOMIC(count);
	for(int i = 0; i < history_length; ++i) {
		strcpy(pos, history[i]->line);
		pos += strlen(history[i]->line);
		*(pos++) = '\n';
	}
	*(pos++) = 0;
	return(buffer);
}
void REPL_append_to_output_buffer(struct REPL* self, char const* text) {
	char* buffer;
	char* pos;
	char* newline;
	for(pos = buffer = GCx_strdup(text); *pos; ) {
		newline = strchr(pos, '\n');
		if(newline == NULL)
			newline = pos + strlen(pos);
		*newline = 0;
		add_history(pos);
		pos += strlen(pos);
		assert(*pos == 0);
		++pos;
	}
}

struct REPL* REPL_new(void) {
	struct REPL* result;
	result = new REPLX::REPL;
	REPL_init(result);
	return(result);
}

}; /* end namespace */

static char** completion_matches(const char* text, rl_compentry_func_t* callback) {
	return(rl_completion_matches(text, callback));
}
static struct REPL* REPL1; // for completion. eew.
static char* command_generator(const char* text, int state) {
	static int len;
	static AST::NodeT iter;
	if(state == 0) {
		iter = REPL1->fEnvironmentNames;
		len = strlen(text);
	}
	while(iter) {
		const char* name;
		name = AST::get_symbol1_name(AST::get_cons_head(iter));
		if(strncmp(name, text, len) == 0) {
			iter = Evaluators::evaluateToCons(AST::get_cons_tail(iter));
			return(strdup(name)); // XXX
		} else
			iter = Evaluators::evaluateToCons(AST::get_cons_tail(iter));
	}
	return(NULL);
}
static char** complete(const char* text, int start, int end) {
	char** matches = NULL;
	//if(start == 0) // or after a brace.
		matches = completion_matches(text, command_generator);
	return(matches);
}
/* will match the wrong one on purpose. */
static int lexForMatchingParen(const char* command, int commandLen, int position) {
	std::stack<int> openParens;
	Scanners::Scanner scanner;
	int originalParen = command[position];
	int direction = 1;
	if(originalParen == '(' || originalParen == '{' || originalParen == '[')
		direction = 1;
	else {
		direction = (-1);
	}
        FILE* inputFile = fmemopen((void*) command, commandLen, "r");
	if(inputFile) {
		try {
			scanner.push(inputFile, 0, "<stdin>");
			scanner.consume();
			while(scanner.input_value != Symbols::SlessEOFgreater) {
				if(scanner.input_value == Symbols::Sleftparen || scanner.input_value == Symbols::Sleftbracket || scanner.input_value == AST::symbolFromStr("{")) {
					openParens.push(scanner.get_position());
				} else if(scanner.input_value == Symbols::Srightparen || scanner.input_value == Symbols::Srightbracket || scanner.input_value == AST::symbolFromStr("}")) {
					if(openParens.empty()) {
						break; /* indicate that there should be one there. */
					} else {
						int prevParenPos = openParens.top();
						openParens.pop();
						if(direction == 1 && prevParenPos == position) {
							fclose(inputFile);
							return(scanner.get_position());
						} else if(direction == (-1) && scanner.get_position() == position) {
							fclose(inputFile);
							return(prevParenPos);
						}
					}
				}
				scanner.consume();
			}
			fclose(inputFile);
		} catch(...) {
			fclose(inputFile);
                }
        }
        return(-1);
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
static int handle_readline_crlf(int x, int key) {
	/* check rl_point and rl_line_buffer[rl_point - 1] */
	try {
		// TODO if we wanted, we could remember this and just use it for the execute() later.
		REPL_parse(REPL1, rl_line_buffer, strlen(rl_line_buffer), 0);
	} catch (...) { /* FIXME just specific errors? */
		rl_insert(x, '\n');
		return(0);
	}
	rl_done = 1;
	printf("\n");
	return(0); /* FIXME */
}
static int startup_readline(void) {
	rl_bind_key('\n', handle_readline_crlf);
	rl_bind_key('\r', handle_readline_crlf);
	rl_bind_key('(', handle_readline_paren);
	rl_bind_key('{', handle_readline_paren);
	rl_bind_key(')', handle_readline_paren);
	rl_bind_key('}', handle_readline_paren);
	rl_bind_key('[', handle_readline_paren);
	rl_bind_key(']', handle_readline_paren);
	return(0); /* FIXME */
}
static void initialize_readline(void) {
	rl_readline_name = "5D";
	rl_attempted_completion_function = complete;
	rl_startup_hook = startup_readline;
	//rl_sort_completion_matches = 1;
}
using namespace REPLX;
//static Scanners::OperatorPrecedenceList* operator_precedence_list;
void run(struct REPL* REPL, const char* text) {
	AST::NodeT result;
	if(exit_P(text)) /* special case for computers which can't signal EOF. */
		exit(0);
	try {
		result = REPL_parse(REPL, text, strlen(text), 0);
		REPL_execute(REPL, result, 0);
	} catch(Scanners::ParseException exception) {
		AST::NodeT err = Evaluators::makeError(exception.what());
		std::string errStr = str(err);
		fprintf(stderr, "%s\n", errStr.c_str());
	} catch(Evaluators::EvaluationException exception) {
		AST::NodeT err = Evaluators::makeError(exception.what());
		std::string errStr = str(err);
		fprintf(stderr, "%s\n", errStr.c_str());
	}
}
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name) {
	self->fEnvironmentName = GCx_strdup(absolute_name);
}
bool REPL_load_contents_by_name(struct REPL* self, const char* file_name) {
	if(!REPL_load_contents_from(self, file_name))
		return(false);
	else {
		// FIXME char* absolute_name = REPL_get_absolute_path(file_name);
		REPL_set_file_modified(self, false);
		REPL_set_current_environment_name(self, file_name);
		return(true);
	}
}
bool REPL_save(struct REPL* self, bool B_force_dialog) {
	bool B_OK = false;
	char temp_name[PATH_MAX + 1];
	char* file_name = self->fEnvironmentName;
	if(snprintf(temp_name, PATH_MAX, "%sXXXXXX", file_name) == -1)
		abort();
	int FD = mkstemp(temp_name);
	if(FD == -1) {
		perror(temp_name);
		return(false);
	}
	FILE* output_file = fdopen(FD, "w");
	if(!output_file) {
		perror(temp_name);
		return(false);
	}
	if(REPL_save_contents_to(self, output_file)) {
		fclose(output_file);
		close(FD);
		if(rename(temp_name, file_name) != -1) {
			//char* absolute_name = REPL_get_absolute_path(file_name);
			B_OK = true;
			REPL_set_current_environment_name(self, file_name); // absolute_name);
			REPL_set_file_modified(self, false);
		}
		//unlink(temp_name);
	}
	return(B_OK);
}
void print_usage(void) {
	fprintf(stderr, "Usage: TUI [-i] [<environment>]\n");
	fprintf(stderr, " -i | --IO  enable input/output processing\n");
	fprintf(stderr, " -t | --test  enable testing\n");
	fprintf(stderr, " -v | --version  print version and return\n");
}
void print_version(void) {
	printf("5D Version %s - Copyright (C) 2011 Danny Milosavljevic et al.\n", VERSION);
	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, and you are welcome to redistribute it under certain conditions. See /usr/share/doc/5d/copyright for details.\n");
}
int main(int argc, char* argv[]) {
	struct REPL* REPL;
	using namespace GUI;
	const char* line;
	int opt;
	setlocale(LC_ALL, "");
	Allocator_init();
	//GC_disable();
	if(argc >= 1)
		Evaluators::set_shared_dir_by_executable(argv[0]);
	REPL = REPL_new();
	REPL1 = REPL;
	const char* prompt = "eval $ ";
	static struct option long_options[] = {
		{"IO", no_argument, NULL, 'i'},
		{"help", no_argument, NULL, 'h'},
		{"test", no_argument, NULL, 't'},
		{"version", no_argument, NULL, 'v'},
	};
	REPL_set_IO(REPL, false);
	while((opt = getopt_long(argc, argv, "ih", long_options, NULL)) != -1) {
		switch(opt) {
		case 'i':
			REPL_set_IO(REPL, true);
			prompt = "runIO $ ";
			break;
		case 'h':
			print_usage();
			exit(0);
		case 'v':
			print_version();
			exit(0);
		case 't':
			/* TODO REPL_set_testing */
			prompt = "runTest $ ";
			break;
		}
	}
	if(argc > 1 && optind < argc && REPL_load_contents_by_name(REPL, argv[optind])) {
	} else {
		char* environment_name = REPL_ensure_default_environment_name(REPL);
		struct stat stat_buf;
		if(stat(environment_name, &stat_buf) != -1)
			REPL_load_contents_by_name(REPL, environment_name);
	}
	printf("5D Version %s - Copyright (C) 2011 Danny Milosavljevic et al.\n", VERSION);
	printf("This program comes with ABSOLUTELY NO WARRANTY.\n");
	printf("This is free software, and you are welcome to redistribute it under certain conditions. See /usr/share/doc/5d/copyright for details.\n");
	install_SIGQUIT_handler();
	install_SIGINT_handler();
	initialize_readline();
	//operator_precedence_list = new Scanners::OperatorPrecedenceList();
	while((line = readline(prompt))) {
		if(!line)
			break;
		if(!line[0])
			continue;
		add_history(line);
		run(REPL, line);
	}
	printf("\n");
	fflush(stdout);
	REPL_save(REPL, false);
	return(0);
}

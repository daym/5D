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
#include <readline/readline.h>
#include <readline/history.h>
#include <limits.h>
#include "Scanners/MathParser"
#include "Formatters/Math"
#include "TUI/Interrupt"
#include "REPL/REPL"
//#include "Config/Config"

namespace REPLX {

struct REPL {
	AST::Cons* fTailEnvironment;
	AST::Cons* fTailUserEnvironment /* =fTailBuiltinEnvironmentFrontier */;
	AST::Cons* fTailUserEnvironmentFrontier;
	int fEnvironmentCount;
	bool fFileModified;
	char* fEnvironmentName;
	//struct Config* fConfig;
};

int REPL_add_to_environment_simple_GUI(REPL* self, AST::Symbol* name, AST::Node* value) {
	return(self->fEnvironmentCount++);
}
void REPL_queue_scroll_down(REPL* self) {
        // TODO
}
        
};

namespace GUI {
void REPL_set_file_modified(struct REPL* self, bool value);
};
using namespace GUI;
#include "REPL/REPLEnvironment"

namespace GUI {

void REPL_clear(struct REPL* self) {
	self->fTailEnvironment = self->fTailUserEnvironment = self->fTailUserEnvironmentFrontier = NULL;
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
	char* HOME = getenv("HOME");
	if(XDG_CONFIG_HOME) {
		if(snprintf(config_dir_name, NAME_MAX, "%s", XDG_CONFIG_HOME) == -1)
			abort();
		if(mkdir(config_dir_name, 0750) == -1 && errno != EEXIST) {
			perror(config_dir_name);
			exit(1);
		}
		if(snprintf(config_dir_name, NAME_MAX, "%s/5D", XDG_CONFIG_HOME) == -1)
			abort();
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
		if(snprintf(config_dir_name, NAME_MAX, "%s/.config/5D/TUI_environment", HOME) == -1)
			abort();
	}
	result = strdup(config_dir_name);
	self->fEnvironmentName = result;
	return(result);
}
void REPL_init(struct REPL* self) {
	self->fFileModified = false;
	self->fEnvironmentName = NULL;
	//self->fConfig = load_Config();
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
	pos = buffer = (char*) malloc(count);
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
	for(pos = buffer = strdup(text); *pos; ) {
		newline = strchr(pos, '\n');
		*newline = 0;
		add_history(pos);
		pos += strlen(pos);
		assert(*pos == 0);
		++pos;
	}
}

bool REPL_execute(struct REPL* self, AST::Node* input) {
	bool B_ok = false;
	try {
		AST::Node* result = input;
		if(!input || dynamic_cast<AST::Cons*>(input) == NULL || ((AST::Cons*)input)->head != AST::intern("define")) {
			result = REPL_close_environment(self, result);
			result = Evaluators::provide_dynamic_builtins(result);
			result = Evaluators::annotate(result);
			result = Evaluators::reduce(result);
		}
		/*std::string v = result ? result->str() : "OK";
		v = " => " + v + "\n";
		gtk_text_buffer_insert(self->fOutputBuffer, destination, v.c_str(), -1);*/
		//if(result)
		//	printf("%s\n", result->str().c_str());
		Formatters::print_math(REPL_ensure_operator_precedence_list(self), stdout, 0, 0, result);
		fprintf(stdout, "\n");
		fflush(stdout);
		/*REPL_enqueue_LATEX(self, result, destination);*/
		REPL_add_to_environment(self, result);
		B_ok = true;
	} catch(Evaluators::EvaluationException e) {
		std::string v = e.what() ? e.what() : "error";
		v = " => " + v + "\n";
		// FIXME gtk_text_buffer_insert(self->fOutputBuffer, destination, v.c_str(), -1);
	}
	return(B_ok);
}
struct REPL* REPL_new(void) {
	struct REPL* result;
	result = (struct REPL*) calloc(1, sizeof(struct REPL));
	REPL_init(result);
	return(result);
}

}; /* end namespace */


/* TODO enable history file */

const char* commands[] = { /* FIXME un-hardcode */
	"\\",
	"if",
	"cond",
	"define",
	"display",
	"newline",
	"begin",
	NULL,
};
static char** completion_matches(const char* text, rl_compentry_func_t* callback) {
	return(rl_completion_matches(text, callback));
}
static char* command_generator(const char* text, int state) {
	static int len;
	static int list_index;
	const char* name;
	if(state == 0) {
		list_index = 0;
		len = strlen(text);
	}
	while((name = commands[list_index])) {
		++list_index;
		if(strncmp(name, text, len) == 0)
			return(strdup(name));
	}
	return(NULL);
}
static char** complete(const char* text, int start, int end) {
	char** matches = NULL;
	//if(start == 0) // or after a brace.
		matches = completion_matches(text, command_generator);
	return(matches);
}
static void initialize_readline(void) {
	rl_readline_name = "5D";
	rl_attempted_completion_function = complete;
	rl_sort_completion_matches = 1;
}
using namespace REPLX;
static Scanners::OperatorPrecedenceList* operator_precedence_list;
void run(struct REPL* REPL, const char* text) {
	AST::Node* result;
	Scanners::MathParser parser;
	FILE* input_file;
	try {
		input_file = fmemopen((void*) text, strlen(text), "r");
		parser.push(input_file, 0);
		result = parser.parse(operator_precedence_list);
		parser.ensure_end();
		fclose(input_file);
		REPL_execute(REPL, result);
	} catch(Scanners::ParseException exception) {
		AST::Node* err = AST::cons(AST::intern("error"), AST::cons(new AST::String(exception.what()), NULL));
		std::string errStr = err->str();
		fprintf(stderr, "%s\n", errStr.c_str());
	} catch(Evaluators::EvaluationException exception) {
		AST::Node* err = AST::cons(AST::intern("error"), AST::cons(new AST::String(exception.what()), NULL));
		std::string errStr = err->str();
		fprintf(stderr, "%s\n", errStr.c_str());
	}
}
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name) {
	self->fEnvironmentName = strdup(absolute_name);
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
	FILE* output_file = fdopen(FD, "w");
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
int main(int argc, char* argv[]) {
	struct REPL* REPL;
	using namespace TUI;
	const char* line;
	REPL = REPL_new();
	if(argc > 1) {
		const char* name = argv[argc - 1];
		REPL_load_contents_by_name(REPL, name);
	} else {
		char* environment_name = REPL_ensure_default_environment_name(REPL);
		struct stat stat_buf;
		if(stat(environment_name, &stat_buf) != -1)
			REPL_load_contents_by_name(REPL, environment_name);
	}
	install_SIGQUIT_handler();
	install_SIGINT_handler();
	initialize_readline();
	operator_precedence_list = new Scanners::OperatorPrecedenceList();
	while((line = readline("\316\1\273\2> "))) {
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

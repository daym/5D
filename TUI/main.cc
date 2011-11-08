#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "Scanners/MathParser"
#include "Formatters/Math"

namespace REPLX {

struct REPL {
	AST::Cons* fTailEnvironment;
	AST::Cons* fTailUserEnvironment /* =fTailBuiltinEnvironmentFrontier */;
	AST::Cons* fTailUserEnvironmentFrontier;
	int fEnvironmentCount;
	bool fBModified;
};

int REPL_add_to_environment_simple_GUI(REPL* self, AST::Symbol* name, AST::Node* value) {
	return(self->fEnvironmentCount++);
}
void REPL_queue_scroll_down(REPL* self) {
        // TODO
}
void REPL_set_file_modified(REPL* self, bool value) {
	self->fBModified = value;
}
void REPL_add_to_environment(struct REPL* self, AST::Node* definition);
        
};

#include "REPL/REPLEnvironment"

namespace REPLX {

void REPL_add_to_environment(struct REPL* self, AST::Node* definition) {
	using namespace AST;
	AST::Cons* definitionCons;
	if(!definition)
		return;
	definitionCons = dynamic_cast<AST::Cons*>(definition);
	if(!definitionCons || !definitionCons->head || !definitionCons->tail || definitionCons->head != intern("define"))
		return;
	definitionCons = definitionCons->tail;
	AST::Symbol* procedureName = dynamic_cast<AST::Symbol*>(definitionCons->head);
	if(!procedureName || !definitionCons->tail)
		return;
	AST::Node* value = follow_tail(definitionCons->tail)->head;
	REPL_add_to_environment_simple(self, procedureName, value);
}

void REPL_clear(struct REPL* self) {
	self->fTailEnvironment = self->fTailUserEnvironment = self->fTailUserEnvironmentFrontier = NULL;
	REPL_init_builtins(self);
}
void REPL_init(struct REPL* self) {
	self->fBModified = false;
	REPL_clear(self);
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
int main() {
	struct REPL* REPL;
	const char* line;
	REPL = REPL_new();
	initialize_readline();
	operator_precedence_list = new Scanners::OperatorPrecedenceList();
	while((line = readline("λ> "))) {
		if(!line)
			break;
		if(!line[0])
			continue;
		add_history(line);
		run(REPL, line);
	}
	return(0);
}

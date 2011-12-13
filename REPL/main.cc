#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <map>
#include <string>
#include "Scanners/SExpressionParser"
#include "Scanners/MathParser"
#include "FFIs/FFIs"
#include "Formatters/SExpression"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "REPL/REPL"
#include "AST/Symbols"

namespace GUI {
bool interrupted_P(void) {
	return(false);
}
};
namespace REPLX {
using namespace Evaluators;

struct REPL : AST::Node {
	AST::Node* fTailEnvironment;
	AST::Node* fTailUserEnvironment /* =fTailBuiltinEnvironmentFrontier */;
	AST::Node* fTailUserEnvironmentFrontier;
	int fEnvironmentCount;
	int fCursorPosition;
	bool fBModified;
	std::map<std::string, AST::Node*>* fModules;
};

int REPL_add_to_environment_simple_GUI(REPL* self, AST::Symbol* name, AST::Node* value) {
	return(self->fEnvironmentCount++);
}

void REPL_queue_scroll_down(REPL* self) {
	// TODO
}


};
namespace GUI {

void REPL_set_file_modified(REPL* self, bool value) {
	self->fBModified = value;
}

char* REPL_get_output_buffer_text(struct REPL* self) {
	return(NULL);
}

void REPL_append_to_output_buffer(struct REPL* self, char const* text) {
}

};
using namespace GUI;

#include "REPL/REPLEnvironment"

namespace GUI {

void REPL_clear(struct REPL* self) {
	self->fTailEnvironment = self->fTailUserEnvironment = self->fTailUserEnvironmentFrontier = NULL;
	REPL_init_builtins(self);
}
void REPL_init(struct REPL* self) {
	self->fBModified = false;
	self->fModules = NULL;
	REPL_clear(self);
}
bool REPL_execute(struct REPL* self, AST::Node* input) {
	bool B_ok = false;
	try {
		AST::Node* result = input;
		if(!Evaluators::define_P(input)) {
			result = REPL_close_environment(self, result);
			result = Evaluators::provide_dynamic_builtins(result);
			result = Evaluators::annotate(result);
			result = Evaluators::reduce(result);
		}
		/*std::string v = result ? result->str() : "OK";
		v = " => " + v + "\n";
		gtk_text_buffer_insert(self->fOutputBuffer, destination, v.c_str(), -1);*/
		Formatters::print_S_Expression(stdout, 0, 0, result);
		fprintf(stdout, "\n");
		fflush(stdout);
		/*REPL_enqueue_LATEX(self, result, destination);*/
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
int main() {
	Scanners::SExpressionParser parser;
	int status = 0;
	bool B_first = true;
	FILE* input_file;
	using namespace REPLX;
	struct REPL* REPL = REPL_new();
	input_file = stdin;
	parser.push(input_file, 0);
	while(!parser.EOFP()) {
		try {
			if(B_first)
				B_first = false;
			else
				parser.parse_closing_brace();
			if(parser.EOFP())
				break;
			AST::Node* source = parser.parse_S_list(false);
			source = Evaluators::programFromSExpression(source);
			// TODO parse left-over ")"
			REPL_execute(REPL, source);
		} catch(Scanners::ParseException exception) {
			AST::Node* err = Evaluators::makeError(exception.what());
			std::string errStr = str(err);
			fprintf(stderr, "%s\n", errStr.c_str());
			status = 1;
			break;
			//parser.consume();
		} catch(Evaluators::EvaluationException exception) {
			AST::Node* err = Evaluators::makeError(exception.what());
			std::string errStr = str(err);
			fprintf(stderr, "%s\n", errStr.c_str());
			status = 2;
		}
	}
	return(status);
}

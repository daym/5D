#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include "Scanners/MathParser"
#include "FFIs/FFIs"
#include "Formatters/SExpression"

namespace REPLX {

struct REPL {
	AST::Cons* fTailEnvironment;
	AST::Cons* fTailUserEnvironment /* =fTailBuiltinEnvironmentFrontier */;
	AST::Cons* fTailUserEnvironmentFrontier;
	int fEnvironmentCount;
};

int REPL_add_to_environment_simple_GUI(REPL* self, AST::Symbol* name, AST::Node* value) {
	return(self->fEnvironmentCount++);
}

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
		Formatters::print_S_Expression(stdout, 0, 0, result);
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
int main() {
	Scanners::MathParser parser;
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
			// TODO parse left-over ")"
			REPL_execute(REPL, source);
		} catch(Scanners::ParseException exception) {
			AST::Node* err = AST::cons(AST::intern("error"), AST::cons(new AST::String(exception.what()), NULL));
			std::string errStr = err->str();
			fprintf(stderr, "%s\n", errStr.c_str());
			status = 1;
			parser.consume();
		} catch(Evaluators::EvaluationException exception) {
			AST::Node* err = AST::cons(AST::intern("error"), AST::cons(new AST::String(exception.what()), NULL));
			std::string errStr = err->str();
			fprintf(stderr, "%s\n", errStr.c_str());
			status = 2;
		}
	}
	return(status);
}

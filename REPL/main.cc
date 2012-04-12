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
#include "AST/HashTable"
#include "FFIs/Allocators"
#include "Evaluators/ModuleLoader"

namespace GUI {
bool interrupted_P(void) {
	return(false);
}
};
namespace REPLX {
using namespace Evaluators;

struct REPL : AST::Node {
	AST::NodeT fTailEnvironment;
	AST::NodeT fTailUserEnvironment /* =fTailBuiltinEnvironmentFrontier */;
	AST::NodeT fTailUserEnvironmentFrontier;
	int fEnvironmentCount;
	int fCursorPosition;
	bool fBModified;
	AST::HashTable* fModules;
	bool fBRunIO;
};

int REPL_add_to_environment_simple_GUI(REPL* self, AST::NodeT name, AST::NodeT value) {
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

void REPL_insert_into_output_buffer(struct REPL* self, int destination, char const* text) {
}
void REPL_append_to_output_buffer(struct REPL* self, char const* text) {
}
static void REPL_enqueue_LATEX(struct REPL* self, AST::NodeT node, int destination) {
}

};
using namespace GUI;
#define FILL_END_ITER 
#define END_ITER 0
#include "REPL/REPLEnvironment"

namespace GUI {

void REPL_clear(struct REPL* self) {
	self->fTailEnvironment = self->fTailUserEnvironment = self->fTailUserEnvironmentFrontier = NULL;
	REPL_init_builtins(self);
}
void REPL_init(struct REPL* self) {
	self->fBModified = false;
	self->fModules = NULL;
	self->fBRunIO = true;
	REPL_clear(self);
}
struct REPL* REPL_new(void) {
	struct REPL* result;
	result = new REPLX::REPL;
	REPL_init(result);
	return(result);
}

}; /* end namespace */
int main(int argc, char* argv[]) {
	Allocator_init();
	if(argc >= 1)
		Evaluators::set_shared_dir_by_executable(argv[0]);
	Scanners::SExpressionParser parser;
	int status = 0;
	bool B_first = true;
	FILE* input_file;
	using namespace REPLX;
	struct REPL* REPL = REPL_new();
	input_file = stdin;
	parser.push(input_file, 0, "<stdin>");
	while(!parser.EOFP()) {
		try {
			if(B_first)
				B_first = false;
			else
				parser.parse_closing_brace();
			if(parser.EOFP())
				break;
			AST::NodeT source = parser.parse_S_list(false);
			source = Evaluators::programFromSExpression(source);
			// TODO parse left-over ")"
			REPL_execute(REPL, source, 0);
		} catch(Scanners::ParseException exception) {
			AST::NodeT err = Evaluators::makeError(exception.what());
			std::string errStr = str(err);
			fprintf(stderr, "%s\n", errStr.c_str());
			status = 1;
			break;
			//parser.consume();
		} catch(Evaluators::EvaluationException exception) {
			AST::NodeT err = Evaluators::makeError(exception.what());
			std::string errStr = str(err);
			fprintf(stderr, "%s\n", errStr.c_str());
			status = 2;
		}
	}
	return(status);
}

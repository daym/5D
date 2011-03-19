#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "Scanners/MathParser"

const char* commands[] = {
	"if",
	"cond",
	"define",
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
	rl_readline_name = "4D";
	rl_attempted_completion_function = complete;
	rl_sort_completion_matches = 1;
}
int main() {
	const char* line;
	Scanners::MathParser parser;
	initialize_readline();
	while((line = readline("λ> "))) {
		if(!line)
			break;
		FILE* input_file = fmemopen((void*) line, strlen(line), "r");
		try {
			try {
				AST::Node* result = parser.parse(input_file);
				if(result)
					printf("%s\n", result->str().c_str());
			} catch(...) {
				fclose(input_file);
			}
		} catch(Scanners::ParseException e) {
			fprintf(stderr, "error: %s\n", e.what());
		}
	}
	return(0);
}

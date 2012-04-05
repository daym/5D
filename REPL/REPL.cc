/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <list>
#include "Scanners/Scanner"
#include "AST/AST"
#include "AST/Symbol"
#include "Scanners/MathParser"
#include "Scanners/SExpressionParser"
#include "Formatters/SExpression"
#include "Evaluators/FFI"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"

namespace REPLX {
struct REPL;
void REPL_set_environment(struct REPL* self, AST::NodeT environment);
AST::NodeT REPL_get_user_environment(struct REPL* self);
void REPL_add_to_environment_simple(struct REPL* self, AST::NodeT name, AST::NodeT value);
};
namespace GUI {
using namespace REPLX;

#if 0
static bool save_integer(FILE* output_file, long value) {
	value = htonl(value);
	return fwrite(&value, sizeof(value), 1, output_file) == 1;
}
static bool save_string(FILE* output_file, const char* s) {
	if(!s)
		s = "";
	int l = strlen(s);
	/*if(!save_integer(output_file, l))
		return(false);
	else*/
		return fwrite(s, l + 1, 1, output_file) == 1;
}
const char* load_string(const char*& string_iter) {
	const char* result;
	result = string_iter;
	int l = strlen(string_iter);
	string_iter += l + 1;
	return(result);
}
#endif
bool REPL_get_file_modified(struct REPL* self);
char* REPL_get_output_buffer_text(struct REPL* self);
bool REPL_confirm_close(struct REPL* self);
void REPL_clear(struct REPL* self);
void REPL_append_to_output_buffer(struct REPL* self, const char* text);
void REPL_add_to_environment(struct REPL* self, AST::NodeT name, AST::NodeT body);
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name);
void REPL_set_file_modified(struct REPL* self, bool value);
static AST::NodeT REPL_filter_environment(struct REPL* self, AST::NodeT environment) {
	return(environment);
}
bool REPL_save_contents_to(struct REPL* self, FILE* output_file) {
	// (REPLV1 'textBufferText "abc" 'environment "xyz" nil)
	// (((((REPLV1 'textBufferText) "abc") 'environment) "xyz") nil)
	//     tbtK--------------------
	//    --tbtV--------------------------
	//   ----envK---------------------------------------
	//  -------envV--------------------------------------------
	// ---------sentinel--------------------------------------------
	using namespace AST;
	char* buffer_text;
	buffer_text = REPL_get_output_buffer_text(self);
	AST::NodeT tbtK = AST::makeApplication(Symbols::SREPLV1, Symbols::StextBufferText);
	AST::NodeT tbtV = AST::makeApplication(tbtK, makeStr(buffer_text));
	AST::NodeT envK = AST::makeApplication(tbtV, Symbols::Senvironment);
	AST::NodeT envV = AST::makeApplication(envK, REPL_filter_environment(self, REPL_get_user_environment(self)));
	//std::string v = Evaluators::str(envV);
	//assert(strstr(v.c_str(), "\\divmoxxd") == NULL);
	AST::NodeT sentinel = AST::makeApplication(envV, NULL);
	Formatters::print_S_Expression(output_file, 0, 0, sentinel);
	return(true);
}
bool REPL_load_contents_from(struct REPL* self, const char* name) {
	REPL_clear(self);
	{
		FILE* input_file;
		AST::NodeT content;
		Scanners::SExpressionParser parser;
		input_file = fopen(name, "r");
		if(!input_file) {
			fprintf(stderr, "could not open \"%s\": %s\n", name, strerror(errno)); // FIXME nicer logging
			return(false);
		}
		try {
			parser.push(input_file, 0, name);
			content = Evaluators::programFromSExpression(parser.parse_S_Expression());
		} catch(Scanners::ParseException exception) {
			fprintf(stderr, "error: failed to load file: \"%s\"\n", name);
			fclose(input_file);
			return(false);
		}
		if(!application_P(content)) {
			fclose(input_file);
			return(false);
		}
		try {
			// (REPLV1 'textBufferText "abc" 'environment "xyz" nil)
			// (((((REPLV1 'textBufferText) "abc") 'environment) "xyz") nil)
			//     tbtK--------------------
			//    --tbtV--------------------------
			//   ----envK---------------------------------------
			//  -------envV--------------------------------------------
			// ---------sentinel--------------------------------------------
			std::list<AST::NodeT> arguments;
			for(; application_P(content); content = get_application_operator(content)) {
				arguments.push_front(get_application_operand(content));
				if(get_application_operator(content) == Symbols::SREPLV1)
					break;
			}
			std::list<AST::NodeT>::const_iterator end_arguments = arguments.end();
			for(std::list<AST::NodeT>::const_iterator iter_arguments = arguments.begin(); iter_arguments != end_arguments; ++iter_arguments) {
				AST::NodeT keywordName = *iter_arguments;
				++iter_arguments;
				if(iter_arguments == end_arguments) // ???
					break;
				AST::NodeT value = *iter_arguments;
				if(keywordName == Symbols::StextBufferText) {
					const char* text;
					text = (value == Symbols::Snil || value == NULL) ? "" : Evaluators::get_string(value);
					REPL_append_to_output_buffer(self, text);
				} else if(keywordName == Symbols::Senvironment) {
					if(value && value != Symbols::Snil)
						assert(application_P(value));
					REPL_set_environment(self, value);
	// GC_gcollect here and it works.
		//AST::NodeT envV = AST::makeApplication(NULL, REPL_filter_environment(self, REPL_get_user_environment(self)));
		//std::string v = Evaluators::str(envV);
		//assert(strstr(v.c_str(), "\\divmod") == NULL);
				}
			}
			fclose(input_file);
		} catch (Evaluators::EvaluationException& e) {
			// TODO print message
			fclose(input_file);
			REPL_set_file_modified(self, false);
			return(false);
		}
	}
	REPL_set_file_modified(self, false);
	return(true);
}
/* caller needs to make sure it would actually work...*/
void REPL_add_to_environment(struct REPL* self, AST::NodeT name, AST::NodeT body) {
	using namespace AST;
	if(AST::get_symbol1_name(name))
		REPL_add_to_environment_simple(self, name, body);
}


}; // end namespace GUI


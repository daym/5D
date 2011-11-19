/*
5D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include "Scanners/Scanner"
#include "AST/AST"
#include "AST/Symbol"
#include "Scanners/MathParser"
#include "Formatters/SExpression"
#include "Evaluators/FFI"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"

namespace REPLX {
struct REPL;
void REPL_set_environment(struct REPL* self, AST::Cons* environment);
AST::Cons* REPL_get_user_environment(struct REPL* self);
void REPL_add_to_environment_simple(struct REPL* self, AST::Symbol* name, AST::Node* value);
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
void REPL_add_to_environment(struct REPL* self, AST::Node* definition);
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name);
void REPL_set_file_modified(struct REPL* self, bool value);
static AST::Node* REPL_filter_environment(struct REPL* self, AST::Node* environment) {
	return(environment);
}
bool REPL_save_contents_to(struct REPL* self, FILE* output_file) {
	using namespace AST;
	char* buffer_text;
	buffer_text = REPL_get_output_buffer_text(self);
	AST::Cons* buffer_text_box = AST::cons(AST::intern("textBufferText"), AST::cons(str_literal(buffer_text), NULL));
	AST::Cons* environment_box = AST::cons(AST::intern("environment"), AST::cons(dynamic_cast<AST::Cons*>(REPL_filter_environment(self, REPL_get_user_environment(self))), NULL));
	AST::Cons* content_box = AST::cons(AST::intern("REPLV1"), AST::cons(buffer_text_box, AST::cons(environment_box, NULL)));
	Formatters::print_S_Expression(output_file, 0, 0, content_box);
	return(true);
}
bool REPL_load_contents_from(struct REPL* self, const char* name) {
	REPL_clear(self);
	{
		FILE* input_file;
		AST::Node* content;
		AST::Cons* contentCons;
		Scanners::MathParser parser;
		input_file = fopen(name, "r");
		if(!input_file) {
			fprintf(stderr, "could not open \"%s\": %s\n", name, strerror(errno)); // FIXME nicer logging
			return(false);
		}
		try {
			parser.push(input_file, 0, false);
			parser.consume();
			content = parser.parse_S_Expression();
		} catch(Scanners::ParseException exception) {
			fprintf(stderr, "error: failed to load file: \"%s\"\n", name);
			fclose(input_file);
			return(false);
		}
		contentCons = dynamic_cast<AST::Cons*>(content);
		if(!contentCons || contentCons->head != AST::intern("REPLV1")) {
			fclose(input_file);
			return(false);
		}
		for(AST::Cons* entryCons = contentCons->tail; entryCons; entryCons = entryCons->tail) {
			AST::Cons* entry = dynamic_cast<AST::Cons*>(entryCons->head);
			if(!entry)
				continue;
			if(entry->head == AST::intern("textBufferText") && entry->tail) {
				char* text;
				text = Evaluators::get_native_string(entry->tail->head);
				REPL_append_to_output_buffer(self, text);
				if(text)
					free(text);
			} else if(entry->head == AST::intern("environment") && entry->tail) {
				AST::Cons* environment = dynamic_cast<AST::Cons*>(entry->tail->head);
				if(!environment)
					continue;
				REPL_set_environment(self, environment);
			}
		}
		fclose(input_file);
	}
	REPL_set_file_modified(self, false);
	return(true);
}
/* FIXME remove */
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

}; // end namespace GUI


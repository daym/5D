#include <gtk/gtk.h>
#include <errno.h>
#include <string.h>
#include "AST/AST"
#include "AST/Symbol"
#include "Scanners/MathParser"
#include "Formatters/SExpression"
#include "Evaluators/FFI"

namespace GUI {

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
AST::Cons* REPL_get_environment(struct REPL* self);
bool REPL_get_file_modified(struct REPL* self);
char* REPL_get_output_buffer_text(struct REPL* self);
bool REPL_confirm_close(struct REPL* self);
void REPL_clear(struct REPL* self);
void REPL_append_to_output_buffer(struct REPL* self, const char* text);
void REPL_add_to_environment(struct REPL* self, AST::Node* definition);
char* REPL_get_absolute_path(const char* name);
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name);
void REPL_set_file_modified(struct REPL* self, bool value);
bool REPL_save_content_to(struct REPL* self, FILE* output_file) {
	using namespace AST;
	char* buffer_text;
	buffer_text = REPL_get_output_buffer_text(self);
	AST::Cons* buffer_text_box = AST::cons(AST::intern("text_buffer_text"), AST::cons(string_literal(buffer_text), NULL));
	AST::Cons* environment_box = AST::cons(AST::intern("environment"), REPL_get_environment(self));
	AST::Cons* content_box = AST::cons(buffer_text_box, AST::cons(environment_box, NULL));
	Formatters::print_S_Expression(output_file, 0, 0, content_box);
	return(true);
}
bool REPL_load_contents_from(struct REPL* self, const char* name) {
	{
		if(REPL_get_file_modified(self))
			if(!REPL_confirm_close(self))
				return(false);
	}
	REPL_clear(self);
	{
		FILE* input_file;
		AST::Node* content;
		AST::Cons* contentCons;
		Scanners::MathParser parser;
		input_file = fopen(name, "r");
		if(!input_file) {
			g_warning("could not open \"%s\": %s", name, strerror(errno));
			return(false);
		}
		content = parser.parse_S_Expression(input_file);
		contentCons = dynamic_cast<AST::Cons*>(content);
		if(!contentCons) {
			fclose(input_file);
			return(false);
		}
		for(AST::Cons* entryCons = contentCons; entryCons; entryCons = entryCons->tail) {
			AST::Cons* entry = dynamic_cast<AST::Cons*>(entryCons->head);
			if(!entry)
				continue;
			if(entry->head == AST::intern("text_buffer_text") && entry->tail) {
				char* text;
				text = Evaluators::get_native_string(entry->tail->head);
				REPL_append_to_output_buffer(self, text);
				g_free(text);
			} else if(entry->head == AST::intern("environment") && entry->tail) {
				AST::Cons* environment = dynamic_cast<AST::Cons*>(entry->tail->head);
				if(!environment)
					continue;
				for(AST::Cons* definition = environment; definition; definition = definition->tail) {
					AST::Node* nameNode = definition->head;
					AST::Symbol* nameSymbol = dynamic_cast<AST::Symbol*>(nameNode);
					if(!nameSymbol)
						continue;
					REPL_add_to_environment(self, AST::cons(AST::intern("define"), AST::cons(nameSymbol, AST::cons(definition->tail->head, NULL))));
					/*key = nameSymbol->name;
					GtkTreeIter iter;
					std::string valueString;
					if(definition->tail && definition->tail->head)
						valueString = definition->tail->head->str();
					value = valueString.c_str();
					gtk_list_store_append(self->fEnvironmentStore, &iter);
					gtk_list_store_set(self->fEnvironmentStore, &iter, 0, key, 1, value, -1);
					g_hash_table_replace(self->fEnvironmentKeys, AST::intern(key), gtk_tree_iter_copy(&iter));*/
				}
			}
		}
		fclose(input_file);
	}
	{
		char* absolute_name = REPL_get_absolute_path(name);
		REPL_set_file_modified(self, false);
		REPL_set_current_environment_name(self, absolute_name);
	}
	return(true);
}

}; // end namespace GUI


/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <netinet/in.h>
#include "GUI/GTKREPL"
#include "Scanners/MathParser"
#include "Config/Config"

namespace GUI {

static gboolean handle_key_press(GtkWidget* widget, GdkEventKey* event, gpointer user_data) {
	if(((event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == GDK_CONTROL_MASK) && event->keyval == GDK_Return) {
		gtk_dialog_response(GTK_DIALOG(user_data), GTK_RESPONSE_OK);
		return(TRUE);
	}
	return(FALSE);
}
static gboolean g_confirm_close(GtkDialog* dialog, GdkEvent* event, GTKREPL* REPL) {
	return(!REPL->confirm_close());
}
static gboolean accept_prefix(GtkEntryCompletion* completion, gchar* prefix, GtkEntry* entry) {
	return(FALSE);
}
static gboolean accept_match(GtkEntryCompletion* completion, GtkTreeModel* model, GtkTreeIter* iter, GtkEntry* entry) {
	printf("MATCH\n");
	return(FALSE);
}
static gboolean complete_command(GtkEntry* entry, GdkEventKey* key, GtkEntryCompletion* completion) {
	if((key->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_MOD2_MASK|GDK_MOD3_MASK|GDK_MOD4_MASK|GDK_MOD5_MASK)) == 0 && key->keyval == GDK_Tab) {
		gtk_entry_completion_set_popup_completion(completion, TRUE);
		g_signal_emit_by_name(entry, "changed", NULL); // GTK bug 584402
		gtk_entry_completion_complete(completion);
		gtk_entry_completion_set_popup_completion(completion, FALSE);
		{
			gint beginning;
			gint end;
			if(gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &beginning, &end)) {
				gtk_editable_select_region(GTK_EDITABLE(entry), end, end);
				gtk_editable_set_position(GTK_EDITABLE(entry), end);
			}
		}
		return(TRUE);
	}
	return(FALSE);
}
GTKREPL::GTKREPL(GtkWindow* parent) {
	fFileModified = false;
	fEnvironmentKeys = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) gtk_tree_iter_free);
	fWidget = (GtkWindow*) gtk_dialog_new_with_buttons("REPL", parent, (GtkDialogFlags) 0, GTK_STOCK_EXECUTE, GTK_RESPONSE_OK, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, GTK_STOCK_OPEN, GTK_RESPONSE_REJECT, NULL);
	fSaveDialog = (GtkFileChooser*) gtk_file_chooser_dialog_new("Save REPL", GTK_WINDOW(fWidget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
	fOpenDialog = (GtkFileChooser*) gtk_file_chooser_dialog_new("Open REPL", GTK_WINDOW(fWidget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(fWidget), GTK_RESPONSE_OK);
	fMainBox = (GtkBox*) gtk_vbox_new(FALSE, 7);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fWidget)->vbox), GTK_WIDGET(fMainBox));
	gtk_widget_show(GTK_WIDGET(fMainBox));
	fEnvironmentView = (GtkTreeView*) gtk_tree_view_new();
	GtkTreeViewColumn* fNameColumn;
	fNameColumn = gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_append_column(fEnvironmentView, fNameColumn);
	fEnvironmentStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(fEnvironmentView, gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(fEnvironmentStore)));
	gtk_widget_show(GTK_WIDGET(fEnvironmentView));
	fEnvironmentScroller = (GtkScrolledWindow*) gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(fEnvironmentScroller, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(fEnvironmentScroller), GTK_WIDGET(fEnvironmentView));
	gtk_widget_show(GTK_WIDGET(fEnvironmentScroller));
	//fShortcutBox = (GtkBox*) gtk_hbutton_box_new();
	//fExecuteButton = (GtkButton*) gtk_button_new_from_stock(GTK_STOCK_OK);
	//gtk_widget_show(GTK_WIDGET(fExecuteButton));
	//gtk_box_pack_start(fShortcutBox, GTK_WIDGET(fExecuteButton), TRUE, TRUE, 7);
	//gtk_widget_show(GTK_WIDGET(fShortcutBox));
	fCommandEntry = (GtkEntry*) gtk_entry_new();
	fCommandCompletion = gtk_entry_completion_new();
	g_signal_connect(G_OBJECT(fCommandEntry), "key-press-event", G_CALLBACK(complete_command), fCommandCompletion);
	g_signal_connect(G_OBJECT(fCommandCompletion), "match-selected", G_CALLBACK(accept_match), fCommandEntry);
	g_signal_connect(G_OBJECT(fCommandCompletion), "insert-prefix", G_CALLBACK(accept_prefix), fCommandEntry);
	gtk_entry_completion_set_model(fCommandCompletion, GTK_TREE_MODEL(fEnvironmentStore));
	gtk_entry_completion_set_text_column(fCommandCompletion, 0);
	gtk_entry_completion_set_popup_completion(fCommandCompletion, FALSE);
	gtk_entry_completion_set_inline_completion(fCommandCompletion, TRUE); // XXX
	//gtk_entry_completion_set_inline_selection(fCommandCompletion, TRUE); // XXX
	gtk_entry_completion_set_popup_single_match(fCommandCompletion, FALSE);
	gtk_entry_set_completion(fCommandEntry, fCommandCompletion);
	gtk_entry_set_activates_default(fCommandEntry, TRUE);
	gtk_widget_show(GTK_WIDGET(fCommandEntry));
	fOutputArea = (GtkTextView*) gtk_text_view_new();
	fOutputScroller = (GtkScrolledWindow*) gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(fOutputScroller, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	// TODO set wrap mode
	gtk_text_view_set_accepts_tab(fOutputArea, FALSE);
	g_signal_connect(G_OBJECT(fOutputArea), "key-press-event", G_CALLBACK(handle_key_press), fWidget);
	gtk_widget_show(GTK_WIDGET(fOutputArea));
	gtk_container_add(GTK_CONTAINER(fOutputScroller), GTK_WIDGET(fOutputArea));
	gtk_widget_show(GTK_WIDGET(fOutputScroller));
	fOutputBuffer = gtk_text_view_get_buffer(fOutputArea);
	fEditorBox = (GtkBox*) gtk_hbox_new(FALSE, 7);
	gtk_box_pack_start(GTK_BOX(fEditorBox), GTK_WIDGET(fEnvironmentScroller), FALSE, FALSE, 7); 
	gtk_box_pack_start(GTK_BOX(fEditorBox), GTK_WIDGET(fOutputScroller), TRUE, TRUE, 7); 
	gtk_widget_show(GTK_WIDGET(fEditorBox));
	//gtk_box_pack_start(GTK_BOX(fMainBox), GTK_WIDGET(fShortcutBox), FALSE, FALSE, 7); 
	gtk_box_pack_start(GTK_BOX(fMainBox), GTK_WIDGET(fEditorBox), TRUE, TRUE, 7); 
	gtk_box_pack_start(GTK_BOX(fMainBox), GTK_WIDGET(fCommandEntry), FALSE, FALSE, 7); 
	g_signal_connect_swapped(GTK_DIALOG(fWidget), "response", G_CALLBACK(&GTKREPL::handle_response), (void*) this);
	gtk_window_set_focus(GTK_WINDOW(fWidget), GTK_WIDGET(fCommandEntry));
	gtk_tree_view_column_set_sort_column_id(fNameColumn, 0);
	gtk_tree_view_column_set_sort_indicator(fNameColumn, TRUE);
	//gtk_tree_view_column_set_sort_order(fNameColumn, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(fEnvironmentStore), 0, GTK_SORT_ASCENDING);
	fConfig = load_Config();
	{
		char* environment_name;
		environment_name = Config_get_environment_name(fConfig);
		if(environment_name && environment_name[0])
			load_contents_from(environment_name);
	}
	g_signal_connect(G_OBJECT(fWidget), "delete-event", G_CALLBACK(g_confirm_close), this);
}
GtkWidget* GTKREPL::widget(void) const {
	return(GTK_WIDGET(fWidget));
}
void GTKREPL::execute(const char* command, GtkTextIter* destination) {
	Scanners::MathParser parser;
	FILE* input_file = fmemopen((void*) command, strlen(command), "r");
	if(input_file) {
		try {
			try {
				AST::Node* result = parser.parse(input_file);
				add_to_environment(result);
				std::string v = result ? result->str() : "OK";
				v = " => " + v + "\n";
				gtk_text_buffer_insert(fOutputBuffer, destination, v.c_str(), -1);
			} catch(...) {
				fclose(input_file);
				throw;
			}
		} catch(Scanners::ParseException e) {
			std::string v = e.what() ? e.what() : "error";
			v = " => " + v + "\n";
			gtk_text_buffer_insert(fOutputBuffer, destination, v.c_str(), -1);
		}
		set_file_modified(true);
	}
}
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
bool GTKREPL::save_content_to(FILE* output_file) {
	GtkTreeIter iter;
	GtkTextIter start;
	GtkTextIter end;
	if(!save_string(output_file, "4DV1"))
		return(false);
	gtk_text_buffer_get_start_iter(fOutputBuffer, &start);
	gtk_text_buffer_get_end_iter(fOutputBuffer, &end);
	char* text = gtk_text_buffer_get_text(fOutputBuffer, &start, &end, FALSE);
	if(!save_string(output_file, text))
		return(false);
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(fEnvironmentStore), &iter)) {
		while(1) {
			char* name;
			char* value;
			gtk_tree_model_get(GTK_TREE_MODEL(fEnvironmentStore), &iter, 0, &name, 1, &value, -1);
			if(!save_string(output_file, name))
				return(false);
			if(!save_string(output_file, value))
				return(false);
			//g_free(name);
			//g_free(value);
			if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(fEnvironmentStore), &iter))
				break;
		}
	}
	return(true);
}
const char* load_string(const char*& string_iter) {
	const char* result;
	result = string_iter;
	int l = strlen(string_iter);
	string_iter += l + 1;
	return(result);
}
static char* get_absolute_path(const char* name) {
	if(g_path_is_absolute(name))
		return(strdup(name));
	else
		return(g_build_filename(g_get_current_dir(), name, NULL));
}
bool GTKREPL::load_contents_from(const char* name) {
	GError* error = NULL;
	gsize size;
	char* contents;
	const char* content_iter;
	GtkTextIter text_start;
	GtkTextIter text_end;
	if(get_file_modified())
		if(!confirm_close())
			return(false);
	gtk_text_buffer_get_start_iter(fOutputBuffer, &text_start);
	gtk_text_buffer_get_end_iter(fOutputBuffer, &text_end);
	gtk_text_buffer_delete(fOutputBuffer, &text_start, &text_end);
	if(!g_file_get_contents(name, &contents, &size, &error)) {
		g_warning("%s", error->message);
		g_error_free(error);
		return(false);
	}
	if(size < 1)
		return(false);
	if(contents[size - 1] != 0) /* should end with 00 */
		return(false);
	content_iter = contents;
	if(strcmp(load_string(content_iter), "4DV1") != 0)
		return(false);
	gtk_text_buffer_insert(fOutputBuffer, &text_start, load_string(content_iter), -1);
	gtk_list_store_clear(GTK_LIST_STORE(fEnvironmentStore));
	const char* key;
	while(key = load_string(content_iter), key[0]) {
		GtkTreeIter iter;
		gtk_list_store_append(fEnvironmentStore, &iter);
		const char* value = load_string(content_iter);
		gtk_list_store_set(fEnvironmentStore, &iter, 0, key, 1, value, -1);
	}
	g_free(contents);
	{
		char* absolute_name = get_absolute_path(name);
		set_file_modified(false);
		set_current_environment_name(absolute_name);
	}
	return(true);
}
void GTKREPL::load(void) {
	bool B_OK = false;
	//gtk_file_chooser_set_filename(dialog, );
	if(gtk_dialog_run(GTK_DIALOG(fOpenDialog)) == GTK_RESPONSE_OK) {
		char* file_name = gtk_file_chooser_get_filename(fOpenDialog);
		if(load_contents_from(file_name)) {
			B_OK = true;
		}
		g_free(file_name);
	}
	gtk_widget_hide(GTK_WIDGET(fOpenDialog));
	if(!B_OK) {
		g_warning("could not open file");
	}
}
bool GTKREPL::save(void) {
	bool B_OK = false;
	gtk_file_chooser_set_do_overwrite_confirmation(fSaveDialog, TRUE);
	//gtk_file_chooser_set_filename(dialog, );
	if(gtk_dialog_run(GTK_DIALOG(fSaveDialog)) == GTK_RESPONSE_OK) {
		char* file_name = gtk_file_chooser_get_filename(fSaveDialog);
		char* temp_name = g_strdup_printf("%sXXXXXX", file_name);
		int FD = mkstemp(temp_name);
		FILE* output_file = fdopen(FD, "w");
		if(save_content_to(output_file)) {
			fclose(output_file);
			close(FD);
			if(rename(temp_name, file_name) != -1) {
				char* absolute_name = get_absolute_path(file_name);
				B_OK = true;
				set_current_environment_name(absolute_name);
			}
			//unlink(temp_name);
		}
		g_free(temp_name);
		g_free(file_name);
	} else {
		gtk_widget_hide(GTK_WIDGET(fSaveDialog));
		return(false);
	}
	gtk_widget_hide(GTK_WIDGET(fSaveDialog));
	if(!B_OK) {
		g_warning("could not save file");
	}
	return(B_OK);
}
void GTKREPL::set_current_environment_name(const char* absolute_name) {
	gtk_window_set_title(fWidget, absolute_name);
	Config_set_environment_name(fConfig, absolute_name);
	Config_save(fConfig);
}
void GTKREPL::handle_response(gint response_id, GtkDialog* dialog) {
	if(response_id != GTK_RESPONSE_OK) {
		if(response_id == GTK_RESPONSE_ACCEPT)
			save();
		else if(response_id == GTK_RESPONSE_REJECT)
			load();
		return;
	}
	GtkTextIter beginning;
	GtkTextIter end;
	gchar* text;
	if(!gtk_text_buffer_get_selection_bounds(fOutputBuffer, &beginning, &end)) {
		gtk_text_buffer_get_start_iter(fOutputBuffer, &beginning);
		gtk_text_buffer_get_end_iter(fOutputBuffer, &end);
		text = strdup(gtk_entry_get_text(fCommandEntry));
		std::string v = text;
		v += "\n";
		gtk_text_buffer_insert(fOutputBuffer, &end, v.c_str(), -1);
	} else {
		text = gtk_text_buffer_get_text(fOutputBuffer, &beginning, &end, FALSE);
	}
	if(text && text[0]) {
		execute(text, &end);
	}
	g_free(text);
}
void GTKREPL::add_to_environment(AST::Node* definition) {
	using namespace AST;
	AST::Cons* definitionCons;
	GtkTreeIter iter;
	definitionCons = dynamic_cast<AST::Cons*>(definition);
	if(!definitionCons || !definitionCons->head || !definitionCons->tail || definitionCons->head != intern("define"))
		return;
	definitionCons = definitionCons->tail;
	AST::Symbol* procedureName = dynamic_cast<AST::Symbol*>(definitionCons->head);
	if(!procedureName || !definitionCons->tail)
		return;
	std::string procedureNameString = procedureName->str();
	//(apply (apply define x 2))
	std::string body = definitionCons->tail->str();
	gpointer hvalue;
	if(!g_hash_table_lookup_extended(fEnvironmentKeys, procedureName, NULL, &hvalue))
		gtk_list_store_append(fEnvironmentStore, &iter);
	else
		iter = * (GtkTreeIter*) hvalue;
	gtk_list_store_set(fEnvironmentStore, &iter, 0, procedureNameString.c_str(), 1, body.c_str(), -1);
	g_hash_table_replace(fEnvironmentKeys, procedureName, gtk_tree_iter_copy(&iter));
	set_file_modified(true);
}
bool GTKREPL::get_file_modified(void) const {
	return(fFileModified);
}
void GTKREPL::set_file_modified(bool value) {
	if(fFileModified == value)
		return;
	fFileModified = value;
	if(value) {
		const char* title = gtk_window_get_title(fWidget);
		char* new_title = g_strdup_printf("%s *", title);
		gtk_window_set_title(fWidget, new_title);
		g_free(new_title);
	}
}
bool GTKREPL::confirm_close(void) {
	if(get_file_modified()) {
		GtkDialog* dialog;
		dialog = (GtkDialog*) gtk_message_dialog_new(GTK_WINDOW(widget()), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, (GtkButtonsType) 0, "Environment has been modified. Save?");
		gtk_dialog_add_buttons(dialog, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
		{
			int result;
			result = gtk_dialog_run(dialog);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			if(result == GTK_RESPONSE_CLOSE)
				return(true);
			return(save());
		}
	}
	return(true);
}

};

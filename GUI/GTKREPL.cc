/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <netinet/in.h>
#include "GUI/GTKREPL"
#include "Scanners/MathParser"
#include "Config/Config"
#include "Formatters/LATEX"

namespace GUI {

struct GTKREPL {
	GtkWindow* fWidget;
	GtkBox* fMainBox;
	GtkTextView* fOutputArea;
	GtkScrolledWindow* fOutputScroller;
	GtkTextBuffer* fOutputBuffer;
	GtkEntry* fCommandEntry;
	//GtkButton* fExecuteButton;
	//GtkBox* fShortcutBox;
	GtkTreeView* fEnvironmentView;
	GtkListStore* fEnvironmentStore;
	GHashTable* fEnvironmentKeys;
	GtkBox* fEditorBox;
	GtkScrolledWindow* fEnvironmentScroller;
	GtkFileChooser* fSaveDialog;
	GtkFileChooser* fOpenDialog;
	struct Config* fConfig;
	GtkEntryCompletion* fCommandCompletion;
	bool fFileModified;
};
void GTKREPL_handle_response(struct GTKREPL* self, gint response_id, GtkDialog* dialog);
void GTKREPL_defer_LATEX(struct GTKREPL* self, const char* text);
void GTKREPL_queue_LATEX(struct GTKREPL* self, AST::Node* node);
void GTKREPL_add_to_environment(struct GTKREPL* self, AST::Node* definition);
void GTKREPL_set_current_environment_name(struct GTKREPL* self, const char* absolute_name);
void GTKREPL_set_file_modified(struct GTKREPL* self, bool value);
bool GTKREPL_save_content_to(struct GTKREPL* self, FILE* output_file);
void GTKREPL_execute(struct GTKREPL* self, const char* command, GtkTextIter* destination);
void GTKREPL_handle_LATEX_image(struct GTKREPL* self, GPid pid);

static gboolean handle_key_press(GtkWidget* widget, GdkEventKey* event, gpointer user_data) {
	if(((event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == GDK_CONTROL_MASK) && event->keyval == GDK_Return) {
		gtk_dialog_response(GTK_DIALOG(user_data), GTK_RESPONSE_OK);
		return(TRUE);
	}
	return(FALSE);
}
static gboolean g_confirm_close(GtkDialog* dialog, GdkEvent* event, GTKREPL* REPL) {
	if(GTKREPL_confirm_close(REPL))
		gtk_widget_hide(GTK_WIDGET(dialog));
	return(TRUE);
	//return(!REPL->confirm_close());
}
/*  the GdkWindow associated to the widget needs to enable the GDK_FOCUS_CHANGE_MASK mask. */
static gboolean g_clear_output_selection(GtkWidget* widget, GdkEventFocus* event, GtkTextBuffer* buffer) {
	if(gtk_text_buffer_get_has_selection(buffer)) {
		GtkTextIter iter;
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_select_range(buffer, &iter, &iter);
	}
	return(FALSE);
}
static gboolean accept_prefix(GtkEntryCompletion* completion, gchar* prefix, GtkEntry* entry) {
	return(FALSE);
}
static gboolean accept_match(GtkEntryCompletion* completion, GtkTreeModel* model, GtkTreeIter* iter, GtkEntry* entry) {
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
/*static gboolean g_hide_window(GtkWidget* widget, GdkEvent* event, gpointer user_data) {
	gtk_widget_hide(GTK_WIDGET(widget));
	return(TRUE);
}*/
void GTKREPL_init(struct GTKREPL* self, GtkWindow* parent) {
	self->fFileModified = false;
	self->fEnvironmentKeys = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) gtk_tree_iter_free);
	self->fWidget = (GtkWindow*) gtk_dialog_new_with_buttons("REPL", parent, (GtkDialogFlags) 0, GTK_STOCK_EXECUTE, GTK_RESPONSE_OK, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, GTK_STOCK_OPEN, GTK_RESPONSE_REJECT, NULL);
	self->fSaveDialog = (GtkFileChooser*) gtk_file_chooser_dialog_new("Save REPL", GTK_WINDOW(self->fWidget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
	self->fOpenDialog = (GtkFileChooser*) gtk_file_chooser_dialog_new("Open REPL", GTK_WINDOW(self->fWidget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(self->fWidget), GTK_RESPONSE_OK);
	self->fMainBox = (GtkBox*) gtk_vbox_new(FALSE, 7);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(self->fWidget)->vbox), GTK_WIDGET(self->fMainBox));
	gtk_widget_show(GTK_WIDGET(self->fMainBox));
	self->fEnvironmentView = (GtkTreeView*) gtk_tree_view_new();
	GtkTreeViewColumn* fNameColumn;
	fNameColumn = gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_append_column(self->fEnvironmentView, fNameColumn);
	self->fEnvironmentStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(self->fEnvironmentView, gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(self->fEnvironmentStore)));
	gtk_widget_show(GTK_WIDGET(self->fEnvironmentView));
	self->fEnvironmentScroller = (GtkScrolledWindow*) gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(self->fEnvironmentScroller, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(self->fEnvironmentScroller), GTK_WIDGET(self->fEnvironmentView));
	gtk_widget_show(GTK_WIDGET(self->fEnvironmentScroller));
	//self->fShortcutBox = (GtkBox*) gtk_hbutton_box_new();
	//self->fExecuteButton = (GtkButton*) gtk_button_new_from_stock(GTK_STOCK_OK);
	//gtk_widget_show(GTK_WIDGET(self->fExecuteButton));
	//gtk_box_pack_start(self->fShortcutBox, GTK_WIDGET(self->fExecuteButton), TRUE, TRUE, 7);
	//gtk_widget_show(GTK_WIDGET(self->fShortcutBox));
	self->fCommandEntry = (GtkEntry*) gtk_entry_new();
	self->fCommandCompletion = gtk_entry_completion_new();
	g_signal_connect(G_OBJECT(self->fCommandEntry), "key-press-event", G_CALLBACK(complete_command), self->fCommandCompletion);
	g_signal_connect(G_OBJECT(self->fCommandCompletion), "match-selected", G_CALLBACK(accept_match), self->fCommandEntry);
	g_signal_connect(G_OBJECT(self->fCommandCompletion), "insert-prefix", G_CALLBACK(accept_prefix), self->fCommandEntry);
	gtk_entry_completion_set_model(self->fCommandCompletion, GTK_TREE_MODEL(self->fEnvironmentStore));
	gtk_entry_completion_set_text_column(self->fCommandCompletion, 0);
	gtk_entry_completion_set_popup_completion(self->fCommandCompletion, FALSE);
	gtk_entry_completion_set_inline_completion(self->fCommandCompletion, TRUE); // XXX
	//gtk_entry_completion_set_inline_selection(self->fCommandCompletion, TRUE); // XXX
	gtk_entry_completion_set_popup_single_match(self->fCommandCompletion, FALSE);
	gtk_entry_set_completion(self->fCommandEntry, self->fCommandCompletion);
	gtk_entry_set_activates_default(self->fCommandEntry, TRUE);
	gtk_widget_show(GTK_WIDGET(self->fCommandEntry));
	self->fOutputArea = (GtkTextView*) gtk_text_view_new();
	self->fOutputScroller = (GtkScrolledWindow*) gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(self->fOutputScroller, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	// TODO set wrap mode
	gtk_text_view_set_accepts_tab(self->fOutputArea, FALSE);
	g_signal_connect(G_OBJECT(self->fOutputArea), "key-press-event", G_CALLBACK(handle_key_press), self->fWidget);
	gtk_widget_show(GTK_WIDGET(self->fOutputArea));
	gtk_container_add(GTK_CONTAINER(self->fOutputScroller), GTK_WIDGET(self->fOutputArea));
	gtk_widget_show(GTK_WIDGET(self->fOutputScroller));
	self->fOutputBuffer = gtk_text_view_get_buffer(self->fOutputArea);
	self->fEditorBox = (GtkBox*) gtk_hbox_new(FALSE, 7);
	g_signal_connect(G_OBJECT(self->fOutputArea), "focus-out-event", G_CALLBACK(g_clear_output_selection), self->fOutputBuffer);
	gtk_box_pack_start(GTK_BOX(self->fEditorBox), GTK_WIDGET(self->fEnvironmentScroller), FALSE, FALSE, 7); 
	gtk_box_pack_start(GTK_BOX(self->fEditorBox), GTK_WIDGET(self->fOutputScroller), TRUE, TRUE, 7); 
	gtk_widget_show(GTK_WIDGET(self->fEditorBox));
	//gtk_box_pack_start(GTK_BOX(self->fMainBox), GTK_WIDGET(self->fShortcutBox), FALSE, FALSE, 7); 
	gtk_box_pack_start(GTK_BOX(self->fMainBox), GTK_WIDGET(self->fEditorBox), TRUE, TRUE, 7); 
	gtk_box_pack_start(GTK_BOX(self->fMainBox), GTK_WIDGET(self->fCommandEntry), FALSE, FALSE, 7); 
	g_signal_connect_swapped(GTK_DIALOG(self->fWidget), "response", G_CALLBACK(GTKREPL_handle_response), (void*) self);
	gtk_window_set_focus(GTK_WINDOW(self->fWidget), GTK_WIDGET(self->fCommandEntry));
	gtk_tree_view_column_set_sort_column_id(fNameColumn, 0);
	gtk_tree_view_column_set_sort_indicator(fNameColumn, TRUE);
	//gtk_tree_view_column_set_sort_order(fNameColumn, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(self->fEnvironmentStore), 0, GTK_SORT_ASCENDING);
	self->fConfig = load_Config();
	{
		char* environment_name;
		environment_name = Config_get_environment_name(self->fConfig);
		if(environment_name && environment_name[0])
			GTKREPL_load_contents_from(self, environment_name);
	}
	g_signal_connect(G_OBJECT(self->fWidget), "delete-event", G_CALLBACK(g_confirm_close), self);
	//g_signal_connect(G_OBJECT(self->fWidget), "delete-event", G_CALLBACK(g_hide_window), NULL);
}
GtkWidget* GTKREPL_get_widget(struct GTKREPL* self) {
	return(GTK_WIDGET(self->fWidget));
}
void GTKREPL_queue_LATEX(struct GTKREPL* self, AST::Node* node) {
	std::stringstream result;
	Formatters::to_LATEX(node, result);
	std::string resultString = result.str();
	GTKREPL_defer_LATEX(self, resultString.c_str());
}
void GTKREPL_execute(struct GTKREPL* self, const char* command, GtkTextIter* destination) {
	Scanners::MathParser parser;
	FILE* input_file = fmemopen((void*) command, strlen(command), "r");
	if(input_file) {
		try {
			try {
				AST::Node* result = parser.parse(input_file);
				GTKREPL_queue_LATEX(self, result);
				GTKREPL_add_to_environment(self, result);
				std::string v = result ? result->str() : "OK";
				v = " => " + v + "\n";
				gtk_text_buffer_insert(self->fOutputBuffer, destination, v.c_str(), -1);
			} catch(...) {
				fclose(input_file);
				throw;
			}
		} catch(Scanners::ParseException e) {
			std::string v = e.what() ? e.what() : "error";
			v = " => " + v + "\n";
			gtk_text_buffer_insert(self->fOutputBuffer, destination, v.c_str(), -1);
		}
		GTKREPL_set_file_modified(self, true);
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
bool GTKREPL_save_content_to(struct GTKREPL* self, FILE* output_file) {
	GtkTreeIter iter;
	GtkTextIter start;
	GtkTextIter end;
	if(!save_string(output_file, "4DV1"))
		return(false);
	gtk_text_buffer_get_start_iter(self->fOutputBuffer, &start);
	gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
	char* text = gtk_text_buffer_get_text(self->fOutputBuffer, &start, &end, FALSE);
	if(!save_string(output_file, text))
		return(false);
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(self->fEnvironmentStore), &iter)) {
		while(1) {
			char* name;
			char* value;
			gtk_tree_model_get(GTK_TREE_MODEL(self->fEnvironmentStore), &iter, 0, &name, 1, &value, -1);
			if(!save_string(output_file, name))
				return(false);
			if(!save_string(output_file, value))
				return(false);
			//g_free(name);
			//g_free(value);
			if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(self->fEnvironmentStore), &iter))
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
bool GTKREPL_load_contents_from(struct GTKREPL* self, const char* name) {
	GError* error = NULL;
	gsize size;
	char* contents;
	const char* content_iter;
	GtkTextIter text_start;
	GtkTextIter text_end;
	if(GTKREPL_get_file_modified(self))
		if(!GTKREPL_confirm_close(self))
			return(false);
	gtk_text_buffer_get_start_iter(self->fOutputBuffer, &text_start);
	gtk_text_buffer_get_end_iter(self->fOutputBuffer, &text_end);
	gtk_text_buffer_delete(self->fOutputBuffer, &text_start, &text_end);
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
	gtk_text_buffer_insert(self->fOutputBuffer, &text_start, load_string(content_iter), -1);
	gtk_list_store_clear(GTK_LIST_STORE(self->fEnvironmentStore));
	const char* key;
	g_hash_table_remove_all(self->fEnvironmentKeys);
	while(key = load_string(content_iter), key[0]) {
		GtkTreeIter iter;
		gtk_list_store_append(self->fEnvironmentStore, &iter);
		const char* value = load_string(content_iter);
		gtk_list_store_set(self->fEnvironmentStore, &iter, 0, key, 1, value, -1);
		g_hash_table_replace(self->fEnvironmentKeys, AST::intern(key), gtk_tree_iter_copy(&iter));
	}
	g_free(contents);
	{
		char* absolute_name = get_absolute_path(name);
		GTKREPL_set_file_modified(self, false);
		GTKREPL_set_current_environment_name(self, absolute_name);
	}
	return(true);
}
void GTKREPL_load(struct GTKREPL* self) {
	bool B_OK = false;
	//gtk_file_chooser_set_filename(dialog, );
	if(gtk_dialog_run(GTK_DIALOG(self->fOpenDialog)) == GTK_RESPONSE_OK) {
		char* file_name = gtk_file_chooser_get_filename(self->fOpenDialog);
		if(file_name && GTKREPL_load_contents_from(self, file_name)) {
			B_OK = true;
		}
		g_free(file_name);
	} else { // user did not want to load file after all.
		B_OK = true;
	}
	gtk_widget_hide(GTK_WIDGET(self->fOpenDialog));
	if(!B_OK) {
		g_warning("could not open file");
	}
}
bool GTKREPL_save(struct GTKREPL* self) {
	bool B_OK = false;
	gtk_file_chooser_set_do_overwrite_confirmation(self->fSaveDialog, TRUE);
	//gtk_file_chooser_set_filename(dialog, );
	if(gtk_dialog_run(GTK_DIALOG(self->fSaveDialog)) == GTK_RESPONSE_OK) {
		char* file_name = gtk_file_chooser_get_filename(self->fSaveDialog);
		char* temp_name = g_strdup_printf("%sXXXXXX", file_name);
		int FD = mkstemp(temp_name);
		FILE* output_file = fdopen(FD, "w");
		if(GTKREPL_save_content_to(self, output_file)) {
			fclose(output_file);
			close(FD);
			if(rename(temp_name, file_name) != -1) {
				char* absolute_name = get_absolute_path(file_name);
				B_OK = true;
				GTKREPL_set_current_environment_name(self, absolute_name);
			}
			//unlink(temp_name);
		}
		g_free(temp_name);
		g_free(file_name);
	} else {
		gtk_widget_hide(GTK_WIDGET(self->fSaveDialog));
		return(false);
	}
	gtk_widget_hide(GTK_WIDGET(self->fSaveDialog));
	if(!B_OK) {
		g_warning("could not save file");
	}
	return(B_OK);
}
void GTKREPL_set_current_environment_name(struct GTKREPL* self, const char* absolute_name) {
	gtk_window_set_title(self->fWidget, absolute_name);
	Config_set_environment_name(self->fConfig, absolute_name);
	Config_save(self->fConfig);
}
void GTKREPL_handle_response(struct GTKREPL* self, gint response_id, GtkDialog* dialog) {
	if(response_id != GTK_RESPONSE_OK) {
		if(response_id == GTK_RESPONSE_ACCEPT)
			GTKREPL_save(self);
		else if(response_id == GTK_RESPONSE_REJECT)
			GTKREPL_load(self);
		return;
	}
	GtkTextIter beginning;
	GtkTextIter end;
	gchar* text;
	if(!gtk_text_buffer_get_selection_bounds(self->fOutputBuffer, &beginning, &end)) {
		gtk_text_buffer_get_start_iter(self->fOutputBuffer, &beginning);
		gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
		text = strdup(gtk_entry_get_text(self->fCommandEntry));
		std::string v = text;
		v += "\n";
		gtk_text_buffer_insert(self->fOutputBuffer, &end, v.c_str(), -1);
	} else {
		text = gtk_text_buffer_get_text(self->fOutputBuffer, &beginning, &end, FALSE);
	}
	if(text && text[0]) {
		GTKREPL_execute(self, text, &end);
	}
	g_free(text);
}
void GTKREPL_add_to_environment(struct GTKREPL* self, AST::Node* definition) {
	using namespace AST;
	AST::Cons* definitionCons;
	GtkTreeIter iter;
	if(!definition)
		return;
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
	if(!g_hash_table_lookup_extended(self->fEnvironmentKeys, procedureName, NULL, &hvalue))
		gtk_list_store_append(self->fEnvironmentStore, &iter);
	else
		iter = * (GtkTreeIter*) hvalue;
	gtk_list_store_set(self->fEnvironmentStore, &iter, 0, procedureNameString.c_str(), 1, body.c_str(), -1);
	g_hash_table_replace(self->fEnvironmentKeys, procedureName, gtk_tree_iter_copy(&iter));
	GTKREPL_set_file_modified(self, true);
}
bool GTKREPL_get_file_modified(struct GTKREPL* self) {
	return(self->fFileModified);
}
void GTKREPL_set_file_modified(struct GTKREPL* self, bool value) {
	if(self->fFileModified == value)
		return;
	self->fFileModified = value;
	if(value) {
		const char* title = gtk_window_get_title(self->fWidget);
		char* new_title = g_strdup_printf("%s *", title);
		gtk_window_set_title(self->fWidget, new_title);
		g_free(new_title);
	}
}
bool GTKREPL_confirm_close(struct GTKREPL* self) {
	if(GTKREPL_get_file_modified(self)) {
		GtkDialog* dialog;
		dialog = (GtkDialog*) gtk_message_dialog_new(GTK_WINDOW(GTKREPL_get_widget(self)), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, (GtkButtonsType) 0, "Environment has been modified. Save?");
		gtk_dialog_add_buttons(dialog, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
		{
			int result;
			result = gtk_dialog_run(dialog);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			if(result == GTK_RESPONSE_CLOSE)
				return(true);
			return(GTKREPL_save(self));
		}
	}
	return(true);
}
static void g_LATEX_child_died(GPid pid, int status, GTKREPL* REPL) {
	// FIXME status, non-death.
	GTKREPL_handle_LATEX_image(REPL, pid);
	g_spawn_close_pid(pid);
}
void GTKREPL_handle_LATEX_image(struct GTKREPL* self, GPid pid) {
	GdkPixbuf* pixbuf;
	pixbuf = gdk_pixbuf_new_from_file("eqn.png"/*FIXME*/, NULL);
	if(pixbuf) {
		GtkTextIter iter;
		gtk_text_buffer_get_end_iter(self->fOutputBuffer, &iter);
		gtk_text_buffer_insert_pixbuf(self->fOutputBuffer, &iter, pixbuf);
		g_object_unref(G_OBJECT(pixbuf));
	}
}
void GTKREPL_defer_LATEX(struct GTKREPL* self, const char* text) {
	GError* error;
	const char* argv[] = {
		"l2p",
		"-i",
		text,
		//"'$I<latex_expression>$'
		NULL,
	};
	GPid pid;
	if(g_spawn_async(NULL, (char**) argv, NULL, (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD|G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL), NULL, self/*user_data*/, &pid, &error)) {
		g_child_watch_add(pid, (GChildWatchFunc) g_LATEX_child_died, self);
	}
}
struct GTKREPL* GTKREPL_new(GtkWindow* parent) {
	struct GTKREPL* result;
	result = (struct GTKREPL*) calloc(1, sizeof(struct GTKREPL));
	GTKREPL_init(result, parent);
	return(result);
}

};

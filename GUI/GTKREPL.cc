/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <errno.h>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "GUI/GTKREPL"
#include "Scanners/MathParser"
#include "Config/Config"
#include "Formatters/LATEX"
#include "Formatters/SExpression"
#include "GUI/UI_definition.UI"
#include "GUI/GTKLATEXGenerator"
#include "Evaluators/FFI"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "FFIs/POSIX"
#include "GUI/Completer"

#define get_action(name) (GtkAction*) gtk_builder_get_object(self->UI_builder, ""#name)
#define add_action_handler(name) g_signal_connect_swapped(gtk_builder_get_object(self->UI_builder, ""#name), "activate", G_CALLBACK(REPL_handle_##name), self)
#define connect_accelerator(key, modifiers, action_name) REPL_connect_accelerator(self, key, modifiers, get_action(action_name), "<Actions>/actiongroup/"#action_name)

namespace GUI {

struct REPL {
	GtkWindow* fWidget;
	GtkBox* fMainBox;
	GtkTextView* fOutputArea;
	GtkScrolledWindow* fOutputScroller;
	GtkTextBuffer* fOutputBuffer;
	GtkEntry* fCommandEntry;
	GtkBox* fCommandBox;
	//GtkButton* fExecuteButton;
	//GtkBox* fShortcutBox;
	GtkTreeView* fEnvironmentView;
	GtkListStore* fEnvironmentStore;
	GHashTable* fEnvironmentKeys;
	GtkButton* fExecuteButton;
	GtkBox* fEditorBox;
	GtkScrolledWindow* fEnvironmentScroller;
	GtkFileChooser* fSaveDialog;
	GtkFileChooser* fOpenDialog;
	struct Config* fConfig;
	struct Completer* fCommandCompleter;
	bool fFileModified;
	GtkBuilder* UI_builder;
	GtkAccelGroup* accelerator_group;
	GTKLATEXGenerator* fLATEXGenerator;
	char* fSearchTerm;
	bool fBSearchUpwards;
	bool fBSearchCaseSensitive;
};
void REPL_add_to_environment(struct REPL* self, AST::Node* definition);
void REPL_add_to_environment_simple(struct REPL* self, AST::Symbol* name, AST::Node* value);
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name);
void REPL_set_file_modified(struct REPL* self, bool value);
bool REPL_save_content_to(struct REPL* self, FILE* output_file);
void REPL_execute(struct REPL* self, const char* command, GtkTextIter* destination);
bool REPL_load_contents_by_name(struct REPL* self, const char* file_name);

static void handle_clipboard_change(GtkClipboard* clipboard, GdkEvent* event, struct REPL* self) {
	gtk_action_set_sensitive(get_action(paste), gtk_clipboard_wait_is_text_available(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)));
}
static gboolean handle_key_press(GtkWidget* widget, GdkEventKey* event, gpointer user_data) {
	if(((event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == GDK_CONTROL_MASK) && event->keyval == GDK_Return) {
		gtk_dialog_response(GTK_DIALOG(user_data), GTK_RESPONSE_OK);
		return(TRUE);
	}
	return(FALSE);
}
static gboolean g_confirm_close(GtkWindow* dialog, GdkEvent* event, REPL* self) {
	if(REPL_confirm_close(self)) {
		{
			gint width;
			gint height;
			gtk_window_get_size(self->fWidget, &width, &height);
			Config_set_main_window_width(self->fConfig, width);
			Config_set_main_window_height(self->fConfig, height);
		}
		Config_save(self->fConfig);
		return(FALSE);
	}
	return(TRUE);
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
static void REPL_handle_execute(struct REPL* self, GtkAction* action) {
	GtkTextIter beginning;
	GtkTextIter end;
	gboolean B_from_entry = false;
	gchar* text;
	if(!gtk_text_buffer_get_selection_bounds(self->fOutputBuffer, &beginning, &end)) {
		gtk_text_buffer_get_start_iter(self->fOutputBuffer, &beginning);
		gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
		text = strdup(gtk_entry_get_text(self->fCommandEntry));
		/*std::string v = text;*/
		std::string v = "\n";
		gtk_text_buffer_insert(self->fOutputBuffer, &end, v.c_str(), -1);
		B_from_entry = true;
	} else {
		text = gtk_text_buffer_get_text(self->fOutputBuffer, &beginning, &end, FALSE);
	}
	if(text && text[0]) {
		REPL_execute(self, text, &end);
		if(B_from_entry)
			gtk_entry_set_text(self->fCommandEntry, "");
	}
	g_free(text);
}
static void REPL_handle_open_file(struct REPL* self, GtkAction* action) {
	REPL_load(self);
}
static void REPL_handle_save_file(struct REPL* self, GtkAction* action) {
	REPL_save(self);
}
static void REPL_handle_cut(struct REPL* self, GtkAction* action) {
	GtkWidget* control;
	control = gtk_window_get_focus(self->fWidget);
	if(control && g_signal_lookup("cut-clipboard", G_OBJECT_TYPE(control)))
		g_signal_emit_by_name(control, "cut-clipboard", NULL);
}
static void REPL_handle_copy(struct REPL* self, GtkAction* action) {
	GtkWidget* control;
	control = gtk_window_get_focus(self->fWidget);
	if(control && g_signal_lookup("copy-clipboard", G_OBJECT_TYPE(control)))
		g_signal_emit_by_name(control, "copy-clipboard", NULL);
}
static void REPL_handle_paste(struct REPL* self, GtkAction* action) {
	GtkWidget* control;
	control = gtk_window_get_focus(self->fWidget);
	if(control && g_signal_lookup("paste-clipboard", G_OBJECT_TYPE(control)))
		g_signal_emit_by_name(control, "paste-clipboard", NULL);
}

/* Gtk 2 compat; TODO: remove */
#define GTK_TEXT_SEARCH_CASE_INSENSITIVE ((GtkTextSearchFlags)(1<<2))

static void REPL_find_text(struct REPL* self, const char* text, gboolean upwards, gboolean case_sensitive) {
	gboolean B_found;
	GtkTextIter match_beginning, match_end;
	GtkTextMark* last_pos;
	last_pos = gtk_text_buffer_get_mark(self->fOutputBuffer, "insert");
	if(last_pos)
		gtk_text_buffer_get_iter_at_mark(self->fOutputBuffer, &match_beginning, last_pos);
	else {
		if(upwards)
			gtk_text_buffer_get_end_iter(self->fOutputBuffer, &match_beginning);
		else
			gtk_text_buffer_get_start_iter(self->fOutputBuffer, &match_beginning);
	}
	B_found = upwards ? gtk_text_iter_backward_search(&match_beginning, text, case_sensitive ? ((GtkTextSearchFlags) 0) : GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_beginning, &match_end, NULL /* TODO maybe just search selection if so requested. */)
	                  : gtk_text_iter_forward_search(&match_beginning, text, case_sensitive ? ((GtkTextSearchFlags) 0) : GTK_TEXT_SEARCH_CASE_INSENSITIVE, &match_beginning, &match_end, NULL /* TODO maybe just search selection if so requested. */);
	if(B_found) {
		/* TODO is there a less iffy way? */
/*		if(upwards)
			last_pos = gtk_text_buffer_create_mark(self->fOutputBuffer, "insert", &match_beginning, FALSE);
		else
			last_pos = gtk_text_buffer_create_mark(self->fOutputBuffer, "insert", &match_end, FALSE);
*/
		gtk_text_buffer_select_range(self->fOutputBuffer, &match_beginning, &match_end);
		gtk_text_view_scroll_mark_onscreen(self->fOutputArea, last_pos);
	}
}
static int REPL_show_search_dialog(struct REPL* self) {
	int response;
	GtkDialog* dialog;
	dialog = (GtkDialog*) gtk_builder_get_object(self->UI_builder, "searchDialog");
	gtk_window_set_transient_for(GTK_WINDOW(dialog), self->fWidget);
	/*g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(handle_search_response), NULL);*/
	{
		GtkEntry* searchTermEntry;
		GtkToggleButton* searchUpwardsButton;
		GtkToggleButton* searchCaseSensitiveButton;
		searchTermEntry = (GtkEntry*) gtk_builder_get_object(self->UI_builder, "searchTerm");
		searchUpwardsButton = (GtkToggleButton*) gtk_builder_get_object(self->UI_builder, "searchUpwardsButton");
		searchCaseSensitiveButton = (GtkToggleButton*) gtk_builder_get_object(self->UI_builder, "searchCaseSensitiveButton");
		if(self->fSearchTerm)
			gtk_entry_set_text(searchTermEntry, self->fSearchTerm);
		gtk_toggle_button_set_active(searchUpwardsButton, self->fBSearchUpwards);
		gtk_toggle_button_set_active(searchCaseSensitiveButton, self->fBSearchCaseSensitive);
	}
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_OK);
	response = gtk_dialog_run(dialog);
	gtk_widget_hide(GTK_WIDGET(dialog));
	return(response);
}
static void REPL_handle_find(struct REPL* self, GtkAction* action) {
	const char* text = NULL;
	if(REPL_show_search_dialog(self) == GTK_RESPONSE_OK) {
		GtkEntry* searchTermEntry;
		GtkToggleButton* searchUpwardsButton;
		GtkToggleButton* searchCaseSensitiveButton;
		searchTermEntry = (GtkEntry*) gtk_builder_get_object(self->UI_builder, "searchTerm");
		searchUpwardsButton = (GtkToggleButton*) gtk_builder_get_object(self->UI_builder, "searchUpwardsButton");
		searchCaseSensitiveButton = (GtkToggleButton*) gtk_builder_get_object(self->UI_builder, "searchCaseSensitiveButton");
		self->fBSearchUpwards = gtk_toggle_button_get_active(searchUpwardsButton);
		self->fBSearchCaseSensitive = gtk_toggle_button_get_active(searchCaseSensitiveButton);
		text = gtk_entry_get_text(searchTermEntry);
	}
	/*gtk_text_buffer_delete_mark_by_name(self->fOutputBuffer, "last_match");*/
	if(!text)
		return;
	if(self->fSearchTerm) {
		g_free(self->fSearchTerm);
		self->fSearchTerm = NULL;
	}
	self->fSearchTerm = strdup(text);
	REPL_find_text(self, text, self->fBSearchUpwards, self->fBSearchCaseSensitive);
}
static void REPL_handle_find_next(struct REPL* self, GtkAction* action) {
	REPL_find_text(self, self->fSearchTerm, self->fBSearchUpwards, self->fBSearchCaseSensitive);
}
static void handle_about_response(GtkDialog* dialog, gint response_id, gpointer user_data) {
	gtk_widget_hide(GTK_WIDGET(dialog));
}
static void REPL_handle_about(struct REPL* self, GtkAction* action) {
	GtkDialog* dialog;
	dialog = (GtkDialog*) gtk_builder_get_object(self->UI_builder, "aboutDialog");
	gtk_window_set_transient_for(GTK_WINDOW(dialog), self->fWidget);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(handle_about_response), NULL);
	gtk_dialog_run(dialog);
	gtk_widget_hide(GTK_WIDGET(dialog));
}
void REPL_connect_accelerator(struct REPL* self, int key, GdkModifierType modifiers, GtkAction* action, const char* path) {
	/*gtk_accel_group_connect(self->accelerator_group, key, modifiers, GTK_ACCEL_VISIBLE, NULL);*/
	gtk_action_set_accel_path(action, path);
	gtk_action_set_accel_group(action, self->accelerator_group);
	gtk_action_connect_accelerator(action);
	gtk_accel_map_add_entry(path, 0, (GdkModifierType) 0);
	gtk_accel_map_change_entry(path, key, modifiers, TRUE);
	/*"4D-REPL/File/Execute"*/
}
static void save_accelerators(struct REPL* self, GtkObject* widget) {
	const char* user_config_dir;
	user_config_dir = g_get_user_config_dir();
	if(!user_config_dir)
		abort();
	g_mkdir_with_parents(user_config_dir, 0744);
	g_mkdir_with_parents(g_build_filename(user_config_dir, "4D", NULL), 0744);
	gtk_accel_map_save(g_build_filename(user_config_dir, "4D", "accelerators", NULL));
}
static gboolean REPL_scroll_down(struct REPL* self) {
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(self->fOutputBuffer, &iter);
	gtk_text_view_scroll_to_iter(self->fOutputArea, &iter, 0.1, TRUE, 0.0, 1.0);
	//printf("DU\n");
	return(FALSE);
}
void REPL_queue_scroll_down(struct REPL* self) {
	// TODO check whether we were at the bottom before...
	g_idle_add((GSourceFunc) REPL_scroll_down, self);
}
static void track_changes(struct REPL* self, GtkTextBuffer* buffer) {
	if(!gtk_widget_is_focus(GTK_WIDGET(self->fOutputArea)) && !gtk_widget_is_focus(GTK_WIDGET(self->fOutputScroller)))
		REPL_queue_scroll_down(self);
	if(!self->fFileModified)
		REPL_set_file_modified(self, true);
}
static gboolean complete_command(GtkEntry* entry, GdkEventKey* key, struct Completer* completer) {
	if((key->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_MOD2_MASK|GDK_MOD3_MASK|GDK_MOD4_MASK|GDK_MOD5_MASK)) == 0 && key->keyval == GDK_Tab) {
		Completer_complete(completer);
		return(TRUE);
	}
	return(FALSE);
}
static FFIs::LibraryLoader libraryLoader;
static Evaluators::Quoter quoter;
static Evaluators::Conser conser;
static Evaluators::HeadGetter headGetter;
static Evaluators::TailGetter tailGetter;
static Evaluators::ConsP consP;
static Evaluators::SmallIntegerP smallIntegerP;
void REPL_add_builtins(struct REPL* self);
void REPL_init(struct REPL* self, GtkWindow* parent) {
	GtkUIManager* UI_manager;
	GError* error = NULL;
	GtkMenuBar* menu_bar;
	self->fBSearchUpwards = TRUE;
	self->fBSearchCaseSensitive = TRUE;
	self->UI_builder = gtk_builder_new();
	if(!gtk_builder_add_from_string(self->UI_builder, UI_definition, -1, &error)) {
		g_warning("UI error: %s", error->message);
		g_error_free(error);
		abort();
	}
	UI_manager = (GtkUIManager*) gtk_builder_get_object(self->UI_builder, "uiman");
	if(!UI_manager)
		abort();
	self->accelerator_group = gtk_ui_manager_get_accel_group(UI_manager);
	self->fFileModified = false;
	self->fEnvironmentKeys = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) gtk_tree_iter_free);
	self->fWidget = (GtkWindow*) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_add_accel_group(self->fWidget, self->accelerator_group);
	/*dialog_new_with_buttons("REPL", parent, (GtkDialogFlags) 0, GTK_STOCK_EXECUTE, GTK_RESPONSE_OK, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, GTK_STOCK_OPEN, GTK_RESPONSE_REJECT, NULL);*/
	self->fSaveDialog = (GtkFileChooser*) gtk_file_chooser_dialog_new("Save REPL", GTK_WINDOW(self->fWidget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
	self->fOpenDialog = (GtkFileChooser*) gtk_file_chooser_dialog_new("Open REPL", GTK_WINDOW(self->fWidget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
	self->fMainBox = (GtkBox*) gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(self->fWidget), GTK_WIDGET(self->fMainBox));
	gtk_widget_show(GTK_WIDGET(self->fMainBox));
	self->fEnvironmentView = (GtkTreeView*) gtk_tree_view_new();
	GtkTreeViewColumn* fNameColumn;
	fNameColumn = gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_append_column(self->fEnvironmentView, fNameColumn);
	self->fEnvironmentStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
	gtk_tree_view_set_model(self->fEnvironmentView, gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(self->fEnvironmentStore)));
	gtk_widget_show(GTK_WIDGET(self->fEnvironmentView));
	self->fEnvironmentScroller = (GtkScrolledWindow*) gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(self->fEnvironmentScroller, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(self->fEnvironmentScroller), GTK_WIDGET(self->fEnvironmentView));
	gtk_widget_show(GTK_WIDGET(self->fEnvironmentScroller));
	//self->fShortcutBox = (GtkBox*) gtk_hbutton_box_new();
	self->fExecuteButton = (GtkButton*) gtk_button_new_from_stock(GTK_STOCK_EXECUTE);
	gtk_action_connect_proxy(get_action(execute), GTK_WIDGET(self->fExecuteButton));
	gtk_button_set_use_stock(self->fExecuteButton, TRUE);
	GTK_WIDGET_SET_FLAGS(self->fExecuteButton, GTK_CAN_DEFAULT);
	gtk_window_set_default(self->fWidget, GTK_WIDGET(self->fExecuteButton));
	gtk_widget_show(GTK_WIDGET(self->fExecuteButton));
	//gtk_box_pack_start(self->fShortcutBox, GTK_WIDGET(self->fExecuteButton), TRUE, TRUE, 7);
	//gtk_widget_show(GTK_WIDGET(self->fShortcutBox));
	self->fCommandBox = (GtkBox*) gtk_hbox_new(FALSE, 7);
	gtk_widget_show(GTK_WIDGET(self->fCommandBox));
	self->fCommandEntry = (GtkEntry*) gtk_entry_new();
	self->fCommandCompleter = Completer_new(self->fCommandEntry, self->fEnvironmentKeys);
	g_signal_connect(G_OBJECT(self->fCommandEntry), "key-press-event", G_CALLBACK(complete_command), self->fCommandCompleter);
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
	g_signal_connect_swapped(G_OBJECT(self->fOutputBuffer), "changed", G_CALLBACK(track_changes), self);
	self->fEditorBox = (GtkBox*) gtk_hbox_new(FALSE, 0);
	g_signal_connect(G_OBJECT(self->fOutputArea), "focus-out-event", G_CALLBACK(g_clear_output_selection), self->fOutputBuffer);
	gtk_box_pack_start(GTK_BOX(self->fEditorBox), GTK_WIDGET(self->fEnvironmentScroller), FALSE, FALSE, 0); 
	gtk_box_pack_start(GTK_BOX(self->fEditorBox), GTK_WIDGET(self->fOutputScroller), TRUE, TRUE, 7); 
	gtk_widget_show(GTK_WIDGET(self->fEditorBox));
	menu_bar = (GtkMenuBar*) gtk_ui_manager_get_widget(UI_manager, "/menubar1");
	gtk_widget_show_all(GTK_WIDGET(menu_bar));
	gtk_box_pack_start(GTK_BOX(self->fMainBox), GTK_WIDGET(menu_bar), FALSE, FALSE, 0); 
	gtk_box_pack_start(GTK_BOX(self->fMainBox), GTK_WIDGET(self->fEditorBox), TRUE, TRUE, 0); 
	gtk_box_pack_start(GTK_BOX(self->fMainBox), GTK_WIDGET(self->fCommandBox), FALSE, FALSE, 0); 
	gtk_box_pack_start(GTK_BOX(self->fCommandBox), GTK_WIDGET(self->fCommandEntry), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(self->fCommandBox), GTK_WIDGET(self->fExecuteButton), FALSE, FALSE, 0);
	/*g_signal_connect_swapped(GTK_DIALOG(self->fWidget), "response", G_CALLBACK(REPL_handle_response), (void*) self);*/
	gtk_window_set_focus(GTK_WINDOW(self->fWidget), GTK_WIDGET(self->fCommandEntry));
	/*
	gtk_tree_view_column_set_sort_column_id(fNameColumn, 0);
	gtk_tree_view_column_set_sort_indicator(fNameColumn, TRUE);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(self->fEnvironmentStore), 0, GTK_SORT_ASCENDING);
	*/
	/* just to make sure */
	REPL_add_builtins(self);
	self->fConfig = load_Config();
	{
		gtk_window_resize(GTK_WINDOW(self->fWidget), Config_get_main_window_width(self->fConfig), Config_get_main_window_height(self->fConfig));
		char* environment_name;
		environment_name = Config_get_environment_name(self->fConfig);
		if(environment_name && environment_name[0]) {
			REPL_load_contents_by_name(self, environment_name);
		}
	}
	g_signal_connect(G_OBJECT(self->fWidget), "delete-event", G_CALLBACK(g_confirm_close), self);
	//g_signal_connect(G_OBJECT(self->fWidget), "delete-event", G_CALLBACK(g_hide_window), NULL);
	add_action_handler(execute);
	add_action_handler(open_file);
	add_action_handler(save_file);
	add_action_handler(cut);
	add_action_handler(copy);
	add_action_handler(paste);
	add_action_handler(find);
	add_action_handler(find_next);
	add_action_handler(about);
	connect_accelerator(GDK_F5, (GdkModifierType) 0, execute);
	connect_accelerator(GDK_F3, (GdkModifierType) 0, open_file);
	connect_accelerator(GDK_F2, (GdkModifierType) 0, save_file);
	connect_accelerator(GDK_x, GDK_CONTROL_MASK, cut);
	connect_accelerator(GDK_c, GDK_CONTROL_MASK, copy);
	connect_accelerator(GDK_v, GDK_CONTROL_MASK, paste);
	connect_accelerator(GDK_f, GDK_CONTROL_MASK, find);
	connect_accelerator(GDK_l, GDK_CONTROL_MASK, find_next);
	{
		const char* user_config_dir;
		int FD;
		user_config_dir = g_get_user_config_dir();
		FD = user_config_dir ? open(g_build_filename(user_config_dir, "4D", "accelerators", NULL), O_RDONLY, 0) : (-1);
		if(FD != -1) {
			gtk_accel_map_load_fd(FD);
			close(FD);
		}
		/* TODO g_get_system_config_dirs */
		g_signal_connect(G_OBJECT(self->fWidget), "destroy", G_CALLBACK(save_accelerators), self);
	}
	self->fLATEXGenerator = GTKLATEXGenerator_new();
	{ /* fix sensitivity of "paste" menu entry. */
		gtk_action_set_sensitive(get_action(paste), gtk_clipboard_wait_is_text_available(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)));
		g_signal_connect(G_OBJECT(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)), "owner-change", G_CALLBACK(handle_clipboard_change), self);
	}
}
void REPL_add_builtins(struct REPL* self) {
	REPL_add_to_environment_simple(self, AST::intern("quote"), &quoter); /* keep at the beginning */
	REPL_add_to_environment_simple(self, AST::intern("loadFromLibrary"), &libraryLoader);
	REPL_add_to_environment_simple(self, AST::intern("nil"), NULL);
	REPL_add_to_environment_simple(self, AST::intern("cons"), &conser);
	REPL_add_to_environment_simple(self, AST::intern("cons?"), &consP);
	REPL_add_to_environment_simple(self, AST::intern("head"), &headGetter);
	REPL_add_to_environment_simple(self, AST::intern("tail"), &tailGetter);
	REPL_add_to_environment_simple(self, AST::intern("smallInteger?"), &smallIntegerP);
}
void REPL_add_to_environment_simple(struct REPL* self, AST::Symbol* name, AST::Node* value) {
	REPL_add_to_environment(self, cons(AST::intern("define"), cons(name, cons(value, NULL))));
}
GtkWidget* REPL_get_widget(struct REPL* self) {
	return(GTK_WIDGET(self->fWidget));
}
static void REPL_enqueue_LATEX(struct REPL* self, AST::Node* node, GtkTextIter* destination) {
	std::stringstream result;
	result << "$ ";
	std::string resultString;
	const char* nodeText = NULL;
	try {
		Formatters::to_LATEX(node, result);
		result << " $";
		resultString = result.str();
		nodeText = resultString.c_str();
	} catch(std::runtime_error e) {
		nodeText = NULL;
	}
	//std::cout << resultString << " X" << std::endl;
	{
		char* alt_text;
		alt_text = strdup(node ? node->str().c_str() : "");
		if(alt_text && strchr(alt_text, '"')) /* contains string */
			nodeText = NULL;
		GTKLATEXGenerator_enqueue(self->fLATEXGenerator, nodeText ? strdup(nodeText) : NULL, alt_text, destination);
	}
}
static AST::Node* follow_tail(AST::Cons* list) {
	if(!list)
		return(NULL);
	while(list->tail)
		list = list->tail;
	return(list->head);
}
static AST::Node* close_environment(AST::Node* node, struct AST::Cons* entries) {
	if(!entries)
		return(node);
	else {
		AST::Cons* entry = dynamic_cast<AST::Cons*>(entries->head);
		return(Evaluators::close(dynamic_cast<AST::Symbol*>(entry->head), follow_tail(dynamic_cast<AST::Cons*>(entry->tail->head)), close_environment(node, entries->tail)));
	}
}
void REPL_execute(struct REPL* self, const char* command, GtkTextIter* destination) {
	Scanners::MathParser parser;
	FILE* input_file = fmemopen((void*) command, strlen(command), "r");
	if(input_file) {
		try {
			try {
				AST::Node* result = parser.parse(input_file);
				if(!result || dynamic_cast<AST::Cons*>(result) == NULL || ((AST::Cons*)result)->head != AST::intern("define")) {
					result = close_environment(result, REPL_get_environment(self));
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
				REPL_enqueue_LATEX(self, result, destination);
				REPL_add_to_environment(self, result);
			} catch(...) {
				fclose(input_file);
				throw;
			}
		} catch(Scanners::ParseException e) {
			std::string v = e.what() ? e.what() : "error";
			v = " => " + v + "\n";
			gtk_text_buffer_insert(self->fOutputBuffer, destination, v.c_str(), -1);
		} catch(Evaluators::EvaluationException e) {
			std::string v = e.what() ? e.what() : "error";
			v = " => " + v + "\n";
			gtk_text_buffer_insert(self->fOutputBuffer, destination, v.c_str(), -1);
		}
		REPL_set_file_modified(self, true);
	}
	REPL_queue_scroll_down(self);
}
AST::Cons* REPL_box_tree_nodes(struct REPL* self, GtkTreeIter* iter) {
	using namespace AST;
	char* name;
	AST::Node* value; /* FIXME test */
	Scanners::MathParser parser;
	//FILE* input_file;
	AST::Cons* tail;
	gtk_tree_model_get(GTK_TREE_MODEL(self->fEnvironmentStore), iter, 0, &name, 1, &value, -1);
	if(!gtk_tree_model_iter_next(GTK_TREE_MODEL(self->fEnvironmentStore), iter))
		tail = NULL;
	else
		tail = REPL_box_tree_nodes(self, iter);
	/*input_file = fmemopen(value, strlen(value), "r");
	if(!input_file)
		abort();
	return(cons(cons(AST::intern(name), cons(parser.parse_S_Expression(input_file), NULL)), tail));*/
	return(cons(cons(AST::intern(name), cons(value, NULL)), tail));
	//g_free(name);
	//g_free(value);
}
AST::Cons* REPL_get_environment(struct REPL* self) {
	GtkTreeIter iter;
	return((gtk_tree_model_get_iter_first(GTK_TREE_MODEL(self->fEnvironmentStore), &iter)) ? REPL_box_tree_nodes(self, &iter) : NULL);
}
void REPL_clear(struct REPL* self) {
	GtkTextIter text_start;
	GtkTextIter text_end;
	gtk_text_buffer_get_start_iter(self->fOutputBuffer, &text_start);
	gtk_text_buffer_get_end_iter(self->fOutputBuffer, &text_end);
	gtk_text_buffer_delete(self->fOutputBuffer, &text_start, &text_end);
	gtk_list_store_clear(GTK_LIST_STORE(self->fEnvironmentStore));
	g_hash_table_remove_all(self->fEnvironmentKeys);
	REPL_add_builtins(self);
}
char* REPL_get_absolute_path(const char* name) {
	if(g_path_is_absolute(name))
		return(strdup(name));
	else
		return(g_build_filename(g_get_current_dir(), name, NULL));
}
void REPL_append_to_output_buffer(struct REPL* self, const char* text) {
	GtkTextIter text_start;
	gtk_text_buffer_get_start_iter(self->fOutputBuffer, &text_start);
	gtk_text_buffer_insert(self->fOutputBuffer, &text_start, text, -1);
}
char* REPL_get_output_buffer_text(struct REPL* self) {
	GtkTextIter start;
	GtkTextIter end;
	gtk_text_buffer_get_start_iter(self->fOutputBuffer, &start);
	gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
	return(gtk_text_buffer_get_text(self->fOutputBuffer, &start, &end, FALSE));
}
bool REPL_load_contents_by_name(struct REPL* self, const char* file_name) {
	if(!REPL_load_contents_from(self, file_name))
		return(false);
	else {
		char* absolute_name = REPL_get_absolute_path(file_name);
		REPL_set_file_modified(self, false);
		REPL_set_current_environment_name(self, absolute_name);
		return(true);
	}
}
void REPL_load(struct REPL* self) {
	bool B_OK = false;
	{
		if(REPL_get_file_modified(self))
			if(!REPL_confirm_close(self))
				return;
	}
	//gtk_file_chooser_set_filename(dialog, );
	if(gtk_dialog_run(GTK_DIALOG(self->fOpenDialog)) == GTK_RESPONSE_OK) {
		char* file_name = gtk_file_chooser_get_filename(self->fOpenDialog);
		if(file_name && REPL_load_contents_by_name(self, file_name))
			B_OK = true;
		g_free(file_name);
	} else { // user did not want to load file after all.
		B_OK = true;
	}
	gtk_widget_hide(GTK_WIDGET(self->fOpenDialog));
	if(!B_OK) {
		g_warning("could not open file");
	}
}
bool REPL_save(struct REPL* self) {
	bool B_OK = false;
	gtk_file_chooser_set_do_overwrite_confirmation(self->fSaveDialog, TRUE);
	//gtk_file_chooser_set_filename(dialog, );
	if(gtk_dialog_run(GTK_DIALOG(self->fSaveDialog)) == GTK_RESPONSE_OK) {
		char* file_name = gtk_file_chooser_get_filename(self->fSaveDialog);
		char* temp_name = g_strdup_printf("%sXXXXXX", file_name);
		int FD = mkstemp(temp_name);
		FILE* output_file = fdopen(FD, "w");
		if(REPL_save_contents_to(self, output_file)) {
			fclose(output_file);
			close(FD);
			if(rename(temp_name, file_name) != -1) {
				char* absolute_name = REPL_get_absolute_path(file_name);
				B_OK = true;
				REPL_set_current_environment_name(self, absolute_name);
				REPL_set_file_modified(self, false);
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
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name) {
	gtk_window_set_title(self->fWidget, absolute_name);
	Config_set_environment_name(self->fConfig, absolute_name);
	Config_save(self->fConfig);
}
void REPL_add_to_environment(struct REPL* self, AST::Node* definition) {
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
	AST::Node* value = definitionCons->tail;
	//std::string body = definitionCons->tail->str();
	gpointer hvalue;
	if(!g_hash_table_lookup_extended(self->fEnvironmentKeys, procedureName, NULL, &hvalue))
		gtk_list_store_append(self->fEnvironmentStore, &iter);
	else
		iter = * (GtkTreeIter*) hvalue;
	gtk_list_store_set(self->fEnvironmentStore, &iter, 0, procedureNameString.c_str(), 1, value, -1);
	g_hash_table_replace(self->fEnvironmentKeys, procedureName, gtk_tree_iter_copy(&iter));
	REPL_set_file_modified(self, true);
}
bool REPL_get_file_modified(struct REPL* self) {
	return(self->fFileModified);
}
void REPL_set_file_modified(struct REPL* self, bool value) {
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
bool REPL_confirm_close(struct REPL* self) {
	if(REPL_get_file_modified(self)) {
		GtkDialog* dialog;
		dialog = (GtkDialog*) gtk_message_dialog_new(GTK_WINDOW(REPL_get_widget(self)), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, (GtkButtonsType) 0, "Environment has been modified. Save?");
		gtk_dialog_add_buttons(dialog, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
		{
			int result;
			result = gtk_dialog_run(dialog);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			if(result == GTK_RESPONSE_CLOSE)
				return(true);
			return(REPL_save(self));
		}
	}
	return(true);
}
struct REPL* REPL_new(GtkWindow* parent) {
	struct REPL* result;
	result = (struct REPL*) calloc(1, sizeof(struct REPL));
	REPL_init(result, parent);
	return(result);
}

};

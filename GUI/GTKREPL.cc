/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "GUI/GTKREPL"
#include "Scanners/MathParser"
#include "Config/Config"
#include "Formatters/LATEX"
#include "Formatters/SExpression"
#include "Formatters/Math"
#include "GUI/UI_definition.UI"
#include "GUI/GTKLATEXGenerator"
#include "Evaluators/FFI"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "FFIs/FFIs"
#include "GUI/GTKCompleter"
#include "FFIs/ResultMarshaller"
#include "GUI/WindowIcon"
#include "Scanners/OperatorPrecedenceList"

#define get_action(name) (GtkAction*) gtk_builder_get_object(self->UI_builder, ""#name)
#define add_action_handler(name) g_signal_connect_swapped(gtk_builder_get_object(self->UI_builder, ""#name), "activate", G_CALLBACK(REPL_handle_##name), self)
#define connect_accelerator(key, modifiers, action_name) REPL_connect_accelerator(self, key, modifiers, get_action(action_name), "<Actions>/actiongroup/"#action_name)

namespace REPLX {

struct Completer;
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
	GtkListStore* fEnvironmentStore2;
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
	GUI::GTKLATEXGenerator* fLATEXGenerator;
	char* fSearchTerm;
	bool fBSearchUpwards;
	bool fBSearchCaseSensitive;
	AST::Node* fTailEnvironment;
	AST::Node* fTailUserEnvironment /* =fTailBuiltinEnvironmentFrontier */;
	AST::Node* fTailUserEnvironmentFrontier;
};
};
namespace GUI {
int REPL_add_to_environment_simple_GUI(struct REPL* self, AST::Symbol* name, AST::Node* value);
void REPL_insert_into_output_buffer(struct REPL* self, GtkTextIter* destination, const char* text);
void REPL_set_file_modified(struct REPL* self, bool value);
void REPL_queue_scroll_down(struct REPL* self);
};
using namespace GUI;
#include "REPL/REPLEnvironment"
namespace GUI {
using namespace Evaluators;
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name);
bool REPL_save_content_to(struct REPL* self, FILE* output_file);
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
char* REPL_get_output_text(struct REPL* self, GtkTextIter* beginning, GtkTextIter* end) {
	/* complicated version of:
	return(gtk_text_buffer_get_text(self->fOutputBuffer, beginning, end, FALSE));
	*/
	gchar* result;
	gchar* text;
	gchar* orig_text;
	gchar* image_match;
	gchar* next_text;
	gchar* temp_result;
	int jitter = 0;
	result = g_strdup("");
	text = gtk_text_buffer_get_slice(self->fOutputBuffer, beginning, end, FALSE);
	orig_text = text;
	/* 0xFFFC = \xef\xbf\xbc */
	while(*text) {
		image_match = strstr(text, "\xef\xbf\xbc");
		if(!image_match) {
			image_match = text + strlen(text);
			next_text = image_match;
		} else {
			next_text = image_match + 3;
			*image_match = 0;
		}
		temp_result = g_strdup_printf("%s%s", result, text); 
		g_free(result);
		result = temp_result;
		// actually handle the image match:
		{
			int offset;
			GdkPixbuf* pixbuf;
			GtkTextIter iter;
			offset = g_utf8_pointer_to_offset(orig_text, image_match);
			//g_warning("offset %d", offset);
			gtk_text_buffer_get_iter_at_offset(self->fOutputBuffer, &iter, gtk_text_iter_get_offset(beginning) + offset - jitter);
			jitter += 2; /* FIXME */
			pixbuf = gtk_text_iter_get_pixbuf(&iter);
			if(pixbuf) {
				const char* alt_text;
				alt_text = gdk_pixbuf_get_option(pixbuf, "alt_text");
				if(alt_text) {
					temp_result = g_strdup_printf("%s\xef\xbf\xbc%s", result, alt_text); /* leave the marker in so we can regenerate the images when loading. */
					g_free(result);
					result = temp_result;
				}
			} else {
				if(image_match != text + strlen(text))
					g_warning("oops... could not find LATEX image in text buffer");
			}
		}
		text = next_text;
	}
	/*g_utf8_pointer_to_offset(str, str+byte_index)*/
	return(result);
}
static void REPL_enqueue_LATEX(struct REPL* self, AST::Node* node, GtkTextIter* destination);
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
void REPL_insert_error_message(struct REPL* self, GtkTextIter* destination, const std::string& prefix, const std::string& errorText) {
	std::string v = prefix + " => " + errorText; // + "\n";
	REPL_insert_into_output_buffer(self, destination, v.c_str());
	//gtk_text_buffer_insert(self->fOutputBuffer, destination, v.c_str(), -1);
	REPL_set_file_modified(self, true);
	REPL_queue_scroll_down(self);
}
static void REPL_handle_execute(struct REPL* self, GtkAction* action) {
	GtkTextIter beginning;
	GtkTextIter end;
	gboolean B_from_entry = false;
	gchar* text;
	AST::Node* input;
	if(!gtk_text_buffer_get_selection_bounds(self->fOutputBuffer, &beginning, &end)) {
		gtk_text_buffer_get_start_iter(self->fOutputBuffer, &beginning);
		gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
		text = strdup(gtk_entry_get_text(self->fCommandEntry));
		/*std::string v = text;*/
		B_from_entry = true;
	} else {
		text = REPL_get_output_text(self, &beginning, &end);
	}
	gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
	if(info_P(text)) {
		REPL_insert_into_output_buffer(self, &end, g_strdup_printf("\n%s", text));
		gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
		AST::Node* body = REPL_eval_info(self, text);
		g_free(text);
		REPL_enqueue_LATEX(self, body, &end);
		return;
	}
	try {
		input = REPL_parse(self, text, &end);
	} catch(Scanners::ParseException& e) {
		std::string v = e.what() ? e.what() : "error";
		REPL_insert_error_message(self, &end, B_from_entry ? (std::string("\n") + text) : std::string(), v);
		input = NULL;
	}
	g_free(text);
	if(input) {
		//printf("%s\n", input->str().c_str());
		if(B_from_entry) {
			std::string v = "\n";
			gtk_text_buffer_insert(self->fOutputBuffer, &end, v.c_str(), -1);
			gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
			REPL_enqueue_LATEX(self, input, &end);
		}
		gtk_text_buffer_insert(self->fOutputBuffer, &end, "=>", -1);
		bool B_ok = REPL_execute(self, input, &end);
		if(B_from_entry && B_ok)
			gtk_entry_set_text(self->fCommandEntry, "");
	}
}
static void REPL_handle_environment_row_activation(struct REPL* self, GtkTreePath* path, GtkTreeViewColumn* column, GtkTreeView* view) {
	using namespace AST;
	char* command;
	bool B_ok = false;
	GtkTreeModel* model;
	GtkTextIter end;
	GtkTreeIter iter;
	std::string escapedCommand;
	model = gtk_tree_view_get_model(view);
	if(gtk_tree_model_get_iter(model, &iter, path)) {
		command = NULL;
		gtk_tree_model_get(model, &iter, 0, &command, -1);
		if(!command)
			return;
		gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
		AST::Node* body = REPL_get_definition(self, *gtk_tree_path_get_indices(path));
		REPL_enqueue_LATEX(self, body, &end);
		B_ok = true;
	}
	if(B_ok)
		gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(self->fEnvironmentView));
}
static void REPL_environment_show_popup_menu(struct REPL* self, GdkEventButton* event) {
	GtkMenu* menu;
	GtkUIManager* UI_manager;
	UI_manager = (GtkUIManager*) gtk_builder_get_object(self->UI_builder, "uiman");
	if(!UI_manager)
		return;
	menu = (GtkMenu*) gtk_ui_manager_get_widget(UI_manager, "/environmentMenu");
	/*gtk_widget_show_all(GTK_WIDGET(menu));*/
	gtk_menu_popup(menu, NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*) event));
}
static gboolean REPL_handle_environment_key_press(struct REPL* self, GdkEventKey* event, GtkWidget* view) {
	if(event->keyval == GDK_Delete && (event->state & GDK_MODIFIER_MASK) == 0) {
		gtk_action_activate(get_action(delete_environment_item));
		return(TRUE);
	}
	return(FALSE);
}
static gboolean REPL_handle_environment_button_press(struct REPL* self, GdkEventButton* event, GtkWidget* view) {
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		{
			GtkTreeSelection *selection;
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->fEnvironmentView));
			if(gtk_tree_selection_count_selected_rows(selection)  <= 1) {
				GtkTreePath* path;
				if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(self->fEnvironmentView), (gint) event->x, (gint) event->y, &path, NULL, NULL, NULL)) {
					gtk_tree_selection_unselect_all(selection);
					gtk_tree_selection_select_path(selection, path);
					gtk_tree_path_free(path);
				}
			}
		}
		REPL_environment_show_popup_menu(self, event);
	}
	return(FALSE);
}
static gboolean REPL_handle_environment_popup_menu(struct REPL* self, GtkWidget* view) {
	REPL_environment_show_popup_menu(self, NULL);
	return(TRUE);
}
static void REPL_handle_open_file(struct REPL* self, GtkAction* action) {
	REPL_load(self);
}
static void REPL_handle_save_file(struct REPL* self, GtkAction* action) {
	REPL_save(self, FALSE);
}
static void REPL_handle_save_file_as(struct REPL* self, GtkAction* action) {
	REPL_save(self, TRUE);
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
static void REPL_handle_print_environment_item(struct REPL* self, GtkAction* action) {
	GtkTreePath* path;
	GtkTreeIter iter;
	GtkTreeSelection* selection;
	selection = gtk_tree_view_get_selection(self->fEnvironmentView);
	if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(self->fEnvironmentStore2), &iter);
		gtk_tree_view_row_activated(self->fEnvironmentView, path, NULL);
		gtk_tree_path_free(path);
	}
}
static void REPL_run_deletion_dialog(struct REPL* self) {
	GtkDialog* dialog;
	int response;
	dialog = (GtkDialog*) gtk_builder_get_object(self->UI_builder, "envitemDeletionConfirmationDialog");
	gtk_window_set_transient_for(GTK_WINDOW(dialog), self->fWidget);
	/*g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(handle_search_response), NULL);*/
	gtk_dialog_set_default_response(dialog, GTK_RESPONSE_OK);
	response = gtk_dialog_run(dialog);
	gtk_widget_hide(GTK_WIDGET(dialog));
}
static void REPL_handle_delete_environment_item(struct REPL* self, GtkAction* action) {
	/* FIXME check multiple rows */
	GtkTreeIter iter;
	GtkTreeSelection* selection;
	selection = gtk_tree_view_get_selection(self->fEnvironmentView);
	if(gtk_tree_selection_get_selected(selection, NULL, &iter)) {
		REPL_run_deletion_dialog(self);
	}
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
	if(self->fSearchTerm)
		REPL_find_text(self, self->fSearchTerm, self->fBSearchUpwards, self->fBSearchCaseSensitive);
}
static void handle_about_response(GtkDialog* dialog, gint response_id, gpointer user_data) {
	gtk_widget_hide(GTK_WIDGET(dialog));
}
static AST::Cons* tips;
static int current_tip_index = -1;
static AST::Cons* current_tip = NULL;
void REPL_load_tips(struct REPL* self) {
	FILE* input_file;
	input_file = fopen("doc/tips", "r"); /* FIXME full path */
	if(input_file) {
		Scanners::MathParser parser;
		AST::Node* contents;
		parser.push(input_file, 0, false);
		parser.consume();
		contents = parser.parse_S_Expression();
		fclose(input_file);
		AST::Cons* contentsCons = dynamic_cast<AST::Cons*>(contents);
		if(contentsCons && contentsCons->head == AST::intern("tips5DV1"))
			tips = dynamic_cast<AST::Cons*>(contentsCons->tail);
		else
			tips = NULL;
	}
}
static void handle_tips_response(GtkDialog* dialog, gint response_id, struct REPL* self) {
	GtkToggleButton* checkButton = (GtkToggleButton*) gtk_builder_get_object(self->UI_builder, "tipsShowAgainButton");
	GtkTextView* view = (GtkTextView*) gtk_builder_get_object(self->UI_builder, "tipView");
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(view);
	if(response_id == 2/*NEXT*/) {
		if(!current_tip)
			current_tip = tips;
		else {
			if(current_tip->tail)
				++current_tip_index;
			current_tip = Evaluators::evaluateToCons(current_tip->tail) ? Evaluators::evaluateToCons(current_tip->tail) : current_tip;
		}
	} else if(response_id == 1/*PREV*/) {
		if(current_tip) {
			AST::Cons* previous_tip = NULL;
			for(AST::Cons* n = tips; n != current_tip; n = Evaluators::evaluateToCons(n->tail))
				previous_tip = n;
			if(previous_tip)
				--current_tip_index;
			current_tip = previous_tip ? previous_tip : tips;
		}
	} else {
		gtk_widget_hide(GTK_WIDGET(dialog));
	}
	{
		Config_set_show_tips(self->fConfig, gtk_toggle_button_get_active(checkButton));
		Config_set_current_tip(self->fConfig, current_tip_index);
		Config_save(self->fConfig); /* for the paranoid */
	}
	{
		char* text;
		text = Evaluators::get_native_string(current_tip->head);
		if(!text)
			text = g_strdup("You can complete text by pressing the Tabulator (Tab) key on your keyboard");
		gtk_text_buffer_set_text(buffer, text, -1);
	}
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
	/*"5D-REPL/File/Execute"*/
}
static void save_accelerators(struct REPL* self, GtkObject* widget) {
	const char* user_config_dir;
	user_config_dir = g_get_user_config_dir();
	if(!user_config_dir)
		abort();
	g_mkdir_with_parents(user_config_dir, 0744);
	g_mkdir_with_parents(g_build_filename(user_config_dir, "5D", NULL), 0744);
	gtk_accel_map_save(g_build_filename(user_config_dir, "5D", "accelerators", NULL));
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
static void REPL_show_tips(struct REPL* self) {
	gtk_widget_realize(GTK_WIDGET(self->fWidget)); /* workaround gtk critical icon_pixmap */
	GtkDialog* dialog;
	dialog = (GtkDialog*) gtk_builder_get_object(self->UI_builder, "tipDialog");
	REPL_load_tips(self);
	gtk_dialog_set_default_response(dialog, 0);
	{
		GtkToggleButton* checkButton = (GtkToggleButton*) gtk_builder_get_object(self->UI_builder, "tipsShowAgainButton");
		gtk_toggle_button_set_active(checkButton, Config_get_show_tips(self->fConfig));
	}
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(handle_tips_response), self);
	/*g_signal_connect(G_OBJECT(dialog), "delete-event", G_CALLBACK(handle_tips_delete_event), NULL);*/
	gtk_window_set_transient_for(GTK_WINDOW(dialog), self->fWidget);
	{
		current_tip_index = Config_get_current_tip(self->fConfig);
		current_tip = current_tip_index > 0 ? tips : NULL;
		for(int i = 0; i < current_tip_index - 1 && current_tip; ++i)
			current_tip = Evaluators::evaluateToCons(current_tip->tail);
		--current_tip_index;
	}
	handle_tips_response(dialog, 2/*NEXT*/, self);
	gtk_widget_show(GTK_WIDGET(dialog));
}
static gboolean REPL_discover_output_pixbuf(struct REPL* self, GdkEventButton* event, GtkTextView* view) {
	GtkTextIter iter;
	GdkPixbuf* pixbuf;
	gint x = 0;
	gint y = 0;
	gtk_text_view_window_to_buffer_coords(view, GTK_TEXT_WINDOW_WIDGET, event->x, event->y, &x, &y);
	gtk_text_view_get_iter_at_position(view, &iter, NULL, x, y);
	pixbuf = gtk_text_iter_get_pixbuf(&iter);
	if(pixbuf) {
		const char* alt_text;
		alt_text = gdk_pixbuf_get_option(pixbuf, "alt_text");
		if(alt_text) {
			gtk_entry_set_text(self->fCommandEntry, alt_text);
		}
	}
	return(FALSE);
}
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
        gtk_window_set_title(self->fWidget, "(5D)");
	gtk_window_set_icon(self->fWidget, gdk_pixbuf_new_from_inline(-1, window_icon, FALSE, NULL));
	gtk_window_add_accel_group(self->fWidget, self->accelerator_group);
	/*dialog_new_with_buttons("REPL", parent, (GtkDialogFlags) 0, GTK_STOCK_EXECUTE, GTK_RESPONSE_OK, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, GTK_STOCK_OPEN, GTK_RESPONSE_REJECT, NULL);*/
	self->fSaveDialog = (GtkFileChooser*) gtk_file_chooser_dialog_new("Save REPL", GTK_WINDOW(self->fWidget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
	self->fOpenDialog = (GtkFileChooser*) gtk_file_chooser_dialog_new("Open REPL", GTK_WINDOW(self->fWidget), GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,  GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
	self->fMainBox = (GtkBox*) gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(self->fWidget), GTK_WIDGET(self->fMainBox));
	gtk_widget_show(GTK_WIDGET(self->fMainBox));
	self->fEnvironmentView = (GtkTreeView*) gtk_tree_view_new();
	g_signal_connect_swapped(G_OBJECT(self->fEnvironmentView), "row-activated", G_CALLBACK(REPL_handle_environment_row_activation), self);
	g_signal_connect_swapped(G_OBJECT(self->fEnvironmentView), "popup-menu", G_CALLBACK(REPL_handle_environment_popup_menu), self);
	g_signal_connect_swapped(G_OBJECT(self->fEnvironmentView), "button-press-event", G_CALLBACK(REPL_handle_environment_button_press), self);
	g_signal_connect_swapped(G_OBJECT(self->fEnvironmentView), "key-release-event", G_CALLBACK(REPL_handle_environment_key_press), self);
	GtkTreeViewColumn* fNameColumn;
	fNameColumn = gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), "text", 0, NULL);
	gtk_tree_view_append_column(self->fEnvironmentView, fNameColumn);
	self->fEnvironmentStore2 = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_tree_view_set_model(self->fEnvironmentView, (GtkTreeModel*) self->fEnvironmentStore2);
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
	g_signal_connect_swapped(G_OBJECT(self->fOutputArea), "button-press-event", G_CALLBACK(REPL_discover_output_pixbuf), self);
	gtk_text_view_set_pixels_below_lines(self->fOutputArea, 7);
	gtk_text_view_set_wrap_mode(self->fOutputArea, GTK_WRAP_WORD_CHAR);
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
	/*gtk_tree_view_column_set_sort_column_id(fNameColumn, 0);
	gtk_tree_view_column_set_sort_indicator(fNameColumn, TRUE);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(self->fEnvironmentStore2), 0, GTK_SORT_ASCENDING);*/
	self->fConfig = load_Config();
	REPL_init_builtins(self);
	self->fLATEXGenerator = GTKLATEXGenerator_new();
	{
		gtk_window_resize(GTK_WINDOW(self->fWidget), Config_get_main_window_width(self->fConfig), Config_get_main_window_height(self->fConfig));
		char* environment_name;
		environment_name = Config_get_environment_name(self->fConfig);
		if(environment_name && environment_name[0]) {
			REPL_load_contents_by_name(self, environment_name);
		} else
			REPL_set_file_modified(self, false);
	}
	g_signal_connect(G_OBJECT(self->fWidget), "delete-event", G_CALLBACK(g_confirm_close), self);
	//g_signal_connect(G_OBJECT(self->fWidget), "delete-event", G_CALLBACK(g_hide_window), NULL);
	add_action_handler(execute);
	add_action_handler(open_file);
	add_action_handler(save_file);
	add_action_handler(save_file_as);
	add_action_handler(cut);
	add_action_handler(copy);
	add_action_handler(paste);
	add_action_handler(find);
	add_action_handler(find_next);
	add_action_handler(about);
	add_action_handler(delete_environment_item);
	add_action_handler(print_environment_item);
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
		FD = user_config_dir ? open(g_build_filename(user_config_dir, "5D", "accelerators", NULL), O_RDONLY, 0) : (-1);
		if(FD != -1) {
			gtk_accel_map_load_fd(FD);
			close(FD);
		}
		/* TODO g_get_system_config_dirs */
		g_signal_connect(G_OBJECT(self->fWidget), "destroy", G_CALLBACK(save_accelerators), self);
	}
	{ /* fix sensitivity of "paste" menu entry. */
		gtk_action_set_sensitive(get_action(paste), gtk_clipboard_wait_is_text_available(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)));
		g_signal_connect(G_OBJECT(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)), "owner-change", G_CALLBACK(handle_clipboard_change), self);
	}
	if(Config_get_show_tips(self->fConfig))
		REPL_show_tips(self);
}
int REPL_add_to_environment_simple_GUI(struct REPL* self, AST::Symbol* name, AST::Node* value) {
	GtkTreeIter iter;
	GtkTreePath* path = NULL;
	g_hash_table_replace(self->fEnvironmentKeys, name, gtk_tree_iter_copy(&iter));
	gtk_tree_view_get_cursor(self->fEnvironmentView, &path, NULL);
	if(path) {
		gint* indices;
		indices = gtk_tree_path_get_indices(path);
		gtk_list_store_insert(self->fEnvironmentStore2, &iter, indices[0]);
	} else
		gtk_list_store_append(self->fEnvironmentStore2, &iter);
	gtk_list_store_set(self->fEnvironmentStore2, &iter, 0, name->name, -1);
	REPL_set_file_modified(self, true);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(self->fEnvironmentStore2), &iter);
	gtk_tree_view_scroll_to_cell(self->fEnvironmentView, path, NULL, FALSE, 0.0f, 0.0f);
	{
		gint* indices;
		gint index;
		indices = gtk_tree_path_get_indices(path);
		index = indices[0];
		gtk_tree_path_free(path);
		// g_warning("%d/%d", index, gtk_tree_model_iter_n_children((GtkTreeModel*) self->fEnvironmentStore2, NULL) - 1);
		return(index);
		//return(gtk_tree_model_iter_n_children((GtkTreeModel*) self->fEnvironmentStore2, NULL) - 1);
	}
}
GtkWidget* REPL_get_widget(struct REPL* self) {
	return(GTK_WIDGET(self->fWidget));
}
static void REPL_enqueue_LATEX(struct REPL* self, AST::Node* node, GtkTextIter* destination) {
	Formatters::print_math(REPL_ensure_operator_precedence_list(self), stdout, 0, 0, node);
	//Formatters::print_S_Expression(stdout, 0, 0, node);
	fprintf(stdout, "\n");
	fflush(stdout);

	std::stringstream result;
	result << "$ ";
	std::string resultString;
	const char* nodeText = NULL;
	try {
		Formatters::to_LATEX(REPL_ensure_operator_precedence_list(self), node, result);
		result << " $";
		resultString = result.str();
		//std::cout << "XXX " << resultString << std::endl;
		nodeText = resultString.c_str();
	} catch(std::runtime_error e) {
		nodeText = NULL;
	}
	//std::cout << resultString << " X" << std::endl;
	{
		char* alt_text;
		alt_text = strdup(str(node).c_str());
		if(alt_text && strchr(alt_text, '"')) /* contains string */
			nodeText = NULL;
		GTKLATEXGenerator_enqueue(self->fLATEXGenerator, nodeText ? strdup(nodeText) : NULL, alt_text, destination);
	}
}
void REPL_clear(struct REPL* self) {
	GtkTextIter text_start;
	GtkTextIter text_end;
	gtk_text_buffer_get_start_iter(self->fOutputBuffer, &text_start);
	gtk_text_buffer_get_end_iter(self->fOutputBuffer, &text_end);
	gtk_text_buffer_delete(self->fOutputBuffer, &text_start, &text_end);
	gtk_list_store_clear(GTK_LIST_STORE(self->fEnvironmentStore2));
	g_hash_table_remove_all(self->fEnvironmentKeys);
	self->fTailEnvironment = self->fTailUserEnvironment = self->fTailUserEnvironmentFrontier = NULL;
	REPL_init_builtins(self);
}
char* REPL_get_absolute_path(const char* name) {
	if(g_path_is_absolute(name))
		return(strdup(name));
	else
		return(g_build_filename(g_get_current_dir(), name, NULL));
}
void REPL_append_to_output_buffer(struct REPL* self, const char* o_text) {
	/* will detect \xef\xbf\xbc and automatically call LATEX */
	GtkTextIter text_end;
	gchar* text;
	gchar* image_match;
	gchar* next_text;
	/* 0xFFFC = \xef\xbf\xbc */
	for(text = g_strdup(o_text); *text; text = next_text) {
		image_match = strstr(text, "\xef\xbf\xbc");
		if(!image_match) {
			image_match = text + strlen(text);
			next_text = image_match;
			gtk_text_buffer_get_end_iter(self->fOutputBuffer, &text_end);
			gtk_text_buffer_insert(self->fOutputBuffer, &text_end, text, -1);
		} else {
			FILE* input_file;
			Scanners::MathParser parser;
			AST::Node* content;
			next_text = image_match + 3;
			*image_match = 0;
			gtk_text_buffer_get_end_iter(self->fOutputBuffer, &text_end);
			gtk_text_buffer_insert(self->fOutputBuffer, &text_end, text, -1);
			try {
				input_file = fmemopen((void*) next_text, strlen(next_text), "r");
				parser.push(input_file, 0, false);
				parser.consume();
				content = Evaluators::programFromSExpression(parser.parse_S_Expression());
				fclose(input_file);
				gtk_text_buffer_get_end_iter(self->fOutputBuffer, &text_end);
				REPL_enqueue_LATEX(self, content, &text_end);
				next_text += parser.get_position();
				/* backtrack whitespace we skipped over */
				for(int i = 0; i < parser.get_position(); ++i) {
					char c = *(next_text - 1);
					if(c == '\n' || c == '\t' || c == ' ')
						--next_text;
					else
						break;
				}
			} catch(Scanners::ParseException& e) {
				fclose(input_file);
				continue;
			}
		}
	}
}
char* REPL_get_output_buffer_text(struct REPL* self) {
	GtkTextIter start;
	GtkTextIter end;
	gtk_text_buffer_get_start_iter(self->fOutputBuffer, &start);
	gtk_text_buffer_get_end_iter(self->fOutputBuffer, &end);
	return(REPL_get_output_text(self, &start, &end));
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
void REPL_insert_into_output_buffer(struct REPL* self, GtkTextIter* destination, const char* text) {
	gtk_text_buffer_insert(self->fOutputBuffer, destination, text, -1);
}
bool REPL_save(struct REPL* self, bool B_force_dialog) {
	bool B_OK = false;
	char* file_name;
	file_name = Config_get_environment_name(self->fConfig);
	if(!(file_name && file_name[0]))
		B_force_dialog = TRUE;
	else
		file_name = g_strdup(file_name);
	gtk_file_chooser_set_do_overwrite_confirmation(self->fSaveDialog, TRUE);
	if(file_name)
		gtk_file_chooser_set_filename(self->fSaveDialog, file_name);
	if(!B_force_dialog || gtk_dialog_run(GTK_DIALOG(self->fSaveDialog)) == GTK_RESPONSE_OK) {
		if(B_force_dialog) {
			g_free(file_name);
			file_name = gtk_file_chooser_get_filename(self->fSaveDialog);
		}
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
	} else {
		gtk_widget_hide(GTK_WIDGET(self->fSaveDialog));
		g_free(file_name);
		return(false);
	}
	gtk_widget_hide(GTK_WIDGET(self->fSaveDialog));
	g_free(file_name);
	if(!B_OK) {
		g_warning("could not save file");
	}
	return(B_OK);
}
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name) {
	gtk_window_set_title(self->fWidget, g_strdup_printf("%s%s", absolute_name ? absolute_name : "(5D)", self->fFileModified ? " *" : ""));
	Config_set_environment_name(self->fConfig, absolute_name);
	Config_save(self->fConfig);
}
bool REPL_get_file_modified(struct REPL* self) {
	return(self->fFileModified);
}
void REPL_set_file_modified(struct REPL* self, bool value) {
	if(self->fFileModified == value)
		return;
	self->fFileModified = value;
	{
		const char* absolute_name;
		absolute_name = Config_get_environment_name(self->fConfig);
		gtk_window_set_title(self->fWidget, g_strdup_printf("%s%s", absolute_name ? absolute_name : "(5D)", self->fFileModified ? " *" : ""));
	}
}
bool REPL_confirm_close(struct REPL* self) {
	if(REPL_get_file_modified(self)) {
		GtkDialog* dialog;
		char* file_name;
		file_name = Config_get_environment_name(self->fConfig);
		dialog = (GtkDialog*) gtk_message_dialog_new(GTK_WINDOW(REPL_get_widget(self)), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, (GtkButtonsType) 0, "Environment has been modified. Save?\n%s", file_name ? file_name : "");
		gtk_window_set_title(GTK_WINDOW(dialog), gtk_window_get_title(GTK_WINDOW(REPL_get_widget(self)))); /* yes, believe it or not, in some Window Managers, dialogs have their own title, defaulting to "Unnamed" */
		g_free(file_name);
		gtk_dialog_add_buttons(dialog, GTK_STOCK_GO_BACK, GTK_RESPONSE_CANCEL, gettext("Exit without Saving"), GTK_RESPONSE_CLOSE, GTK_STOCK_SAVE, GTK_RESPONSE_OK, NULL);
		gtk_dialog_set_default_response(dialog, GTK_RESPONSE_OK);
		{
			int result;
			result = gtk_dialog_run(dialog);
			gtk_widget_destroy(GTK_WIDGET(dialog));
			if(result == GTK_RESPONSE_CLOSE)
				return(true);
			else if(result == GTK_RESPONSE_CANCEL)
				return(false);
			return(REPL_save(self, FALSE));
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

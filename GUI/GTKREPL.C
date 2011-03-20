/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "GUI/GTKREPL"
#include "Scanners/MathParser"

namespace GUI {

static gboolean handle_key_press(GtkWidget* widget, GdkEventKey* event, gpointer user_data) {
	if(((event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == GDK_CONTROL_MASK) && event->keyval == GDK_Return) {
		gtk_dialog_response(GTK_DIALOG(user_data), GTK_RESPONSE_OK);
		return(TRUE);
	}
	return(FALSE);
}

GTKREPL::GTKREPL(GtkWindow* parent) {
	fWidget = (GtkWindow*) gtk_dialog_new_with_buttons("REPL", parent, (GtkDialogFlags) 0, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(fWidget), GTK_RESPONSE_OK);
	fMainBox = (GtkBox*) gtk_vbox_new(FALSE, 7);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fWidget)->vbox), GTK_WIDGET(fMainBox));
	gtk_widget_show(GTK_WIDGET(fMainBox));
	fEnvironmentView = (GtkTreeView*) gtk_tree_view_new();
	GtkTreeViewColumn* fNameColumn;
	fNameColumn = gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), NULL);
	gtk_tree_view_append_column(fEnvironmentView, fNameColumn);
	fEnvironmentStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(fEnvironmentView, GTK_TREE_MODEL(fEnvironmentStore));
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
	g_signal_connect_swapped(GTK_DIALOG(fWidget), "response", G_CALLBACK(&GTKREPL::handle_response), this);
	gtk_window_set_focus(GTK_WINDOW(fWidget), GTK_WIDGET(fCommandEntry));
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
	}
}
void GTKREPL::handle_response(gint response_id, GtkDialog* dialog) {
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

};

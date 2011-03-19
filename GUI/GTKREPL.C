/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "GUI/GTKREPL"

namespace GUI {

GTKREPL::GTKREPL(GtkWindow* parent) {
	fWidget = (GtkWindow*) gtk_dialog_new_with_buttons("REPL", parent, (GtkDialogFlags) 0, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(fWidget), GTK_RESPONSE_OK);
	fMainBox = (GtkBox*) gtk_vbox_new(FALSE, 7);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(fWidget)->vbox), GTK_WIDGET(fMainBox));
	gtk_widget_show(GTK_WIDGET(fMainBox));
	//fShortcutBox = (GtkBox*) gtk_hbutton_box_new();
	//fExecuteButton = (GtkButton*) gtk_button_new_from_stock(GTK_STOCK_OK);
	//gtk_widget_show(GTK_WIDGET(fExecuteButton));
	//gtk_box_pack_start(fShortcutBox, GTK_WIDGET(fExecuteButton), TRUE, TRUE, 7);
	//gtk_widget_show(GTK_WIDGET(fShortcutBox));
	fCommandEntry = (GtkEntry*) gtk_entry_new();
	fOutputArea = (GtkTextView*) gtk_text_view_new();
	fOutputScroller = (GtkScrolledWindow*) gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(fOutputScroller, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(GTK_WIDGET(fOutputArea));
	gtk_container_add(GTK_CONTAINER(fOutputScroller), GTK_WIDGET(fOutputArea));
	gtk_widget_show(GTK_WIDGET(fOutputScroller));
	fOutputBuffer = gtk_text_view_get_buffer(fOutputArea);
	//gtk_box_pack_start(GTK_BOX(fMainBox), GTK_WIDGET(fShortcutBox), FALSE, FALSE, 7); 
	gtk_box_pack_start(GTK_BOX(fMainBox), GTK_WIDGET(fOutputScroller), TRUE, TRUE, 7); 
	//gtk_box_pack_start(GTK_BOX(fWidget), GTK_WIDGET(fCommandEntry), FALSE, FALSE, 7); 
}
GtkWidget* GTKREPL::widget(void) const {
	return(GTK_WIDGET(fWidget));
}

};

/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <gtk/gtk.h>
#include "GUI/GTKREPL"
using namespace GUI;

static GtkWindow* make_REPL_window(void) {
	GtkWindow* window;
	window = (GtkWindow*) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	GTKREPL* REPL = new GTKREPL;
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(REPL->widget()));
	gtk_widget_show(GTK_WIDGET(window));
	return(window);
}

/* TODO popup command dialog with TextView, TextArea and completion, history. */
/* TODO main perspective window with maybe multiple perspective views */
/* TODO LATEX display worker that displays the actual equation in the text view once it's done */
int main(int argc, char* argv[]) {
	gtk_init(&argc, &argv);
	g_signal_connect(G_OBJECT(make_REPL_window()), "delete-event", G_CALLBACK(gtk_main_quit), NULL);
	gtk_main();
	return(0);
}

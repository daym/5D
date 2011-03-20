/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <gtk/gtk.h>
#include "GUI/GTKREPL"
#include "GUI/GTKView"
using namespace GUI;

static GtkWindow* make_view_window() {
	GtkWindow* window;
	GdkGeometry geometry;
	GtkBox* box;
	GtkBox* hbox;
	window = (GtkWindow*) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	box = (GtkBox*) gtk_vbox_new(FALSE, 2);
	hbox = (GtkBox*) gtk_hbox_new(FALSE, 2);
	GTKView* viewXZ = new GTKView(MODE_XZ);
	GTKView* viewXY = new GTKView(MODE_XY);
	GTKView* viewPerspective = new GTKView(MODE_PERSPECTIVE);
	gtk_box_pack_start(box, GTK_WIDGET(viewXZ->widget()), TRUE, TRUE, 2);
	gtk_box_pack_start(box, GTK_WIDGET(viewXY->widget()), TRUE, TRUE, 2);
	gtk_widget_show(GTK_WIDGET(box));
	gtk_box_pack_start(hbox, GTK_WIDGET(box), TRUE, TRUE, 2);
	gtk_box_pack_start(hbox, GTK_WIDGET(viewPerspective->widget()), TRUE, TRUE, 2);
	gtk_widget_show(GTK_WIDGET(hbox));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(hbox));
	geometry.min_width = 400;
	geometry.min_height = 400;
	gtk_window_set_geometry_hints(window, NULL, &geometry, GDK_HINT_MIN_SIZE);
	gtk_widget_show(GTK_WIDGET(window));
	return(window);
}
static GtkWindow* make_REPL_window(GtkWindow* parent, const char* source_file_name) {
	GTKREPL* REPL = new GTKREPL(parent);
	if(source_file_name)
		REPL->load_contents_from(source_file_name);
	gtk_widget_show(GTK_WIDGET(REPL->widget()));
	return(GTK_WINDOW(REPL->widget()));
}

/* TODO popup command dialog with TextView, TextArea and completion, history. */
/* TODO main perspective window with maybe multiple perspective views */
/* TODO LATEX display worker that displays the actual equation in the text view once it's done */
int main(int argc, char* argv[]) {
	gtk_init(&argc, &argv);
	g_signal_connect(G_OBJECT(make_REPL_window(make_view_window(), argc > 1 ? argv[argc - 1] : NULL)), "delete-event", G_CALLBACK(gtk_main_quit), NULL);
	gtk_main();
	return(0);
}

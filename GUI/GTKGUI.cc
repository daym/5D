/*
5D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <gtk/gtk.h>
#include "GUI/GTKREPL"
#include "GUI/GTKView"
using namespace GUI;

static GtkWindow* REPL_window;
static void open_REPL(GtkToolButton* button, GtkWindow* view) {
	gtk_widget_show(GTK_WIDGET(REPL_window));
}
/*
static GtkWindow* make_view_window() {
	GtkWindow* window;
	GdkGeometry geometry;
	GtkBox* box;
	GtkBox* hbox;
	GtkBox* mainVbox;
	GtkToolbar* toolbar;
	GtkToolButton* REPL_item;
	window = (GtkWindow*) gtk_window_new(GTK_WINDOW_TOPLEVEL);
	REPL_item = (GtkToolButton*) gtk_tool_button_new_from_stock(GTK_STOCK_EXECUTE);
	gtk_tool_button_set_label(REPL_item, "REPL...");
	g_signal_connect(G_OBJECT(REPL_item), "clicked", G_CALLBACK(open_REPL), window);
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(REPL_item), "Talk to computer...");
	gtk_widget_show(GTK_WIDGET(REPL_item));
	toolbar = (GtkToolbar*) gtk_toolbar_new();
	gtk_toolbar_set_style(toolbar, GTK_TOOLBAR_BOTH);
	gtk_toolbar_insert(toolbar, GTK_TOOL_ITEM(REPL_item), 0);
	mainVbox = (GtkBox*) gtk_vbox_new(FALSE, 7);
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
	gtk_widget_show(GTK_WIDGET(toolbar));
	gtk_box_pack_start(mainVbox, GTK_WIDGET(toolbar), FALSE, FALSE, 7);
	gtk_box_pack_start(mainVbox, GTK_WIDGET(hbox), TRUE, TRUE, 7);
	gtk_widget_show(GTK_WIDGET(mainVbox));
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(mainVbox));
	geometry.min_width = 400;
	geometry.min_height = 400;
	gtk_window_set_geometry_hints(window, NULL, &geometry, GDK_HINT_MIN_SIZE);
	gtk_widget_show(GTK_WIDGET(window));
	return(window);
}
*/
static GtkWindow* make_REPL_window(GtkWindow* parent, const char* source_file_name) {
	REPL* REPL = REPL_new(parent);
	if(source_file_name)
		REPL_load_contents_from(REPL, source_file_name);
	//gtk_widget_show(GTK_WIDGET(REPL->widget()));
	return(GTK_WINDOW(REPL_get_widget(REPL)));
}
int main(int argc, char* argv[]) {
	/*GtkWindow* view;*/
	gtk_init(&argc, &argv);
	/*view = make_view_window();*/
	REPL_window = make_REPL_window(NULL, argc > 1 ? argv[argc - 1] : NULL);
	g_signal_connect(G_OBJECT(REPL_window), "delete-event", G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_show(GTK_WIDGET(REPL_window));
	gtk_main();
	return(0);
}

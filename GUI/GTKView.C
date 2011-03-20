#include "GUI/GTKView"

namespace GUI {

GTKView::GTKView(void) {
	fScrolledWindow = (GtkScrolledWindow*) gtk_scrolled_window_new(NULL, NULL);
	fDrawingArea = (GtkDrawingArea*) gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(fScrolledWindow), GTK_WIDGET(fDrawingArea));
	gtk_widget_show(GTK_WIDGET(fDrawingArea));
	gtk_widget_show(GTK_WIDGET(fScrolledWindow));
}

GtkWidget* GTKView::widget() const {
	return(GTK_WIDGET(fScrolledWindow));
}


}; // end namespace GUI


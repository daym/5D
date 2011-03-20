#include "GUI/GTKView"

namespace GUI {

static gboolean g_repaint(GtkWidget* widget, GdkEventExpose* event, GTKView* view) {
	view->repaint(event);
	return(FALSE);
}
static void g_allocate_resources(GtkWidget* widget, GTKView* view) {
	view->allocate_resources();
}
GTKView::GTKView(void) {
	fScrolledWindow = (GtkScrolledWindow*) gtk_scrolled_window_new(NULL, NULL);
	fDrawingArea = (GtkDrawingArea*) gtk_drawing_area_new();
	gtk_scrolled_window_add_with_viewport(fScrolledWindow, GTK_WIDGET(fDrawingArea));
	gtk_widget_show(GTK_WIDGET(fDrawingArea));
	gtk_widget_show(GTK_WIDGET(fScrolledWindow));
	g_signal_connect(G_OBJECT(fDrawingArea), "realize", G_CALLBACK(g_allocate_resources), this);
	g_signal_connect(G_OBJECT(fDrawingArea), "expose-event", G_CALLBACK(g_repaint), this);
}
GtkWidget* GTKView::widget() const {
	return(GTK_WIDGET(fScrolledWindow));
}
void GTKView::allocate_resources(void) {
	fGC = gdk_gc_new(GTK_WIDGET(fDrawingArea)->window);
}
void GTKView::repaint(GdkEventExpose* event) {
	GdkPoint points[3];
	points[0].x = 0;
	points[0].y = 0;
	points[1].x = 90;
	points[1].y = 0;
	points[2].x = 0;
	points[2].y = 90;
	gdk_draw_polygon(GTK_WIDGET(fDrawingArea)->window, fGC, TRUE, points, 3);
}

}; // end namespace GUI


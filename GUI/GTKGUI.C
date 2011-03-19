#include <gtk/gtk.h>

static GtkWindow* make_REPL_window(void) {
	GtkWindow* window;
	window = (GtkWindow*) gtk_window_new(GTK_WINDOW_TOPLEVEL);
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

#include <gtk/gtk.h>

/* TODO popup command dialog with TextView, TextArea and completion, history. */
/* TODO main perspective window with maybe multiple perspective views */
int main(int argc, char* argv[]) {
	gtk_init(&argc, &argv);
	gtk_main();
	return(0);
}

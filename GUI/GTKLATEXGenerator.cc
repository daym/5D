#include <gtk/gtk.h>
#include <stdlib.h>
#include <sstream>
#include <limits.h>
#include "GUI/GTKLATEXGenerator"

namespace GUI {

struct GTKLATEXGenerator {
	char* fCacheDirectoryName;
	char* fLATEXCacheDirectoryName;
	GTKLATEXGeneratorFailure_t* fFailureCallback;
};
static char* GTKLATEXGenerator_init_cache_directory(void) {
	char* result;
	const char* user_cache_dir;
	user_cache_dir = g_get_user_cache_dir();
	if(!user_cache_dir)
		abort();
	g_mkdir_with_parents(user_cache_dir, 0744);
	result = g_build_filename(user_cache_dir, "4D", NULL);
	g_mkdir_with_parents(result, 0744);
	return(result);
}
void GTKLATEXGenerator_init(struct GTKLATEXGenerator* self) {
	self->fCacheDirectoryName = GTKLATEXGenerator_init_cache_directory();
	self->fLATEXCacheDirectoryName = g_build_filename(self->fCacheDirectoryName, "LATEX", NULL);
}
static void GTKLATEXGenerator_print_fallback_at_iter(struct GTKLATEXGenerator* self, const char* alt_text, GtkTextIter* iter) {
	gtk_text_buffer_insert(gtk_text_iter_get_buffer(iter), iter, alt_text, -1);
}
static void GTKLATEXGenerator_print_fallback(struct GTKLATEXGenerator* self, const char* alt_text, GtkTextMark* destination) {
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(gtk_text_mark_get_buffer(destination), &iter, destination);
	GTKLATEXGenerator_print_fallback_at_iter(self, alt_text, &iter);
}
//		GTKLATEXGenerator_print_fallback_at_iter(self, node, destination);
struct LATEXChildData {
	struct GTKLATEXGenerator* generator;
	GtkTextMark* mark;
	const char* alt_text;
	const char* document;
};
static void GTKLATEXGenerator_handle_LATEX_image(struct GTKLATEXGenerator* self, GtkTextIter* iter, const char* name, const char* alt_text) {
	GdkPixbuf* pixbuf;
	pixbuf = gdk_pixbuf_new_from_file("eqn.png"/*FIXME*/, NULL);
	if(pixbuf) {
		gtk_text_buffer_insert_pixbuf(gtk_text_iter_get_buffer(iter), iter, pixbuf);
		g_object_unref(G_OBJECT(pixbuf));
		unlink("eqn.png");
	} else
		GTKLATEXGenerator_print_fallback_at_iter(self, alt_text, iter);
}
static void GTKLATEXGenerator_handle_LATEX_image_at_mark(struct GTKLATEXGenerator* self, GtkTextMark* mark, const char* name, const char* alt_text) {
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark(gtk_text_mark_get_buffer(mark), &iter, mark);
	GTKLATEXGenerator_handle_LATEX_image(self, &iter, name, alt_text);
	gtk_text_buffer_delete_mark(gtk_text_mark_get_buffer(mark), mark);
}
static void g_LATEX_child_died(GPid pid, int status, struct LATEXChildData* data) {
	GTKLATEXGenerator_handle_LATEX_image_at_mark(data->generator, data->mark, data->document, data->alt_text);
	// FIXME GTKLATEXGenerator_queue_scroll_down(data->generator);
	// FIXME status, non-death.
	g_spawn_close_pid(pid);
	g_free(data);
}
struct GTKLATEXGenerator* GTKLATEXGenerator_new(void) {
	struct GTKLATEXGenerator* result;
	result = (struct GTKLATEXGenerator*) calloc(1, sizeof(struct GTKLATEXGenerator));
	GTKLATEXGenerator_init(result);
	return(result);
}
void GTKLATEXGenerator_enqueue(struct GTKLATEXGenerator* self, const char* document, const char* alt_text, GtkTextIter* destination) {
	GError* error;
	const char* argv[] = {
		"l2p",
		"-T",
		"-d",
		"120",
		"-i",
		document,
		//"'$I<latex_expression>$'
		NULL,
	};
	GPid pid;
	char name[PATH_MAX];
	if(snprintf(name, PATH_MAX, "%s/%s", self->fLATEXCacheDirectoryName, document) == 1 && g_file_test(name, G_FILE_TEST_EXISTS)) {
		GTKLATEXGenerator_handle_LATEX_image(self, destination, document, alt_text);
		// FIXME GTKLATEXGenerator_queue_scroll_down(self);
		return;
	}
	if(g_spawn_async(NULL, (char**) argv, NULL, (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD|G_SPAWN_SEARCH_PATH|G_SPAWN_STDOUT_TO_DEV_NULL), NULL, self/*user_data*/, &pid, &error)) {
		struct LATEXChildData* data;
		data = (struct LATEXChildData*) calloc(1, sizeof(struct LATEXChildData));
		data->generator = self;
		data->alt_text = alt_text;
		data->document = document;
		data->mark = gtk_text_buffer_create_mark(gtk_text_iter_get_buffer(destination), NULL, destination, TRUE);
		g_child_watch_add(pid, (GChildWatchFunc) g_LATEX_child_died, data);
	}
}
void GTKLATEXGenerator_set_failure_callback(struct GTKLATEXGenerator* self, GTKLATEXGeneratorFailure_t* value) {
	self->fFailureCallback = value;
}

}; // end namespace GUI

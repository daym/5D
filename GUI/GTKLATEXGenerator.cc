/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* for gdk_pixbuf_set_option */
#define GDK_PIXBUF_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
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
	g_mkdir_with_parents(self->fLATEXCacheDirectoryName, 0744);
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
static void get_cached_file_name(struct GTKLATEXGenerator* self, const char* document, char* destination, size_t destination_size) {
	int i;
	if(!document)
		abort();
	if(strlen(document) > 100) {
		GChecksum* checksum;
		checksum = g_checksum_new(G_CHECKSUM_MD5);
		g_checksum_update(checksum, (const guchar*) document, -1);
		if(snprintf(destination, destination_size, "%s/%s", self->fLATEXCacheDirectoryName, g_checksum_get_string(checksum)) == -1)
			abort();
		g_checksum_free(checksum);
	} else
		if(snprintf(destination, destination_size, "%s/%s", self->fLATEXCacheDirectoryName, document) == -1)
			abort();
	for(i = strlen(self->fLATEXCacheDirectoryName) + 1; destination[i]; ++i)
		switch(destination[i]) {
		case '/':
			destination[i] = '_';
			break;
		case '\n':
			destination[i] = ' ';
			break;
		}
}
static void GTKLATEXGenerator_handle_LATEX_image(struct GTKLATEXGenerator* self, GtkTextIter* iter, const char* document, const char* alt_text) {
	GdkPixbuf* pixbuf;
	char name[PATH_MAX];
	if(document)
		get_cached_file_name(self, document, name, PATH_MAX);
	pixbuf = document ? gdk_pixbuf_new_from_file(name, NULL) : NULL;
	if(pixbuf) {
		gdk_pixbuf_set_option(pixbuf, "alt_text", alt_text);
		gtk_text_buffer_insert_pixbuf(gtk_text_iter_get_buffer(iter), iter, pixbuf);
		g_object_unref(G_OBJECT(pixbuf));
		/*unlink(name);*/
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
	char name[PATH_MAX];
	if(!document) {
		GTKLATEXGenerator_handle_LATEX_image(self, destination, NULL, alt_text);
		return;
	}
	{
		get_cached_file_name(self, document, name, PATH_MAX);
		if(g_file_test(name, G_FILE_TEST_EXISTS)) {
			GTKLATEXGenerator_handle_LATEX_image(self, destination, document, alt_text);
			return;
		}
	}
	const char* argv[] = {
		"l2p",
		"-o",
		name,
		"-T",
		"-d",
		"120",
		"-i",
		document,
		//"'$I<latex_expression>$'
		NULL,
	};
	GPid pid;
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

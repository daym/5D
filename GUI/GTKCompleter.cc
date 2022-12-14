#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <set>
#include <stdlib.h>
#include <5D/Values>
#include <5D/Allocators>
#include "GUI/GTKCompleter"

namespace REPLX {
using namespace Values;
struct Completer {
	GtkEntry* fEntry;
	GHashtable* fHaystack; /* hash table's entry's value is unused */
	char* fEntryNeedle;
	Hashtable* fMatches;
	int fEntryNeedlePos;
	const char* fEntryText;
};
};
namespace GUI {
using namespace REPLX;
void Completer_accept_match_GUI(struct Completer* self, const char* new_text, int pos) {
	gtk_entry_set_text(self->fEntry, new_text);
	/*if(gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &beginning, &end)) {*/
	gtk_editable_select_region(GTK_EDITABLE(self->fEntry), pos, pos);
	gtk_editable_set_position(GTK_EDITABLE(self->fEntry), pos);
}

};
#include "GUI/CommonCompleter"
namespace GUI {
using namespace Values;
void Completer_complete(struct Completer* self) {
	const char* entry_text;
	int pos;
	entry_text = gtk_entry_get_text(self->fEntry);
	self->fEntryText = entry_text;
	pos = gtk_editable_get_position(GTK_EDITABLE(self->fEntry));
	Completer_complete_internal(self, entry_text, pos);
}
void Completer_init(struct Completer* self, GtkEntry* entry, GHashtable* haystack) {
	self->fEntry = entry;
	self->fHaystack = haystack;
	self->fEntryNeedle = NULL;
	self->fMatches = NULL;
	self->fEntryNeedlePos = 0;
	self->fMatches = new Hashtable;
}
struct Completer* Completer_new(GtkEntry* entry, GHashtable* haystack) {
	struct Completer* result;
	result = (struct Completer*) g_malloc0(sizeof(struct Completer)); // FIXME
	Completer_init(result, entry, haystack);
	return(result);
}

}; /* namespace */

#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "AST/AST"
#include "AST/Symbol"
#include "GUI/GTKCompleter"
#include "Scanners/MathParser"

namespace GUI {
struct Completer {
	GtkEntry* fEntry;
	GHashTable* fHaystack; /* hash table's entry's value is unused */
	char* fEntryNeedle;
	GHashTable* fMatches;
	int fEntryNeedlePos;
};
void Completer_accept_match_GUI(struct Completer* self, const char* new_text, int pos) {
	gtk_entry_set_text(self->fEntry, new_text);
	/*if(gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &beginning, &end)) {*/
	gtk_editable_select_region(GTK_EDITABLE(self->fEntry), pos, pos);
	gtk_editable_set_position(GTK_EDITABLE(self->fEntry), pos);
}

};
#include "GUI/CommonCompleter"
namespace GUI {
void Completer_complete(struct Completer* self) {
	const char* entry_text;
	int pos;
	entry_text = gtk_entry_get_text(self->fEntry);
	self->fEntryText = entry_text;
	pos = gtk_editable_get_position(GTK_EDITABLE(self->fEntry));
	const char* entry_needle;
	if(self->fMatches) {
		g_hash_table_destroy(self->fMatches);
		self->fMatches = NULL;
	}
	if(self->fEntryNeedle) {
		g_free(self->fEntryNeedle);
		self->fEntryNeedle = NULL;
	}
	self->fMatches = g_hash_table_new(g_direct_hash, g_direct_equal);
	/* TODO take caret position into account */
	entry_needle = strrchrset(entry_text, " ()\"", entry_text + pos); /* TODO only the stuff BEFORE the cursor */
	if(!entry_needle)
		entry_needle = entry_text;
	else
		++entry_needle;
	self->fEntryNeedle = g_strdup(entry_needle);
	self->fEntryNeedle[pos] = 0;
	self->fEntryNeedlePos = entry_needle - entry_text;
	g_hash_table_foreach(self->fHaystack, (GHFunc) match_entry, self);
	GList* keys;
	keys = g_hash_table_get_keys(self->fMatches);
	if(g_hash_table_size(self->fMatches) <= 1) { /* unambiguous match or non-match */
		AST::Symbol* key = keys ? (AST::Symbol*) keys->data : NULL;
		if(key)
			Completer_accept_match(self, key->name, TRUE);
	} else { /* ambiguous */
		/*assert(g_hash_table_size(self->fMatches) >= 2);*/
		char common_prefix[100];
		AST::Symbol* firstKey = keys ? (AST::Symbol*) keys->data : NULL;
		const char* firstKeyName = firstKey->name;
		int i;
		/* TODO show combo box, if that's actually needed... */
		for(i = 0; i < 100 - 1 && firstKeyName[i]; ++i) {
			if(in_all_keys_P(keys, i, firstKeyName[i]))
				common_prefix[i] = firstKeyName[i];
			else
				break;
		}
		common_prefix[i] = 0;
		if(common_prefix[0])
			Completer_accept_match(self, common_prefix, FALSE);
	}
	g_list_free(keys);
}
void Completer_init(struct Completer* self, GtkEntry* entry, GHashTable* haystack) {
	self->fEntry = entry;
	self->fHaystack = haystack;
	self->fEntryNeedle = NULL;
	self->fMatches = NULL;
	self->fEntryNeedlePos = 0;
}
struct Completer* Completer_new(GtkEntry* entry, GHashTable* haystack) {
	struct Completer* result;
	result = (struct Completer*) g_malloc0(sizeof(struct Completer));
	Completer_init(result, entry, haystack);
	return(result);
}

}; /* namespace */

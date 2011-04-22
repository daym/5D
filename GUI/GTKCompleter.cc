#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "AST/AST"
#include "AST/Symbol"
#include "GUI/Completer"
#include "Scanners/MathParser"

namespace GUI {
struct Completer {
	GtkEntry* fEntry;
	GHashTable* fHaystack;
	const char* fEntryNeedle;
	GHashTable* fMatches;
	int fEntryNeedlePos;
};
static void match_entry(AST::Symbol* key, AST::Node* value, struct Completer* self) {
	const char* possible_text;
	possible_text = key->name;
	if(!self->fEntryNeedle || strncmp(possible_text, self->fEntryNeedle, strlen(self->fEntryNeedle)) == 0) /* match */
		g_hash_table_insert(self->fMatches, key, value);
}
static void Completer_accept_match(struct Completer* self, const char* match, bool B_automatic_space) {
	const char* entry_text;
	char* new_text;
	int i;
	int pos;
	entry_text = gtk_entry_get_text(self->fEntry);
	if(!self->fEntryNeedle || !entry_text || !match) /* huh */
		return;
	new_text = (char*) g_malloc0(strlen(entry_text) + strlen(match) + 2); /* too much */
	for(i = 0; i < self->fEntryNeedlePos; ++i)
		new_text[i] = entry_text[i];
	/* i = self->fEntryNeedlePos */
	new_text[i] = 0;
	strcat(new_text, match);
	if(B_automatic_space)
		strcat(new_text, " "); /* TODO remove? */
	pos = strlen(new_text);
	while(entry_text[i] && Scanners::symbol_char_P(entry_text[i]))
		++i;
	strcat(new_text, &entry_text[i]);
	gtk_entry_set_text(self->fEntry, new_text);
	/*if(gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &beginning, &end)) {*/
	gtk_editable_select_region(GTK_EDITABLE(self->fEntry), pos, pos);
	gtk_editable_set_position(GTK_EDITABLE(self->fEntry), pos);
}
static bool in_all_keys_P(GList* keys, int i, char c) {
	for(; keys; keys = keys->next) {
		AST::Symbol* key = (AST::Symbol*) keys->data;
		if(key->name[i] != c)
			return(false);
	}
	return(true);
}
void Completer_complete(struct Completer* self) {
	const char* entry_text;
	entry_text = gtk_entry_get_text(self->fEntry);
	const gchar* entry_needle;
	if(self->fMatches)
		g_hash_table_destroy(self->fMatches);
	self->fMatches = g_hash_table_new(g_direct_hash, g_direct_equal);
	/* TODO take caret position into account */
	entry_needle = g_strrstr((gchar*) entry_text, " "); /* TODO make this more sophisticated */
	if(!entry_needle)
		entry_needle = entry_text;
	else
		++entry_needle;
	self->fEntryNeedle = entry_needle;
	self->fEntryNeedlePos = self->fEntryNeedle - entry_text;
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

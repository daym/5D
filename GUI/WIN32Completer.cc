#include "stdafx.h"
#include <malloc.h>
#include <set>
#include "GUI/WIN32Completer"
#include "Scanners/Scanner"
#include "Scanners/MathParser"

namespace GUI {

struct Completer {
	HWND fEntry;
	std::set<AST::Symbol*>* fHaystack;
	int fEntryNeedlePos;
	char* fEntryNeedle;
	std::string fEntryText;
	std::set<AST::Symbol*>* fMatches;
};

void Completer_accept_match_GUI(struct Completer* self, const char* new_text, int pos) {
	std::wstring textU;
	textU = FromUTF8(new_text);
	SetWindowTextW(self->fEntry, textU.c_str());
	/*if(gtk_editable_get_selection_bounds(GTK_EDITABLE(entry), &beginning, &end)) {*/
	/* FIXME gtk_editable_select_region(GTK_EDITABLE(self->fEntry), pos, pos);
	gtk_editable_set_position(GTK_EDITABLE(self->fEntry), pos);*/
	SendMessage(self->fEntry, EM_SETSEL, pos, pos);
}

}; /* end namespace GUI */

#include "GUI/CommonCompleter"
namespace GUI {

static std::wstring GetTextCXX(HWND item) {
	WCHAR buffer[20000] = {0};
	GetWindowTextW(item, buffer, 20000 - 1);
	// TODO error handling (if at all possible).
	return(buffer);
}

/* note that haystack's values are unused. Only the keys are used and assumed to be AST::Symbol* */
void Completer_init(struct Completer* self, HWND entry, std::set<AST::Symbol*>* haystack) {
	self->fMatches = new std::set<AST::Symbol*>;
	self->fEntry = entry;
	self->fHaystack = haystack;
}
struct Completer* Completer_new(HWND entry, std::set<AST::Symbol*>* haystack) {
	struct Completer* result;
	result = (struct Completer*) calloc(1, sizeof(struct Completer));
	Completer_init(result,  entry, haystack);
	return(result);
}
static int GetTextCursorPositionCXX(HWND control) {
	WPARAM beginning = 0;
	LPARAM end = 0;
	WCHAR buffer[20000];
	SendMessage(control, EM_GETSEL, (WPARAM) &beginning, (LPARAM) &end);
	return(end);
}
void Completer_complete(struct Completer* self) {
	const char* entry_text;
	int pos;
	std::wstring entryTextW = GetTextCXX(self->fEntry);
	self->fEntryText = ToUTF8(entryTextW);
	entry_text = self->fEntryText.c_str();
	pos = GetTextCursorPositionCXX(self->fEntry);
	const char* entry_needle;
	self->fMatches->clear();
	if(self->fEntryNeedle) {
		free(self->fEntryNeedle);
		self->fEntryNeedle = NULL;
	}
	/* TODO take caret position into account */
	entry_needle = strrchrset(entry_text, " ()\"", entry_text + pos); /* TODO only the stuff BEFORE the cursor */
	if(!entry_needle)
		entry_needle = entry_text;
	else
		++entry_needle;
	self->fEntryNeedle = strdup(entry_needle);
	self->fEntryNeedle[pos] = 0;
	self->fEntryNeedlePos = entry_needle - entry_text;
	/*g_hash_table_foreach(self->fHaystack, (GHFunc) match_entry, self);*/
	{
		std::set<AST::Symbol*>::const_iterator end_iter = self->fHaystack->end();
		for(std::set<AST::Symbol*>::const_iterator iter = self->fHaystack->begin(); iter != end_iter; ++iter)
			match_entry(*iter, NULL, self);
	}
	if(self->fMatches->size() <= 1) { /* unambiguous match or non-match */
		AST::Symbol* key = !self->fMatches->empty() ? *self->fMatches->begin() : NULL;
		if(key)
			Completer_accept_match(self, key->name, TRUE);
	} else { /* ambiguous */
		/*assert(g_hash_table_size(self->fMatches) >= 2);*/
		char common_prefix[100];
		AST::Symbol* firstKey = !self->fMatches->empty() ? *self->fMatches->begin() : NULL;
		const char* firstKeyName = firstKey->name;
		int i;
		/* TODO show combo box, if that's actually needed... */
		for(i = 0; i < 100 - 1 && firstKeyName[i]; ++i) {
			if(in_all_keys_P(*self->fMatches, i, firstKeyName[i]))
				common_prefix[i] = firstKeyName[i];
			else
				break;
		}
		common_prefix[i] = 0;
		if(common_prefix[0])
			Completer_accept_match(self, common_prefix, FALSE);
	}
}

}; /* end namespace */

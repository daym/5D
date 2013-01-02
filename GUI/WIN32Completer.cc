#include "stdafx.h"
#include <malloc.h>
#include <set>
#include "GUI/WIN32Completer"
#include <5D/Values>
#include <5D/FFIs>

namespace REPLX {
using namespace Values;
struct Completer : Node {
	HWND fEntry;
	Hashtable* fHaystack;
	int fEntryNeedlePos;
	char* fEntryNeedle;
	char* fEntryText;
	Hashtable* fMatches;
};

};
namespace GUI {
using namespace REPLX;

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

/* note that haystack's values are unused. Only the keys are used and assumed to be Symbol* */
void Completer_init(struct Completer* self, HWND entry, Hashtable* haystack) {
	self->fMatches = new Hashtable;
	self->fEntry = entry;
	self->fHaystack = haystack;
}
struct Completer* Completer_new(HWND entry, Hashtable* haystack) {
	struct Completer* result;
	result = new (UseGC) Completer;
	Completer_init(result,  entry, haystack);
	return(result);
}
static int GetTextCursorPositionCXX(HWND control) {
	WPARAM beginning = 0;
	LPARAM end = 0;
	SendMessage(control, EM_GETSEL, (WPARAM) &beginning, (LPARAM) &end);
	return(end);
}
void Completer_complete(struct Completer* self) {
	const char* entry_text;
	int pos;
	std::wstring entryTextW = GetTextCXX(self->fEntry);
	self->fEntryText = ToUTF8(entryTextW);
	entry_text = self->fEntryText;
	pos = GetTextCursorPositionCXX(self->fEntry);
	Completer_complete_internal(self, entry_text, pos);
}

}; /* end namespace */

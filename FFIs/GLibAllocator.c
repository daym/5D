#include <glib.h>
#include <gc/gc.h>
#include "FFIs/GLibAllocator"

void GLibAllocator_init(void) {
	GMemVTable vtable = {
	.malloc = GC_malloc,
	.realloc = GC_realloc,
	.free = NULL,
	.calloc = GC_malloc,
	.try_malloc = NULL,
	.try_realloc = NULL,
	};
	g_mem_set_vtable(&vtable);
}

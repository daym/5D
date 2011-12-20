#include <glib.h>
#include <gc/gc.h>
#include "FFIs/GLibAllocator"

static void* GCx_calloc(size_t nmemb, size_t size) {
	size_t sz = nmemb * size; // FIXME handle overflow here.
	return(GC_malloc(sz));
}
static void GCx_free(void* p) {
}
void GLibAllocator_init(void) {
	GMemVTable vtable = {
	.malloc = GC_malloc,
	.realloc = GC_realloc,
	.free = GCx_free,
	.calloc = GCx_calloc,
	.try_malloc = NULL,
	.try_realloc = NULL,
	};
	g_mem_set_vtable(&vtable);
}

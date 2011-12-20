#include <glib.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <gc/gc.h>
#include "FFIs/GLibAllocator"

static void* GCx_calloc(size_t nmemb, size_t size) {
	size_t sz = nmemb * size; // FIXME handle overflow here.
	return(GC_malloc(sz));
}
static void GCx_free(void* p) {
}
static inline char* GCx_strdup(const char* value) {
	char* result;
	result = (char*) GC_malloc_atomic(strlen(value) + 1);
	memcpy(result, value, strlen(value) + 1);
	return(result);
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
	// TODO move this into 5DLibs or even into FFIs/POSIX.cc , maybe
	xmlGcMemSetup(GCx_free, GC_malloc, GC_malloc_atomic, GC_realloc, GCx_strdup);
}

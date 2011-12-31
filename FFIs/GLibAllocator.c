#include <gc/gc.h>
#include <glib.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include "FFIs/GLibAllocator"

static void* GCx_malloc(size_t size) {
	return GC_MALLOC(size);
}
static void* GCx_malloc_atomic(size_t size) {
	return GC_MALLOC_ATOMIC(size);
}
static void* GCx_realloc(void* p, size_t size) {
	//GC_gcollect();
	return GC_REALLOC(p, size);
}
static void* GCx_calloc(size_t nmemb, size_t size) {
	size_t sz = nmemb * size; // FIXME handle overflow here.
	return(GC_MALLOC(sz));
}
static void GCx_free(void* p) {
	//memset(p, 0, GC_size(p));
	GC_FREE(p);
}
static inline char* GCx_strdup(const char* value) {
	char* result;
	result = (char*) GC_MALLOC_ATOMIC(strlen(value) + 1);
	memcpy(result, value, strlen(value) + 1);
	return(result);
}
void GLibAllocator_init(void) {
	g_thread_init(NULL);
/*
	FIXME preload something that overrides pthread_create instead. 
	g_thread_init_with_errorcheck_mutexes(&thread_functions);
*/
	//thread_functions.thread_create = create_thread,
	//thread_functions.thread_exit = ;
	g_mem_gc_friendly = TRUE;
/*
	GMemVTable vtable = {
	.malloc = GCx_malloc,
	.realloc = GCx_realloc,
	.free = GCx_free,
	.calloc = GCx_calloc,
	.try_malloc = GCx_malloc,
	.try_realloc = GCx_realloc,
	};
	//g_thread_init(&thread_functions);
	g_mem_set_vtable(&vtable);
*/
	// TODO move this into 5DLibs or even into FFIs/POSIX.cc , maybe
	xmlGcMemSetup(GCx_free, GCx_malloc, GCx_malloc_atomic, GCx_realloc, GCx_strdup);
}

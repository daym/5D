#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#endif
#include <gc/gc.h>
#include <glib.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include "FFIs/Allocators"

extern "C" {

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
void Allocator_init(void) {
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
	//xmlGcMemSetup(GCx_free, GCx_malloc, GCx_malloc_atomic, GCx_realloc, GCx_strdup);
}


void* ealloc(size_t size, void* source) {
	void* result;
#if defined(WIN32)
	result = VirtualAlloc(0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#elif defined(__linux__) || defined(__APPLE__)
	result = mmap(NULL, size + sizeof(long), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON, NULL, 0);
	//*(long*)p = size;
	//result = (long*)p + 1;
#endif
	memcpy(result, source, size);
	return(result);
}

void efree(void* address) {
#if defined(WIN32)
	VirtualFree(address, 0, MEM_RELEASE);
#elif defined(__linux__) || defined(__APPLE__)
	munmap((long*)address - 1, *((long*)address - 1));
#endif
}

}

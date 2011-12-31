#include <gc/gc.h>
#include <glib.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include "FFIs/GLibAllocator"

static void* GCx_malloc(size_t size) {
	return GC_MALLOC(size);
}
static void* GCx_realloc(void* p, size_t size) {
	GC_gcollect();
	return GC_REALLOC(p, size);
}
static void* GCx_calloc(size_t nmemb, size_t size) {
	size_t sz = nmemb * size; // FIXME handle overflow here.
	return(GCx_malloc(sz));
}
static void GCx_free(void* p) {
	memset(p, 0, GC_size(p));
	//GC_free(p);
}
static inline char* GCx_strdup(const char* value) {
	char* result;
	result = (char*) GC_MALLOC_ATOMIC(strlen(value) + 1);
	memcpy(result, value, strlen(value) + 1);
	return(result);
}
void GLibAllocator_init(void) {
#if 0
	GThreadFunctions thread_functions = {
  g_mutex_new_posix_impl,
  (void (*)(GMutex *)) pthread_mutex_lock,
  g_mutex_trylock_posix_impl,
  (void (*)(GMutex *)) pthread_mutex_unlock,
  g_mutex_free_posix_impl,
  g_cond_new_posix_impl,
  (void (*)(GCond *)) pthread_cond_signal,
  (void (*)(GCond *)) pthread_cond_broadcast,
  (void (*)(GCond *, GMutex *)) pthread_cond_wait,
  g_cond_timed_wait_posix_impl,
  g_cond_free_posix_impl,
  g_private_new_posix_impl,
  g_private_get_posix_impl,
  g_private_set_posix_impl,
  g_thread_create_posix_impl,
  g_thread_yield_posix_impl,
  g_thread_join_posix_impl,
  g_thread_exit_posix_impl,
  g_thread_set_priority_posix_impl,
  g_thread_self_posix_impl,
  g_thread_equal_posix_impl
	};
	//thread_functions.thread_create = create_thread,
	//thread_functions.thread_exit = ;
	g_mem_gc_friendly = TRUE;
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
	// TODO move this into 5DLibs or even into FFIs/POSIX.cc , maybe
#endif
	xmlGcMemSetup(GCx_free, GC_malloc, GC_malloc_atomic, GC_realloc, GCx_strdup);
}

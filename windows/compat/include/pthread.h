/* <pthread.h> for BlueWake on Windows: the few thread and mutex calls the host
 * makes, on Win32 threads and slim reader/writer locks (see
 * ../bw_posix_compat.c). */
#ifndef BW_COMPAT_PTHREAD_H
#define BW_COMPAT_PTHREAD_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef uintptr_t pthread_t;
typedef struct { int unused; } pthread_attr_t;
typedef struct { void* lock; } pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER {0}
int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*fn)(void*), void* arg);
int pthread_join(pthread_t thread, void** result);
int pthread_detach(pthread_t thread);
int pthread_mutex_init(pthread_mutex_t* mutex, const void* attr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);
#ifdef __cplusplus
}
#endif
#endif

/* <dlfcn.h> for BlueWake on Windows: LoadLibrary and GetProcAddress (see
 * ../bw_posix_compat.c). The game module is a DLL beside the executable. */
#ifndef BW_COMPAT_DLFCN_H
#define BW_COMPAT_DLFCN_H
#ifdef __cplusplus
extern "C" {
#endif
#define RTLD_LAZY 0x1
#define RTLD_NOW 0x2
#define RTLD_LOCAL 0x0
#define RTLD_GLOBAL 0x100
void* dlopen(const char* path, int flags);
void* dlsym(void* handle, const char* name);
int dlclose(void* handle);
char* dlerror(void);
#ifdef __cplusplus
}
#endif
#endif

#ifndef __DLFCN_H__
#define __DLFCN_H__

#ifdef __cplusplus
extern "C" {
#endif

#define RTLD_LAZY       0
#define RTLD_NOW        0
#define RTLD_GLOBAL     0
#define RTLD_LOCAL      0

#define RTLD_DEFAULT    ((void*) NULL)
#define RTLD_NEXT       ((void*) NULL)

/**
 * Open DLL, returning a handle.
 */
void* dlopen(
  const char *file,   /** DLL filename. */
  int mode            /** mode flags (ignored). */
);

/**
 * Close DLL.
 */
int dlclose(
  void* handle        /** Handle from dlopen(). */
);

/**
 * Look up symbol exported by DLL.
 */
void* dlsym(
  void* handle,       /** Handle from dlopen(). */
  const char* name    /** Name of exported symbol. */
);

/**
 * Return message describing last error.
 */
char* dlerror();

#ifdef __cplusplus
}
#endif

#endif

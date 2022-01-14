#ifndef utils_h_
#define utils_h_

unsigned get_filesize(const char *path);
void* openlib(const char *path);
void* getfunc(void *handle,const char* funcname);
void freelib(void *handle);

#endif
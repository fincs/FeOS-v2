#pragma once
#include "../types.h"

#ifdef __cplusplus
extern "C" {
#endif

void* memcpy(void*, const void*, size_t);
void* memmove(void*, const void*, size_t);
void* memset(void*, int, size_t);
int   memcmp(const void*, const void*, size_t);

size_t strlen(const char*);
char*  strcpy(char*, const char*);
char*  strcat(char*, const char*);
int    strcmp(const char*, const char*);
char*  strdup(const char*);
int    stricmp(const char*, const char*);
char*  strncpy(char*, const char*, size_t);
char*  strncat(char*, const char*, size_t);
int    strncmp(const char*, const char*, size_t);
int    strnicmp(const char*, const char*, size_t);

#ifdef __cplusplus
}
#endif

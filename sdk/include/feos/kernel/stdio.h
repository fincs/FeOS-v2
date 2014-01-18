#pragma once
#include "../types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tag_FILE FILE;

FILE* IoGetStdin();
FILE* IoGetStdout();
FILE* IoGetStderr();

#define stdin  IoGetStdin()
#define stdout IoGetStdout()
#define stderr IoGetStderr()

enum
{
	SEEK_SET = 0,
	SEEK_CUR,
	SEEK_END,
};

#define EOF (-1)
#define FOPEN_MAX 1024 // fake limit
#define FILENAME_MAX 1024

FILE* fopendev(const char* devName, int id); // Nonstandard

FILE* fopen(const char*, const char*); // Not implemented yet
FILE* freopen(const char*, const char*, FILE*); // Not implemented yet
int fclose(FILE*);

size_t fread(void*, size_t, size_t, FILE*);
size_t fwrite(const void*, size_t, size_t, FILE*);
int feof(FILE*);

int fseek(FILE*, int, int);
int ftell(FILE*);

static inline void rewind(FILE* f)
{
	fseek(f, 0, SEEK_SET);
}

int fflush(FILE*);
int ferror(FILE*);
void clearerr(FILE*);

enum
{
	_IONBF = 0,
	_IOLBF,
	_IOFBF,
};

int setvbuf(FILE*, char*, int, size_t); // Not implemented yet

int fprintf(FILE*, const char*, ...);
int vfprintf(FILE*, const char*, va_list);
int printf(const char*, ...);
int vprintf(const char*, va_list);

int fgetc(FILE*);
int fputc(int, FILE*);
char* fgets(char*, int, FILE*);
int fputs(const char*, FILE*);

#define getc fgetc
#define putc fputc

int getchar();
char* gets(char*);

int ungetc(int, FILE*);

int putchar(int);
int puts(const char*);

int remove(const char*); // Not implemented yet
int rename(const char*, const char*); // Not implemented yet

#ifdef __cplusplus
}
#endif

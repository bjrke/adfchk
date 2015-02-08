#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#else
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
typedef void * HMODULE;
typedef char TCHAR;
#define _T(a) (a)
#endif

#include "adfstruct.h"
#include "LiteUnzip.h"

int openLib(HMODULE);

extern char adfFilepath[MAXPATHLEN];

/* extern inline void strToUpper (char*); */
extern void waitKey(void);
extern FILE* initLog (char*);
extern chsS_t* splitsector (uint16);
extern void splitpath (char * i, char * o1, char * o2);
extern void writeLog (FILE *, const char *, ...);

extern inline void strToUpper (char* s);
/* uint64 getCRC32(unsigned char *, uint16); */
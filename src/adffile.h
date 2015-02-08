/* adffile.h */
#ifndef ADFFILE_H
#define ADFFILE_H

#include <stdio.h>
#include <stdlib.h> /* malloc () */
#include <stdarg.h> /* va_list */
#include <string.h>

#include "byteorder.h"

#define _LOGFILENAME_GENERIC "ADFCHK.LOG"

/* big / little endian stuff */

#if defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)

#define xformHtons(A) (A)
#define xformHtonl(A) (A)
#define xformNtohs(A) (A)
#define xformNtohl(A) (A)

#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)

#define xformHtons(A) ((((uint16)(A) & 0xff00) >> 8) | \
(((uint16)(A) & 0x00ff) << 8))
#define xformHtonl(A) ((((uint32)(A) & 0xff000000) >> 24) | \
(((uint32)(A) & 0x00ff0000) >> 8) | \
(((uint32)(A) & 0x0000ff00) << 8) | \
(((uint32)(A) & 0x000000ff) << 24))
#define xformNtohs xformHtons
#define xformNtohl xformHtohl

#else
/* #error */ /* this directive just does not work here, dunno whats wrong */
#endif

#define LWORDSIZE 8

/* these #defines are actually offsets for the memset routine which
   clears the old checksum; this way we can use the same routine for 
   both checksum types! */

#define LOG_DEFAULT "ADFCHK.LOG"

/* 
----------------------------------
  EXTERNAL FUNCTION AND VARIABLE DECLARATIONS
---------------------------------- 
*/


extern int argcnt;
extern void waitKey(void);
extern bool chkErr (dskImgS_t*, dskSecProps_t*, FILE*, inZIP_t*, uint8);
extern uint8 evalBootblkType(singleSecS_t*);


/* extern int extractZipAll (unzFile, int, int); */ /* <- adfmyunzip.c */
/* extern int extractZipCurrent (unzFile, const int*, int*); */ /* <- adfmyunzip.c */

uint64 getCRC32(uint8 *buf, uint64 len);
FILE * loadFile (char*);
FILE * initLog (char*);
void evalBitmap(singleSecS_t*, dskSecProps_t*);
void splitpath (char * i, char * o1, char * o2);
uint8 imgToMem (FILE * i, FILE * debugLog, dskImgS_t*);
uint64 computeChksum (singleSecS_t*,
                   const uint16,
									 const uint8 /* , FILE * dbg_out */);
#endif
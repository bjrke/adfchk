#include <stdio.h>
#include <math.h>
#include <string.h>
#ifdef __TURBOC__
#include <conio.h>
#endif
#include "typedefs.h"
#include "adfstruct.h"

/* string constants */

/* #define DISK_FILENAME "TOTALE~1.ADF" */
#define DISK_DEFAULT "DEFAULT1.ADF"

#define BITMAP_VALIDATION_FLAG 0x0138
/* This word may be OR'ed with the rootbase
   to get information about the
   validation longword of disk:  */
#define BITMAP_VALID 0xFFFFFFFF                     

/* ----------------------------------
      EXTERNAL FUNCTION DECLARATIONS
   ---------------------------------- */

extern int unzMain  (char*, char*); /* <- adfmyunzip.c */
extern int unDMSMain(char*, char*, uint8*, bool); /* <- adfmyundms.c */

extern void prgPurposeDesc(void);
extern int prgSelect(dskImgS_t*, uint8, uint8);
extern FILE * initLog(char*);
extern uint8 imgToMem (FILE * i, FILE * debugLog, dskImgS_t*);
extern uint8 evalBlk0Type(singleSecS_t*);
extern void evalBitmap(singleSecS_t*, dskSecProps_t*);
extern void chkBAMKey (dskImgS_t*, dskSecProps_t*);
extern bool isADOSRoot (singleSecS_t*);
extern uint8 getSecDataByte (dskImgS_t * d, uint16 secnum, uint16 ofs);
extern uint16 getSecDataWord (dskImgS_t * d, uint16 secnum, uint16 ofs);
extern uint64 getSecDataLong (dskImgS_t * d, uint16 secnum, uint16 ofs);
extern uint64 getSingleSecDataLong (singleSecS_t * s, uint16 ofs);

extern FILE * loadFile (char*);
extern void splitpath (char * i, char * o1, char * o2);
extern void writeLog (FILE *, const char *, ...);


/* extern inline void clearScreen (void); */
extern inline uint16 menuSelect(void);
/* extern inline void prgHeader(bool); */
extern void strToUpper (char* s);

void waitKey(void);

dskImgS_t* buildImgS(void);
void disposeImgS (dskImgS_t*);
uint32 chkdisk (FILE *, uint64, uint32);
uint8 chkusrdir (dskImgS_t*, uint32, uint8);
uint8 destroydiskstruct(dskImgS_t*);
uint8 filetoarray(FILE *, FILE *, dskImgS_t*);
uint64 getBlkLW (dskImgS_t*, uint32, uint8);
uint8 getBootType(dskImgS_t*);
uint64 calcChksum2(uint8 data[], 
														const uint32,
														const uint8,
														FILE *);
uint64 calcChksum(uint64 data[], 
														const uint32,
														const uint8,
														FILE *);

char * getBlkLWBCPL (dskImgS_t*, uint32, uint8);
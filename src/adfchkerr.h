/* adfchkerr.h */
#ifndef ADFCHKERR_H
#define ADFCHKERR_H

#include <stdio.h>

#include "typedefs.h"
#include "adfstruct.h"
#include "blktypes.h"
/* #include "adffile.h" */

extern char adfFilepath[MAXPATHLEN];
extern char adfFilename[MAXFILENAMELEN];
extern char logFilenameMain[100];
extern int argcnt;

extern unsigned char irakDataFirst[32];
extern unsigned char irakDataSecond[12];
extern unsigned char irakFileHdrSig[16]; 

#define SUFFIX_ISOK "_OK.log"
#define SUFFIX_ERRO "_ERR.log"
#define SUFFIX_VIRU "_VIRUS.log"
#define SUFFIX_NDOS "_NDOS.log"
#define SUFFIX_QSET "_QBSET.log"
#define SUFFIX_FAST  "_FFS.log"
#define SUFFIX_DERR  "_DMSERR.log"

#define CHKSUM_TYPE_BOOT_L 1
#define CHKSUM_TYPE_BAM_L 0
#define CHKSUM_TYPE_OTHER_L 5

extern char * DOS_TYPES[12];
extern char * BLK_TYPES[20];

/* 
----------------------------------
  EXTERNAL FUNCTION DECLARATIONS
---------------------------------- 
*/

/* getSecDataByte(), getSecDataWord(), getSecDataLong: ()
   These functions fetch a byte, word or long from the large *DISK* structure
   transforming the values accordingly to endian settings.
*/   
    
extern uint8  getSecDataByte (dskImgS_t*, uint16, uint16);
extern uint64 getSecDataLong (dskImgS_t*, uint16, uint16);

extern uint64 getSingleSecDataLong (singleSecS_t*, uint16);
extern uint64 getSingleSecDataLongDebug (singleSecS_t*, uint16);

extern void splitpath (char*, char*, char*);
extern void waitKey(void);
extern void writeLog (FILE*, const char*, ...);
/* extern void evalBitmap(singleSec_S_t*, dskSecProps_t*); */

void chkBAMKey (dskImgS_t*, dskSecProps_t*);

uint64 computeChksum (singleSecS_t*,
                   const uint16,
									 const uint8 /* , FILE* */);									 
bool chkErr (dskImgS_t*, dskSecProps_t*, FILE**, inZIP_t*, uint8);
chsS_t* splitsector (uint16);
bool isADOSRoot (singleSecS_t*);
bool hasADOSBitmap (singleSecS_t*);

#endif

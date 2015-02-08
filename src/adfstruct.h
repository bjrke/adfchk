/* adfstruct.h */
/* Structure definitions etc. */

#ifndef ADFSTRUCT_H
#define ADFSTRUCT_H

#include <stdlib.h> /* malloc() */

#include "blktypes.h"
#include "typedefs.h"

/* ADF-related stuff */
#define EXTADF_SIGNATURE_HI     0x5541452D /* UAE-*/
#define EXTADF_SIGNATURE_LO_OLD 0x2D414446 /* -ADF */
#define EXTADF_SIGNATURE_LO_NEW 0x31414446 /* 1ADF */

#define ADF_STANDARD_SIZE 901120

#define IS_ADF_STANDARD    0x00
#define IS_ADF_EXTENDEDOLD 0x01
#define IS_ADF_EXTENDEDNEW 0x02
#define IS_ADF_OVERDUMP    0x03
#define IS_ADF_UNDERDUMP   0x04

/* Archive-related stuff */
#define ZIPFILE_SIGNATURE     0x504B0304 /* PK\\003\\004 */
#define DMSFILE_SIGNATURE     0x444D5321 /* DMS!     */
#define STANDARD_DMS_CHUNK 11264

#define MAXPATHLEN 400
#define MAXFILENAMELEN 255

/* single sector, at 512 bytes each ( 1760 in total ) */
typedef union
{
 /* NOTE: the following 'union' is NOT the same as a 'struct', since
    it uses SAME memory for all three datatypes. Sometimes we need 
    a longword, sometimes a byte, but would like to avoid building up
    a memory area for each integer type. 
    Thanks to M. Kalms for this fantastic concept. */
    
    uint64 blklword[512/4];
    uint16 blkword [512/2];
    uint8  blkbyte [512];
} singleSecS_t;

/*  char *strInLW [512/4]; */ /* string in longword, single-lw */
/* char  *multiStr; */ 				 /* string that spans several longwords in a seq;
										      				cannot be longer than bytes per block */

/* disk image, 1760 sectors in total */
typedef struct {
  singleSecS_t sec [BLOCKSPERDISK];
  char volName[40];
  int16 bamKey;
  uint8 bootblktype;
  uint64 crc32;
  bool hasADOSRootBlk;
  bool hasValidBAMKeyOn80;
  bool suspectIRAK;  
} dskImgS_t;

/* this structure stores additional sector properties:
   bitmapFlag (USED/FREE)
   blkMask (bitmask; only evaluated for data blocks)
*/   
typedef struct {
  bool  bitmapFlag [BLOCKSPERDISK];  
  uint8 blkMask    [BLOCKSPERDISK];
/* FOR LATER USE */  
/*  uint8 blkType [BLOCKSPERDISK];  */
} dskSecProps_t;

typedef struct {
	uint16 cyl;  /* 0 .. 79  */
	uint16 trk;  /* 0 .. 159 */
	uint8  hd;   /* 0 .. 1  */
	uint8  sec;  /* 0 .. 11 */
} chsS_t;

typedef struct {
	char name[255];
	char ext [10];
	uint16 pos;
	uint64 crc32;
} inZIP_t;

#endif
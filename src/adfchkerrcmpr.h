/* adfchkerrcmpr.h */
#ifndef ADFCHKERRCMPR_H
#define ADFCHKERRCMPR_H

#include <stdio.h>

#include "adfstruct.h"

void chkCompressedADF (dskImgS_t*, FILE*, inZIP_t*, uint8);

extern uint64 getSingleSecDataLong (singleSecS_t*, uint16);
extern void evalBitmap(singleSecS_t*, dskSecProps_t*);
extern uint8 evalBlk0Type(singleSecS_t* bootdata);
extern uint8 getSecDataByte (dskImgS_t*, uint16, uint16);
extern bool chkErr (dskImgS_t*,dskSecProps_t*, FILE*, inZIP_t*, uint8);
extern bool isADOSRoot (singleSecS_t*);
extern void chkBAMKey (dskImgS_t*, dskSecProps_t*);

#endif
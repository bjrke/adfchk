/* adfmenu.h */
#ifndef ADFMENU_H
#define ADFMENU_H
#include <stdio.h>
#include "typedefs.h"
#include "adfstruct.h"

/* #include "adfmisc.h" */

#define PRG_STR_ADD ""
#define PRGV_MIN 2

#define PRGV_MID 2
#define PRGV_MAJ 0

#define COPYLEFTYR_UBOUND 2010

#define SMALLSTRLEN 80

static inline uint16 menuSelect(void);
inline void prgHeader(bool);
extern inline void clearScreen (void);

extern int argcnt; /* from adfmain.c */
extern void waitKey(void);
extern uint8 evalBootType(singleSecS_t*); /* from adffile.c */

/* from blktypes.h, included into adffile.h */
extern char* DOS_TYPES[]; 
extern char* BLK_TYPES[];

static inline uint16 menuSelect(void)
{
 /* asks user about menu item and returns the item chosen by the user */
 char iStr[SMALLSTRLEN];
 gets (iStr);

 return (*iStr - 48);
}

#endif
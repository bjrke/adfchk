/* adfmisc.c */
#include "adfmisc.h"

/* currently only containing inline functions */

inline void clearScreen (void);

inline void clearScreen (void)
{
  /* FIXME: poor clrscr() replacement.
	 if Borland's Turbo C compiler is used, true clrscr() from
	 conio.h is used; otherwise a replacement is called.   
	 This won't be an issue anymore once the GUI is finished.
	 */

#ifdef __TURBOC__

 clrscr();
 printf("\n\n");

#else
  int iCnt = 0;
  do
	printf ("\n");
  while (iCnt++ < 22);
#endif
}

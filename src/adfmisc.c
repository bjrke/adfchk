/* adfmisc.c */
#include "adfmisc.h"

/* currently only containing inline functions */

void clearScreen (void) {
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

void strToUpper (char* s) {
 while (*s)
 {
	if(*s > 96 && *s<123)
		 *s-=32;
		 s++;
 }
}

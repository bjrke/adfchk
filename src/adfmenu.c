#include "adfstruct.h"
#include "adfmenu.h"

void waitKey(void);

void prgPurposeDesc(void)
{
 /* Describes the purpose of this program. */

 printf ("\n   ======================= ADF CHECKER ================== \n\n");
 printf ("This program has been coded for checking ADFs, i. e. Amiga Disk \n");
 printf ("files. Actually, this ought to be the task of real Amiga programs\n");
 printf ("especially made for this purpose.\n");
 printf ("However, many people that have never been involved into the Amiga days,\n");
 printf ("would not even be able to set up a Workbench, let alone install a *.lha\n");
 printf ("archive from Aminet. Those would rather have a handy application able to\n");
 printf ("do this error-checking from within their native OS.\n");
 printf ("Did I say 'native OS'? Of course, this application is portable and can\n");
 printf ("be compiled with gcc and used under UNIX/Linux as well!\n\n");
 printf ("\n");

 waitKey();
 clearScreen();
}

inline void prgHeader(bool in_batch)
{
	if (!in_batch)
  {
  	 printf ("\n==========================================================\n");
  	 printf ("=                                                        =\n");
  	 printf ("=             AMIGA DISK FILE ERROR CHECKER              =\n");
  	 printf ("=                                                        =\n");
  	 
  	 printf ("=            BY ANDREAS EIBACH (C) 2007-%d             =\n",
  	   COPYLEFTYR_UBOUND);  	 
  	 printf ("=                                                        =\n");
     printf ("=                                                        =\n");
     printf ("=                VERSION : %2d.%d.%d%s                        =\n",
		   PRGV_MAJ, PRGV_MID, PRGV_MIN, PRG_STR_ADD);
     printf ("=                                                        =\n");
     printf ("==========================================================\n\n"); 
   }
   else  /* compacted form of header for batch mode operation */  
   {
   	 printf ("\n----------------------------------------------------------\n");
     printf ("|  adfchk (c) aeibach '07-'%d  [batch mode] v%d.%d.%d%s       |\n",
        COPYLEFTYR_UBOUND-2000, PRGV_MAJ, PRGV_MID, PRGV_MIN, PRG_STR_ADD);
     printf ("----------------------------------------------------------\n");
   }
}

int prgSelect(dskImgS_t* d, uint8 iSucc, uint8 iChksumErr)
{
	
  int iChoice = 0;

  while (iChoice < MINMENU || iChoice > MAXMENU)
  {
	 printf (" \nPlease choose: \n");
	 printf (" (1) Purpose of the program     \n");
    printf (" (2) Read ADF into buffer");

    if (iSucc > 0)
    { if (d != NULL && d->volName != NULL)
       printf ("  [Volume in buffer: '%s' {%s}]", d->volName, DOS_TYPES[d->bootblktype]);
      else
       printf ("       [<no file in buffer>]");
    }
    
    printf ("\n (3) Check directory structure of ADF in buffer \n");
    printf (" (4) Check for block checksum errors on disk image   ");
	     
    if (iChksumErr > 0)
    {
    	 /* found checksum errors */
    	 printf ("[<checksum error(s) found>]");	
    }
    else
    {
     if (iSucc > 0)
       printf ("  [<volume not checked yet>]\n");
     else
       printf ("  [<no file in buffer>]\n");
    }    
    	 
	 printf (" (5) Quit program               \n\n");

	 printf ("Your selection:  ");

	 iChoice = 0;

	 iChoice = menuSelect();

	 if (iChoice < MINMENU || iChoice > MAXMENU)
	 {
	   printf ("\n\nIllegal selection.\n\n");
	 }

    if ((iChoice == 3 || iChoice == 4) && iSucc == 0)    
    	  printf ("ERROR: no ADF loaded in buffer!");
				
  }
 printf("\n");
 if (iChoice < 3)
    clearScreen();
 return iChoice;
}

void waitKey(void)
{ 
 uint8 ch = 0;
 char d[1];
 printf ("\n Press RETURN to continue ... \n");
 while (!EOF) gets(d);
 
#undef MSDOS

#if defined (MSDOS) || defined (__TURBOC__)
  /* getch() is proprietary to DOS/Turbo C, but works great! */
  while (ch == 0)
  {
   ch = getch();
  }
#else
  scanf ("%1c",  (char *)&ch);
#endif
}
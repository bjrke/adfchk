#include "adfstruct.h"
#include "adfmenu.h"
#include "adfparams.h"
#include "adfmain.h"
#include "adffile.h" /* invokes byteorder.h as well */ 
#include "adfchkerrcmpr.h"

/*
  CHANGELOG
  ---------
[date]    [ver]
   
2007-??-?? 0.0.1         pre-alpha version, fairly unstable; nothing much going on;
  								       everything is a bloody mess
  
2008-05-30 0.0.2         pre-alpha version 2; lots of fixes in the reading routine;
									       ugly memory leak fixed, first reconception of "Amiga Disk" structure
	
2008-10-25 0.0.5         structure enhanced, dangerous malloc() typo fixed; routine can now properly
								         read ONE single block (remember: always start on a shoestring :))

2008-10-27 0.0.10        major update:
								         - structure now contains components for multi-word BCPL strings and block types
								         - routine can now read the whole disk with all its bells and whistles!									   
								         - added #define's for internal block type IDs (makes life easier)									 
								         - reading directory structure from root block is now possible in a very basic way
								         TODO: get rid of hardcoded disk name (maybe let user specify disk name?)
								         TODO: support subdirectories up to 2nd or 3rd level 
								         (ideally, make things *recursive*, but this is nothing for the faint of heart)

2008-10-28 0.0.12        fixed stupid bug in memory allocation routine, which built up the structure 
 								         perfectly, but did not free it properly - sometimes caused nasty GPFs.
 								         Also merged block[i] and block[i]->lword[i] allocations into one routine 
 								         (allocation was previously done in two separate ones, which made no sense)
  
2008-10-30 0.0.14        another major update:
 								         - removed lots of obsolete code
 								         - removed a lot of debug stuff from recursive directory routine and started 
 								       	   creating some "graphical" display for the directory tree found on the disk image								      
								         - some experiments with recursive directory reading: basically works but needs much 
								           more work in incorporating the output into the other program part.
								     
2008-10-31 0.0.16        even another major update:
  									     boot checksum routine is working now. 							   
  								       Postponed concepts for recursive directory routine to make way for a 
  								       major rewrite of the application's fundamentals.
								    
2008-11-03 0.0.20        complete rewrite of structure concept: many things were no good
  									     idea at second thought. Many thanks must go to M. Kalms for his
  									     invaluable suggestion of a union concept which can do bytes, words
    									   and long words within the _same_ memory space. Pointer arithmetics 
    									   are a mere walk in the park now! Hooray! Thanks M. K.!
    									   Endian problems by hton* and ntoh* macro family also solved.
    									   TODO: linked list for directory structure (this is the only 
    									   rational way of doing it, but it won't be a breeze for sure)
    									   YAY, all of a sudden something big and robust is coming up!
									
2008-11-04 0.0.22        Put old menu routines back in from first version.
    								     Wrote __endian-aware__ get...() functions which can get a 
    								     byte, word or long word from either the superstructure or the
    								     substructure. A Linux user encouraged me to avoid using original 
    								     hton*()/ntoh*() macro family names, because on his OS, these 
    								     would appear "in use" and cause trouble! Thanks, fixed!

2008-11-05 0.0.50        HUGE update.
  									     - made the application suitable for BATCH processing by implementing
  									       a comprehensive command-line parameter control.
  									     - added an additional boolean array to represent the bitmap of the AmigaDOS disk.
  									     This is extremely important because free blocks (marked unused) usually have 
  									     an invalid checksum; these must *not* be detected as "bad blocks".

2008-11-06 0.0.55        Fixed another dangerous bug - program carelessly attempted to evaluate the bitmap with 
                         ILLEGAL BAM keys (with unpredictable results in every respect).
  									     Also added NULL initializations for the main pointer variables because in contrary to popular 
  									     belief, those do NOT initialize with NULL automatically!
  									     Stripped down the "graphical" header for batch mode, because it makes no sense having 
  									     5+ lines for header if the thing is supposed to run by itself, without user intervention.
  									     (Explicit log file specification with the -l option will cause the entire header
  									     to be suppressed.)
									 
2008-11-07 0.0.60        LAMER virus detection on block is now possible !
 									       Memory leak due to misplaced free() statement fixed! (got buried too far inside the nested
 									       if-construction) 									   
 									       Removed silly "illegal bitmap" screen message flood, which caused a load of unnecessary 
 									       overhead in batch mode.
 									   
 									       Still TODO: Saddam/IRAK virus infection detection and decoding. The latter is not that easy 
 									       because the whole block is (XOR?) encoded against something yet to figure out.

2008-11-11 0.1.0         ZIP files are now supported! After some fiddling with Gilles Vollant's "minizip" contribution
                         (part of JL Gailly's & Mark Adler's zlib package), it turned out that minizip unpacked to hard disk 
                         instead of to memory buffer - no use for that, sorry! Also in respect to  users wanting to 
                         check their READ-ONLY ADF CD-Rs!!)
                         So I switched to LiteUnzip.DLL API for unzipping. Not only this works very well, but it also keeps
                         the executable size reasonably small. 
                         Fixed ugly protection fault in a loop inside evalBitmap()!
                         (Don't you EVER attempt to read one boolean word BEYOND the 1759th one! (arghh!!))
                   
2008-11-12 0.1.1         ZIP file support code continued; now also more detail possible in log file output. 
                         RC is virtually ready for release!

2008-11-13 0.1.2         Some more minor fixes; trimmed down program header in batch mode even more, because the BAT 
[RC1]                    file is supposed to run unattended.
                         Added another variant of the LAMER virus (the one randomly infecting blocks) with other case 
                         in spelling (LAMER vs. Lamer).                     

2008-11-15 0.1.2         A boatload of changes:
  [RC2]                  - Rewrote ENTIRE command-line parameter parser; auto-batch mode must be forced now
  										   Adding parameters in random order is now possible (yeah!)
                         Combinations like e. g. -b without filename specification will be blocked, though.
                         adfchk -f <ADF_FILENAME> invokes menu mode and auto-reads the image into buffer.
                         - added shiny graphical box to tell about missing LiteUnzip.dll
                         - nice Linux Makefile by Oddbod. (Thanks!)  
                         - LiteUnzip.h moved out of zipsupp/ subfolder. (appeared to have confused MAKE)  
                         - [adfmyunzip.c]: Fixed RARE Windows-only bug in speedy mass-batch mode caused by
                         GlobalFree() Win32! Sometimes memory was not correctly free()'d when operating in paths
                         with VERY long names inside deep trees; using good ol' free() fixed this.

2008-11-17 0.1.3         Lots of major changes again:
                         - added block types; this prevents "checksum error" warnings with blocks that are in no 
                           known AmigaDOS format. Type of block gets written to the log file as well now.
                         - lots of special formats are now recognized (e. g. root block ptr cleared but rest of
                           disk being AmigaDOS, etc.)
                         - error checking routine grew quite big and now resides in its own file, adfchkerr,c
                         - lots and lots of minor tweaks and fixes.
                  
2008-11-18 0.1.3bis      source updates only! Win32 binary works and needs no fixes.
                         Minor Linux tweaks:
                         - got rid of stricmp() completely by simply using a combination of strcmp() and my own 
                           strToUpper() function.
                         - due to malfunctioning endian macros: complete rewrite of endian preprocessor stuff.
                           Now supports specialties of Linux, FreeBSD, NetBSD and Cygwin. Put into its extra file now,
                           byteorder.h. Should build OOTB now on Linux-ish systems!

2008-11-21 0.1.4         Now uses typified log files: _NDOS, _ERR(oneus), _VIRUS, _FFS, _OK, _IOERR (for I/O errors).
                         Fixed false reports of allegedly "correct" BAM key detection with ZIP files, also helped 
                         to trim down log file size even more.
                         DATA block checks now include header key check to avoid phony detection of "data blocks"
                         in NDOS disks. Tricky stuff, because DATA blocks got no secondary type.
                         Now supports over- and underdumps, as well as both types of extended ADFs. 
                         (Note: Due to their MFM-based nature, extended ADFs cannot yet be scanned. Sorry!)

2009-10-13 0.2.0alpha1   New:
                         - VIRUS detection: 
                           -- SADDAM (bogus disk-validator + data block signature replaced by 'IRAK')
                             (Decoding of encoded blocks is currently still WIP. Sorry.).
  											   -- Jeff/Butonic v1.3.1 (3408 bytes)
                         - CRC32 calculation
                         - Quarterback backup set detection (common DosType replaced by 'Qb??' on boot block header)                         
                         Fixes/Enhancements:                
                         Stupid lower boundary problem with BAM location fixed (was: 3; now: 2).
                         (NB: Try finding a disk with the bitmap on block 2 - they're _very_ rare, Really!)
                         Major rewrites and enhancements in ADOS/NDOS detection logic.
                         Refined (and at part "embellished") log file information output.
                         TODO until 0.2.1 (final): WORKING DMS SUPPORT! (might as well take some more days,
                         albeit xdms code has already been properly 'woven' in by now)

2009-10-19 0.2.0alpha2   New:
											   - Fixed bogus detection of very special pseudo-ADOS format in games Project-X and Puffy's Saga
											   - DMS error detection ("DMS!!ERR" in block) implemented (will also work for NDOS disks).
											     Log files of disk images with DMS errors are recognizable by their (new) _DMSERR suffix.
											                          
2009-10-23 0.2.0alpha3   Hooray, DMS checking finally works great now! (uses a heavily stripped-down xDMS code
												 which had to be partly rewritten so that it unpack directly to memory buffer in order
												 to avoid ugly temp file writeouts)

2009-10-28 0.2.0alpha4   New: 
												 - More BOOT block viruses detected now:
												 -- Byte Bandit (3 variants supported)
												 -- Byte Warrior
												 -- Joshua 1 + 2
												 -- Pentagon Circle 1 + 2
												 -- Little Sven					 												 
												 - FILE viruses that ADFCHK can detect now:
												 -- Jeff/Butonic (v3.10) [1.31 was already recognized correctly in alpha1]
												 -- Eleni 1 (MessAngel-B)												 
												 Other stuff:
												 - Rob Northen Copylock block ID ("RNC" signature) is now detected as well
												 and ... FINALLY! Decomplexified adfchkerr.c by a great extent! Lots of redundant stuff has now 
												 been removed.
                         Made printBAMInfo() more general and bigger in order not to clog up the fairly comprehensive
                         core routine.chkErr() too much.
												 Forced _NDOS suffix for even more NDOS disks (still too many NDOS disks were misreported as
												 '_OK' in the logs, which makes no sense since this condition is indeterminable when NDOS!)

2009-10-31 0.2.0alpha5   New: 
												 - Added LEVIATHAN and SEPULTURA viruses to boot block detection.
												 Fixes:
												 - Fixed STUPID (!) crash caused by plain adf image at size < 901120.
												 (worked with ZIPPED adf underdumps!)		 
												 Enhancements:
												 - Since deep block T_DATA (0x08) scanning is still unsatisfying to me (if one of the criteria
												 is not met, block is deemed BLKTYPE_UNKNOWN), I've now rewritten the routines which handle
												 recognition of data blocks (e. g. bad seq number, BUT valid header key etc.)
												 Everything is much more refined now. 
												 Rescanning TOSEC set brought up a handful of new disk images with this type
												 of errors (previously undetected)

2009-11-01 0.2.1 FINAL   R E L E A S E D  v0.2.1 (final) to the public,

2009-12-03 0.2.2         That was sort of infuriating! 
                         Accidentally left in a superfluous free() from an old debug session, which was perfectly
                         cloaked by the complex Win XP/Vista/7 architecture, but turned out to bail out with a 
                         General Protection Fault on my Windows 2000 installation!!
                         This update might also fix a rare possible heap corruption on XP/7 (if it occurs at all) 
                         after 10,000 files or more. Glad this is settled now!
                         Some minor command-line output fixes were applied too.
*/

char filespec [MAXPATHLEN + MAXFILENAMELEN];
extern char adfFilepath[MAXPATHLEN];
extern char adfFilename[MAXFILENAMELEN];
extern char logFilenameMain[100];
extern char logFilenameIOErr[100];

int argcnt;

int main(int argc, char* argv[])
{	
	/* those pointers should always be initialized with NULL, to be on the safe shore */
  dskImgS_t *dskptr = NULL; 
  dskSecProps_t *secProps = NULL;
  uint8 *fbuf = NULL;
  FILE *fileptr = NULL;   
  FILE *log = NULL;  
  char *vol = NULL;
  
  int8 iUserChoice = 0;
  uint8 iSuccess = 0;
  uint8 iError = 0;
  uint8 ok = 0, ok2 = 0, resultDMS = 0 /* , ok3 = 0 */;
  bool useBatch = false, hasFileSet = false, isDMS = false;
  
  int8 optMask = 0; /* logical combination of command-line options! */   
  
  argcnt = argc; /* pass argc to external variable for (adffile.c) */
 
/* once we're here, everything worked OK */  
/* strcpy (lev, "[LEVEL "); */

/* process parameters */
  	
  if ((optMask = parseParams(argc, argv)) == -1)  	
  	exit(0);         
  	           	 
 /* although parameter parser is already very nifty, there are some combinations
    which can only be checked after the main parsing is done, since we never know where
    they will be specified! (e. g. specified only log file but no file to check) */ 
 
  /* if (optFile = 0 AND (optBatch = 1 OR optLog = 1)) */ /* no file given, but log or batch mode */
  
  if ((optMask & 1) == 0)  /* 0..6 even numbers = error conditions! */
  {
  	switch (optMask)
  	{
  		case 0x00:
  			/* 00 is only valid if no parameter was specified, else error */
  			if (argc > 1) 
  				 fprintf (stderr, "\nERROR: A parameter was given without option, or vice-versa,\nABORTED.\n\n");
  		break;
  		case 0x02:
  		/* ERROR */
  	     fprintf (stderr, "\nERROR: Need -f followed by a filename to operate: -l alone is useless.\nABORTED.\n\n");     	  	
  		break;
  	  
  	  case 0x04:
  	  case 0x06:
  	     fprintf (stderr, "\nERROR: Must specify -f followed by a filename to run in batch mode. \nABORTED.\n\n");		
  		break;
  	} /* endswitch */   	
   if (!(optMask == 0 && argc == 1)) exit(0);
  }    		

 /* allocate mem */
 
 secProps = malloc (BLOCKSPERDISK * sizeof(dskSecProps_t));
 
 /* build amiga disk structure */
 if (!(dskptr = buildImgS()))
  {
    fprintf (stderr, "Not enough memory left to build structure!\n");
    exit(0);
  }
  else
  {
    /* debug */
    /* printf ("Mem allocated @ addr %p ", &dskptr); */
  }	   	
 
/* use parameter(s) and start checking! */

hasFileSet = ((optMask & 1) > 0);  /* check "file bit" */
useBatch   = ((optMask & 4) > 0);  /* check "batch bit" */

if (hasFileSet)
	 /* check if batch mode is set */   /* fixme */                       
   iUserChoice = (useBatch == true) ? 2 : 2;  /* (2 = auto-read ADF specified from command line - NO BATCH - USER INTERACTIVE!) */

/* debug */
/* if (useBatch) printf ("Using batch mode ... \n") else printf ("Using menu mode ... \n"); */
 
 		while (iUserChoice <= MAXMENU-1)
  	{					 
		   if (useBatch)
		   {   /* batch + *NO* user-defined log file; filespec optional */
		   		 if ((iUserChoice == 2) && ((optMask & 5) == 0x05) && ((optMask & 2) == 0))
		       {
		  		   prgHeader(useBatch);
		         fprintf (stdout, "\n ---- RUNNING IN NON-INTERACTIVE BATCH MODE ... ---- \n\n");
		       }		 		
			 }
			 else		
			 {   
			 	   if ((iUserChoice <= 0 || iUserChoice == 4) && !isDMS) clearScreen();
			 	   prgHeader(useBatch);	
			 }	   	
			
			if (iSuccess > 0)  /* disk was read in */
		 	  vol = dskptr->volName;
 	
 			if (!useBatch && !hasFileSet) /* file bit cleared; do NOT branch into here 
 				                               if we do have parameters given */
 				  iUserChoice = prgSelect(dskptr, iSuccess, iError);
  
       /* printf ("\nuserchoice = %d\n" , iUserChoice); */
  
  
   		switch (iUserChoice)
			{
	   		case 1:
		 		prgPurposeDesc();
		 		break;

      	case 2: /* read adf */
       	
        if (dskptr != NULL)
        {        
        	   char* ext;
        	   	
        	 	 /* split up filespec into path + filename and 
        	 	    store both into external variable */
        	 	          	 	  
        	 	  splitpath (filespec, adfFilepath, adfFilename); 
        	 	  
        	 	  if (strcmp (adfFilepath, "") == 0)
        	 	     strcpy (adfFilepath, ".\\ (current directory)");
        	 	  
        	 	  
        	 	  ext = adfFilename+strlen(adfFilename)-4;
              strToUpper(ext); /* all uppercase */

              /* FIXME: should be transformed into an array structure later;
                 but for now, this may do */
        	 	  if (strcmp (ext, ".ADF") == 0 || 
        	 	  	  strcmp (ext, ".DMS") == 0 ||
        	 	  	  strcmp (ext, ".ZIP") == 0)        	 	      
        	    {        	    	        	    	
        	    	uint64 fHdr;        	    	
        	    	
        	    	fileptr = loadFile(filespec); /* file AND path */							  
        	    	fread(&fHdr, 1, 4, fileptr);								
								
                fHdr = xformHtonl(fHdr);
                
                /* debug */
                /* printf ("fhdr = %08lx\n", fHdr); */
                
        	    	/* for archive files, check header first! */
        	    	if (strcmp (ext, ".ADF") != 0)
        	    	{
        	    		 printf ("\n\nVerifying integrity of %s file header...\n", ext+1);
         	    		 
        	    		 switch (fHdr)
        	    		 {
        	    		   case ZIPFILE_SIGNATURE:	
        	    		 
        	    		 		  /*  fprintf (stderr, "File is a true ZIP file\n"); */
        	    	
        	    		         	/* --------------------------- */
        	    							/* ZIP archive handling      - */
        	    							/* --------------------------- */ 	
        	   							 if (fileptr != NULL)
        	    						 {
        	    		  					fileptr = NULL; /* reset! */
        	    	  					 	fprintf (stdout, "\nProcessing ADF image(s) inside ZIP archive, if there ... \n");
        	    
        	        						/* call large external unzipping routine */
        	        						/* note: due to the fact that (unlike DMS) there may be SEVERAL files in the zip, 
        	        						   we CANNOT get back here immediately, because we either need to successively 
        	        						   process the whole ZIP file or otherwise be doomed to do a lot of awkward 
        	        						   FSEEKing until the next file contained in the zip has been located! */
        	         
        	         						if ((ok2 = unzMain (filespec, logFilenameMain)) == 1)
        	         							break;
        	         						else
        	         						{
        	         	 						if (!useBatch) waitKey();
        	         	 
        	         	 						fprintf (stderr, "An I/O error occurred while processing this file! ABORTED.\n\n");
        	         							exit (0);
        	         						}		
        	    						 }
        	    				  break;
        	    		  
        	    		   case DMSFILE_SIGNATURE:
        	    		   
        	    		 	  /*  fprintf (stderr, "File is a true DMS file\n"); */
    
              							/* --------------------------- */
        	    							/* DMS archive handling (NEW!) */
        	    							/* --------------------------- */ 	
       								     
       								     /* call large external unDMSing routine */
       								     
       								     if (fileptr != NULL)
       								     {
       								     	  log = initLog (logFilenameMain);
       								     	  /* allocate mem of standard ADF size;
														     plus one additional chunk to cover the case of 
														     overdumped ADFS inside DMS archives */
  														fbuf = (uint8 *) calloc ((size_t) (ADF_STANDARD_SIZE + STANDARD_DMS_CHUNK), 1);

       								        resultDMS = unDMSMain (filespec, logFilenameMain, fbuf, true);  	         								       		 
        	         					  isDMS = true;	        	         				    		     	  
       								     }       								           								    
        	    		      break;
        	    		   
        	    		   default:        	    		   	        	    		         	    		         	    		         	    		 
        	    		 	  
        	    		 	   fprintf (stderr, "HEADER ERROR: This is not a %s, although the file extension suggests so!\nAborting ... \n\n", ext+1);
        	             exit(0);        	         
        	         }  /* endswitch */
        	         
        	    	}
        	    	        	    		
        	    }	
        	    else
        	    {
        	      if (fileptr == NULL)
        	      {
        	        fprintf (stderr, "ERROR: unsupported file type! Aborting ... \n\n");        	        
        	        exit(0); 	
        	      }	
        	    } 	  	        	            	            	              
       
					/* if we are working in unattended batch mode, even the error-checking
					   is supposed to run automatically, without user interaction. 
					   This is done by "pre-programming" the next choice. */      	            	    
        	  
        	 if (useBatch) iUserChoice = 4;
        	   
           if (fileptr != NULL)   
           { 
           	
           	if (isDMS)
           	{
           		 memcpy (dskptr, fbuf, ADF_STANDARD_SIZE);
           	   dskptr->crc32 = getCRC32 (fbuf, ADF_STANDARD_SIZE);
           	   free (fbuf);
           	   chkCompressedADF (dskptr, log, NULL, 0);           	   
           	   iSuccess = 1;
           	}
           	else 
           	{
           		 if ((iSuccess = imgToMem(fileptr, NULL, dskptr)) != 0)
           		 {	
           	  	 	if (dskptr != NULL)
				          {	
				     	      /* first, read the bamKey into a long local variable. 
				     	         This must be done to avoid misdetections! */
								  
								   dskptr->bootblktype = evalBlk0Type(&dskptr->sec[0]);
	 							   chkBAMKey (dskptr, secProps);
	 							   /* debug */ 
				           /* printf ("BAM points to block %0d", lword79); */	 								 							
           		 
				           /* fclose (fileptr); */ /* we do not need the physical file anymore */
				           /* evaluate bitmap, which is found at the block indicated by 
				             the 79th long word of the ROOT block */
				          }				     
				       }
				       else 
				       {
				       	/* BROKEN ADF handling: if we arrived here, some I/O error has happened! */
				       	/* imgToMem() only returns a FALSE if size is less than 880K.                 */
				       	
				       	fprintf(stderr, "\nERROR in file: '%s'!\n\nThis is a true Amiga Disk File, but its size is below standard.\n\nUnable to proceed! - ABORTED.\n\n", adfFilename);
				        exit(0);  	
				       } 	
				     }   
				   }				 
				 }
				 else
				 	  fprintf (stderr, "FATAL ERROR: out of memory - unable to build structure!");
					
				 if (!useBatch)	
         {
         	 if (iSuccess != 0)
            fprintf (stdout, "\n\n\n\n%d sectors of disk successfully read.\n\n", BLOCKSPERDISK);
           waitKey(); 
			     iUserChoice = prgSelect(dskptr, iSuccess, iError);
			   }
			    			    	
        break;
      	case 3: /* check adf in buffer */

        /* check whether there is an adf in buffer */
        
        if (iSuccess != 0)
        {
        	/* TODO */
          /* ok = chkusrdir (dskptr, ROOTBLOCK, 0); */
          
        }        
        /* === WAIT FOR KEYPRESS === */		 
				waitKey();
				
	   		break;

		 		case 4: 
		 	  if (iSuccess != 0)
        {  
        	int dummy = 0;   /** RESERVED for extended ADF handling **/
        	
        	/* check disk image in buffer for checksum errors */         	        	          	          	        

          if (!isDMS ^ (isDMS && !useBatch))  /* in batch mode (and only there!) DMS files have already been checked at this time! */
          {   
          	  log = initLog (logFilenameMain);
          	  iError = chkErr (dskptr, secProps, log, NULL, dummy);
          }	  
        	
        	if (argc > 2) 
        	{ 
        		iUserChoice = -1; 
        	}
        	else 
        	  waitKey();        		
        } /* endif (if iSuccess == 0) */ 
        
        break;
        
	   		case MAXMENU:       		
       		if (argc <= 2) fprintf (stdout, "\nGoodbye - see you next time.\n\n");	    
	   		
	   		default:
	   		break;
			} /* endswitch */
		 
		  if ((iUserChoice == -1 && argc > 2) || (ok2 == 1 && !isDMS))
		  		  break;

		 } /* endwhile */
 
 if (log) fclose (log);
 if (secProps) free (secProps);
 disposeImgS(dskptr);
 return ok; 
}

/* ------------------------------
          SUBROUTINES 
   ------------------------------ */

dskImgS_t * buildImgS(void)
{ 
  dskImgS_t * pImg;

  return (pImg = malloc (sizeof(dskImgS_t)));
}

void disposeImgS (dskImgS_t *pImg)
{
	 /*  printf ("Freeing ... \n"); */
  	if (pImg) free (pImg);
}

/* adffile.c */ 
/* All file-related stuff (building structure of ADF and releasing 
   memory at end as well as reading in the bytes from disk) goes here */

#include "adfstruct.h"
#include "adffile.h"
#include "adfchkerr.h"

/* These three are GLOBAL variables, which actually, ought to be avoided. 
   Unfortunately though, it is not possible to get a file specification (string)
   from a file pointer, at least not in a portable way. 
   Same with the argument count, which we'll also need in this external file.
*/

char adfFilepath[MAXPATHLEN];
char adfFilename[MAXFILENAMELEN];
char logFilenameMain[100] = _LOGFILENAME_GENERIC;
char logFilenameIOErr[100] = "ADFCHKIOERR.LOG";

extern char* DOS_TYPES[];
extern char* BLK_TYPES[];
extern uint64 crcLookupTable[256];
extern int argcnt;


void splitpath (char* pathIn, char* pathOut, char* filenameOut)
{ 
	/* FIXME: this definitely needs a rewrite,
	   as it looks a bit too clumsy.  But well, it works :p */
	
  int lastslash = 0, i; 
  int pos = 0;
  int plength = strlen(pathIn);
  for (i = 0; i < plength; i++) { 
    if (pathIn[i] == '/' || pathIn [i] == '\\') 
    { 
      lastslash = i; 
    } 
  } 
  
  if (lastslash > 0) pos = lastslash+1;
  
  if (pathOut != NULL)
     pathOut[pos] = '\0';
  
  if (lastslash == 0) /* no path spec */ 
   strcpy(filenameOut, pathIn);
  else 
  { 
  	if (pathOut != NULL)
       strncpy(pathOut, pathIn, pos);
  	strncpy(filenameOut, pathIn+lastslash+1, plength-lastslash+1);
    filenameOut[plength-lastslash] = 0;  
  } 
} 

FILE* initLog (char* fstr)
{
/* open output (log) file */
	
	FILE * ofile;
	
	/* char fpath[200]; */
  char ffile[100];
	  
  strcpy (ffile, logFilenameMain);

  ofile = fopen (/* (strcat (fpath,  */ ffile , "w"); 
  if (!ofile)
  {
		printf ("Error while opening log file! (no write permission on drive?)\n");        
    exit(0);
  }
	return ofile; 
}


void writeLog (FILE* ofile, const char* str_and_fmt, ...)
{
		/* Writes debug messages to both screen and to file, 
	   while accepting a variable list of arguments. */
   va_list arg_p;
         
   va_start (arg_p, str_and_fmt);       /* Initializing argument list *AFTER* str */
   		vfprintf (ofile, str_and_fmt, arg_p);
	 va_end (arg_p);

  fputs ("\n", ofile);	
}

FILE* loadFile (char* fstr)
{
  /* Loads the ADF input file either from the spec given by menu,
     or by command line. 
     The spec may be given with or without preceding path specification. 
     We cannot arrive here unless the file is a true ADF; everything else 
     must be handled outside.
  */
  FILE * f;
  
  if (fstr == NULL)
  {
  	 	  printf ("Illegal input file name!\n");
  	 	  waitKey();
  			exit(0);
  }
    
	/* open input file */
	/* FIXME - THIS SHOULD BE TURNED INTO CMD LINE PARAMETERS OR 
	   BETTER SOMETIME */
	
	f =  fopen (fstr , "rb");   
  if (f)
  {
   /* debug */	
	 /* printf ("File opened.\n"); */
	  
	  rewind(f);
	  return (f); 
  }
 
  /* non-existing file! */
	printf ("Error while opening input file '%s'!\n", fstr);
  waitKey();
  exit(0);  
}

uint8 imgToMem (FILE* ifilePtr, FILE* debugLogPtr, dskImgS_t *pImg)
{
  /* writes file contents into array, which 
     must have been properly loaded before. */
   
	/* read whole disk into buffer */
	uint8 len = 0;
	int k;		
	uint8* rawbuf; /* note: since we need a raw buffer first for the CRC32 calculation,
	                  we have to recopy this buffer to our structure a bit LATER! */
	uint64 fsize;
	uint64 crc; 
	
	/* get REAL size of file (can also be smaller (ERROR) or bigger than 880K, e. g. extended ADFs */
	fseek (ifilePtr, 0, SEEK_END);
  fsize = ftell(ifilePtr);
  fseek (ifilePtr, 0, SEEK_SET);

	rewind (ifilePtr);
  if (!(rawbuf = malloc(fsize)))
	{
		printf("ALLOCATION ERROR!");
		return 0;
		
	} /* endif (malloc 1 failed) */		
	
	if (fsize >= ADF_STANDARD_SIZE)
	 /*	(do NOT attempt to process files < 880K) */
	
	{ 
		fread(rawbuf, 1, fsize, ifilePtr);
    crc = getCRC32(rawbuf, fsize);
    memcpy (pImg, rawbuf, ADF_STANDARD_SIZE);
    free (rawbuf);
    pImg->crc32 = crc;
  
    /* get disk volume name (long word #78ff. @ block 880 */  
    /* first, get length of name */
    
    len = *(pImg->sec[ROOTBLOCK].blkbyte+432);
  
    if (len > 30) /*illegal length */
      len = 0;
    else	  
    {
    	 for (k = 0; k < len; k++)
    	 pImg->volName[k] = *(pImg->sec[ROOTBLOCK].blkbyte+(433+k));
    }
  	
    pImg->volName[len] = '\0'; /* since we're not using strcpy(), we should make sure
  														  the string is properly terminated!
  													 */
   /* debug */
   /*  printf ("volname = %s", pImg->volName); */
 
    pImg->hasADOSRootBlk = (len > 0) && isADOSRoot (&pImg->sec[ROOTBLOCK]);
  }
    													 
  return (fsize >= ADF_STANDARD_SIZE);
}


void evalBitmap(singleSecS_t* bamdata, dskSecProps_t* secProps)
{  
  /* evaluate bitmap of the Amiga DOS disk image.	 
     The bitmap is 1758 bits long (1760 minus 2 for the two boot blocks, 0 and 1)
     and spans (in FORWARD direction, unlike the header blocks!) the long words 
     #1 to #55, the last of which only uses the first 30 (of 32) bits for the bitmap - 
     the other 2 bits of the mask are 0.
     
     The BOOLEAN mask for the result value has:
       1 (true)  for "block used"
       0 (false) for "block free"
   */
   uint64 longword = 0x00000000;
   uint16 blockcnt = 2; /* block 2 is the first defined in the bitmap */
   int i = 1, k = 0;
   
   if (bamdata == NULL)
 	 {
 	   secProps = NULL;
 	   exit(1);
   }
       
   do
   { 
   	  k = 0; 
      do
      {		 
       longword =  ((*(bamdata->blkbyte+i*4+0) << 24) | 
    					     (*(bamdata->blkbyte+i*4+1) << 16) | 
    					     (*(bamdata->blkbyte+i*4+2) << 8)  | 
    					     (*(bamdata->blkbyte+i*4+3)));
           
           secProps->bitmapFlag[blockcnt] = false;
           if ((longword & 1<<k) == 0)  /* bit CLR = block USED */           	           
              /* printf ("Block %d is used ... ", blockcnt); waitKey(); */
      	      secProps->bitmapFlag[blockcnt] = true;           
       /*  else
           {
           	  printf ("Block %d is free ... ", blockcnt); waitKey();
           }	
       */
              	       
      } /* endwhile (inner) */
      while (k++ < 32 && ++blockcnt < BLOCKSPERDISK);
      
		 /* log to file */
    i++;
   } 
   while (blockcnt < BLOCKSPERDISK);  /* endwhile (outer) */
}

uint8 evalBlk0Type(singleSecS_t* bootdata)
{
	/* Determines boot type of disk (ADOS (+ subtype), NDOS, etc.) */
	 /* build up first LONG word in block 
	  (bytes would not work, because we need to test the "DOS" string too! */
	 
	 uint64 bdata    =  ((*(bootdata->blkbyte+0)<< 24) | 
    					        (*(bootdata->blkbyte+1) << 16) | 
    					        (*(bootdata->blkbyte+2) << 8)  |     		
    					        (*(bootdata->blkbyte+3)));
   
   /* SUBTRACT DOS base value from the current longword and check resulting difference 
      which MUST be in 0x00..0x07 interval; this algorithm will also affect disks with
      the first 3 bytes tampered with in some way */   
   
   /* the following does not work properly, but it should! (?) */
   /* uint64 bdataLSB = bdata & (~BLKTYPE_PRI_DOSBASE); */
      
   uint8 bdataLSB = (uint8) (bdata ^ BLKTYPE_PRI_DOSBASE);
   
   if ((bdataLSB >= 0x00 && bdataLSB < 0x08) && ((bdata ^ bdataLSB) == BLKTYPE_PRI_DOSBASE))      
   	 return bdataLSB;
   else
   {
      /* Once we arrive here, no DOS[?]-type disk was found. Disk could also be:
      	 - a Quarterback archive (Qb?? [?? = 01..99])   	
      	 - a Rob Northen Copylock disk (RNC)
      	 - an A1000 kickdisk     (KICK)                   *//* ======= TODO ======  */   	
      
       /* RNC check */
       if ((bdata & ~0xff) == BLKTYPE_PRI_RNCOPYLK)
       {
         /* fprintf (stderr, "\ncopylock block\n"); */
         return BLKTYPE_RNCOPYLOCK;
       }	
       else
       {
       		
        /* QB check */	
        /* lets convert the ASCII stuff to a simple integer to make life easier when comparing later        */   	
        /* NOTE: >> 4 would not work, because we need to convert hex to decimal: thus a 40 would actually mean a '64'!    */
        /* !! So we have to shift one more to the right, and ensure to keep the value of all digits below 10 ! */         
         uint16 qbID = (uint16) (((bdata & 0x0f00) >> 8) * 10 + (bdata & 0x0f));   	
         return ((((bdata & ~0xffff) == BLKTYPE_PRI_QBACKDISK1) || 
      	          ((bdata & ~0xffff) == BLKTYPE_PRI_QBACKDISK2)) &&
                 (qbID > 0           && qbID <= 99          )) ? BLKTYPE_QBACKDISK:BLKTYPE_UNKNOWN;
       }       
    }   
}

uint64 getCRC32(uint8 *buf, uint64 len)
{
   register uint64 crc;
   uint64 i;

   crc = 0xFFFFFFFF;
   for (i = 0; i < len; i++)        
     crc = ((crc >> 8) & 0x00FFFFFF) ^ crcLookupTable[(crc ^ *buf++) & 0xFF];      
      
  return (crc ^ 0xFFFFFFFF);
}

/* getSecDataByte(), getSecDataWord(), getSecDataLong: ()
   These functions fetch a byte, word or long from the large *DISK* structure
   transforming the values accordingly to endian settings.
*/   
    
uint8 getSecDataByte (dskImgS_t* d, uint16 secnum, uint16 ofs)
{
 return (*(d->sec[secnum].blkbyte+ofs));
}

uint16 getSecDataWord (dskImgS_t* d, uint16 secnum, uint16 ofs)
{
#if defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)	
	return (xformHtons(*(d->sec[secnum].blkword+ofs)));
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
	return (xformNtohs(*(d->sec[secnum].blkword+ofs)));
#endif
 return 0;
}


uint64 getSecDataLong (dskImgS_t* d, uint16 secnum, uint16 ofs)
{
#if defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)	
	return (xformHtonl(*(d->sec[secnum].blklword+ofs)));
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
	return (xformNtohl(*(d->sec[secnum].blklword+ofs)));
#endif
 return 0;
}

/* getSingleSecDataByte(), getSingleSecDataWord(), getSingleSecDataLong: ()
   These functions fetch a byte, word or long from the smaller *SECTOR* 
   substructure, transforming the values accordingly to endian settings.
*/   
uint8 getSingleSecDataByte (singleSecS_t* s, uint16 ofs)
{
  return (*(s->blkbyte+ofs));
}

uint16 getSingleSecDataWord (singleSecS_t* s, uint16 ofs)
{
 #if defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)	
    return (xformHtons(*(s->blkword+ofs)));
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
    return (xformNtons(*(s->blkword+ofs)));
#endif
 return 0;
}

uint64 getSingleSecDataLong (singleSecS_t* s, uint16 ofs)
{
#if defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)	       
    return (xformHtonl(*(s->blklword+ofs)));
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
    return (xformNtohl(*(s->blklword+ofs)));
#endif
 return 0;
}

uint64 getSingleSecDataLongDebug (singleSecS_t* s, uint16 ofs)
{
#if defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
   return (xformHtonl(*(s->blklword+ofs)));
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
    return (xformNtohl(*(s->blklword+ofs)));
#endif
 return 0;
}

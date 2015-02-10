#include "adfmyunzip.h"
#include "adfchkerrcmpr.h"
#include "adffile.h"       /* CRC32 */

/* Where we store the pointers to LiteUnzip.dll's functions that we call */
UnzipOpenFilePtr		*lpUnzipOpenFile;
UnzipGetItemPtr			*lpUnzipGetItem;
UnzipItemToBufferPtr	*lpUnzipItemToBuffer;
UnzipClosePtr			*lpUnzipClose;
UnzipFormatMessagePtr	*lpUnzipFormatMessage;

int unzMain(char* zipfilename, char* logfilename)
{	
	HMODULE		unzipDll = NULL;
	HUNZIP		huz;
	DWORD		result;

  int ok;
  
  /* these are identical definitions like in adfmain.c;
     since we cannot get back and forth from the calling routine to here,
     these get redefined here and all the checking is done here too */
     
  dskImgS_t* destbuf2 = NULL;
  inZIP_t inZIP;
  uint8 adfType = IS_ADF_STANDARD;
  bool abortchk = false;
  char logFilenameCopy[100];
  char* p;
  
  /* open log file*/
  FILE * logMain = initLog (logfilename);
  
  /* open library */
  if ((ok = openLib(unzipDll)) == 0) return 0;   /* ERROR */
  
  if (logMain) 
  {
  	strcpy (logFilenameCopy, logfilename);
    p = logFilenameCopy;
    *(p+strlen(p)-4) = '\0';  /* strip off extension */
  }  
  
		/* open ZIP file on disk */
		if (!(result = lpUnzipOpenFile(&huz, _T(zipfilename), 0)))
		{
			ZIPENTRY		ze;
			DWORD			numitems;
			unsigned char* destbuf;

			/* Find out how many items are in the archive. */
			ze.Index = (DWORD)-1;
			if ((result = lpUnzipGetItem(huz, &ze))) 
			{
				 /* FIXME: goto!! */
				 goto bad2;
			}			
			/* Unzip each item, using the name stored (in the zip) for that item. */
			
			numitems = ze.Index;
			/* ------------------------------------------------------------------ */
			for (ze.Index = 0; ze.Index < numitems; ze.Index++)
			{
				/* Get info about the next item */
				if ((result = lpUnzipGetItem(huz, &ze)) > 0) 
				{
					 /* FIXME: goto!! */
						 goto bad2;
				}							
				
				/* this is NOT initiating an if block! */				
				{					
					/* Allocate a memory buffer to decompress the item */
#ifdef WIN32
					if (!(destbuf = GlobalAlloc(GMEM_FIXED, ze.UncompressedSize)))
#else
					if (!(destbuf = malloc(ze.UncompressedSize)))
#endif
					{
#ifdef WIN32
						printf("ALLOCATION ERROR!\n");
#else
						printf("ALLOCATION ERROR!  - %s\n", strerror(errno));
#endif
						break;
					} /* endif (malloc 1 failed) */
					
#ifdef WIN32
					if (!(destbuf2 = GlobalAlloc(GMEM_FIXED, sizeof(dskImgS_t))))
#else
					if (!(destbuf2 = malloc(sizeof(dskImgS_t))))
#endif
          {
#ifdef WIN32
						printf("ALLOCATION 2 ERROR!\n");
#else
						printf("ALLOCATION 2 ERROR! - %s\n", strerror(errno));
#endif
						break;
					} /* endif (malloc 2 failed) */
			
					
					/* Decompress the item into our buffer */
				 	if ((result = lpUnzipItemToBuffer(huz, destbuf, ze.UncompressedSize, &ze)) > 0)
					{
						/* error: nothing found to read into buffer! */
#ifdef WIN32
						GlobalFree(destbuf);
#else
						free(destbuf);
#endif

bad2:					lpUnzipClose(huz);
						
						/* FIXME: goto!! */
						goto bad;
					} 
          else
          {						  
            /* copy raw buffer content into disk image structure,
               but check size first! */
            
            abortchk = false;
            
            /* Now we must "rescue" the CRC32 from the (yet raw) buffer data! */
            inZIP.crc32 = getCRC32(destbuf, ze.UncompressedSize);                                    
            strcpy (inZIP.name, ze.Name);
            inZIP.pos = ze.Index;
                        
            /* get extension */
            strcpy (inZIP.ext, inZIP.name+strlen(inZIP.name)-4);
            strToUpper(inZIP.ext);
            
            /* printf ("extension is %s\n", inZIP.ext); */            
                        
            if (ze.UncompressedSize != ADF_STANDARD_SIZE)
            {
            	/* check for overdump; only .ADF extension is accepted!  */
            	 if (strcmp (inZIP.ext, ".ADF") > 0)
            	 {
            	 		printf ("ERROR: This does not appear to be an ADF image. CHECKING ABORTED.\n");
            	    abortchk=true;
            	 }
            	 else
            	 {
            	 		if (ze.UncompressedSize > ADF_STANDARD_SIZE)
            	    {
            	    	uint64 lwHi = 0x0, lwLo = 0x0;
                    /* extended adf? */                             
                    
                    memcpy (destbuf2, destbuf, 8); /* 2 longwords */                    
                    lwHi = getSingleSecDataLong (&destbuf2->sec[0], 0);
                    lwLo = getSingleSecDataLong (&destbuf2->sec[0], 1);
                    
                    /* check hi long word of signature */
                    
                    if (lwHi == EXTADF_SIGNATURE_HI)
                    { 
                    	if (lwLo == EXTADF_SIGNATURE_LO_OLD || 
                    	    lwLo == EXTADF_SIGNATURE_LO_NEW)
                    	{
                    		/* write header */
            	    	    strcat (logFilenameCopy, "_EXTADF.log");
            	    	    remove (logFilenameCopy);
                    		
                    		writeLog (logMain, "---------------------------------------------------------------------------------------------");
                        writeLog (logMain, "CHECKING DISK:                 '%s'\n\nPATH TO FILE:                  '%s'", zipfilename, adfFilepath);
                         printf ("\n                                                        ");
                             
                        /*  filename inside ZIP archive (optional) */      		  
                        if (inZIP.name != NULL)
                        {
                         char fn[MAXFILENAMELEN];
                     		 splitpath (inZIP.name, NULL, fn);
                         writeLog (logMain, "\nCHECKING FILE #%d INSIDE ZIP:   '%s'", (inZIP.pos)+1, fn);                 
                        }                    	
                    	
                    	  writeLog (logMain, "\nCRC32 CHECKSUM:                %08lx", inZIP.crc32);
                    	
                    	  writeLog (logMain, "\n*****************************************************************************");
            	  
            	          if (lwLo == EXTADF_SIGNATURE_LO_OLD)            	       	              	  
            	          {  writeLog (logMain, " Sorry, this is an extended ADF (OLD format) which is not yet\
            	            \n supported for scanning! CHECKING ABORTED.");            	                      	            
            	            adfType = IS_ADF_EXTENDEDOLD;
            	          }
            	          else
            	          {  writeLog (logMain, " Sorry, this is an extended ADF (NEW format) which is not yet\
            	            \n supported for scanning! CHECKING ABORTED.");
            	            adfType = IS_ADF_EXTENDEDNEW;
            	          }  
            	       
            	    	    writeLog (logMain, "*****************************************************************************\n");
                    	  
                    	  printf ("\nSorry, extended ADFs are not yet supported for scanning. \n\n");
                    	  fclose (logMain);
                        rename (logfilename, logFilenameCopy);      
                    	  abortchk = true;                    	                      	
                    	}		
                    	else break;
                    	                       
                    }
                    else
                    	 adfType = IS_ADF_OVERDUMP;
                     

            	    }
            	    else
            	    {
            	    	/* underdumps */            	    	            	    	
            	    	 adfType = IS_ADF_UNDERDUMP;
            	    	 
            	    	 strcat (logFilenameCopy, "_IOERR.log");
            	    	 remove (logFilenameCopy);
            	    	 
                     writeLog (logMain, "---------------------------------------------------------------------------------------------");
                     writeLog (logMain, "CHECKING DISK:          '%s'\n\nPATH TO FILE:           '%s'", zipfilename, adfFilepath);
                     printf ("\n                                                        ");
                             
                     /*  filename inside ZIP archive (optional) */      		  
                     if (inZIP.name != NULL)
                     {
                         char fn[MAXFILENAMELEN];
                     		 splitpath (inZIP.name, NULL, fn);
                         writeLog (logMain, "\nCHECKING FILE #%d INSIDE ZIP:   '%s'", (inZIP.pos)+1, fn);                 
                     }
                     
                       	 /* check for overdump */            	    	 
            	    	 writeLog (logMain, "\n*****************************************************************************");
            	  
            	       if (ze.UncompressedSize < 20000)            	       	              	  
            	          writeLog (logMain, "ERROR: This appears to be a small patch file (originally DMS) - unable to check!\n");
            	       else
            	       	  writeLog (logMain, "ERROR: This is an UNDERDUMPED file, likely to have a lot of data corruption \nbeyond the %u. data byte! Avoid using it!\
            	       	   \nCHECKING ABORTED.", ze.UncompressedSize);
            	       
            	    	 writeLog (logMain, "*****************************************************************************\n");

            	    	fclose (logMain);
                    rename (logfilename, logFilenameCopy);            	    	
            	    	
            	    	abortchk=true;
            	    }	/* endif */
            	 		
            	 }            	            	
            }
            else
            {
            		
            	/* standard ADF expected */
            	
            	/* first, check extension */
            	
            	if (strcmp (inZIP.ext, ".ADF") > 0)
            		{
            			printf ("WARNING: This image has no .ADF extension, but is in correct size. Checking anyway...\n");            		  
            		}            		           		
            }                                    
            
            if (abortchk)
            {            	      	             	             	             	
              
            	if (destbuf2)
              {  
#ifdef WIN32
						    GlobalFree(destbuf2);
#else
						    free(destbuf);
#endif
            	}
            		 
            	if (destbuf) 
            	{  
#ifdef WIN32
						    GlobalFree(destbuf);
#else
						    free(destbuf);
#endif
            	}
            	            	
              if ((inZIP.pos+1) == numitems)  /* only one item or last item */
              {
              	/* do not return false when extended ADF was processed */              	              	             	
              	
              	 if (adfType == IS_ADF_EXTENDEDOLD || adfType == IS_ADF_EXTENDEDNEW)
              	 	return 1;              	 	
              	 else
              	 {
              	 	 printf ("\nERROR: Invalid Amiga Disk File: '%s'! \n\n", inZIP.name);
              	 	 return 0;
              	 }	 
              }		 	
            }	 
            else
            {
            	memcpy (destbuf2, destbuf, ADF_STANDARD_SIZE);              
              chkCompressedADF (destbuf2, &logMain, &inZIP, adfType);
            }
          }
#ifdef WIN32
					GlobalFree(destbuf);
#else
					free(destbuf);
#endif
				} /* end of separate block (NOT an if!) */
			
			  /* we'll continue with next file ... */
			} 
			/* endfor */
			/* ------------------------------------------------------------------ */

			/* Done unzipping files, so close the ZIP archive. */
			lpUnzipClose(huz);
		}
		else
		{
			TCHAR	msg[100];

bad:		
	    lpUnzipFormatMessage(result, msg, sizeof(msg));
			printf("UNZIP ERROR: %s\n", &msg[0]);
		}

		/* Free the LiteUnzip.DLL */
#ifdef WIN32
		FreeLibrary(unzipDll);
#else
		dlclose(unzipDll);
#endif

  if (destbuf2) free (destbuf2);
  if (logMain) {
	  fclose(logMain);
	  logMain = NULL;
  }
	return(1);
}


int openLib(HMODULE theDll)
{
#ifdef WIN32
	/* Open the LiteUnzip.DLL. Note: If LiteUnzip.dll is not placed in a path that can be found
	   by this app, then LoadLibrary will fail. So, either copy LiteUnzip.dll to the same
	   directory as this EXE, or to some directory that Windows is set to search. */
	if ((theDll = (HMODULE)LoadLibrary(_T("LiteUnzip.dll"))))
	{
		/* Get the addresses of 5 functions in LiteUnzip.dll -- UnzipOpenFile(), UnzipGetItem()
		 UnzipItemToFile(), UnzipClose(), and UnzipFormatMessage. */
		lpUnzipOpenFile = (UnzipOpenFilePtr*)GetProcAddress(theDll, UNZIPOPENFILENAME);
		lpUnzipGetItem = (UnzipGetItemPtr*)GetProcAddress(theDll, UNZIPGETITEMNAME);
		lpUnzipItemToBuffer = (UnzipItemToBufferPtr*)GetProcAddress(theDll, UNZIPITEMTOBUFFERNAME);
		lpUnzipClose = (UnzipClosePtr*)GetProcAddress(theDll, UNZIPCLOSENAME);
		lpUnzipFormatMessage = (UnzipFormatMessagePtr*)GetProcAddress(theDll, UNZIPFORMATMESSAGENAME);	

#else  /* Linux & Co */

	if ((theDll = (HMODULE)dlopen("libliteunzip.so", RTLD_LAZY)))
	{
		/* Get the addresses of 5 functions in LiteUnzip.dll -- UnzipOpenFile(), UnzipGetItem()
		   UnzipItemToFile(), UnzipClose(), and UnzipFormatMessage.*/
		lpUnzipOpenFile = (UnzipOpenFilePtr*)dlsym(theDll, UNZIPOPENFILENAME);
		lpUnzipGetItem = (UnzipGetItemPtr*)dlsym(theDll, UNZIPGETITEMNAME);
		lpUnzipItemToBuffer = (UnzipItemToBufferPtr*)dlsym(theDll, UNZIPITEMTOBUFFERNAME);
		lpUnzipClose = (UnzipClosePtr*)dlsym(theDll, UNZIPCLOSENAME);
		lpUnzipFormatMessage = (UnzipFormatMessagePtr*)dlsym(theDll, UNZIPFORMATMESSAGENAME);
#endif
  
  return 1; 
 } 
 else        /* ERROR */
 {	
#ifdef WIN32
		printf ("-----------------------------------------------------\n");
		printf ("|                                                   |\n");
		printf ("|                 FATAL ERROR:                      |\n");
		printf ("|                                                   |\n");
		printf ("|            LiteUnzip.dll NOT FOUND!               |\n");
    printf ("|                                                   |\n");
    printf ("|  Please make sure the DLL is in the current path. |\n");
    printf ("|                                                   |\n");
    printf ("-----------------------------------------------------\n");
#else
		printf("\nDLL ERROR: %s\n", dlerror());
#endif
	  printf ("\nPROGRAM ABORTED. \n");
	}

return 0;
}

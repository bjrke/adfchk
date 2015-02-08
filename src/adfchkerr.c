#include <stdio.h>
#include <string.h> /* also memset() */

#include "adfchkerr.h"
#include "adffile.h"
#include "adfvdata.h"

char * DOS_TYPES[] = {"DOS0-OFS", "FAST FILE SYSTEM", "OFS + INTL. MODE", 
												"FFS + INTL. MODE", "OFS + DIR. CACHE MODE + INTL.",
											  "FFS + DIR. CACHE MODE + INTL.", "OFS + DIR. CACHE MODE + INTL.",
											  "FFS + DIR. CACHE MODE + INTL.", "QUARTERBACK BACKUP SET DISK", 
											  "A1000 KICKDISK", "ROB NORTHEN COPYLOCK", "UNKNOWN/NDOS/NOBOOT"
											 };

char * BLK_TYPES[] = {"DATA", "", "", "", "", "", "", "", "", 
	                    "FILE-HEADER", "FILE-LIST",
                      "ROOT", "USER DIR", "FORMATTED","INVALID1", "INVALID2", "UNKNOWN"};

void printBAMInfo (FILE*, int16, bool, bool, bool, bool);
bool chkDMSerr(singleSecS_t* /*, uint16 */);
uint8 getBlkType(singleSecS_t*, uint8*,    uint16);


bool chkErr (dskImgS_t* pImg, dskSecProps_t* pSecProps, FILE* log, inZIP_t* inZIP, uint8 type)
{
	/* THIS IS ACTUALLY THE CORE FUNCTION OF THE WHOLE THING. 
	   Due to its high complexity level, it has been put into its own file. 
	   (which requires lots of #include guard magic for proper linking!) */
	
	uint16 currBlk =0;
  uint64 blkChecksum = 0, bamChecksum, calcChksum = 0, calcChksumBAM = 0;
  
  uint8 blkType = 0;
  
  chsS_t chs; /* cyl - header - sector */
  chsS_t* pchs = &chs;
  
  /* DEBUG */
 /*  if (pImg->bamKey != -1)	
		printf ("[DEBUG] Found BAM key of disk image located on block %d\n", pImg->bamKey);
   waitKey();
 */
   
  char logFilenameCopy[200];
  bool bootChksumInvalid = false, bamKeyInvalid = false, 
       bamChksumCleared = false, bamChksumInvalid = false;
  bool noDeepChk = false, hasInvalidName = false, isFFS = false,
       hasSuffix = false, hasCheckedBogusDV = false, 
       
       hasCheckedJeffOld = false, hasCheckedJeffNew = false, 
       hasCheckedBratYawns = false, hasCheckedEleni = false;
  uint64 rootnum, bootprg = 0;
  uint8 errcnt = 0, vircnt = 0 /*, dmserrcnt = 0 */;
    
  char* pLog = logFilenameCopy;
 /* printf ("l = %s", logFilenameMain); */
  
  strcpy (logFilenameCopy, logFilenameMain);  
  
  *(pLog+strlen(pLog)-4) = '\0';  /* strip off extension by "shifting left" the null byte */
 
 /* ========================  START OF BOOT BLOCK CHECKING ROUTINE ============================== */
  
  /* let's do the bootblock stuff before the loop, makes things neater 
     and does not require a boatload of 'if currBlk == 0' inside checks 
     note also that computeChksum must be called with TWICE the size, because 
     we need to sum up blocks 0 AND 1                                          */
    
  if ((calcChksum = computeChksum (&pImg->sec[0], 512 * 2, CHKSUM_TYPE_BOOT_L /*, NULL */))
  	  
  	  !=
  	  
  	  (blkChecksum = (getSingleSecDataLong(&pImg->sec[0], CHKSUM_TYPE_BOOT_L))))
  {
     /* wrong boot checksum: disk cannot boot!                       */
     /* (this need not be an error, since it may also refer to non-bootable data disks) */     	
     bootChksumInvalid = true;
  }		
  
  /* debug */
  /* printf ("Calculated boot checksum is = %08lx, actual boot checksum = %08lx", calcChksum, blkChecksum); */
    
  /* now check BAM validity, but only if BAM key is valid, otherwise it makes no sense  */
 
    if (pImg->bamKey != -1)     
    {	  
    	 calcChksumBAM = computeChksum (&pImg->sec[pImg->bamKey], 512, CHKSUM_TYPE_BAM_L /*, NULL */);
       bamChksumInvalid = (((bamChecksum = getSingleSecDataLong(&pImg->sec[pImg->bamKey], 0)) != calcChksumBAM));
    	   
    	 bamChksumCleared = (calcChksumBAM == 0);
        
       /* debug */
    	 /* printf ("BAM checksum "); if (!bamChksumInvalid) printf ("correct!"); else printf ("incorrect! (need %08lx  have  %08lx)\n", bamChecksum, calcChksumBAM); */
    }
    else
    	bamKeyInvalid = true; /* illegal BAM key (could still have valid ADOS root block, though! */
    		   
    rootnum = getSingleSecDataLong (&pImg->sec[0], 2);    
    bootprg = getSingleSecDataLong (&pImg->sec[0], 3);
    
    /* -------------------------- */
    /* WRITE HEADER INTO LOG FILE */
    /* -------------------------- */            
    				   			     				    			    				 
    writeLog (log, "---------------------------------------------------------------------------------------------");  
    writeLog (log, "CHECKING DISK:          '%s'\n\nPATH TO FILE:           '%s'\n", adfFilename, adfFilepath);

    writeLog (log, "BLOCK 0 TYPE:           '%s'\n", DOS_TYPES[pImg->bootblktype]);
    		   
    fprintf (stdout, "\n                                                                    ----------------------------------------------------------\n");
    fprintf (stdout, "\n                                                                     ");
           
           
    if (pImg->hasADOSRootBlk)
    {
    		  fprintf (stdout, "Processing ADOS volume:  %s\n\n", pImg->volName);
          writeLog (log, "AMIGA DOS VOLUME NAME:  '%s'\n\n", pImg->volName);
    }
    else
    {	       
        if (pImg->bootblktype == BLKTYPE_QBACKDISK)
        {
    	      /* ATTENTION: Quarterback backup disks must have priority in checks from everything else, because
    	         it is possible that a true AmigaDOS disk was used as LAST QB backup set.
    	         That means, we cannot just connect the conditions "bitmap pointer invalid" and "is QB disk"! */       
         
           	fprintf (stdout, "Processing volume:  <name unavailable>");
          	fprintf (stdout, "\n                                                                      (illegal volume name length)\n\n");
            writeLog (log, "AMIGA DOS VOLUME NAME:  <not available with QB backup sets (with few exceptions)>");     
            hasInvalidName = true;
        }
        else
        {
          	if (bamKeyInvalid) /* invalid BAM key */
            { 
              	 /* Invalid BAM key does NOT necessarily mean that the disk name will be invalid as well!
           	   Since in (somewhat) rare cases, the disk has been deliberately OBFUSCATED but 
           	   the disk name is valid nonetheless! 
           	   
           	   To test this, we have to (re-)check the ROOT block for required ADOS characteristics:
           	   
           	   primary type MUST be 0x00000002
           	   secondary type MUST be 0x00000001    	   
           	    
           	   bitmap flag must be FFFFFFFF (valid) or 00000001 (valid; rare) or 00000000 (invalid)
           	   */    	
           	
           	   if (pImg->hasADOSRootBlk)
                {       	 
             	    printf ("Processing ADOS volume:  %s\n\n", pImg->volName);
           	      writeLog (log, "AMIGA DOS VOLUME NAME:  '%s'", pImg->volName);
                }
                else  
                {  
                  fprintf (stdout, "Processing NDOS volume: <name unavailable>");
           		    fprintf (stdout, "\n                                                                     (illegal volume name length)\n\n");
           		
           		    writeLog (log, "AMIGA DOS VOLUME NAME:  <not available> (illegal length - NDOS disk?)\n");
           	  
           	      hasInvalidName = true;
           	   }	
            }
            else
            { 
    	        /* BAM key valid; it can be safely assumed that the volume name is OK! */
    	        fprintf (stdout, "Processing ADOS volume:  %s\n\n", pImg->volName);
    	        writeLog (log, "AMIGA DOS VOLUME NAME:  '%s'", pImg->volName);
            }
        }
    
    } /* endif (if pImg->hasADOSRootBlk) */
    
    fprintf (stdout, "                                                                    ----------------------------------------------------------\n\n");
        
      /* filename inside ZIP archive (optional) */      		  
    if (inZIP->name != NULL)
    {
   	  char fn[MAXFILENAMELEN];
      splitpath (inZIP->name, NULL, fn);
      writeLog (log, "CRC32 CHECKSUM:                %08lX", inZIP->crc32);
      writeLog (log, "\nCHECKING FILE #%d INSIDE ZIP:   '%s'\n", (inZIP->pos)+1, fn);
    }
    else
    	writeLog (log, "CRC32 CHECKSUM:         %08lX\n\n", pImg->crc32);


    /* avoid nonsensical output of < 1760 "bad sectors" or "NDOS"-misdetection 
       of non-bootable or special ADOS disks,
       BUT only return to main program if the disk image is from a Quarterback backup set! 
       Note that DMS error checks must always be performed in any case:
       _regardless_ if the disk is of ADOS or NDOS type.
    */
                                   
    if (pImg->bootblktype == BLKTYPE_QBACKDISK)
    {
   	/* illegal volume = QB backup set*/
    	
    	fprintf (stderr, "CHECKING ABORTED - This disk is part of a Quarterback backup set and cannot be checked due to its special format.\n\n");
    	writeLog (log, "\nCHECKING ABORTED - This disk is part of a Quarterback backup set and cannot be checked due to its custom format.\n\n");
    	
    	fclose(log);
      strcat (logFilenameCopy, SUFFIX_QSET);          
      remove (logFilenameCopy);
      rename (logFilenameMain, logFilenameCopy);
      return false;         	  
    }
    else if ((pImg->bootblktype < 8) && (*(pImg->sec[0].blkbyte+3) & 1) > 0)
    {
    	 /* valid volume AND FFS detected */
       fprintf (stderr, "\nThis is a FFS (Fast File System) disk, which does not use any checksums for data blocks.\n\n");
       writeLog (log, "\nThis is a FFS (Fast File System) disk, which does not use any checksums for data blocks.\n");
       isFFS = true;          
       noDeepChk = true;
    }
    else
    {	               		      
       /* =================================================================================== */
     
       /* all other cases follow */            	            	
     	   
     	   /* debug */        
         /* printf ("bootprg is %081x; rootnum is %08lx", bootprg, rootnum); */
        
       /* ========== 																											  */
       /*  FIXME:    																											  */
       /* ==========
          Please consider this rather _preliminary_ code, which is subject to change!
          I know it's not very flexible at the moment, but WTH: it works! :)
       */
 
       /* ----------------------------------------------------------- */   		  	 
       /* test for BYTE BANDIT virus infection in block 0             */     
       /* -- THIS VIRUS TYPE DOES NOT MESS UP CHECKSUM!               */
       /* ----------------------------------------------------------- */
    
        uint8 virSigBuf[128];
        uint8 *v1buf = virSigBuf;
        uint16 q = 0;
        uint8 i = 0;
       		 
        /* reliable identical signature is on area 4th-16th longword (56 bytes wide)*/
        for (q = 0; q< 56; q++)
          (*v1buf++) = *(pImg->sec[0].blkbyte + 16 + q);
    
       /* clear bytes #11 + #22 before the memory comparison takes place, 
          because there are in fact three variants:          
          - "by Byte Bandit\\000in  9.87" 
          - "by Byte Bandit\\020in  9.87"
          - "by By4e Bandit\\020in  9.87" */
                 
       for (i = 1; i <= 2; i++)
          memset (virSigBuf + (i*11), 0x0, 1);
    
       if ((memcmp (virSigBuf, byteBdtSig, q)) == 0)
       {
       	 writeLog (log, "\n*********************************************************************************");
     	   writeLog (log, "\nVIRUS ALERT: Found Byte Bandit virus on block 0 - this disk is infected!\n");
         writeLog (log, "*********************************************************************************\n");     	   
         fprintf (stderr, "\nATTENTION: BYTE BANDIT VIRUS DETECTED ON BLOCK 0!\n\n");
         vircnt++;
       }
       
       v1buf = virSigBuf;  /* RESET! */       
       for (q = 0; q< 128; q++)
          (*v1buf++) = *(pImg->sec[0].blkbyte + 16 + q);
       if ((memcmp (virSigBuf, leviathanSig, q)) == 0)
       {
       	 writeLog (log, "\n*********************************************************************************");
     	   writeLog (log, "\nVIRUS ALERT: Found LEVIATHAN virus on block 0 - this disk is infected!\n");
         writeLog (log, "*********************************************************************************\n");
         fprintf (stderr, "\nATTENTION: LEVIATHAN VIRUS DETECTED ON BLOCK 0!\n\n");
         vircnt++;       	
       }	
       
       v1buf = virSigBuf;  /* RESET! */       
       for (q = 0; q< 128; q++)
          (*v1buf++) = *(pImg->sec[0].blkbyte + 32 + q);
       
       if ((memcmp (virSigBuf, sepulBBSig, q)) == 0)
       {
       	 writeLog (log, "\n*********************************************************************************");
     	   writeLog (log, "\nVIRUS ALERT: Found SEPULTURA virus on block 0 - this disk is infected!\n");
         writeLog (log, "*********************************************************************************\n");     	   
         fprintf (stderr, "\nATTENTION: SEPULTURA VIRUS DETECTED ON BLOCK 0!\n\n");
         vircnt++;
       }                           
       else if ((memcmp (virSigBuf, lilSvenSig, q-24)) == 0)
       {
       	 writeLog (log, "\n*********************************************************************************");
     	   writeLog (log, "\nVIRUS ALERT: Found LITTLE SVEN virus on block 0 - this disk is infected!\n");
         writeLog (log, "*********************************************************************************\n");     	   
         fprintf (stderr, "\nATTENTION: LITTLE SVEN VIRUS DETECTED ON BLOCK 0!\n\n");
         vircnt++;
       }      
       
       v1buf = virSigBuf;
       for (q = 0; q< 128; q++)
          (*v1buf++) = *(pImg->sec[0].blkbyte + 80 + q);    
       if ((memcmp (virSigBuf, byteWarSig, q)) == 0)
       {
     	   writeLog (log, "\n*********************************************************************************");
     	   writeLog (log, "\nVIRUS ALERT: Found Byte Warrior (DASA) virus on block 0 - this disk is infected!\n");
         writeLog (log, "*********************************************************************************\n");
         fprintf (stderr, "\nATTENTION: BYTE WARRIOR (DASA) VIRUS DETECTED ON BLOCK 0!\n\n");
         vircnt++;
       }       
       else if ((memcmp (virSigBuf, joshua1Sig, q)) == 0 || 
       	        (memcmp (virSigBuf, joshua2SOSig, q)) == 0)      
       {
       	 uint8 ver = ((memcmp (virSigBuf, joshua2SOSig, q)) == 0) ? 2: 1;       	 
       	 char * z = (ver == 2) ? " (Switchoff) " : " ";
       	 
       	 writeLog (log, "\n*********************************************************************************");
     	   writeLog (log, "\nVIRUS ALERT: Found JOSHUA %d%svirus on block 0 - this disk is infected!\n", ver, z);
         writeLog (log, "*********************************************************************************\n");
         fprintf (stderr, "\nATTENTION: JOSHUA %d%sVIRUS DETECTED ON BLOCK 0!\n\n", ver, z);
         vircnt++;
       }
       else if ((memcmp (virSigBuf, pentagonC1Sig, q)) == 0 || 
       	        (memcmp (virSigBuf, pentagonC2Sig, q)) == 0)
       {
       	 uint8 ver = ((memcmp (virSigBuf, pentagonC2Sig, q)) == 0) ? 2: 1;        
       	 
       	 writeLog (log, "\n*********************************************************************************");
     	   writeLog (log, "\nVIRUS ALERT: Found PENTAGON CIRCLE %d virus on block 0 - this disk is infected!\n", ver);
         writeLog (log, "*********************************************************************************\n");
         fprintf (stderr, "\nATTENTION: PENTAGON CIRCLE %d VIRUS DETECTED ON BLOCK 0!\n\n", ver);
         vircnt++;
       }
              
    }
    	
    if (rootnum == 0x00000370)
    {
    	/* rootblock = 0x370 */
    	
    	printBAMInfo(log, pImg->bamKey, 
    	                  bamChksumCleared, bamChksumInvalid, 
    	                  !bootChksumInvalid, pImg->hasADOSRootBlk);
    	
    	if (!bamKeyInvalid)
    	{
    		 /* BAM key OK */
    		 
           if (bootChksumInvalid) /* determined from checksum routine */
  	       {
  	        	/* invalid boot checksum, but correct BAM key! */
  	   	      fprintf (stdout, "This AmigaDOS disk has an invalid boot checksum and is not bootable.\n\n");
              writeLog (log, "\nThis is a true AmigaDOS disk with an invalid boot checksum and is therefore not bootable.");
           }
           else
           {	
           	 /* SPECIAL format! (used by Project-X et al) */
           	  
           	  /* check key 880 checksum */
  	   		    uint64 calcChksumRoot = computeChksum (&pImg->sec[ROOTBLOCK], 512, CHKSUM_TYPE_OTHER_L);                      	   		
  	   		    
  	   		    if (((getSingleSecDataLong(&pImg->sec[ROOTBLOCK], 5)) != calcChksumRoot) && pImg->hasADOSRootBlk)
  	   		    {             	   		    	 	            	   		 	             	   		 	   
  	   		 	     fprintf (stdout, "Found SPECIAL AmigaDOS/NDOS mixed format: Root block pointer on boot block is 0x370;\nVolume name, BAM checksum and BAM key are OK but the rest of the disk \nis actually NDOS with an incorrect checksum on the root block (%d).\n", ROOTBLOCK);
  	   		 	     fprintf (stdout, "This is a RARE format typically found on disks from games like PROJECT-X, PUFFY'S SAGA and some others.\n\n");
  	   		 	     fprintf (stdout, "The great part of this disk is NDOS, so it cannot be checked for errors.\n\n");
  	   		 	     writeLog (log, "\nFound SPECIAL AmigaDOS/NDOS mixed format: Root block pointer on boot block is 0x370;\nBAM checksum is OK but the rest of the disk actually consists of NDOS data and the\nchecksum on the root block (%d) is incorrect.\n", ROOTBLOCK);
                 writeLog (log, "This is a RARE format typically found on disks from games like PROJECT-X, PUFFY'S SAGA and some others.\n\n");
                 writeLog (log, "The great part of this disk is NDOS, so it cannot be checked for errors.\n\n");
                 noDeepChk = true;
                
              }                                              
          } 
      } 
      else if (!bootChksumInvalid)
      {                	
       	/* INCORRECT BAM key, but valid boot checksum = BOOTABLE */
       		
  	   	if (hasInvalidName)
  	   	{
  	   	   fprintf (stderr, "This is not an AmigaDOS disk. It appears to be a bootable NDOS disk with a custom (track) loader.\n");
           writeLog (log, "This is not an AmigaDOS disk. It has an invalid disk name, and appears to be\na bootable NDOS disk with a custom (track) loader.\n");
  	   		 noDeepChk = true;
  	   	}
  	   	else if (pImg->bamKey != -1)
  	   	{  
  	   		/* VALID boot checksum */           	   	                    
           fprintf (stdout, "Found valid AmigaDOS volume name: this could be a standard bootable AmigaDOS or mixed-format disk.\n");
           writeLog (log, "\nFound valid AmigaDOS volume name: this could be a standard bootable AmigaDOS or mixed-format disk.\n");
  	    }			
      }
      else
      {
      	 char btype[4] = "";         		 
				/* let's distinguish AmigaDOS + NDOS disk */
				if (bootChksumInvalid) strcpy (btype, "non-");

			  fprintf (stderr, "This disk looks like a %sbootable NDOS disk and cannot be in-depth checked (except for DMS errors).\n\n", btype);
				writeLog (log, "This disk looks like a %sbootable NDOS disk and cannot be in-depth checked (except for DMS errors).\n\n", btype);
      	
        noDeepChk = true;                     
      } 
    }  
    else
    {
    	 /* ================================================================== */
  	   /* non-existing (0x00), non-standard or illegal pointer to ROOT block */
  	   /* ================================================================== */
  	   
  	   /* case 1: root != @0x370 AND bootprg is non-zero = BOOTABLE NDOS */             	   
   	   /* case 2: root != @0x370 AND bootprg is zero = NOT BOOTABLE       */
  	
  	   if (bootprg != 0x00000000 && !bootChksumInvalid)
  	   { 
  	 	 /* = there was a boot program found AND root block pointer is non-standard;
  	 	      checksum is valid - disk is BOOTABLE                                   */
  	 	
  	  	/* first, we check the BAM key: if it's incorrect, testing everything else makes no sense! */
  	    /* Without BAM key, there is not the slightest idea of where the BAM is located on the disk! */ 	     	            	    
  	    
  	    printBAMInfo(log, pImg->bamKey,
  	                      bamChksumCleared, bamChksumInvalid, 
  	                      !bootChksumInvalid, pImg->hasADOSRootBlk);
  	              	              	      
  	      if (bamKeyInvalid)            	                  	      
  	      {	
  	      	/* bootable AND invalid BAM key */
  	        fprintf (stderr, "Unable to compute BAM checksum!\n");
  	        
  	        if (pImg->hasADOSRootBlk)
  	        {
  	        	   fprintf (stderr, "A valid ADOS disk name was found, but no valid BAM key!");
  	             writeLog (log, "Unable to compute BAM checksum: a valid ADOS name was found, but no valid BAM key!");
  	        }

  	        fprintf (stderr, "\nThis is a bootable NDOS disk, or an ADOS disk beyond ANY repair.\n\n");
  	        
  	        writeLog (log, "\nThis appears to be a bootable NDOS disk, or an ADOS disk beyond ANY repair.\n\n");
  	        noDeepChk = true;            	    
  	      }            	             	 	
  	                  	        
  	      /* SPECIAL CASE:
  	      We found a boot program AND the boot checksum is CORRECT, but the root 
  	      block entry is CLEARED. But it might be there anyway, so this must be checked -
  	      however, the conditions are rather complex! */
  	
  	    /* 1. test LW #0 (block) type of root block, must be 0x00000002
  	      2. test LW #127 (block) type of root block, must be 0x00000001
  	      3. test LW #3 (hash size) of root block, must be 0x00000048
  	      4. BAM key must be valid (between 3 and 1759)
  	      5. BAM checksum must be correct
  	      6. BAM secondary type must be 0xFFFFFFFD ( dec.: -3) */
  	 
  	   /* FIXME: this is likely to spawn an extra routine later, getDataBlkType()! */
     	   if (calcChksumBAM != 0 && !bamKeyInvalid && !bamChksumInvalid && pImg->hasADOSRootBlk)
     	   	{
     	   	  /* get key 880 checksum */ 
     	   	  /* (same as some lines above, but this time the case if root block pointer == 0!) */
     	       uint64 calcChksumRoot = computeChksum (&pImg->sec[ROOTBLOCK], 512, CHKSUM_TYPE_OTHER_L);
     	                    	         
     	   		  if ((getSingleSecDataLong(&pImg->sec[ROOTBLOCK], 5)) != calcChksumRoot)
     	   		  { 
     	   		     fprintf (stdout, "Found SPECIAL AmigaDOS/NDOS mixed format: Root block pointer on boot block is 0;");
     	   		     fprintf (stdout, "\nVolume name, BAM checksum and BAM key are OK but the rest of the disk \nis actually NDOS with an incorrect checksum on the root block (%d).\n", ROOTBLOCK);
     	   		 	   fprintf (stdout, "This is a RARE format typically found on disks from PROJECT-X, PUFFY'S SAGA and some other games.\n\n");
     	   		 	   fprintf (stdout, "The great part of this BOOTABLE disk is NDOS, so it cannot be checked for errors.\n\n");
     	   		 	   writeLog (log, "\nFound SPECIAL AmigaDOS/NDOS mixed format: Root block pointer on boot block is 0;\nBAM checksum is OK but the rest of the disk actually consists of NDOS data and the\nchecksum on the root block (%d) is incorrect.\n", ROOTBLOCK);
                 writeLog (log, "This is a RARE format typically found on disks from PROJECT-X, PUFFY'S SAGA and some other games.\n\n");
                 writeLog (log, "The great part of this BOOTABLE disk is NDOS, so it cannot be checked for errors.\n\n");
                 noDeepChk = true;
              }                                                 	                 	     
     	        else
     	        {
     	        	 fprintf (stdout, "Found SPECIAL AmigaDOS FORMAT: Root block pointer on boot block is 0, but the disk \nis a valid BOOTABLE AmigaDOS disk with a custom boot block.\n\n");
                 writeLog (log, "\nFound SPECIAL AmigaDOS FORMAT: Root block pointer on boot block is 0 (cleared),\nbut the disk is a valid BOOTABLE AmigaDOS disk with a custom boot block,", pImg->bamKey);
                 writeLog (log, "\nIt appears to be an AmigaDOS disk with a custom bootloader or boot menu.\n\n");                                     
     	        }
     	    }                                  
       }
       else
       {
       		char dtype[10];
       		/* let's distinguish AmigaDOS + NDOS disk */
       		if (pImg->hasADOSRootBlk) 
       			strcpy (dtype, "AmigaDOS");
       		else 
       		{
       			strcpy (dtype, "NDOS");
       		  noDeepChk = true;
       	  }                 		                 		
       		
       		/* first longword of bootprg is 0x00000000 */                 	  
       	                   	                   	  
       	  fprintf (stdout, "Boot program starts at 0 - this %s disk is not bootable. It might be a data disk.\n\n", dtype);
       	  writeLog (log, "Found no address greater than 0x00000000 to jump in the boot program: ");
       	  writeLog (log, "This %s disk is not bootable. It might be a data disk.\n", dtype);                    
       }  
    }
    
 	  if (type == IS_ADF_OVERDUMP)
     {
 		
 		 	 /* check for overdump */            	    	 
   	 	 writeLog (log, 
   	    	  "*****************************************************************************\
   	  \n  WARNING: This is an OVERDUMPED image file.\
   	    \n  Only the first %d bytes will be checked for errors!\
   	    	  \n*****************************************************************************\n",ADF_STANDARD_SIZE);
   	   fprintf (stderr, "\nThis is an OVERDUMPED image file. Only the first %d bytes will be checked! \n", ADF_STANDARD_SIZE);            	      
     }
    
    /* ===================== END OF LARGE TESTING ROUTINE BLOCKS 0 AND 1 ==================================== */
    
  if (pImg->hasADOSRootBlk)
  {
  	  /* SPECIAL case: SADDAM infection suspect */
      /* note this makes no sense for non-ADOS disks, so these should be excluded */
  	 if (pImg->hasValidBAMKeyOn80 == true)
     {
  	   printf ("BAM key backup found on longword NEXT to normal location;\ndisk might be infected with the SADDAM virus!\n\n");
 	     writeLog (log, "BAM key got cleared and backup found on longword NEXT to normal location;\ndisk might be (or previously have been) infected with the SADDAM virus!\n\n");
 	   }
 	
 	   if (pImg->suspectIRAK == true && !hasInvalidName)
 	   {  
 		   if (pImg->hasValidBAMKeyOn80 == false)
 		   {
 		 	   printf ("This disk is suspected to be infected with the SADDAM virus (unless it is NDOS)!\n\n");
 	       writeLog (log, "This disk is suspected to be infected with the SADDAM virus,\neven though the backed-up bitmap pointer was not found!");
 	       writeLog (log, "In worst case, the pointer was already deleted by the virus,\nand you might be forced to scan the disk manually to find the BAM key!\n\n");
 	     }
 	    vircnt++;
 	  }
  }
  else
  {    
    if (noDeepChk)
    {
    	 fprintf (stderr, "Sorry, but disks of this type cannot be in-depth checked (except for DMS errors).\n\n");
		   writeLog (log,   "Sorry, in-depth checks are not available with this disk type (except for DMS errors).\n");
		}        
  }	
  
  writeLog (log, "---------------------------------------------------------------------------------------------");
  
    /* ======================================================= */
    /* now let's process the rest of the disk! (blocks 2-1759) */
    /* ======================================================= */
    
  currBlk = 2;  /* simply pretend that block 1 is not there */
  while (currBlk < BLOCKSPERDISK)
  {
    /* check for checksum errors */
    
    /*first, get checksum from block offset:
      here it must be distinguished between boot and other blocks 
      Boot blocks have the checksum on longword #1 (second);
      BAM blocks have the checksum on longword #0 (first);
      other blocks have the checksum on longword #5 (sixth).
    */        
    
     /* get block type by checking first and last long word */    
     
     /* BUT: let's do the DMS error checking on the very beginning, since 
        we can skip ALL block checksum checking in this case. This may
        give quite a decent performance boost with large collections! */
     
     pchs = splitsector (currBlk);
     
     if (!chkDMSerr (&pImg->sec[currBlk]) && !noDeepChk)
     {	     
     	   uint8 chksumT = CHKSUM_TYPE_OTHER_L;
     	
         if (!bamKeyInvalid && currBlk == pImg->bamKey)
        	 chksumT = CHKSUM_TYPE_BAM_L;             

         blkType = getBlkType(&pImg->sec[currBlk], pSecProps[currBlk].blkMask, currBlk);                  
         blkChecksum = getSingleSecDataLong(&pImg->sec[currBlk], CHKSUM_TYPE_OTHER_L);
         
        /* now compare to the checksum that is calculated */               
         calcChksum = computeChksum (&pImg->sec[currBlk], 512, chksumT /*, NULL */);
              
      	 if (blkChecksum == 0x00000000) /*empty block */
      	 {
      	 	 	calcChksum = 0x00;
      	    /* writeLog (log, "Finished: block # %d - block checksum is %08lx - block appears EMPTY!", 
      	                    currBlk, blkChecksum);
      	    writeLog (log, "-------------------------------------------------------------------\n\n");
      	   */
      	 }
      	 else	
      	 {        	 	     			
         		/* yet it is not possible to say that it's a checksum error;
         		   we'll first have to check the bitmap flags which correspond to this block! */
  						
  					if (blkChecksum != calcChksum && currBlk > 1)
         		{ 		
         			/* checksum error: calculated and block checksums do not match! */       			  						  						
  						       			        			
  						if (pImg->bamKey != -1)
  						{       			        										
         				if (pSecProps->bitmapFlag[currBlk] == true && currBlk != 1)
           		  {             		  	            		  	         		  	 
           		  	 unsigned char virusstrbuf[16]; 
           		  	 int j;  												
           		  	 unsigned char *pbuf = virusstrbuf;
  
           		  	 /* -----------------------------------------------------------   */   		  	 
           		  	 /* test for LAMER and SADDAM (type I) virus infection in block   */
           		  	 /* (SADDAM type I: IRAK instead of $00000008 in data block       */
           		  	 /* NOTE: There is also a SADDAM which does NOT falsify checksum; */
           		  	 /* that means this will have to be tested elsewhere!             */
           		  	 /* ------------------------------------------------------------- */
           		  	 
           		  	 for (j = 0; j < sizeof(virusstrbuf); j++)
           		  	 {         		  	 	 
           		  	 	 (*pbuf++) = *(pImg->sec[currBlk].blkbyte+j);     		  	     
       		  	     }
           			   *pbuf = '\0'; /* without strcpy() end-of-string must be set manually! */
           			           		  	 
           		  	 if ((memcmp (LAMERV_STR1, virusstrbuf, sizeof(LAMERV_STR1)) == 0) ||
           		  	 	   (memcmp (LAMERV_STR2, virusstrbuf, sizeof(LAMERV_STR2)) == 0) ||
           		  	 	   (memcmp (LAMERV_STR3, virusstrbuf, sizeof(LAMERV_STR3)) == 0) ||
           		  	 	   (memcmp (SADDAMV_STR, virusstrbuf, sizeof(SADDAMV_STR)) == 0))
           		  	 {         		  	 	         		  	 	
           		  	 	
           		  	 	 /* LAMER */
           		  	 	  if ((memcmp (LAMERV_STR1, virusstrbuf, sizeof(LAMERV_STR1)) == 0) ||
           		  	 	  	  (memcmp (LAMERV_STR2, virusstrbuf, sizeof(LAMERV_STR2)) == 0) ||
           		  	 	      (memcmp (LAMERV_STR3, virusstrbuf, sizeof(LAMERV_STR3)) == 0))
           		  	 	  {
           		  	 	
           		  	 	    printf ("* Block %d: found CHECKSUM ERROR, caused by LAMER virus infection!\n", currBlk);
           		  	 	    writeLog (log, "Block # %d (cyl. %02d surface %d sector %02d) - CHECKSUM ERROR detected,\ncaused by LAMER virus infection - data is irreversibly lost!",
           		                  currBlk, pchs->cyl, pchs->hd, pchs->sec);
           		        }
           		        else
           		        {
           		          /* SADDAM */
           		  	 	    printf ("* Block %d: found CHECKSUM ERROR, caused by SADDAM virus infection!\n", currBlk);             		  	 	                             		  	 	
           		  	 	    writeLog (log, "Block # %d (cyl. %02d surface %d sector %02d) - CHECKSUM ERROR on data block detected,\ncaused by SADDAM virus infection - data must be decoded!",
           		                  currBlk, pchs->cyl, pchs->hd, pchs->sec);
           		          	
           		        }
           		               		      
           		        writeLog (log, "____________________________________________________________________\n");
           		      
           		        if (++vircnt == 1) 
           		        {
           		        	/* if multiple blocks were destroyed by LAMER or SADDAM, 
           		        	   do NOT repeatedly add suffix to filename              */
           		        	strcat (logFilenameCopy, SUFFIX_VIRU);
           		          hasSuffix = true;
           		        }        		      
           		     }
           		     else        	
           		     {		    
           		     	 uint8 msk = *(pSecProps[currBlk].blkMask);
           		     	 switch (blkType)
           		     	 {
           		     	   case BLKTYPE_CUST_FORMATT: 
           		     	   	/* do not write info about formatted blocks to screen */
           		     	 	  writeLog (log, "\nBlock # %d (cyl. %02d surface %d sector %02d) is formatted with 0 bytes.\n",
           		                     currBlk, pchs->cyl, pchs->hd, pchs->sec);
                       break;         		            
           		         case BLKTYPE_CUST_UNKNOWN:
           		     	 	  
           		     	 	  /* fprintf (stderr, "Block %d: Checksums do not match! \n", currBlk); */ 
           		     	 	  
           		     	 	  /* "checksum error" on block with no known AmigaDOS id is NOT to be worried about (in general) */
           		          if (currBlk != pImg->bamKey)
           		          {    
           		          	   /* FIXME: could maybe be forced by extra option later on. For now, I'll document this in the logs only
           		          	      to avoid an ugly screen flood for no obvious reason */
           		          	   /* printf ("Block # %d is in a format unknown to AmigaDOS or (un)formatted - don't worry about it.\n", currBlk); */
           		     	         writeLog (log, "Block # %d (cyl. %02d surface %d sector %02d) is in a checksum-less format UNKNOWN to AmigaDOS or (un)formatted",
           		                         currBlk, pchs->cyl, pchs->hd, pchs->sec);
           		     	    }
           		     	   break;
           		     	   case BLKTYPE_CUST_INVALID1:
           		     	   break;
           		     	   default:
           		     	   	 /* DATA block handling */
           		     	   	 /* Since this is fairly tricky, everything else has been prepared in getBlkType()
           		     	   	    outside the chkErr() function! 
           		     	   	 
           		     	   	 */
           		     	   	 /* SPECIAL (DANGEROUS) CASE that nextData points to SAME (current) block!! */
           		     	   	 /* User must be warned to prevent the worst, e. g. in WB use.              */           		     	   	            		     	   	 
           		     	   	 
           		     	   	 if ((msk & 8) > 0)
           		     	   	 {
           		     	   	   printf ("__________________________________________________________________________________________________________\n");
           		       	     writeLog (log, "\nBlock # %d - %s - (cyl. %02d surface %d sector %02d):\nATTENTION: Found 'nextData' block which points back to current block - disk may give unpredictable results!!\n", 
           		                    currBlk, BLK_TYPES[blkType], pchs->cyl, pchs->hd, pchs->sec);
           		             printf ("\n* Block # %d: ATTENTION: 'NextData' points back to current block - disk may give unpredictable results!!\n", currBlk);
           		             printf ("__________________________________________________________________________________________________________\n\n");
           		             writeLog (log, "---------------------------------------------------------------------------------------------");
           		             
           		            errcnt++; 
           		           }  
           		     	   	 
           		     	   	 if (msk == 7)
           		     	   	 {
           		     	   	   printf ("__________________________________________________________________________________________________________\n");
           		       	     writeLog (log, "\nBlock # %d - %s - (cyl. %02d surface %d sector %02d):\nATTENTION: Neither the sequence number, nor the valid data byte, nor the \n'NextData' pointer are valid - This DATA block is badly damaged!!\n", 
           		                    currBlk, BLK_TYPES[blkType], pchs->cyl, pchs->hd, pchs->sec);
           		             printf ("\n* Block # %d:   ATTENTION: Neither the sequence number, nor the valid data byte, nor the \n                'NextData' pointer are valid- This DATA block is badly damaged!!\n", currBlk);
           		             printf ("__________________________________________________________________________________________________________\n\n");
           		             writeLog (log, "---------------------------------------------------------------------------------------------");
           		             
           		            errcnt++; 
           		           } 
           		           else
           		           {
           		           	 /* Cases of single or double damage: combinations of bad sequence number AND bad nextData block, etc.
           		           	 */
           		             /* bad seq num */
           		             if ((msk & 1) > 0)
           		             {	
           		               printf ("__________________________________________________________________________________________________________\n");
           		       	       writeLog (log, "\nBlock # %d - %s - (cyl. %02d surface %d sector %02d):\nERROR: Bad sequence number on block (i. e. less than 2 or more than %d);\nRead errors are likely to occur when copying files!\n",
           		                    currBlk, BLK_TYPES[blkType], pchs->cyl, pchs->hd, pchs->sec, BLOCKSPERDISK-1);
           		               printf ("\n* Block # %d: ERROR: Bad sequence number on block (i. e. less than 2 or more than %d);\nRead errors are likely to occur!!\n", currBlk, BLOCKSPERDISK-1);
           		               printf ("__________________________________________________________________________________________________________\n\n");
           		               writeLog (log, "---------------------------------------------------------------------------------------------");
           		             
           		               errcnt++; 
           		             } 
           		             if ((msk & 2) > 0)
           		             {	
           		               printf ("__________________________________________________________________________________________________________\n");
           		       	       writeLog (log, "\nBlock # %d - %s - (cyl. %02d surface %d sector %02d):\nERROR: Bad amount of valid data words in block (must be in range 0x02-0x1E8)!\n",
           		                    currBlk, BLK_TYPES[blkType], pchs->cyl, pchs->hd, pchs->sec);
           		               printf ("\n* Block # %d: ERROR: Bad amount of valid data words in block (must be in range 0x02-0x1E8)!\n", currBlk);
           		               printf ("__________________________________________________________________________________________________________\n\n");
           		               writeLog (log, "---------------------------------------------------------------------------------------------");
           		             
           		               errcnt++; 
           		             }
           		             if (((msk & 4) > 0) && ((msk & 8) == 0))
           		             {	
           		               printf ("__________________________________________________________________________________________________________\n");
           		       	       writeLog (log, "\nBlock # %d - %s - (cyl. %02d surface %d sector %02d):\nERROR: Bad pointer to next data block! (can be in range 2-%d or 0)\nRead errors are likely to occur when copying files!\n",
           		                    currBlk, BLK_TYPES[blkType], pchs->cyl, pchs->hd, pchs->sec, BLOCKSPERDISK-1);
           		               printf ("\n* Block # %d: ERROR: Bad pointer to next data block! (can be in range 2-%d or 0)\n  Read errors are likely to occur when copying files!\n", currBlk, BLOCKSPERDISK-1);
           		               printf ("__________________________________________________________________________________________________________\n\n");
           		               writeLog (log, "---------------------------------------------------------------------------------------------");
           		             
           		               errcnt++; 
           		             }
           		             
           		           }

           		     	   	  printf ("____________________________________________________________________\n"); 	
           		       	    writeLog (log, "\nBlock # %d - %s - (cyl. %02d surface %d sector %02d) - CHECKSUM ERROR detected!", 
           		                     currBlk, BLK_TYPES[blkType], pchs->cyl, pchs->hd, pchs->sec);
           		            printf ("\n* Found CHECKSUM ERROR on block %d! \n", currBlk);
           		            printf ("____________________________________________________________________\n\n");
           		            writeLog (log, "---------------------------------------------------------------------------------------------");
           		     	      errcnt++;
           		     	      
           		     	   break;         		     	   
           		     	 } /* endswitch */  		     	          		     	    
           		     }         		             		             		   
           		    
           		  }
           		  else if (currBlk > 1)   /* free/used flags only work from block 2ff. */
           		  {
           		  	/* do not flood the screen with this stuff in batch mode */
           		  	
           		    if (argcnt <= 2) 
           		    {
           		       printf ("-----------------------------------\n");
           			     printf ("Block # %d has invalid checksum, but is not used.\n", currBlk);
           	         printf ("-----------------------------------\n"); 
           	      }
           	          
           	      writeLog (log, "Block # %d (cyl. %02d surface %d sector %02d) has invalid checksum, but is not used.", 
           	                currBlk, pchs->cyl, pchs->hd, pchs->sec);
           	    }         		         		
           	  }
         		 
         		}
         		else
         		{
         			/* ----------------------- */
         			/* block checksum CORRECT! */
         			/* ----------------------- */
         			
         			/* However, this could also mean a virus! 
         			   SADDAM (type II) does NOT falsify checksum, so the test far above had to be postponed!
         		  */
         		  
    uint8 virSigBuf1[32];
    uint8 virSigBuf2[12]; 		 
    uint8 virSigBuf3[66];
    
    uint8 *v1buf = virSigBuf1;
    uint8 *v2buf = virSigBuf2;
    uint8 *v3buf = virSigBuf3;
      
    
    /* uint16 saddDVHdrKey = 0; */
    uint16 q = 0;
    
           	  /* ----------------------------------------------------------- */   		  	 
           		/* test for SADDAM (type II) virus infection in block          */
           		/* (SADDAM type II: fake Disk-Validator, called «««Saddam»»»   */
           		/* -- THIS VIRUS SUBTYPE KEEPS CHECKSUM INTACT!                   */
           		/* ----------------------------------------------------------- */         		 
           		 
           		 /* once found, we can safely believe there is a false disk-validator ;-) */
           		 if (!hasCheckedBogusDV)
           		 {         		    
           		          		 
           		    /* reliable identical signature (1st part) is on area 8th-15th longword (32 bytes wide)*/
           		    for (q = 0; q< 32; q++)         		 
           		    	(*v1buf++) = *(pImg->sec[currBlk].blkbyte + 32 + q);   
           		          		 
           		    *v1buf = '\0';		          		 
           		    /* 2nd (shorter) part is on 19th-21st longword */
           		 
           		    for (q = 0; q< 12; q++)
           		      (*v2buf++) = *(pImg->sec[currBlk].blkbyte + 76 + q);
           		  	 
           		      *v2buf = '\0';
           		  
           		 			/* do NOT use strcmp() here since we need to be "sign-sensitive" here! */             		    	
           		  
           		      if ((memcmp (virSigBuf1, irakDataFirst , sizeof(irakDataFirst)) == 0) &&
           		 	        (memcmp (virSigBuf2, irakDataSecond, sizeof(irakDataSecond)) == 0))
           		 			 
           		 	    {
           		 	      /* SADDAM disk validator on disk!! */
           		 	  
        		     	   	 printf ("____________________________________________________________________\n"); 	
           		 	       printf ("\nBlock # %d: VIRUS ALERT: Found bogus disk-validator in L - \n this disk is infected with the SADDAM virus!\n", currBlk);
           		         printf ("____________________________________________________________________\n\n"); 	
           		         writeLog (log, "\nBlock # %d (cyl. %02d surface %d sector %02d): ATTENTION: Found bogus disk-validator in L directory - \nThis disk is infected with the SADDAM virus!\n",
           		                   currBlk, pchs->cyl, pchs->hd, pchs->sec);
           		 	  
           		 	       hasCheckedBogusDV = true;
           		 	       vircnt++;
           		 	    } 
         			 }
         			 
         			 /* test for jeff v1.31 */
         			    
         			   if (!hasCheckedJeffOld)
         			   {
         			   	 for (q = 0; q< 64; q++)         		 
           		    	(*v3buf++) = *(pImg->sec[currBlk].blkbyte + 32 + q);
           		          		 
           		      *v3buf = '\0';		
         			                
                   if (memcmp (virSigBuf3, jeffSig_old, 64) == 0)
                 	 {
                   	 	fprintf (stderr, "______________________________________________________________________________________________\n"); 	
                   	 	fprintf (stderr, "\nBlock # %d: VIRUS ALERT: Found Jeff/Butonic v1.31 in root directory - this disk is infected!\n", currBlk);
           		        fprintf (stderr, "______________________________________________________________________________________________\n\n"); 	
           		         writeLog (log, "\nBlock # %d (cyl. %02d surface %d sector %02d): VIRUS ALERT: Found Jeff/Butonic v1.31 in root directory - this disk is infected!\n",
           		                   currBlk, pchs->cyl, pchs->hd, pchs->sec);
           		 	  
           		 	      hasCheckedJeffOld = true;
                     vircnt++;
                   }                                
                 }

                 v3buf = virSigBuf3; /* RESET! */
                 
         			   if (!hasCheckedJeffNew)
         			   {
         			   	 for (q = 0; q< 128; q++)         		 
           		    	(*v3buf++) = *(pImg->sec[currBlk].blkbyte + 32 + q);
           		          		 
           		      *v3buf = '\0';		
         			                
                   if (memcmp (virSigBuf3, jeffSig_new, q) == 0)
                 	 {
                 	  	fprintf (stderr, "______________________________________________________________________________________________\n"); 	
                 	  	fprintf (stderr, "\nBlock # %d: VIRUS ALERT: Found Jeff/Butonic v3.10 in root directory - this disk is infected!\n", currBlk);
           		        fprintf (stderr, "______________________________________________________________________________________________\n\n"); 	
           		        writeLog (log, "\nBlock # %d (cyl. %02d surface %d sector %02d): VIRUS ALERT: Found Jeff/Butonic v3.10 in root directory - this disk is infected!\n",
           		                   currBlk, pchs->cyl, pchs->hd, pchs->sec);
           		 	  
           		 	      hasCheckedJeffNew = true;
                  	  vircnt++;
                   }                                
                 }
                 
                 v3buf = virSigBuf3; /* RESET! */
                 
                 if (!hasCheckedBratYawns)
         			   {
         			   	 for (q = 0; q< 112; q++)         		 
           		    	(*v3buf++) = *(pImg->sec[currBlk].blkbyte + 32 + q);
           		          		 
           		      *v3buf = '\0';		
         			                
                   if (memcmp (virSigBuf3, bratYawnsSig, q) == 0)
                 	 {
                 	  	fprintf (stderr, "______________________________________________________________________________________________\n"); 	
                 	  	fprintf (stderr, "\nBlock # %d: VIRUS ALERT: Found Bret Hawnes file virus in root directory - this disk is infected!\n", currBlk);
           		        fprintf (stderr, "______________________________________________________________________________________________\n\n"); 	
           		        writeLog (log, "\nBlock # %d (cyl. %02d surface %d sector %02d): VIRUS ALERT: Found Bret Hawnes file virus in root directory - this disk is infected!\n",
           		                   currBlk, pchs->cyl, pchs->hd, pchs->sec);
           		 	  
           		 	      hasCheckedBratYawns = true;
                  	  vircnt++;
                   }                                
                 }
                 
                 v3buf = virSigBuf3; /* RESET! */
                 
                 if (!hasCheckedEleni)
         			   {
         			   	 for (q = 0; q< 80; q++)         		 
           		    	(*v3buf++) = *(pImg->sec[currBlk].blkbyte + 80 + q);
           		          		 
           		      *v3buf = '\0';		
         			                
                   if (memcmp (virSigBuf3, eleni1Sig, q) == 0)
                 	 {
                 	  	fprintf (stderr, "______________________________________________________________________________________________\n"); 	
                 	  	fprintf (stderr, "\nBlock # %d: VIRUS ALERT: Found ELENI 1 (MessAngel-B) file virus in root directory - this disk is infected!\n", currBlk);
           		        fprintf (stderr, "______________________________________________________________________________________________\n\n"); 	
           		        writeLog (log, "\nBlock # %d (cyl. %02d surface %d sector %02d): VIRUS ALERT: Found ELENI 1 (MessAngel-B) file virus in root directory - this disk is infected!\n",
           		                   currBlk, pchs->cyl, pchs->hd, pchs->sec);
           		 	  
           		 	      hasCheckedEleni = true;
                  	  vircnt++;
                   }                                
                 }
                 
                 		

                 
         			 /*	if (pSecProps->bitmapFlag[currBlk] == false)       				    
         				 printf ("[DEBUG] Free entry found on block %d - Block OK\n", currBlk); */       				   			
         		   /* printf ("Block %d is OK!\n", currBlk); */
         		   /* writeLog (log, "Finished: Block # %d - block is OK! (sum = %08lx)", currBlk, blkChecksum);  */
         		}         		       
         
         } /* endif (if blkChecksum == 0x00000000) */
     }
     else
     {
     	  if (chkDMSerr (&pImg->sec[currBlk]))
      	{ 
      		/*DMS error on block detected! (note these checks are independent of DOS type!) */
      	      	
      	   fprintf (stderr, "* Block %d: DMS ERROR detected - probably caused by a bad source disk.\n", currBlk);
           writeLog (log, "Block # %d (cyl. %02d surface %d sector %02d): DMS ERROR detected - probably caused by a bad source disk.",
                     currBlk, pchs->cyl, pchs->hd, pchs->sec);     	      	              	     
           if (pchs->sec == 10) writeLog (log, ""); /* implicit line feed */
           	
           if (!hasSuffix)
           { 
       	     strcat (logFilenameCopy, SUFFIX_DERR);   /* append "DMSERR" suffix  */
       	     hasSuffix = true;       	     
           }	
        }   
     }	
    
    free(pchs); 
      	
    if ((argcnt <= 2) && errcnt > 0 && errcnt % 22 == 0) waitKey();
        
    currBlk++;
     
  } /* ===================== */
    /* endwhile (huge loop)  */
    /* ===================== */
  
  if (!hasSuffix)
  {   	  	   	 
    char strSuffix[16];
    
  	 if (errcnt > 0 || vircnt > 0)    
     {  		                  
        if (vircnt > 0)
        	strcpy (strSuffix, SUFFIX_VIRU);        /* set "VIRUS" suffix */
        else
        	strcpy (strSuffix, SUFFIX_ERRO);        /* set "ERR" suffix */
        	       
       fprintf (stderr, "\nPlease see log file '%s%s' for details.\n\n", logFilenameCopy, strSuffix);
     }
     else if (noDeepChk)
  	 {
  	 	 if (isFFS) 
  	 	 	 strcpy (strSuffix, SUFFIX_FAST);        /* set "FFS" suffix */
  	 	 else
  	 	 	 strcpy (strSuffix, SUFFIX_NDOS);        /* set "NDOS" suffix */
  	 } 	     
     else
     {
  		 fprintf (stdout, "\nThere were no errors found on this disk!\n\n--------------------------------------- \n\n");
       writeLog (log, "\nThis disk is OK - there were no errors found.\n");
       /* before we rename the file, we must close it first! */
       fclose(log);
       
       strcpy (strSuffix, SUFFIX_ISOK);        /* set "OK" suffix */
     }
  
    strcat (logFilenameCopy, strSuffix);      /* now append preset suffix */
  
    hasSuffix = true;
  }
   
 fclose(log); 
 remove (logFilenameCopy);
 rename (logFilenameMain, logFilenameCopy);

 return ((errcnt > 0) ? true : false);
}
/* ============================== */
/* END of huge routine chkErr()   */
/* ============================== */



uint64 computeChksum (singleSecS_t* secdata,
                      const uint16 len,
									    const uint8  chksumTypeL /* , FILE *dbg_out */)
{		
	uint64 oldsum = 0, sum = 0;
	uint64 precarrysum = 0;
  uint64 longword = 0;
  int i;

  /* unknown chksum type! Hence we cannot clear the old checksum because
	   we do not know where to look! */
	if (!(chksumTypeL == CHKSUM_TYPE_BOOT_L || 
		    chksumTypeL == CHKSUM_TYPE_BAM_L  ||
		    chksumTypeL == CHKSUM_TYPE_OTHER_L))		    
	return 0;

  oldsum = *(secdata->blkword+chksumTypeL);
	
	for(i=0; i < len/(sizeof(uint64)); i++) 
	{       
    if (chksumTypeL == i)
    	 i++; /* simply skip check of longword indicated by value in chksumTypeL
    	         (contains checksum itself; this avoids cumbersome memset() operation) */
   
    longword =  ((*(secdata->blkbyte+i*4+0) << 24) | 
    					   (*(secdata->blkbyte+i*4+1) << 16) | 
    					   (*(secdata->blkbyte+i*4+2) << 8)  | 
    					   (*(secdata->blkbyte+i*4+3)));

      /* log to file for debugging purposes */
     
      /* if ((longword > 0x00000000) && (dbg_out != NULL))
        	 writeLog (dbg_out, "\nChecksum before add of non-zero LW (0x%08lx) = %08lx\n", longword, sum);
      */
      
     if (chksumTypeL == CHKSUM_TYPE_BOOT_L)
     	 precarrysum = sum;
     
     sum = sum + longword;

		 /* log to file */
		 
		 if (chksumTypeL == CHKSUM_TYPE_BOOT_L)
		 { 
		 	  /* only use carry algorithm for boot block! */
		 	
		 		/* if (longword > 0x00000000) */
      		/*   writeLog (dbg_out, "Pre-carry sum now = %08lx  - Checksum now = %08lx\n", precarrysum, sum); */
  
    		if (sum < precarrysum)
    		{ 
    		   ++sum; /* add carry (boot block only!) */	
				   /* if (dbg_out != NULL) writeLog (dbg_out, "GOT CARRY - adding to temporary checksum\n"); */
    		}
     
     }		
      
   }  /* endfor */

/* debug stuff */
 
 /* writeLog (dbg_out, "Checksum value after exiting loop: %08x\n or %08x\n", sum); */
 
  
 if (chksumTypeL == CHKSUM_TYPE_BOOT_L)
 sum = ~sum;        /* 2's complement */
 else sum = -sum  ; /* true negation (not necessarily same as 2's complement!) */
 
/* debug stuff */
/*  writeLog (dbg_out, "Negating checksum value - final value is: %08x\n", sum); */
 
  return sum;
}

chsS_t* splitsector (uint16 sec)
{
	/* get cyl, surface (head) and sector */
	
	chsS_t* chsS = malloc (sizeof(chsS_t));
	
	chsS->trk = sec / 11;
	chsS->cyl = chsS->trk / 2;
	chsS->hd  = chsS->trk % 2;
	chsS->sec = sec % 11;

  return chsS; 
}


void printBAMInfo (FILE* f, int16 key, 
									 bool isCleared, bool isInvalid, 
									 bool isBootable, bool hasADOSRoot)
{
   if (key != -1 && hasADOSRoot)
   {   	    	
     	fprintf (stdout, "BAM found at block %d", key);
     	
     	if (isCleared)
       { 
       	 /* special case sum == 0 */
         fprintf (stderr, "\nBAM checksum is 0 - the BAM on this disk is non-existing or has been cleared!\n\n");
         writeLog (f, "BAM checksum on this disk is 0. The bitmap is either non-existing\nor has been cleared in some way.\n");
       }
       else
       {       			   
  		   if (!isInvalid)
   		   {
   	  	  	fprintf (stdout, " - BAM checksum OK\n\n");
	    	  	writeLog (f, "BAM found at block %d - BAM checksum OK\n", key);
   		   }
   		   else
   		   {	     			
     			fprintf (stderr, "\n\nBAM CHECKSUM ERROR DETECTED!\n\n");
     			
     			writeLog (f, "BAM found at block %d\n\nBAM CHECKSUM ERROR detected!\n\n", key);     			
   		   
   		     if (hasADOSRoot)
           {
             fprintf (stderr, "However, a valid AmigaDOS disk name was found. Checking anyway ... \n\n");
             writeLog (f, "However, a valid AmigaDOS disk name was found. Checking anyway ... \n");
           }
           else
           {
         	  /* name is not AmigaDOS-compliant AND BAM is zero */
             fprintf (stderr, "This appears to be a NDOS disk...in-depth error checking is not possible.\n");
             writeLog (f, "This appears to be an NDOS disk...in-depth error checking is not possible.\n");
            	           	          
           }   		   
   		   }   		      		   	            		   
   		 }     		    	
   	   		 	 
   }
   else
   {
   	 /* illegal BAM key AND incorrect BAM checksum */   	 
   	 
	  	 fprintf (stderr, "Neither a valid BAM key nor valid BAM data could be found on disk.\nThis disk appears to be a ");
	  	 writeLog (f, "\nNeither a valid BAM key nor valid BAM data could be found on disk.");
	  	 
	  	 if (isBootable)
	  	 {
	  	 		fprintf (stderr, "bootable NDOS or mixed-format disk.\n");
	  	    writeLog (f, "This disk appears to be a bootable NDOS or mixed-format disk. Further checking in progress...\n");
	  	 }     
	  	 else
	  	 {
	  	    fprintf (stderr, "non-bootable NDOS or mixed-format disk; probably a data disk.\n");	  	    
	  	    writeLog (f, "This disk appears to be a non-bootable NDOS or mixed-format disk; probably a data disk.");
	  	    writeLog (f, "Further checking in progress...\n");
	  	 }	  	 	  	
	    
	    fprintf (stderr, "Further checking in progress...\n\n");
	  	  
   }			 

}
 
uint8 getBlkType(singleSecS_t* secdata, uint8 *mask, uint16 n)
{
	/* Determine block type by reading the first and last long word 
	   and return a single custom value                             */
	 uint64 priType = getSingleSecDataLong(secdata, 0);
	 uint64 secType = getSingleSecDataLong(secdata, 127);
	   
	switch (priType)
  {
    
   
    case BLKTYPE_PRI_UNUSED:      /* 0x00 */
      /*  printf ("This block appears to be empty or formatted."); */
      return BLKTYPE_CUST_FORMATT;

    break;
    case BLKTYPE_PRI_STRUCTINIT:      /* 0x02 */

   /* The 0x02 type can stand for a lot of block types.
      Eventually determined by checking the block's last long word, "Sec.Type".

      sectype = -3 (0xfffffffd) AND primary type = 0x02: FILE HEADER
      sectype = -3 (0xfffffffd) AND primary type = 0x10: FILE-LIST
      sectype =  1 (0x01) : ROOT BLOCK
      sectype =  2 (0x02) : USER DIR

    */
          switch (secType)
          {      
            case BLKTYPE_SEC_FILEHDRLIST:  /* 0xfffd (-3) */                                                        
               return BLKTYPE_CUST_FILEHDR; 
             break;                          
           case BLKTYPE_SEC_ROOT:                           
               /*    printf ("This is the root block."); */
             return BLKTYPE_CUST_ROOTBLK;
           break;
         
           case BLKTYPE_SEC_USERDIR:                                            
             /* strcpy(strLog, "[DEBUG] Block %08x is a user directory; '%s' \n"); */
             return BLKTYPE_CUST_USERDIR;          
            break;
          } /* endswitch (inner) */
       
      break;
       
      case BLKTYPE_PRI_FILELIST:        /* 0x10 */
        if (secType == BLKTYPE_SEC_FILEHDRLIST)  /* 0xfffffffd (-3) */
         { 
           return BLKTYPE_CUST_FILELST;
         }
         else
         	 return BLKTYPE_CUST_INVALID1;
      break;
      
      case BLKTYPE_PRI_DATA:
      {
         /* printf ("This appears to be a DATA block"); */
            
        /* NOTE: DATA blocks are the only blocks to not use a secondary type, so it could be
           an alien format which coincidentally has a 0x00000008 at the first longword (#0).
           This must not be a criteria for "is a data block", though. 
         */
         uint8  valMask   = 0; 
         uint64 hdrKey    = getSingleSecDataLong (secdata, 1); /* 0x01 < hdrKey    <  0x6e0             */
         uint64 seqNum    = getSingleSecDataLong (secdata, 2); /* 0x01 < seqNum    <  0x6e0             */
         uint64 validData = getSingleSecDataLong (secdata, 3); /* 0x00 < validData <= 0x1e8             */
         
         /* 0x00 is allowed here, for it denotes 'no next data block'! */
         uint64 nextData  = getSingleSecDataLong (secdata, 4); /* 0x00 <= seqNum < 0x6e0 && seqNum != 1 */         
         
         /* Tricky! */ 
         /* We have to do a lot of distinguishing here, because we should be able to find
            a bad sequence number in a data block! If we only meet 100% of the criteria
            for a valid data block, every other type would be set to UNKNOWN!
            (Checksum errors are NOT reported with blocks of the UNKNOWN type!)
         */
         
         /* LSB */
         bool hasValidHdrKey = ((hdrKey    == (hdrKey    & 0x0FFFFF))  && (hdrKey   > 0x01  && hdrKey    <  0x06e0));
                  
                 
         /* bit 0 (LSB): valid seq number? */
         bool hasBadSeqNum = !((seqNum    == (seqNum    & 0x0FFFFF))  && (seqNum   > 0x01  && seqNum    <  0x06e0));         
         /* bit 1 */
         bool hasBadValDta = !((validData == (validData & 0x0FFFFF))  && (validData > 0x00 && validData <= 0x01e8));         
         /* bit 3 (MSB) */
         /* DANGER: does next data point to to CURRENT sequence block?! (this should never happen!) */
         bool nextDtaPtsToCurrBlk = (nextData == seqNum);
         
         /* bit 2 */
         bool hasBadNxtDta = !(
                               (nextData == (nextData & 0x0FFFFF))  && 
                               (nextData > 0x00 && nextData < 0x06e0) &&
                               (!nextDtaPtsToCurrBlk)
                              );
         
        /* if (n == 45) fprintf (stderr, "\nhasbadseq = %d    seq = %08lx\n\nhasbadvaldata = %d   valdata = %08lx\n\nhasbadnextdata = %d   nextdata = %08lx\n", 
         	                             hasBadSeqNum, seqNum, hasBadValDta, validData, hasBadNxtDta, nextData);
        */ 
         
         /* ----------------------------------------------------------------- */
         /* Header key beyond the 2..1759 boundary? Can't do anything, sorry! */
         /* ----------------------------------------------------------------- */         	
         
         valMask |= ((nextDtaPtsToCurrBlk << 3) | (hasBadNxtDta << 2) | (hasBadValDta << 1) | hasBadSeqNum);         
         *mask = valMask;
         
         return ((hasValidHdrKey) ? BLKTYPE_CUST_DATABLK_ALL_OK : BLKTYPE_CUST_UNKNOWN);         

      }    
      break;
      
      default:      	 
      	 	/* printf ("[DEBUG] This block is of an unknown type\n"); */
      break;
  } /* endswitch (outer) */
 
	return BLKTYPE_CUST_UNKNOWN; /* 0 */
}    
          
void chkBAMKey (dskImgS_t* dptr, dskSecProps_t* bMap)
{	 
	 /* ---------------------------------------------*/
   /* check BAM key                                 */  
   /* ---------------------------------------------*/
  
  uint64 lword79 = getSingleSecDataLong(&dptr->sec[ROOTBLOCK], 79);

  dptr->hasValidBAMKeyOn80 = false;
  dptr->suspectIRAK = false;				     	    				        
  
  /* debug */ 
  /* printf ("BAM points to block %d", (int16)lword79); */

  if (!(dptr->hasADOSRootBlk))
  {
  	 /* no ADOS root block? 
     ANY further testing would be pointless! Bitmap ptr will be hardcoded to -1 (invalid). */
     dptr->bamKey = -1;
  }
  else   
  {
      if (lword79 == 0x00000000)
      {
       int16 bamKeyBackup;
    
     /* bamkey = 0x0 ! */
     /* Could be an indication of SADDAM virus infection!
        SADDAM zeroes the longword containing the BAM block number 
        and NORMALLY (!) stores the original key in the LW next to it. */
      
       bamKeyBackup = getSingleSecDataLong(&dptr->sec[ROOTBLOCK], 80);   
     
       if (bamKeyBackup > 0x01 && bamKeyBackup < 0x6e0)	 
       {
       	  dptr->bamKey = bamKeyBackup;
     	    evalBitmap (&dptr->sec[dptr->bamKey], bMap);
          dptr->hasValidBAMKeyOn80 = true;
       }
       else
     	    dptr->bamKey = -1;
     	  
     /* in any case, assume SADDAM infection! */				          
     /* note: we need TWO flags here in case that SADDAM did NOT
        bother to backup the bitmap pointer in $80! */
          
       dptr->suspectIRAK = true;
      }
      else
      {
        if (lword79 == (lword79 & 0x0FFFFF) && 
    	 	  (lword79 > 0x01 && lword79 < 0x06e0)) /* first two bytes + 1 nibble MUST be null */
    	    
    	  {
    	  	 dptr->bamKey = (int16) lword79;
    	  	 evalBitmap (&dptr->sec[dptr->bamKey], bMap);
    	  }	 
    	  else   
          dptr->bamKey = -1;  /* invalid BAM key */    
          				          
        /* now let's evaluate type of bootblock */         
    
      /* NOTE: This does NOT yet give a reliable information about the disk type!
      It is merely a rough idea about the type of disk. */				     
      }
  }	   	  
}  /* end of function chkBamKey() */

bool isADOSRoot (singleSecS_t* secdata)
{
	/* Checks for true ADOS root block, which must underlie 
	   the following characteristics:  
	   primary type MUST be $00000002
	   secondary type MUST be $00000001
	   long word 03 must be $48 (constant value, though non-constant on hard disks)
	   
	  NB: Checking for "non-cryptic" ASCII disk name would not work at all, since
	  scene releases in particular DO use a lot of special characters (hearts, diamonds, ...)
	*/	
	return (getSingleSecDataLong (secdata, 0)   == 0x00000002 &&
	        getSingleSecDataLong (secdata, 3)   == 0x00000048 &&
	        getSingleSecDataLong (secdata, 127) == 0x00000001 &&
	        hasADOSBitmap (secdata)); 	        
}

bool hasADOSBitmap (singleSecS_t* secdata)
{
	/* Common values are: 
	   00000000 (invalid, can't write to disk)
	   00000001 ( 1)  OK (rare)
	   FFFFFFFF (-1) 	OK
	*/
	
	uint64 bitmapFlag =  getSingleSecDataLong (secdata, 78);
	return (bitmapFlag == 0xFFFFFFFF || bitmapFlag == 0x00000000 || bitmapFlag == 0x00000001);
}

bool chkDMSerr (singleSecS_t* secdata /* , uint16 blknum */)
{
  uint8 dmsERRbuf[sizeof(DMSERR_STR)-1];
  int j;  												
  uint8 *pdmsERRbuf = dmsERRbuf;

	for (j = 0; j < sizeof(dmsERRbuf); j++)
	{         		  	 	 
 	  (*pdmsERRbuf++) = *(secdata->blkbyte+j);   
  }

  return (memcmp (DMSERR_STR, dmsERRbuf, sizeof(DMSERR_STR)-1) == 0);  
}
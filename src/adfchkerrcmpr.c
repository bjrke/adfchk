#include "adfchkerrcmpr.h"

void chkCompressedADF (dskImgS_t* d, FILE** mainLog, inZIP_t* inArc, uint8 flag)
{
 /* All the checking in a nutshell for in-archive operation. */
 bool iError = false;
 uint8 len = 0;
 dskSecProps_t* bMap = malloc (BLOCKSPERDISK * sizeof(dskSecProps_t));
   	 
	/* check disk image in buffer for checksum errors */
  
  /* first, get length of name */
  len = getSecDataByte(d, ROOTBLOCK, 432);  
  if (len > 30) /*illegal length */
  {  
     len = 0;
  }   
  else	  
  {
  	int k;
  	for (k = 0; k < len; k++)
  	d->volName[k] = getSecDataByte(d, ROOTBLOCK, 433+k);
  }
  	
  d->volName[len] = '\0'; /* since we're not using strcpy(), we should make sure
  													 that this string is properly terminated!            */
  													 
  d->bootblktype = evalBlk0Type(&d->sec[0]);
  d->hasADOSRootBlk = (len > 0) && isADOSRoot(&d->sec[ROOTBLOCK]);
  chkBAMKey (d, bMap); 
  													 
  printf ("\n");    
  iError = chkErr (d, bMap, mainLog, inArc, flag);
  
  if (bMap) free (bMap);
}

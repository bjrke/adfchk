
/* adfmyunDMS.c */

/* (c) 2009-2010 aeibach                                                           */
/* Uses code portions from xDMS v1.3, (c) 1999 Andre R. de la Rocha. Public Domain.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "typedefs.h" /* needed for 'bool' type */
#include "xdms/cdata.h"
#include "xdms/pfile.h"
#include "xdms/crc_csum.h"

#ifdef UNDER_DOS
#include <io.h>
#include <fcntl.h>
#endif

#define FNAME_MAXC 512


/* static void Usage(void); */ 
static int strcmpnc(char *, char *);
static void strcpymax(char *, char *, int);
static void strcatmax(char *, char *, int);
static void ErrMsg(USHORT, char *);

int unDMSMain(char *dmsfilename, char *logfilename, UCHAR *fbuffer, bool beQuiet)
{	
	/* boolean parameter beQuiet can be set to false to 
	   gain some output useful for bugtracking. */
	
	USHORT cmd=0, opt=0, ret, PCRC=0, pwd=0;
	int ext;
	char iname[FNAME_MAXC+1], oname[FNAME_MAXC+1], 
	     /* cmdstr[FNAME_MAXC+20], *tname, */ *inm, *p, *q, *destdir=NULL;
			
  if (beQuiet)
  	opt = OPT_QUIET;
  else
  	opt = OPT_VERBOSE;

  /*	case 'p' :
				if (++i == argc) {
					Usage();
					exit(EXIT_FAILURE);
				}
				PCRC = CreateCRC((UCHAR*)argv[i],(ULONG)strlen(argv[i]));
				pwd = 1;
				break; */	

  cmd = CMD_UNPACK;

	ext = EXIT_SUCCESS;

		if (!strcmpnc("stdin", dmsfilename)) 
		{			
			inm = NULL;
		}
		else 			
		{						
			strcpymax(iname, dmsfilename, FNAME_MAXC);
			if ((strlen(iname)<4) || (strcmpnc(".dms",iname+strlen(iname)-4))) strcatmax(iname,".dms",FNAME_MAXC);
			inm = iname;
		}

		/*  generate the output filename  */
	/* 	if ((i < argc) && */ /* ( argv[i][0] =='+'))
		{
  */		
			
		if ((!strcmpnc("stdout", dmsfilename /* argv[i]+1 */ )) || 
		    (destdir && (!strcmpnc("stdout",destdir)))) 
		{
			strcpy(oname,"");
		  /* fbuffer = NULL; */ 
		}
		else 
		{
				if (destdir) {
					strcpymax(oname,destdir,FNAME_MAXC-1);
					p = oname + strlen(oname) - 1;
					if (!strchr(DIR_SEPARATORS,*p)) {
						*(p+1) = DIR_CHAR;
						*(p+2) = '\0';
					}
				} 
				else strcpy(oname,"");
				
				strcatmax(oname, dmsfilename,FNAME_MAXC);
				if ((cmd == CMD_UNPACK) && (strlen(oname)>0))
			  {
					p = oname + strlen(oname) - 1;
					if (strchr(DIR_SEPARATORS,*p))
				  {
						if (inm) {
							p = q = iname;
							while(*p) {
								if (strchr(DIR_SEPARATORS,*p)) q = p+1;
								p++;
							}
							strcatmax(oname,q,FNAME_MAXC);
						}
					}
				}
				/* fbuffer = oname; */
		} /* endif */
		
/*
			i++;
		} 
		else if (destdir && (!strcmpnc("stdout",destdir))) 
		{
			strcpy(oname,"");
			fbuffer = NULL;
		}
		else  */
		
		
		if (destdir)
			strcpymax(oname,destdir,FNAME_MAXC-1);
		else
			strcpy(oname,"");

		if (cmd == CMD_UNPACK) {
			
				if (inm) {
					p = q = iname;
					while(*p) {
						if (strchr(DIR_SEPARATORS,*p)) q = p+1;
						p++;
					}
					strcatmax(oname,q,FNAME_MAXC);
				
				} 				

			}

			/* fbuffer = oname; */
		

		if (opt == OPT_VERBOSE) {
			if ((cmd == CMD_UNPACK)) {
				if (inm)
					fprintf(stderr,"Unpacking file %s to ",inm);				
			}
			else if (cmd == CMD_TEST) 
			{
				if (inm)
					fprintf(stderr,"Testing file %s\n",inm);				
			} 
			else if (cmd == CMD_SHOWBANNER) {
				if (inm)
					printf("Showing Banner in %s :\n",inm);
				else
					printf("Showing Banner in stdin :\n");
			}

		} /* endif (verbose) */

		#ifdef UNDER_DOS
		if (!inm) setmode(fileno(stdin),O_BINARY);
		if ((cmd == CMD_UNPACK) && (!fbuffer)) setmode(fileno(stdout),O_BINARY);
		#endif

    /* extract code removed; we do not intend to extract single files */
    
		/* process data and preserve the fbuffer! */	
		ret = Process_File(inm, fbuffer, cmd, opt, PCRC, pwd);

		/* DEBUG */		
		/* printf ("[DEBUG][OUT2] bytes =  %02x %02x %02x %02x %02x %02x %02x %02x",
		       *(fbuffer+901114), *(fbuffer+901115), *(fbuffer+901116), *(fbuffer+901117), *(fbuffer+901118), *(fbuffer+901119),
		       *(fbuffer+6), *(fbuffer+7));	*/		
		
		/* if (opt != OPT_QUIET) */
		ErrMsg(ret, inm); 

		if (ret == NO_PROBLEM) 
		{
			if (opt != OPT_QUIET)
			{
				switch (cmd) 
		  	{
					case CMD_UNPACK:
						if (inm)
							fprintf(stderr,"File %s was correctly unpacked to ",inm);
						else
							fprintf(stderr,"Data from stdin was correctly unpacked to ");
						
						/* if (fbuffer)
							fprintf(stderr,"%s\n",fbuffer);
						else
							fprintf(stderr,"stdout\n"); */
						break; 
					
					case CMD_TEST:
						if (inm)
							fprintf(stderr,"File %s is ok\n",inm);
						else
							fprintf(stderr,"Data from stdin is ok\n");
						break;
					default:
						break;
				} /* endswitch */				
		  }
		}
		else ext = EXIT_FAILURE;
			  

		if (opt != OPT_QUIET) fprintf(stderr,"\n");
	
/*	} */ /* endwhile */
  
	/* return((int)ext); */
  return (ret);
}

static int strcmpnc(char *s1, char *s2){
	while (*s1 && (tolower(*s1)==tolower(*s2))) {s1++; s2++;}
	return tolower(*s1)-tolower(*s2);
}

static void strcpymax(char *s1, char *s2, int max){
	if (strlen(s2)>max){
		memcpy(s1,s2,max);
		*(s1+max) = 0;
	} else
		strcpy(s1,s2);
}

static void strcatmax(char *s1, char *s2, int max){
	if (strlen(s1)+strlen(s2)>max){
		memcpy(s1+strlen(s1),s2,max-strlen(s1));
		*(s1+max) = 0;
	} else
		strcat(s1,s2);
}

static void ErrMsg(USHORT err, char *i)
{

	if (!i) i = "stdin";
	/* if (!o) o = "stdout"; */

  if (err != NO_PROBLEM)
  {	
  	fprintf (stderr, "\n=========================================================================================\n");  	
  	fprintf (stderr, "[          THERE WAS AN ERROR REPORTED BY THE EXTERNAL DMS UNPACKING ROUTINE:             ]\n\n");
    fprintf (stderr, "'");
  }
  
	switch (err) {
		case NO_PROBLEM:
		case FILE_END:
			return;
		case ERR_NOMEMORY:
			fprintf(stderr,"Not enough memory for buffers!");
			break;
		case ERR_CANTOPENIN:
			fprintf(stderr,"Can't open %s for reading!",i);
			break;
		/* case ERR_CANTOPENOUT:
			fprintf(stderr,"Can't open %s for writing!\n",o);
			break; */
		case ERR_NOTDMS:
			fprintf(stderr,"File %s is not a DMS archive!",i);
			break;
		case ERR_SREAD:
			fprintf(stderr,"Error reading file %s : unexpected end of file!",i);
			break;
		case ERR_HCRC:
			fprintf(stderr,"Error in file %s : header CRC error!",i);
			break;
		case ERR_NOTTRACK:
			fprintf(stderr,"Error in file %s : track header not found!",i);
			break;
		case ERR_BIGTRACK:
			fprintf(stderr,"Error in file %s : track too big!",i);
			break;
		case ERR_THCRC:
			fprintf(stderr,"Error in file %s : track header CRC error!",i);
			break;
		case ERR_TDCRC:
			fprintf(stderr,"Error in file %s : track data CRC error!",i);
			break;
		case ERR_CSUM:
			fprintf(stderr,"Error in file %s : checksum error after unpacking!\n",i);
			fprintf(stderr,"This file seems ok, but the unpacking failed.\n");
			fprintf(stderr,"This can be caused by a bug in xDMS. Please contact the author");
			break;
		/* case ERR_CANTWRITE:
			fprintf(stderr,"Error : can't write to file %s  !\n",o);
			break; */
		case ERR_BADDECR:
			fprintf(stderr,"Error in file %s : error unpacking!\n",i);
			fprintf(stderr,"This file seems ok, but the unpacking failed.\n");
			fprintf(stderr,"This can be caused by a bug in xDMS. Please contact the author");
			break;
		case ERR_UNKNMODE:
			fprintf(stderr,"Error in file %s : unknown compression mode used!",i);
			break;
		case ERR_NOPASSWD:
			fprintf(stderr,"Can't process file %s : file is encrypted!",i);
			break;
		case ERR_BADPASSWD:
			fprintf(stderr,"Error unpacking file %s!. The password is probably wrong.",i);
			break;
		case ERR_FMS:
			fprintf(stderr,"Error in file %s : this file is not really a compressed disk image, but an FMS archive!",i);
			break;
		default:
			fprintf(stderr,"Error while processing file  %s : internal error !\n",i);
			fprintf(stderr,"This is a bug in xDMS\n");
			fprintf(stderr,"Please contact the current maintainer.");
			break;
	}
	
	if (err != NO_PROBLEM)
	{		  
	  fprintf (stderr, "'\n=========================================================================================\n");
  }
}

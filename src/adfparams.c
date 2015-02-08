#include <stdio.h>
#include <string.h>
#include "typedefs.h"
#include "adfparams.h"

int8 parseParams(int argc, char* argv[])
{  
	uint8 optFile = 0, optLog = 0, optBatch = 0;
	int8 mask = 0;
	
	/* Description of bits:    SET = option active / CLR = option inactive
	bit 0: FILE option (-f)
	bit 1: LOG option (-l)
	bit 2: BATCH option (-b)
	bit 3: <yet unused>
	*/
  	   int i;
        for (i=1;i<argc;i++)
        {
          if ((*argv[i])=='-')
            {
                const char *p=argv[i]+1;

                while ((*p) != '\0')
                {
                    char c = *(p++);;
                    
                    if (c == 'f' && ((i+1)<argc))
                    {
                    	/* -f given; may never be last option; 
                    	  now next parameter must be a filename */
                    	optFile = 1;
                      
                      strcpy (filespec, argv[i+1]);
                      i++;                       
                      if (filespec[0] == '-')
                      {
                      	 printf ("\nERROR: Option not allowed here! Must specify a file. \nABORTED.\n\n");
                         return -1;
                      }	
                    }
                    else if (c == 'l' && ((i+1)<argc))
                    {
                    	/* -l given; may never be last option; 
                    	  now next parameter must be a filename */                    	
                      
                      strcpy (logFilenameMain, argv[i+1]);                                             
                      i++;
                      if (logFilenameMain[0] == '-')
                      {
                      	 printf ("\nERROR: Option not allowed here! Must specify a file. \nABORTED.\n\n");
												 return -1;
                      }	
                    	optLog = 1;
                    }	
                    else if ( c == 'b')
                    {	
                    	if (argc == 2)
                    	{
                    		printf ("\nERROR: Must specify -f followed by a filename to run in batch mode. \nABORTED.\n\n");
                        return -1;
                      }  
                    	else if ((i+1) < argc && (*argv[i+1] != '-'))  /* not last option */
                      /* test for next option: must begin with a '-'. */
                      {
                        printf ("\nERROR: Not allowed! Next parameter must be an option if -b is not last option. \nABORTED.\n\n");
                        return -1;
                      }	
                                          	
                    	optBatch = 1;
                    }
                    else if (!(c == 'f' || c == 'l' || c == 'b'))
                    {   
                    	  printf ("Unsupported option or illegal syntax!\n\n");
                    	                     		
                    		printf ("Usage: %s [-f <ADF filename> [-l <log filename>][-b(atch mode)]] \n",argv[0]);
                    		return -1;	                         
                    }
                } /* endwhile */
            }
            else
            {
            	printf ("Illegal option syntax! \n\n");
              printf ("Usage: %s [-f <ADF filename> [-l <log filename>][-b(atch mode)]] \n",argv[0]);            
            } /* endif */ 
        } /* endfor */

   return (mask = ((optBatch << 2) | (optLog << 1) | optFile));
}

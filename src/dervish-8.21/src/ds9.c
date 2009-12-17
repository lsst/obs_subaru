/*
 * Support connecting to the DS9 display program from within dervish using xpa
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>

#include "shCErrStack.h"
#include "shCAssert.h"
#include "shCFitsIo.h"
#include "shCMask.h"
#include "shCDs9.h"

#if defined(HAVE_DS9)

/******************************************************************************/
static XPA xpa = NULL;			/* Our connection to the XPA server,
					   and hence DS9 */

/******************************************************************************/
/*
 * A binding for XPAGet that talks to only one server, but doesn't have to talk 
 * (char **) with SWIG
 */
const char *
shXpaGet(const char *cmd)
{
   char *xtemplate = "ds9";
   char *mode = "";
   static char *buf = NULL;		/* desired response (malloced by XPAget) */
   int len = 0;				/* length of buf; ignored */
   char *error = NULL;			/* returned error if any*/
   int n;				/* length returned by XPAGet */
   int ntry = 2;			/* number of tries */

   if (xpa == NULL) {
      if ((xpa = XPAOpen(NULL)) == NULL) {
	 shError("Failed to open xpa");
	 return(NULL);
      }
   }

   if (buf != NULL) {			/* allocated last time we called XPAGet */
       free(buf); buf = NULL;
   }

   while (ntry-- > 0) {
      n = XPAGet(xpa, xtemplate, (char *)cmd, mode,
		 &buf, &len, NULL, &error, 1);
      
      if(n == 0) {
	 return("");
      }
      if(error == NULL) {
	 buf[len - 1] = '\0';		/* may not be NUL terminated */
	 if (buf[len - 1] == '\n') {
	    buf[len - 1] = '\0';
	 }
	 return(buf);
      }
   }

   shAssert(error != NULL);

   shError("%s", error);

   XPAClose(xpa); xpa = NULL;
   
   return(NULL);
}

/******************************************************************************/

int
shXpaSet(const char *cmd, char *buf)
{
   char *xtemplate = "ds9";
   char *mode = "";
   int len = strlen(buf);		/* length of buf */
   char *error = NULL;			/* returned error if any */

   if (xpa == NULL) {
      if ((xpa = XPAOpen(NULL)) == NULL) {
	 shError("Failed to open xpa");
	 return -1;
      }
   }

   int n = XPASet(xpa, xtemplate, (char *)cmd, mode,
		  buf, len, NULL, &error, 1);
   
   if(n == 0) {
      return 0;
   }
   if(error != NULL) {
      shError("%s", error);

      XPAClose(xpa); xpa = NULL;
      return -1;
   }
   
   return 0;
}

/*****************************************************************************/
/*
 * Display a FITS file or mask
 */
int
shXpaFits(const REGION *reg,		/* region to display */
	  const MASK *mask)		/* or mask to display */
{
   FILE *xpa_file;			/* xpa file descriptor */
   char *xpaCmd = NULL;			/* command to send to ds9 */

   shAssert(reg == NULL || mask == NULL);

   if(reg != NULL) {
       xpaCmd = "xpaset ds9 fits";
   } else {
      xpaCmd = "xpaset ds9 fits mask";
   }

   {
       void (*hand_INT)(int) = signal(SIGINT, SIG_IGN);
       void (*hand_TSTP)(int) = signal(SIGTSTP, SIG_IGN);
       xpa_file = popen(xpaCmd, "w");
       (void)signal(SIGINT, hand_INT);
       (void)signal(SIGTSTP, hand_TSTP);

       if (xpa_file == NULL) {
	   shErrStackPushPerror("Talking to ds9");
	   return(-1);
       }
   }

   if (reg != NULL) {
       shRegWriteAsFits((REGION *)reg, NULL, STANDARD, 2, DEF_NONE, xpa_file, 0);
   } else {
       shMaskWriteAsFits((MASK *)mask, NULL, DEF_NONE, xpa_file, 0);
   }
   
   pclose(xpa_file);

   return(0);
}
#else
int
shXpaSet(const char *cmd, char *buf)
{
   assert (cmd != NULL);		/* keep compiler happy */

   fprintf(stderr, "shXpaSet: "
	   "XPA is not compiled in (setup xpa and rebuild dervish)\n");
   return(-1);
}

const char *
shXpaGet(const char *cmd)
{
   assert (cmd != NULL);		/* keep compiler happy */
   fprintf(stderr, "shXpaGet: "
	   "XPA is not compiled in (setup xpa and rebuild dervish)\n");
   return(NULL);
}

int
shXpaFits(const REGION *reg,		/* region to display */
	  const MASK *mask)		/* or mask to display */
{
   assert ((void *)reg != (void *)mask); /* keep compiler happy */
   fprintf(stderr, "shXpaFits: "
	   "XPA is not compiled in (setup xpa and rebuild dervish)\n");
   return(-1);
}

#endif

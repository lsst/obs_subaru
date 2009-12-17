/*<AUTO>
 *
 * FILE:
 *   tclSaoDisplay.c
 *
 * ABSTRACT:
 *   This file contains the routines used to invoke and stop SAO image from
 *   the TCL level.
 *</AUTO>
 *
 * ENTRY POINT            SCOPE    DESCRIPTION
 * ----------------------------------------------------------------------------
 * tclSaoDel              local    Kill the requested SAOimage processes.
 * tclSaoDisplay          local    TCL implementation of SAO image spawning
 *                                    program
 * regPixGetInfo          local    Get pixel information/fill pixel buffer
 * makeHeader             local    Construct a mimimum FITS header
 * SaoImageRun            local    Routine that forks/execs to run a sao
 *                                    image.
 * SaoImageProc           local    Routine used by Tk and DERVISH as a callback
 *                                    handler to converse with sao image
 *
 * shTclSaoDisplayDeclare public   World's interface to SAO image TCL verbs
 *
 * AUTHORS:
 *   Gary Sergey
 *   Eileen Berman
 *
 * MODIFICATIONS:
 * July 1993  vijay  Extensive modifications: broke down cmdPgm.c to manageable
 *                   chunks.
 * Aug 3, 1993 efb   added output of saonumber at end of saoDisplay command,
 *                   removed the 'spawning...' printf statement.
 * Oct 10, '93 vijay modified SaoImageRun() to prevent zombie (defunct)
 *                   processes.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>	  /* For index */
#include <stdlib.h>       /* For prototype of getenv() */
#include <ctype.h>        /* For prototype of isdigit() and friends */
#include <unistd.h>       /* For prototype of close() */
#include <sys/types.h>
#include <signal.h>       /* For SIGKILL */
#include <sys/wait.h>
#include <alloca.h>
#include <sys/stat.h>
#include "shCAssert.h"
#include "shTclParseArgv.h"

#include "tcl.h"
#include "tk.h"
#include "region.h"
#include "shTclRegSupport.h"
#include "shTclUtils.h"
#include "shCFitsIo.h"
#include "dervish_msg_c.h"
#include "shTclSao.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "libfits.h"
#include "shCGarbage.h"
#include "shCUtils.h"	/* Prototype for shFatal */

#ifdef __sgi    /* for kill prototype */
extern int kill (pid_t pid, int sig);
#endif

/*
 * structure holding information on the mouse command the user is waiting for
 */
/*extern SHMOUSEWAIT g_mouseWait;*/

/*
 * A pointer to this data structure is passed to the Tk callback handler.
 * This is a local structure definitions: used in this source file only
 */
typedef struct callbackdata  {
   Tcl_Interp   *interp;       /* Obviously, the TCL interp being used      */
   SAOCMDHANDLE *hndlp;        /* Pointer to command handle structure       */
   int          pipeIndex;     /* Pipe we are communicating w/ sao image on */
} CALLBACKDATATYPE;
 
/*
 * Global variables
 */
static int save_mouse_commands = 1;
FITSFILETYPE g_fileType;
SAOCMDHANDLE g_saoCmdHandle;

/*
 * External variables referenced in this file
 */
extern char *g_hdrVecTemp[MAXHDRLINE+1];
extern int g_saoMouseVerbose;
extern SHSAOPROC    g_saoProc[CMD_SAO_MAX];

/*
 * Locally used function prototypes. These functions are used in this source
 * file only.
 */
static int tclSaoDisplay(ClientData, Tcl_Interp *, int, char **);
static int regPixGetInfo(Tcl_Interp *, const char *, void ***, PIXDATATYPE *,
                         int *, int *);
static RET_CODE makeHeader(int, int, PIXDATATYPE, char ***, double *,
			   double *);
static void SaoImageProc(ClientData, int);
static RET_CODE SaoImageRun(char *, int);

static char *tclSaoDelHelp =
            "Delete specified FSAOimage process(es).";
static char *tclSaoDelUse =
            "USAGE: saoDel [-a | FSAO # [Next FSAO #]] ";
/*
 * ROUTINE:
 *   tclSaoDel
 * 
 *<AUTO>
 * TCL VERB:
 *   saoDel
 *
 * DESCRIPTION:
 *   Deletes the specified FSAOimage process
 *
 * SYNTAX:
 *   saoDel [-a | FSAO # [Next FSAO #]]
 *	-a	Delete all FSAOimage processes
 *	FSAO #	Delete specified FSAOimage process
 *
 *</AUTO>
 */
static int
tclSaoDel(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
int         rstatus = TCL_OK;
int	    i = 1, j;			/* set i=1 to skip over command name */
int         saonum;
char        dash[2];

dash[0] = '-';
dash[1] = '\0';

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc == 1)
   {
   /* No params entered, send out help message */
   rstatus = TCL_ERROR;
   Tcl_SetResult(interp, tclSaoDelUse, TCL_VOLATILE);
   }
else
   {
   while (i < argc)
      {
      if (argv[i][0] == dash[0])   /* is an option */
         {
         switch (argv[i][1]) 
            {
            case 'a':
               /* We should kill ALL FSAO processes. Do not worry about the rest
                  of the line. */
               p_shSaoImageCleanFunc();
               return(rstatus);
            default:
               /* Invalid option argument */
               rstatus = TCL_ERROR;
               Tcl_SetResult(interp, "ERROR : Invalid option.", TCL_VOLATILE);
               Tcl_AppendResult(interp, "\n", tclSaoDelUse, (char *)NULL);
               return(rstatus);
            }
         }
      else
         {
         /* This must be an sao number.  Kill the process. */
         saonum = atoi(argv[i]);		/* convert to integer */
         for (j = 0 ; j < CMD_SAO_MAX ; ++j)
            {
            /* Look for the matching saonumber to get the pid */
            if (saonum == g_saoProc[j].saonum)
               {
               kill(g_saoProc[j].pid, SIGKILL);
               break;
               }
            }
         if (j == CMD_SAO_MAX)
            {
            /* Did not find the requested process to kill */
            Tcl_AppendResult(interp, "\n", "Warning: No FSAOimage with number ",
                             argv[i], " exists.", (char *)NULL);
            }
         }
      i++;                                         /* Go to next parameter */
      }
   }

return(rstatus);
}


static char *tclSaoDisplayHelp =
            "Display region data in FSAOimage.";
static char *tclSaoDisplayUse =
            "USAGE: saoDisplay <region name> [FSAO #] [\"FSAO options\"]";
/*
 * ROUTINE:
 *   tclSaoDisplay
 * 
 *<AUTO>
 * TCL VERB:
 *   saoDisplay
 *
 * DESCRIPTION:
 *  The saoDisplay command will display the designated region using FSAOimage.
 *  If a copy of the FSAOimage program is not running (or is not specified), a
 *  new one will be started.
 *
 * SYNTAX:
 *   saoDisplay  <name>  [FSAO #]  [{FSAO options}]
 *	<name>           Name of region to display.
 *	[FSAO #]         Optional FSAOimage program to display region in.
 *	                 The default is to start a new FSAOimage program.
 *	[{FSAO options}] Optional FSAOimage options to be used when a new
 *                       FSAOimage program is started (i.e., FSAO # is not
 *                       specified). Note that these options must be
 *                       enclosed in curly braces.
 *
 *  The DERVISH TCL command 'saoDisplay' will start FSAOimage.  The
 *  DISPLAY variable must be defined to the correct X display and the Fermi
 *  FSAO product must be setup prior to issuing the saoDisplay command.  This
 *  can be done by typing in the following at the UNIX prompt. Note that
 *  FSAO is setup by dervish automatically.
 *
 *	% setenv DISPLAY mynode:0.0
 *	% setup dervish
 *
 *  According to experience, it appears to be better to use -rmin and -rmax
 *  options to set the scaling for an image than to use -max and -min.
 *
 *  If an FSAO # is specified, previous FSAO options (from previous saoDisplay
 *  commands to that FSAO #) remain in effect, except the those overridden by
 *  options passed on this command line.
 *
 *</AUTO>
 *
 * RETURN VALUES:
 *   TCL_OK
 *   TCL_ERROR
 */
static int
tclSaoDisplay(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
RET_CODE    rstatus;
char	    syscmd[300];
int	    i, nrows, ncols;
char	    **hdr;
void	    **pixels_ptr;
int	    saonumber = -1;		    /* Assume no SAOimage number */
int         saoindex  = -1;
char        asaonum[8];                     /* Ascii version of saonumber */
char	    *opts     = "";		    /* Assume no SAOimage options */
char        *cmdOpt = ((char *)0);	    /* SAOimage options for FitsMsg */
PIXDATATYPE pixelType;
double      bscale;
double      bzero;
CALLBACKDATATYPE *cbdp;

#define L_ERR_MSG_LEN	80
char	    errmsg[L_ERR_MSG_LEN];	    /* Error message */

errmsg[0] = '\0';

/*---------------------------------------------------------------------------
**
** CHECK COMMAND ARGUMENTS
*/
if (argc < 2)
   {
   goto syntax_err_exit;
   }

/* Check for optional arguments */
if (argc >= 3)
   {
   /* Check if SAOimage number was specified.  This number identifies the
      desired copy of SAOimage to load. */
   if (isdigit((char)argv[2][0]))
      {
      saonumber = atoi(argv[2]);
      saoindex = saonumber - 1;
      if ((saoindex < 0) || (saoindex >= CMD_SAO_MAX))
         {
         sprintf(errmsg, "Error: FSAOimage number out of range (1 - %d)\n",
                 CMD_SAO_MAX);
         Tcl_SetResult(interp, errmsg, TCL_VOLATILE);
         goto syntax_err_exit;
         }
      if (argc >= 4)
         {cmdOpt = opts = argv[3];}
      if (argc >  4)
         {
         rstatus = TCL_ERROR;
         Tcl_SetResult(interp, "ERROR : Too many arguments.", TCL_VOLATILE);
         Tcl_AppendResult(interp, "\n", tclSaoDisplayUse, (char *)NULL);
         return(rstatus);
         }
      }
   /* Otherwise, must be SAOimage options */
   else
      {
      opts = argv[2];
      if (argc > 3)
         {
         rstatus = TCL_ERROR;
         Tcl_SetResult(interp, "ERROR : Too many arguments.", TCL_VOLATILE);
         Tcl_AppendResult(interp, "\n", tclSaoDisplayUse, (char *)NULL);
         return(rstatus);
         }
      }
   }

/* Only support STANDARD FITS header files */
g_fileType = STANDARD;

/*---------------------------------------------------------------------------
**
** GET REGION AND REGION HEADER DATA
*/
if (regPixGetInfo(interp, argv[1], &pixels_ptr, &pixelType, &nrows,
    &ncols))
   {
   Tcl_SetResult(interp, "tclSaoDisplay: error getting pixel info", 
                 TCL_VOLATILE);
   goto err_exit2;
   }

if (makeHeader(nrows, ncols, pixelType, &hdr, &bscale, &bzero) != SH_SUCCESS)
   {
   shTclInterpAppendWithErrStack(interp);
   goto err_exit;
   }

/*---------------------------------------------------------------------------
**
** SPAWN A NEW COPY OF SAOIMAGE IF ONE WASN'T SPECIFIED ...
*/
if (saoindex < 0)
   { 

   /*
   ** FIRST LOOK FOR UNUSED HANDLE IN LIST OF SAOIMAGE HANDLES
   */
   for (i=0; i < CMD_SAO_MAX; i++)
      {
      if (g_saoCmdHandle.saoHandle[i].state == SAO_UNUSED)
         {
         g_saoCmdHandle.saoHandle[i].state = SAO_INUSE; /* We got it */
         saoindex = i;				/* Found one, save the index */
         saonumber = saoindex +1;
         break;					/* Now move on */
         }
      }

   if (saoindex < 0)
      {
      Tcl_SetResult(interp, 
                    "Can't spawn FSAOimage - Maximum number already in use",
                    TCL_VOLATILE);
      goto err_exit;
      }

   /*
   ** OPEN COMMUNICATION PATH TO SAOIMAGE (PIPES)
   */
   rstatus = shSaoPipeOpen(&(g_saoCmdHandle.saoHandle[saoindex]));
   if (rstatus != SH_SUCCESS)
      {
      Tcl_SetResult(interp, "Trouble with pipes required for FSAOimage",
                    TCL_VOLATILE);
      shSaoClearMem(&(g_saoCmdHandle.saoHandle[saoindex]));  /* Clean up */
      goto err_exit;
      }

   /* Add SAOimage pipe FD to "select" function list */
   cbdp = (CALLBACKDATATYPE *) shMalloc(sizeof(CALLBACKDATATYPE));
   if (cbdp == NULL)  {
       Tcl_SetResult(interp, "saoDisplay: shMalloc failed (out of heap space)\n",
                     TCL_VOLATILE);
       goto err_exit;
   }
   cbdp->hndlp = &g_saoCmdHandle;
   cbdp->pipeIndex = saoindex;
   cbdp->interp = interp;
   Tk_CreateFileHandler(g_saoCmdHandle.saoHandle[saoindex].rfd, TK_READABLE, 
                        SaoImageProc, (ClientData) cbdp);
   /*
   ** SPAWN SAOIMAGE
   */
   if (getenv("FSAO_DIR") == NULL)  {
       Tcl_SetResult(interp, 
          "Product fsao is not setup. Please set it up first.\n", TCL_VOLATILE);
       goto err_exit;
   }
   sprintf(syscmd,
      "%s/bin/%s %s -dscname \"%s %d\" -fsaonumber %d -p %d -pipe -idev %d -odev %d  %s",
	   getenv("FSAO_DIR"), CMD_SAO_PNAME, CMD_SAO_OPTIONS, CMD_SAO_WNAME,
	   saonumber, saonumber, CMD_SAO_CLRS, 
	   g_saoCmdHandle.saoHandle[saoindex].cwfd,
	   g_saoCmdHandle.saoHandle[saoindex].crfd, opts);

   /* The output of the following line has been replaced by the output of the
      saonumber to the user via TCL in such a way that the user can capture it
      in TCL. */
   /* printf("Spawning %s %d\n", CMD_SAO_WNAME, saonumber);*/

   rstatus = SaoImageRun(syscmd, saonumber);

   /* Check for errors from the 'SaoImageRun' call. */
   if (rstatus != SH_SUCCESS)  {
      /* Error while trying to fork off fsaoimage */
      shTclInterpAppendWithErrStack(interp);
      shSaoClearMem(&(g_saoCmdHandle.saoHandle[saoindex]));  /* Clean up */
      goto err_exit;
   }

   /* Close the child's end of the pipes.  We need to do this in order to be
      notified when FSAOimage exits (i.e., read on pipe will return 0 bytes). */
   close(g_saoCmdHandle.saoHandle[saoindex].cwfd);
   close(g_saoCmdHandle.saoHandle[saoindex].crfd);
   }

/*---------------------------------------------------------------------------
**
** OTHERWISE, A COPY OF SAOIMAGE WAS SPECIFIED - CHECK IF IT'S THERE
 *
 * If user specified a sao number to connect to, let's make sure that
 * a pipe has been created on that sao number and a FSAOimage is indeed
 * waiting on the other end.
*/
else
   {
    if (g_saoCmdHandle.saoHandle[saoindex].state == SAO_UNUSED)  {
        sprintf(errmsg, 
                "No FSAOimage associated with number %d", saoindex+1);
        Tcl_SetResult(interp, errmsg, TCL_VOLATILE);
        goto err_exit;
    }
   }


/*---------------------------------------------------------------------------
**
** SEND IMAGE TO SAOIMAGE
*/

/*
** SAVE THE REGION NAME SO IT CAN BE RETURNED AS PART OF THE TEXT STRING
** WHEN A MOUSE CLICK OCCURS.  ALSO SAVE THE CURRENT REGION COUNTER.
*/
p_shSaoRegionSet(&(g_saoCmdHandle.saoHandle[saoindex]), argv[1]);

/* 
** SAVE THE NUMBER OF ROWS AND COLUMNS IN THIS IMAGE SO IF WE NEED TO 
** DOWNLOAD A MASK, WE CAN MAKE SURE IT IS OF THE SAME SIZE.
*/
g_saoCmdHandle.saoHandle[saoindex].rows = nrows;
g_saoCmdHandle.saoHandle[saoindex].cols = ncols;

/*
** SEND REGION HEADER AND IMAGE DATA TO SAOIMAGE
*/
rstatus = shSaoSendFitsMsg(&(g_saoCmdHandle.saoHandle[saoindex]), hdr,
			   (char **)pixels_ptr,
                           pixelType, bscale, bzero, nrows, ncols, cmdOpt);
if (rstatus != SH_SUCCESS)
   {
   sprintf(errmsg, 
                "Trouble when writing to FSAOimage %d\n", saoindex+1);
   Tcl_SetResult(interp, errmsg, TCL_VOLATILE);
   Tcl_AppendResult(interp,
         "Make sure the fsao product has been setup before doing saoDisplay.\n"
		    ,(char *)NULL);
   /* We've commented out clearing of sao struct below since it's now being
      done in SaoImageProc */
   /* shSaoClearMem(&(g_saoCmdHandle.saoHandle[saoindex]));*/
   goto err_exit;
   }

/*---------------------------------------------------------------------------
**
** RETURN
*/
/* Delete all the header lines added.  The header will always have a bzero and
   a bscale. */
rstatus = p_shFitsDelHdrLines(hdr, (int )TRUE);

sprintf(asaonum, "%d", saonumber);
Tcl_SetResult(interp, asaonum, TCL_VOLATILE);
return(TCL_OK);

syntax_err_exit:
Tcl_AppendResult(interp,"\n", tclSaoDisplayUse, (char *)NULL);
return(TCL_ERROR);

err_exit:
/* Delete all the header lines added.  The header will always have a bzero and
   a bscale. */
rstatus = p_shFitsDelHdrLines(hdr, (int )TRUE);
return(TCL_ERROR);

err_exit2:
return(TCL_ERROR);

}

/*
 * ROUTINE:
 *   regPixGetInfo
 *
 * CALL:
 *   (static int) regPixGetInfo(
 *                   Tcl_Interp  *a_interp,      // IN: TCL Interpreter
 *                   const char  *a_regName,     // IN: Region name
 *                   void        ***a_pixel_ptr, // OUT: pixel buffer
 *                   PIXDATATYPE *a_pixelType,   // OUT: pixel type
 *                   int         *a_nrows,       // OUT: number of rows
 *                   int         *a_cols         // OUT: number of columns
 *                );
 *
 * DESCRIPTION:
 *   This routine gets all relevant pixel information
 *
 * RETURNS:
 *   1 : on error, and places reason of error in a_interp->result
 *   0 : otherwise
 */
static int regPixGetInfo(Tcl_Interp *a_interp, const char *a_regName, 
                              void ***a_pixel_ptr, PIXDATATYPE *a_pixelType, 
                              int *a_nrows, int *a_ncols)
{
   REGION *reg;

   if (shTclRegAddrGetFromName(a_interp, a_regName, &reg) != TCL_OK) {
      Tcl_AppendResult(a_interp, "regPixGetInfo: can't get REGION from ",
		       a_regName,(char *)0);
      return 1;
   }
   
   switch (reg->type)  {
      case TYPE_U8   :
           *a_pixel_ptr = (void **)reg->rows_u8;
           break;
      case TYPE_S8   :
           *a_pixel_ptr = (void **)reg->rows_s8;
           break;
      case TYPE_U16  :
           *a_pixel_ptr = (void **)reg->rows;
           break;
      case TYPE_S16  :
           *a_pixel_ptr = (void **)reg->rows_s16;
           break;
      case TYPE_U32  :
           *a_pixel_ptr = (void **)reg->rows_u32;
           break;
      case TYPE_S32  :
           *a_pixel_ptr = (void **)reg->rows_s32;
           break;
      case TYPE_FL32 :
           *a_pixel_ptr = (void **)reg->rows_fl32;
           break;
      default :
           /* We need a default statement since certain pixel types are
              not allowed by this function */
           shFatal("regPixGetInfo: pixel type %d is not handled", (int)reg->type);
           break;
   }
  
   if (*a_pixel_ptr == NULL)  {
       Tcl_AppendResult(a_interp, "regPixGetInfo: no pixels in region",
                        (char *)0);
       return 1;
   }

   *a_nrows = reg->nrow;
   *a_ncols = reg->ncol;
   *a_pixelType = reg->type;

   return 0;
}

/*============================================================================  
**============================================================================
**
** ROUTINE: makeHeader
*/
static RET_CODE makeHeader
   (
   int		a_nr,		/* IN: Number of rows in image */
   int		a_nc,		/* IN: Number of columns in image */
   PIXDATATYPE	a_pixType,	/* IN: Pixel data type */
   char		***a_header,	/* MOD: Image header vector */
   double       *a_bscale,      /* OUT: amount to transform pixels with */
   double       *a_bzero        /* OUT: amount to transform pixels with */
   )   
/*
** DESCRIPTION:
**	Construct a new header with the minimal number of lines -
**		SIMPLE, BITPIX, NAXIS, NAXIS1, NAXIS2, BZERO, BSCALE, END
**
** RETURN VALUES:
**
** CALLED BY:
**	tclSaoDisplay
**
** CALLS TO:
**
**============================================================================
*/
{
RET_CODE    rstatus;
PIXDATATYPE fsaoPixType;
int         i;
int         transform;
int         newBitpix = -16;
char        line[81];

/* Initialize the header */
for (i = 0 ; i < (MAXHDRLINE+1) ; ++i)
   {g_hdrVecTemp[i] = 0;}

fsaoPixType = a_pixType;

switch (a_pixType)
   {
   case TYPE_S8:
      /* Mimic U8 so that bzero and bscale will not get added to the header */
      fsaoPixType = TYPE_U8;
      break;
   case TYPE_U16:
      /* Mimic S16 so that bzero and bscale will not get added to the header */
      fsaoPixType = TYPE_S16;
      break;
   case TYPE_U32:
      /* Mimic S32 so that bzero and bscale will not get added to the header */
      fsaoPixType = TYPE_S32;
      break;
   default:
      break;
   }

rstatus = p_shFitsAddToHdr(2, a_nr, a_nc, fsaoPixType, g_fileType, a_bzero,
                              a_bscale, &transform);	/* NAXIS=2 */

/* The value of bitpix must be changed to -16 if this data is unsigned 16 bit
   data.  This is peculiar to saoimage to support and display unsigned 16 bit
   data. */
if (a_pixType == TYPE_U16)
   {
   f_mkikey(line, "BITPIX", newBitpix, "");
   f_hlrep(g_hdrVecTemp, f_lnum(g_hdrVecTemp, "BITPIX"), line);
   }

*a_header = g_hdrVecTemp;
return(rstatus);
}

/*
 * ROUTINE: SaoImageProc
 * 
 * CALL:
 *   Called by Tk function, Tk_CreateFileHandler()
 *
 * DESCRIPTION:
 *   A callback function called by the Tk Event loop. It gets data from a
 *   pipe opened between the sao image executable and dervish.
 *
 * RETURN VALUES:
 *   Nothing.
 *
 * CALLED BY:
 *   Tk_CreateFileHandler()
 *
 */
static void
SaoImageProc(ClientData clientData,
	     int mask)			/* NOTUSED */
{
   RET_CODE rstatus;
   int count;
   SAOCMDHANDLE *hndlp;
   Tcl_Interp *interp;
   CALLBACKDATATYPE *cbdp;
   int pipeIndex,
       result,
       mapStatus;
   char buf[BUFSIZ - 1];

   cbdp = (CALLBACKDATATYPE *) clientData;   /* Retreive the data */
   hndlp = (SAOCMDHANDLE *) cbdp->hndlp;
   pipeIndex = cbdp->pipeIndex;
   interp = cbdp->interp;

   rstatus = p_shSaoMouseKeyGet(&(hndlp->saoHandle[pipeIndex]));
   if (rstatus != SH_SUCCESS && rstatus != SH_PIPE_CLOSE)  {
       fprintf(stderr, "SaoImageProc(): p_shSaoMouseKeyGet() failed! Return");
       fprintf(stderr, " status = %u\n", rstatus);
       return;
   }

   /*
    * Let's see if FSAOImage exited ...
    */
   if (rstatus == SH_PIPE_CLOSE)  {
       /*
        * Yup, it did. Clean up by de-registering the file handler with Tk
        * and closing the pipe.
        */
       Tk_DeleteFileHandler(hndlp->saoHandle[pipeIndex].rfd);
       shSaoPipeClose(&(hndlp->saoHandle[pipeIndex]));
       shSaoClearMem(&(hndlp->saoHandle[pipeIndex]));
       hndlp->saoHandle[pipeIndex].state = SAO_UNUSED;
       /*
        * Now match the closed process with the pid structure.  The saonum
        * should = pipeIndex + 1 as pipeIndex is from 0 and saonum starts
        * at 1. 
        */
       g_saoProc[pipeIndex].saonum = INIT_SAONUM_VAL;
       /*
        * Free memory occupied by the callback structure, since we do not
        * need it anymore.
        */
       shFree(cbdp);
   }
   else  {
      /*
       * Otherwise, process message on the pipe.
       */
      mapStatus = p_shSaoMouseKeyMap(&(hndlp->saoHandle[pipeIndex]), 
                  g_saoProc[pipeIndex].saonum, buf);
      /*
       * Check if this is the command that we are waiting for
       */
      if (p_shSaoMouseCheckWait(pipeIndex)) {
         /*
          * Yes, this is the command that we are waiting for.
          */
         if (mapStatus)  {
            /*
             * If we made it to here, then a TCL command was previously
             * assigned to this mouse button (by the user), so return the
             * TCL command to the caller, but only if the region has not
             * been changed - otherwise, just keep going.
             */
            count = 0;
            /* count = shRegionGetModCntr(hndlp->saoHandle[pipeIndex]->region);*/
            if (count == hndlp->saoHandle[pipeIndex].region_cnt)  {
               /*
                * Yes, the region has not been changed (since the counter is
                * same as when we sent the image to SAOimage), so return the
                * TCL command to the caller.
                */
               hndlp->SAOEntered = 1;
               if (g_saoMouseVerbose)  {
                   fprintf(stdout, "\nCalling SAOimage mouse button TCL ");
                   fprintf(stdout, " command.\n");
               }
               /*
                * Execute the TCL command.
                */
               strcat(buf, "\n");
	       if(save_mouse_commands) {
		  result = Tcl_RecordAndEval(interp, buf, 0);
	       } else {
		  result = Tcl_Eval(interp, buf);
	       }
               if (*interp->result != 0)
                   if (result != TCL_OK)
                       fprintf(stdout, "%s\n", interp->result);
            }
            else if (g_saoMouseVerbose)  {
               fprintf(stdout, "\nSAOimage mouse click ignored - region no");
               fprintf(stdout, " longer current\n");
            }
         }
         else if (g_saoMouseVerbose)  {
             fprintf(stdout, "\nSAOimage mouse click ignored - No TCL");
             fprintf(stdout, " command assigned to button\n");
         }
      }
   }   /* else */
 
   return;
}

/*
 * ROUTINE: SaoImageRun
 *
 * DESCRIPTION:
 *   Spawn a new copy of sao image. Additionally, record the process id 
 *   of the spawned image in g_saoProc. This structure will be read later
 *   to send a kill signal to the process id numbers of the saoimages 
 *   running on the system.
 *
 * RETURN VALUE:
 *   SH_SUCCESS : on success
 *   ...        : otherwise
 * 
 * GLOBALS REFERENCED:
 *   g_saoProc
 */

#define	SaoImageRun_argvMax	50

#if ((SaoImageRun_argvMax) <= 0)
#error	SaoImageRun code expects argv to contain 1 or more elements
#endif

static RET_CODE
SaoImageRun(char *cmdp, int a_saonumber)
{
   char     *argv[SaoImageRun_argvMax], /* Cmd line args for spawned process */
             buf[10],    /* Temporary buffer to hold process ids */
            *tempArgv;   /* Temporary buffer to assign alloca to */
   int       argc,       /* Command line argument count */
             pstatus,    /* process status on return from waitpid */
             pipefd[2];  /* Communication pipes */
   RET_CODE  retStatus;  /* Return status of this function */
   pid_t pid, pid2, rpid;        /* Process id of the spawned process */

		  int   i;
                  char	*lcl_cmd;
                  char	*lcl_word;
                  char	*lcl_wordEnd;
         unsigned int	 lcl_wordLen;
static            char	*lcl_whites = " \t";	/* Word delimiters            */
static            char	*lcl_quotes = "\"'";	/* Quoted word delimiters     */
                  char	 lcl_quote;		/* Current quote character    */
		  struct stat fdStat;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Break up the passed command line into argv/argc format.
    */

   argv [0]  = NULL;				/* For particular error case  */
   retStatus = SH_SUCCESS;			/* Assume success             */
   lcl_cmd   = cmdp;

   for (argc = 0; *(lcl_cmd += strspn (lcl_cmd, lcl_whites)) != 0; )
      {
      if (argc == (SaoImageRun_argvMax))
         {
         retStatus = SH_TOO_MANY_ARGS;		/* Too many cmd line tokens   */
	 shErrStackPush("Too many command line arguments.");
         goto rtn_cleanup;			/* No point in continuing     */
         }

      if (strchr (lcl_quotes, *lcl_cmd) != NULL) /* Quoted string?            */
         {
         /*
          * Handle a quoted word.  All white space is preserved within the word.
          */

         lcl_quote   = *lcl_cmd++;		/* Save and skip quote char   */
         lcl_word    =  lcl_cmd;		/* Start of quoted word       */
         if ((lcl_wordEnd = strchr (lcl_word, lcl_quote)) == NULL)
            {
            retStatus = SH_NO_END_QUOTE;	/* Improperly terminated quote*/
	    shErrStackPush("Improperly terminated quotes.");
            goto rtn_cleanup;			/* No point in continuing     */
            }
         lcl_wordLen = lcl_wordEnd - lcl_word;
         lcl_cmd    += lcl_wordLen + 1;		/* Skip ending quote          */
         }

      else
         {
         /*
          * Handle an unquoted word.  It is terminated by white space.
          */

         lcl_word    = lcl_cmd;
         lcl_wordLen = strcspn (lcl_word, lcl_whites);
         lcl_cmd    += lcl_wordLen;
         }

      if ((tempArgv = (char *)alloca ((unsigned int )(lcl_wordLen + 1)))
	              == NULL)
         {
         retStatus = SH_MALLOC_ERR;		/* Unable to malloc arg space */
	 shErrStackPush("Unable to alloca space for argument list.");
         goto rtn_cleanup;			/* No point in continuing     */
         }
      argv [argc] = tempArgv;
      strncpy (argv [argc], lcl_word, (unsigned int )lcl_wordLen);
               argv [argc][lcl_wordLen] = 0;	/* Null terminate string      */

      argv [++argc] = 0;	/* Null terminate argv array (done each loop  */
				/* ... to permit goto rtn_cleanup on a error) */
      }

   if (g_saoMouseVerbose)  {
       for (argc = 0; argv[argc] != 0; argc++)
            fprintf(stdout, "%s ", argv[argc]);
       fprintf(stdout, "\n");
   }

   /*
    * In order to prevent zombie (defunct) processes, we have to fork twice.
    * First the current process forks and spawns a child. Then the child
    * forks, and the child of the first child execs FSAOimage. The child of
    * the first child is this processes grandchild. The current processes
    * then waits for it's child to finish (NOTE: it waits for it's _child_,
    * not _grandchild_, since the grandchild has been overlayed with FSAOimage
    * and will never return). All this gyration is neccessary so that the
    * grandchild can be inherited by init (pid 1) and thus be prevented from
    * becoming a zombie process.
    * Since the current process needs to store the process id of it's 
    * grandchild, we have to open a pair of pipes for communication between 
    * the current process and it's child. The child passes the process id of 
    * it's child (this processes grandchild) to the current process; which 
    * then stores it in g_saoProc[].
    */
    if (pipe(pipefd) != 0)  {
       retStatus = SH_PIPE_OPENERR;
       shErrStackPush("Pipe open error.");
       goto rtn_cleanup;
   }

   memset(buf, 0, sizeof(buf));

   /*
    * Now, fork and exec. Have the parent return immedeately so that
    * DERVISH can continue.
    */
   pid = fork();

   switch (pid)  {
       case -1 : 
	       shErrStackPush(strerror(errno));   /* Get system error */
               retStatus = SH_FORK_ERR;    /* Error condition, cannot fork */
	       shErrStackPush("Error forking child.");
               break;
       case 0  :                 /* First child */
	       signal(SIGINT,SIG_IGN);	/* ignore ^C */
#if defined(SIGTSTP)
	       signal(SIGTSTP,SIG_IGN);	/* ignore ^Z */
#endif

	       /* Close all the socket file descriptors so we do not
		* interfere with a parents attempt to close and reopen
		* ports.  
		*/

#ifdef S_ISSOCK
	       for (i = 0 ; i < OPEN_MAX ; i++) {
		 fstat(i,&fdStat);
		 if (S_ISSOCK(fdStat.st_mode)) { 
		   close(i);
		 }
	       }
#endif
   
               pid2 = fork();
               switch (pid2)  {
                   case -1 :
                           close(pipefd[1]);
                           close(pipefd[0]);
			   shErrStackPush(strerror(errno));/* Get system err */
			   retStatus = SH_FORK_ERR;    /* Error, cannot fork */
			   shErrStackPush("Error forking grandchild.");
                           goto rtn_cleanup;
                   case 0  :                     /* Grandchild */
                           close(pipefd[1]);
                           close(pipefd[0]);
                           execv(argv[0], argv); /* Grandchild does the work */
                           exit(0);	         /* NOTREACHED */
			   break;
                   default :                     /* First child */
                           sprintf(buf, "%ld", pid2);  
                           write(pipefd[1], buf, strlen(buf));
                           close(pipefd[1]);
                           close(pipefd[0]);
                           /*
                            * Note that we have to call _exit() here, and not
                            * exit(). The reason is that exit() calls all
                            * cleanup functions including 
                            * p_shSaoImageCleanFunc() which was registered
                            * earlier using atexit(). This function kills
                            * all current sao images. Thus when the first
                            * child exits, all sao images are killed - an
                            * undesired side-effect
                            */
                           _exit(0);
               }
	       break;
       default :
               /*
                * Parent; do a few things before returning:
                *
                *   1) Read the process id of spawned FSAOimage
                *   2) write this process id to g_saoProc
                *   3) handle the signal generated from the killed child
                *
                * The whole idea behind doing the above is so that when 
                * dervish exits, and if there are any FSAOimages lying
                * around, we will send them a kill -9 so that the user does
                * not have to manually terminate all the FSAOimages.
                */
               rpid = waitpid(pid, &pstatus, 0);
               if (rpid != pid)  {
		  shErrStackPush(strerror(errno));/* Get system err */
		  retStatus = SH_WAITPID_ERR;    /* Error, cannot fork */
		  shErrStackPush("Error while waiting for child to terminate.");
                  goto rtn_cleanup;
               }

               read(pipefd[0], buf, sizeof(buf));
               close(pipefd[1]);
               close(pipefd[0]);

               retStatus = SH_SUCCESS;
               /* Find the next element that has not been used.  Saoimage
                * numbers start from 1, the unused elements have
                * saonum = INIT_SUM_VAL
                */
               for (i = 0 ; i < CMD_SAO_MAX ; i++)   {
                  if (g_saoProc[i].saonum == INIT_SAONUM_VAL)   {
                     g_saoProc[i].saonum = a_saonumber;
                     g_saoProc[i].pid = atoi(buf);
                     break; 
                  }
               }
               break;
   }

rtn_cleanup:

   return retStatus;
}


/*****************************************************************************/
static char *tclSaoSaveMouseCommands_use =
  "USAGE: saoSaveMouseCommands [0|1]";
#define tclSaoSaveMouseCommands_hlp \
  "Save SAO mouse commands on the history list"

static ftclArgvInfo saoSaveMouseCommands_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSaoSaveMouseCommands_hlp},
   {"<yesno>", FTCL_ARGV_INT, NULL, NULL,
      "Save (1) or don't save (0) mouse commands as history"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define saoSaveMouseCommands_name "saoSaveMouseCommands"

static int
tclSaoSaveMouseCommands(ClientData clientData,
			Tcl_Interp *interp,
			int ac,
			char **av)
{
   int i;
   int yesno = 0;			/* do/don't save mouse cmds as hist */

   shErrStackClear();

   i = 1;
   saoSaveMouseCommands_opts[i++].dst = &yesno;
   shAssert(saoSaveMouseCommands_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, saoSaveMouseCommands_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     saoSaveMouseCommands_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   save_mouse_commands = yesno ? 1 : 0;

   return(TCL_OK);
}


/*
 * ROUTINE:
 *   shTclSaoDisplayDeclare
 *
 * CALL:
 *   (void) shTclSaoDisplayDeclare(
 *               Tcl_Interp *a_interp  // IN: TCL interpreter
 *          )
 *
 * DESCRIPTION:
 *   The world's access to SAO display function.
 *
 * RETURN VALUE:
 *   Nothing
 */
void
shTclSaoDisplayDeclare(Tcl_Interp *a_interp)
{
   char *helpFacil = "shFsao";
 
   shTclDeclare(a_interp, "saoDisplay", (Tcl_CmdProc *) tclSaoDisplay,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                tclSaoDisplayHelp, tclSaoDisplayUse);
 
   shTclDeclare(a_interp, "saoDel", (Tcl_CmdProc *) tclSaoDel,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                tclSaoDelHelp, tclSaoDelUse);
 
   shTclDeclare(a_interp,saoSaveMouseCommands_name,
		(Tcl_CmdProc *)tclSaoSaveMouseCommands, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		helpFacil, tclSaoSaveMouseCommands_hlp,
		tclSaoSaveMouseCommands_use);
}

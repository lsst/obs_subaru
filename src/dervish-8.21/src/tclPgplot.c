/* tclPgplot.c --
 *
 *	Driver for TCL of PGPLOT
 *
 *  26-Jan-1993	Penelope Constanta-Fanourakis
 *
 */

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <alloca.h>

#include "ftcl.h"
#include "tclExtend.h"
#include "cpgplot.h"
#include "pgplot_c.h"
#include "shTclUtils.h"
#include "region.h"			/* prototype of PIXDATATYPE */
#include "shCSao.h"			/* prototype of CMD_HANDLE */
#include "shCFitsIo.h"			/* definition of MAXDDIRLEN */
#include "shTclVerbs.h"
#include "shTclParseArgv.h"
#include "shCUtils.h"
#include "shCGarbage.h"
#include "shCEnvscan.h"

/*
 *	The following prototyping is due to FTCL problems...
 */

/* 
 *  Global variables for this package
 */

/* The following statments define error returns from gmfplot_. */
#define NO_ERR          0
#define OPEN_ERR        1
#define DEVICE_OPEN_ERR 2
#define PRECISION_ERR   3
#define CMD_ERR         4
#define SET_3D_MODE_ERR 5
#define VERTICES_ERR    6
#define ILLEGAL_CMD_ERR 7

#define SH_ARRAY_LEN_F 128
#define SH_SYSCMD_LEN   L_tmpnam + 5 + 200
static char *pg_tcl_print;
static char *pg_tcl_xtrans;
static char *pg_tcl_ytrans;
static int  pg_tcl_error;

/* The following variable is set to 1 after shTclPgplotDeclare has been called.
   This is done since shTclPgplotDeclare is called in dervish_template (which we
   cannot change), and needs to be called in shMainTcl_Declare so the users
   only need call that one routine to get all of the dervish verbs. */
static int shPg_shTclPgplotDeclare_called = 0;

/************************  private utility routines *******************************/
static void pg_tcl_plot (int *visble, float *x, float *y, float *z)
{
   float  xworld, yworld;
   double tmp;
   char   *cmd;
   Tcl_Interp *new_interp;

   new_interp = Tcl_CreateInterp();

   cmd = (char *) alloca ((strlen(pg_tcl_xtrans) + 10));
   if (cmd == (char *)0)
      {
       if (pg_tcl_error == 0) 
          {
          printf ("pg_tcl_plot error allocating internal buffer space\n");
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return;
       }
      
   sprintf (cmd, "set x %f", *x); Tcl_Eval(new_interp, cmd);

   sprintf (cmd, "set y %f", *y); Tcl_Eval(new_interp, cmd);

   sprintf (cmd, "set z %f", *z); Tcl_Eval(new_interp, cmd);

   sprintf (cmd, "expr {%s}", pg_tcl_xtrans);
   if (Tcl_Eval(new_interp, cmd) == TCL_ERROR)
     {
     if (strlen(new_interp->result) > 0)
       {
       if (pg_tcl_error == 0) 
          {
          printf ("pg_tcl_plot error evaluating x transformation: %s\n", new_interp->result);
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return;
       }
     }

    if (Tcl_GetDouble(new_interp, new_interp->result, &tmp) != TCL_OK) 
       {
       Tcl_DeleteInterp(new_interp);
       return; 
       }
   xworld = (float)tmp;

   cmd = (char *) alloca ((strlen(pg_tcl_ytrans) + 10));
   if (cmd == (char *)0)
      {
       if (pg_tcl_error == 0) 
          {
          printf ("pg_tcl_plot error allocating internal buffer space\n");
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return;
       }
   sprintf (cmd, "expr {%s}", pg_tcl_ytrans);
   if (Tcl_Eval(new_interp, cmd) == TCL_ERROR)
     {
     if (strlen(new_interp->result) > 0)
       {
       if (pg_tcl_error == 0)
          {
          printf ("pg_tcl_plot error evaluating y transformation: %s\n", new_interp->result);
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return;
       }
     }

    if (Tcl_GetDouble(new_interp, new_interp->result, &tmp) != TCL_OK) 
       {
       Tcl_DeleteInterp(new_interp);
       return; 
       }
   yworld = (float)tmp;

   if (*visble == 0)
      cpgmove(xworld, yworld);
   else
      cpgdraw(xworld, yworld);

   Tcl_DeleteInterp(new_interp);
}

static float pg_tcl_fxt (float *t)
{
   double tmp;
   float  x;
   char   *cmd;
   Tcl_Interp *new_interp;

   new_interp = Tcl_CreateInterp();
   TclX_Init (new_interp);

   cmd = (char *) alloca ((strlen(pg_tcl_xtrans) + 10));
   if (cmd == (char *)0)
      {
       if (pg_tcl_error == 0) 
          {
          printf ("pg_tcl_fxt error allocating internal buffer space\n");
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return (0.0);
       }

   sprintf (cmd, "set t %f", *t); Tcl_Eval(new_interp, cmd);

   x = 0.0;
   sprintf (cmd, "expr {%s}", pg_tcl_xtrans);
   if (Tcl_Eval(new_interp, cmd) == TCL_ERROR)
     {
     if (strlen(new_interp->result) > 0)
       {
       if (pg_tcl_error == 0)
          {
          printf ("pg_tcl_fxt error evaluating x coordinate: %s\n", new_interp->result);
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return x;
       }
     }

    if (Tcl_GetDouble(new_interp, new_interp->result, &tmp) != TCL_OK) 
       {
       Tcl_DeleteInterp(new_interp);
       return x;
       }
    x = (float) tmp;

    Tcl_DeleteInterp(new_interp);
    return x;
}

static float pg_tcl_fyt (float *t)
{
   double tmp;
   float  y;
   char   *cmd;
   Tcl_Interp *new_interp;

   new_interp = Tcl_CreateInterp();
   TclX_Init (new_interp);

   cmd = (char *) alloca ((strlen(pg_tcl_ytrans) + 10));
   if (cmd == (char *)0)
      {
       if (pg_tcl_error == 0) 
          {
          printf ("pg_tcl_fyt error allocating internal buffer space\n");
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return (0.0);
       }

   sprintf (cmd, "set t %f", *t); Tcl_Eval(new_interp, cmd);

   y = 0.0;
   sprintf (cmd, "expr {%s}", pg_tcl_ytrans);
   if (Tcl_Eval(new_interp, cmd) == TCL_ERROR)
     {
     if (strlen(new_interp->result) > 0)
       {
       if (pg_tcl_error == 0)
          {
          printf ("pg_tcl_fyt error evaluating y coordinate: %s\n", new_interp->result);
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return y;
       }
     }

    if (Tcl_GetDouble(new_interp, new_interp->result, &tmp) != TCL_OK) 
       {
       Tcl_DeleteInterp(new_interp);
       return y;
       }
    y = (float) tmp;

    Tcl_DeleteInterp(new_interp);
    return y;
}

static float pg_tcl_fx (float *y)
{
   double tmp;
   float  x;
   char   *cmd;
   Tcl_Interp *new_interp;

   new_interp = Tcl_CreateInterp();
   TclX_Init (new_interp);

   cmd = (char *) alloca ((strlen(pg_tcl_xtrans) + 10));
   if (cmd == (char *)0)
      {
       if (pg_tcl_error == 0) 
          {
          printf ("pg_tcl_fx error allocating internal buffer space\n");
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return(0.0);
       }

   sprintf (cmd, "set y %f", *y); Tcl_Eval(new_interp, cmd);

   x = 0.0;
   sprintf (cmd, "expr {%s}", pg_tcl_xtrans);
   if (Tcl_Eval(new_interp, cmd) == TCL_ERROR)
     {
     if (strlen(new_interp->result) > 0)
       {
       if (pg_tcl_error == 0)
          {
          printf ("pg_tcl_fx error evaluating x coordinate: %s\n", new_interp->result);
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return x;
       }
     }

    if (Tcl_GetDouble(new_interp, new_interp->result, &tmp) != TCL_OK) 
       {
       Tcl_DeleteInterp(new_interp);
       return x;
       }
    x = (float) tmp;

    Tcl_DeleteInterp(new_interp);
    return x;
}

static float pg_tcl_fy (float *x)
{
   double tmp;
   float  y;
   char   *cmd;
   Tcl_Interp *new_interp;

   new_interp = Tcl_CreateInterp();
   TclX_Init (new_interp);

   cmd = (char *) alloca ((strlen(pg_tcl_ytrans) + 10));
   if (cmd == (char *)0)
      {
       if (pg_tcl_error == 0) 
          {
          printf ("pg_tcl_fy error allocating internal buffer space\n");
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return(0.0);
       }

   sprintf (cmd, "set x %f", *x); Tcl_Eval(new_interp, cmd);

   y = 0.0;
   sprintf (cmd, "expr {%s}", pg_tcl_ytrans);
   if (Tcl_Eval(new_interp, cmd) == TCL_ERROR)
     {
     if (strlen(new_interp->result) > 0)
       {
       if (pg_tcl_error == 0)
          {
          printf ("pg_tcl_fy error evaluating y coordinate: %s\n", new_interp->result);
          pg_tcl_error++;
          }
       Tcl_DeleteInterp(new_interp);
       return y;
       }
     }

    if (Tcl_GetDouble(new_interp, new_interp->result, &tmp) != TCL_OK) 
       {
       Tcl_DeleteInterp(new_interp);
       return y;
       }
    y = (float) tmp;

    Tcl_DeleteInterp(new_interp);
    return y;
}

/************************  Tcl for pgplot routines  *******************************/
static char *pg_Get_hlp = 
  "Retrieve a plot previously stored in a disk file \n   and send it to a supported Pgplot device.";
static char *pg_Get_use =
          "USAGE: pgGet <filename> [-o outputFilename] [-d outputDevice]";
/*============================================================================
**============================================================================
**
** ROUTINE: shTclPgGet
**
** Description:
**      Retrieve a pgplot file in the 'GMF' format previously written to disk
**      and display the plot on the requested device.  All devices supported
**      by pgplot are allowed.  The filename parameter can be a TCL list of
**      files.  So can the outputFilename option.
**
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** CALLS TO:
**      gmfplot
**      Tcl_SetResult
**      Tcl_AppendResult
**
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static int shTclPgGet
   (
   ClientData   a_clientData,   /* IN: */
   Tcl_Interp   *a_interp,      /* IN: */
   int          a_argc,         /* IN: */
   char         **a_argv        /* IN: */
   )
{
long int    gstatus, gerr;
int         rstatus = TCL_OK;
int         i = 1, j;
int         uDELen, dlen, flen;
int         maxArgs = 6;
char        *tmpPtr;
char        *fileName = NULL;
char        *deviceName = NULL;
int         deviceNameLen = 0;
char        *outputFileName = NULL;
static char defaultDeviceName[] = "/XWINDOW";
int         listArgc, oListArgc;
char        **listArgv = NULL, **oListArgv = NULL;
char        ebuf[150];
char        userDir[MAXDDIRLEN];
char        userExt[MAXDEXTLEN];
char        fileName_f[SH_ARRAY_LEN_F];   /* Length of buffers for gmfplot_. */
char        deviceName_f[SH_ARRAY_LEN_F]; /*   They must be space filled. */

deviceName_f[0] = '\0';    /* Nothing here yet */

/* Parse the command */
if (a_argc > maxArgs)
   {
   /* Too many arguments on the command line */
   rstatus = TCL_ERROR;
   Tcl_SetResult(a_interp, "Syntax error - too many arguments\n",
                 TCL_VOLATILE);
   Tcl_AppendResult(a_interp, pg_Get_use, (char *)(NULL));
   }
else if (a_argc == 1)
   {
   /* No arguments, output the usage text */
   Tcl_SetResult(a_interp, pg_Get_use, TCL_VOLATILE);
   }
else if (((a_argc/2)*2) != a_argc)
   {
   /* a_argc is odd - it can only have values of 1, 2, 4, or 6 due to the
      fact that all the options all have values, and the case where argc
      equal 1 was taken care of above. */
   rstatus = TCL_ERROR;
   Tcl_SetResult(a_interp, "Invalid command line - check syntax.\n",
		 TCL_VOLATILE);
   Tcl_AppendResult(a_interp, pg_Get_use, (char *)(NULL));
   }
else
   {
   while (i < a_argc)
      {
      if (a_argv[i][0] == '-')   /* is an option */
         {
         switch (a_argv[i][1])
	    {
            case 'd':
               /* Get the device name */
	       deviceName = a_argv[++i];
	       deviceNameLen = strlen(deviceName);
	       break;
	    case 'o':
	       /* Get the output file name */
	       outputFileName = a_argv[++i];
	       break;
	    default:
               /* Invalid option argument */
	       rstatus = TCL_ERROR;
	       i = a_argc;                    /* Get out of while loop */
	       Tcl_SetResult(a_interp, "Invalid option\n", TCL_VOLATILE);
	       Tcl_AppendResult(a_interp, pg_Get_use, (char *)(NULL));
	       return(rstatus);
	    }
	 }
      else
	 {
	 /* Must be the filename */
	 fileName = a_argv[i];
	 }
      ++i;                 /* Advance to the next argument */
      }

   /* Now see which arguments we found and fill in defaults for others */
   if (deviceName == '\0')            /* No device name entered */
      {deviceName = defaultDeviceName;}

   if (fileName == NULL)                   /* No filename entered */
      {
      rstatus = TCL_ERROR;
      Tcl_SetResult(a_interp, "Syntax error - a filename must be entered\n",
                    TCL_VOLATILE);
      Tcl_AppendResult(a_interp, pg_Get_use, (char *)(NULL));
      }
   else
      {
      /* See if the filename passed us is really a Tcl list.
         If so , we must call gmfplot for each file that was entered. */
      if (Tcl_SplitList(a_interp, fileName, &listArgc, &listArgv)
                        != TCL_OK)
         {
         rstatus = TCL_ERROR;
         Tcl_SetResult(a_interp,
             "Error from Tcl_SplitList when trying to split filename parameter. Check syntax.",
             TCL_VOLATILE);
         }
      }

   if (outputFileName != NULL)             /* An output filename was entered */
      {
      /* The filename may be a Tcl list, so split it if possible.
         There had better be the same number of output filenames as
	 there were input filenames. */
      if (Tcl_SplitList(a_interp, outputFileName, &oListArgc, &oListArgv)
                        != TCL_OK)
         {
         rstatus = TCL_ERROR;
         Tcl_SetResult(a_interp,
             "Error from Tcl_SplitList when trying to split output filename parameter. Check syntax.",
             TCL_VOLATILE);
         }
      else
	 {
	 /* Make sure the number of input filenames matches the number of
	    output ones. */
	 if (listArgc != oListArgc)
	    {
	    rstatus = TCL_ERROR;
	    sprintf(ebuf,
                 "Number of input filenames (%d) \n   must equal number of output filenames (%d).\n",
		    listArgc, oListArgc);
	    Tcl_SetResult(a_interp, ebuf, TCL_VOLATILE);
	    }
	 }
      }

   /* Continue only if no error was found above */
   if (rstatus != TCL_ERROR)
      {
      /* Now get any directory spec that should be tacked onto the front of
	 the filename. And an extension to be tacked on the end. */
      shDirGet(DEF_PLOT_FILE, userDir, userExt);
      uDELen = strlen(userDir) + strlen(userExt);      /* used below */

      for (i = 0 ; i < listArgc ; ++i)
         {
         tmpPtr = deviceName_f;               /* Need this for strcat. */
         if (outputFileName == NULL)
            {strcpy(deviceName_f,deviceName);}   /* None was entered */
         else
            {
            /* Check to see that the device and file name will fit */
            if ((deviceNameLen + strlen(oListArgv[i])) > SH_ARRAY_LEN_F)
	       {
	       rstatus = TCL_ERROR;
	       Tcl_SetResult(a_interp,
                      "Device+Output File Names too large for pgplot array.\n",
			     TCL_VOLATILE);
	       Tcl_AppendResult(a_interp,
				"Device & File Name must be less than %d.\n",
				SH_ARRAY_LEN_F, (char *)(NULL));
	       break;                  /* Get out of loop */
	       }
	    else
	       {
	       strcpy(deviceName_f, oListArgv[i]);
	       tmpPtr = strcat(tmpPtr, deviceName);
	       }
	    }

         /* Space fill the rest of the device name array */
	 dlen = strlen(tmpPtr);
         for ( j = dlen ; j < SH_ARRAY_LEN_F ; ++j)
            {tmpPtr[j] = ' ';}

         /* Tack on the directory and extension to the file name. Make sure
            there is room first. */
         if ((strlen(listArgv[i]) + uDELen) > SH_ARRAY_LEN_F)
            {
            /* There is not enough room in the array for the file name. */
            rstatus = TCL_ERROR;
            Tcl_SetResult(a_interp,"Filename too large for pgplot array.\n",
			  TCL_VOLATILE);
	    Tcl_AppendResult(a_interp,
			     "Name & directory must be less than %d.\n",
			     SH_ARRAY_LEN_F, (char *)(NULL));
	    break;                  /* Get out of loop */
	    }
         else
            {
            tmpPtr = fileName_f;
            strcpy(fileName_f, userDir);         /* Start with directory, */
            tmpPtr = strcat(tmpPtr, listArgv[i]);    /*    filename, */
            tmpPtr = strcat(tmpPtr, userExt);        /*    extension */

            /* Space fill the rest of the array */
	    flen = strlen(tmpPtr);
            for ( j = flen ; j < SH_ARRAY_LEN_F ; ++j)
	       {tmpPtr[j] = ' ';}

	    gstatus = pgplot_gmfplot(fileName_f, deviceName_f, &gerr);

	    /* free the space allocated by Tcl_SplitList */
	    if (listArgv != NULL) free((char *)listArgv);
	    if (oListArgv != NULL) free((char *)oListArgv);

	    /* Make sure the strings are null terminated */
	    fileName_f[flen] = '\0';
	    deviceName_f[dlen] = '\0';

	    /* Now handle any errors - cast to int to avoid warning under osf1.
	       The values are small anyway. */
	    switch ((int )gstatus)
	      {
	      case NO_ERR:
		break;
	      case OPEN_ERR:
		rstatus = TCL_ERROR;
		sprintf(ebuf, "OPEN ERROR - %ld - while opening file - %s.\n",
			gerr, fileName_f);
		Tcl_SetResult(a_interp, ebuf, TCL_VOLATILE);
		break;
	      case DEVICE_OPEN_ERR:
		rstatus = TCL_ERROR;
		sprintf(ebuf, "OPEN ERROR - %ld - while opening device - %s.\n",
			gerr, deviceName_f);
		Tcl_SetResult(a_interp, ebuf, TCL_VOLATILE);
		break;
	      case PRECISION_ERR:
		rstatus = TCL_ERROR;
		Tcl_SetResult(a_interp,
	         "BEGIN_METAFILE:command does not request 15-bit precision.\n",
			      TCL_VOLATILE);
		break;
	      case CMD_ERR:
		rstatus = TCL_ERROR;
		Tcl_SetResult(a_interp,
			      "BEGIN_METAFILE : command is misplaced.\n",
			      TCL_VOLATILE);
		break;
	      case SET_3D_MODE_ERR:
		rstatus = TCL_ERROR;
		Tcl_SetResult(a_interp,"SET_3D_MODE command not allowed.\n",
			      TCL_VOLATILE);
		break;
	      case VERTICES_ERR:
		rstatus = TCL_ERROR;
		Tcl_SetResult(a_interp,
		        "DRAW_POLYGON : command has more than 512 vertices.\n",
			      TCL_VOLATILE);
		break;
	      case ILLEGAL_CMD_ERR:
		rstatus = TCL_ERROR;
		sprintf(ebuf, "GMFPLOT, unrecognized command 0x%04lx.\n", gerr);
		Tcl_SetResult(a_interp, ebuf, TCL_VOLATILE);
	      }
	    }
	 }
      }
   }

return(rstatus);
}
static char *pg_Save_hlp = 
  "Save the current plot in a metafile disk file.";
static char *pg_Save_use =
          "USAGE: pgSaveMf <outputFilename>";
/*============================================================================
**============================================================================
**
** ROUTINE: shTclPgSave
**
** Description:
**      Save a plot in a pgplot metafile file.  Previous to creating the
**      plot, the environmental variable PGPLOT_SAVE must have been defined
**      [to anything].  When the plot is created, the commands are also saved 
**      in a unique temporary file.  This routine just copies the temporary
**      file to a user specified area.  The directory and file extension may
**      be set using dirSet.
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERRORS
**
** CALLS TO:
**      Tcl_SetResult
**      Tcl_AppendResult
**
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static int shTclPgSave
   (
   ClientData   a_clientData,   /* IN: */
   Tcl_Interp   *a_interp,      /* IN: */
   int          a_argc,         /* IN: */
   char         **a_argv        /* IN: */
   )
{
int         rstatus = TCL_OK;
int         uDELen, totalLen;
long int    mlen;
int         maxArgs = 2;
char        *tmpPtr;
char        *fileName = NULL;
char        userDir[MAXDDIRLEN];
char        userExt[MAXDEXTLEN];
char        syscmd[SH_SYSCMD_LEN];
char        metaFile[L_tmpnam];     /* Max size as specified in stdio.h */
static char copyCmd[4] = "cp ";
static char space[2]   = " ";

/* Parse the command */
if (a_argc > maxArgs)
   {
   /* Too many arguments on the command line */
   rstatus = TCL_ERROR;
   Tcl_SetResult(a_interp, "Syntax error - too many arguments\n",
                 TCL_VOLATILE);
   Tcl_AppendResult(a_interp, pg_Save_use, (char *)(NULL));
   }
else if (a_argc == 1)
   {
   /* No arguments, output the usage text */
   Tcl_SetResult(a_interp, pg_Save_use, TCL_VOLATILE);
   }
else
   {
   /* Must be the filename */
   fileName = a_argv[1];

   /* Now get any directory spec that should be tacked onto the front of
      the filename. And an extension to be tacked on the end. */
   shDirGet(DEF_PLOT_FILE, userDir, userExt);
   uDELen = strlen(userDir) + strlen(userExt);      /* used below */

   /* Get the temporary file that the metafile data was written to. This
      filename is in a FORTRAN common. */
   if ((mlen = pgplot_getmfile(metaFile)) == 0)
      {
      /* There was no metaFile name stored.  Either PGPLOT_SAVE is not defined
	 or no plot has been created yet. */
      Tcl_SetResult(a_interp,
                "No plot buffered to save.  Make sure PGPLOT_SAVE is defined.",
		   TCL_VOLATILE);
      rstatus = TCL_ERROR;
      }
   else
      {
      /* Null terminate the file name. It came from beyond (oops, i mean
	 FORTRAN) so who knows what it ends with. */
      metaFile[mlen] = '\0';

      /* Make sure that the copy command will fit in the static buffer.  We
	 do not want to malloc each time this is called so hopefully the
	 buffer has been made large enough. If not, malloc. */
      totalLen = 4 + strlen(metaFile) + strlen(fileName) + uDELen;
      if (totalLen > SH_SYSCMD_LEN)
	 {
	 /* We do not have enough room in the buffer, malloc enough */
	 if ((tmpPtr = (char *)alloca(totalLen)) == NULL)
	    {
	    /* Could not malloc enough space. Things are seriously wrong. */
	    shFatal(
		"shTclPgSave: cannot alloca space (%d bytes) for copy command",
		    totalLen);
	    }
	 }
      else
	 {
	 /* There is enough room in the current buffer */
	 tmpPtr = syscmd;
	 }

      /* Concatenate the following together - 
	 "cp"   metaFile   userDir   filename   userExt
	 This will make the text string that says copy the temporary
	 metafile to the user defined area and file. */
      strcpy(tmpPtr, copyCmd);               /* "cp " */
      tmpPtr = strcat(tmpPtr, metaFile);     /* metaFile name */
      tmpPtr = strcat(tmpPtr, space);        /* " " */
      tmpPtr = strcat(tmpPtr, userDir);      /* default directory */
      tmpPtr = strcat(tmpPtr, fileName);     /* user filename */
      tmpPtr = strcat(tmpPtr, userExt);      /* default extension */

      /* Execute the command at the system level */
      system(tmpPtr);

      /* Now delete the temporary file */
      unlink(metaFile);
      }
   }

return(rstatus);
}

static char *pg_ask_hlp = "Change the prompt state ";
static char *pg_ask_use = "USAGE: pgAsk flag";

static int pg_ask (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int flag;
   
   if (argc == 2){
      if (Tcl_GetInt(interp, argv[1], &flag)   != TCL_OK) return(TCL_ERROR);
      cpgask(flag);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_ask_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_bbuf_hlp = "Begin saving graphical output commands in an internal buffer ";
static char *pg_bbuf_use = "USAGE: pgBbuf";
static int pg_bbuf (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1) {
      cpgbbuf();
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_bbuf_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_begin_hlp = "Begin PGPLOT, open the plot file ";
static char *pg_begin_use = "USAGE: pgBegin [device [xsub ysub]]";

static int  pg_begin (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int unit;
   int nxsub, nysub;
   char string[80]; 

   if ((argc != 1) && (argc != 2) && (argc != 4))
      {
      Tcl_SetResult(interp, pg_begin_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   unit = 0;
   nxsub = 1;
   nysub = 1;
   strcpy(string,"/XWINDOW");

   if (argc == 2)
      { 
      strcpy(string, argv[1]); 
      }

   if (argc == 4)
      { 
      strcpy(string, argv[1]); 
      if (Tcl_GetInt(interp, argv[2], &nxsub)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt(interp, argv[3], &nysub)   != TCL_OK) return(TCL_ERROR);
      }

   if (cpgbeg(unit, string, nxsub, nysub) == 1)
      {
      return (TCL_OK);
      }
   else
      {
      Tcl_SetResult(interp,"Initialization failed", TCL_VOLATILE);
      return (TCL_ERROR);
      }
}

static char *pg_open_hlp = "Open the plot device and make it the current one ";
static char *pg_open_use = "USAGE: pgOpen [device]";

static int  pg_open (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int devNum;
   char string[80], buf[10]; 

   if ((argc != 1) && (argc != 2))
      {
      Tcl_SetResult(interp, pg_open_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   strcpy(string,"/XWINDOW");

   if (argc == 2)
      { 
      strcpy(string, argv[1]); 
      }

   if ((devNum = cpgopen(string)) >= 0)
      {
      sprintf(buf, "%d", devNum);
      Tcl_SetResult(interp, buf, TCL_VOLATILE);
      return (TCL_OK);
      }
   else
      {
      Tcl_SetResult(interp,"Initialization failed", TCL_VOLATILE);
      return (TCL_ERROR);
      }
}
  
static char *pg_ebuf_hlp = "End the batch of graphical commands beggun with last call to pgBbuf ";
static char *pg_ebuf_use = "USAGE: pgEbuf ";

static int  pg_ebuf (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1){
      cpgebuf();
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_ebuf_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_end_hlp = "End PGPLOT, close the plot file, release the graphical device ";
static char *pg_end_use = "USAGE: pgEnd ";

static int  pg_end (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1){
      cpgend();
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_end_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_close_hlp = "Close the current plot device";
static char *pg_close_use = "USAGE: pgClose ";

static int  pg_close (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1){
      cpgclos();
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_close_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_page_hlp = "Advance plotter to a new (sub)page, clearing the screen ";
static char *pg_page_use = "USAGE: pgPage ";

static int  pg_page (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1){
      cpgpage();
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_page_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_paper_hlp = "Change the size of viewing surface ";
static char *pg_paper_use = "USAGE: pgPaper width aspect";

static int  pg_paper (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float width, aspect;
   if (argc == 3){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); width=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); aspect=(float)tmp;
      cpgpap(width, aspect);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_paper_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_updt_hlp = "Update the graphics display ";
static char *pg_updt_use = "USAGE: pgUpdt ";

static int  pg_updt (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1){
      cpgupdt();
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_updt_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_box_hlp = "Annotate viewport with frame, axes, numeric labels, etc ";
static char *pg_box_use = "USAGE: pgBox [xopt xtick nxsub yopt ytick nysub] ";

static int  pg_box (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  xtick, ytick;
   int    nxsub,nysub;
   char   xopt[12];
   char   yopt[12];

   if ((argc != 1) && (argc != 7))
      {
      Tcl_SetResult(interp, pg_box_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   xopt[0] =  0;
   yopt[0] =  0;
   strcpy (xopt, "BCTN");
   strcpy (yopt, "BCNST");
   xtick = 0.0;
   ytick = 0.0;
   nxsub = 0;
   nysub = 0;

   if (argc == 7)
      {
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); xtick=(float)tmp;
      if (Tcl_GetDouble(interp, argv[5], &tmp)   != TCL_OK) return(TCL_ERROR); ytick=(float)tmp;
      if (Tcl_GetInt(interp, argv[3], &nxsub)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt(interp, argv[6], &nysub)   != TCL_OK) return(TCL_ERROR);
      strcpy (xopt, argv[1]);
      strcpy (yopt, argv[4]);
      }

   cpgbox(xopt, xtick,nxsub,yopt,ytick,nysub);
   return (TCL_OK);
}

static char *pg_env_hlp = "Set PGPLOT plotter environment ";
static char *pg_env_use = "USAGE: pgEnv xmin xmax ymin ymax just axis";

static int  pg_env (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  xmin, xmax, ymin, ymax;
   int    just, axis;
   if (argc == 7){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); xmin=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); xmax=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp)   != TCL_OK) return(TCL_ERROR); ymin=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp)   != TCL_OK) return(TCL_ERROR); ymax=(float)tmp;
      if (Tcl_GetInt   (interp, argv[5], &just)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt   (interp, argv[6], &axis)   != TCL_OK) return(TCL_ERROR);

      cpgenv(xmin, xmax, ymin, ymax, just, axis);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_env_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_vport_hlp = "Change size and position of viewport (input in normalized device coordinates)";
static char *pg_vport_use = "USAGE: pgVport xleft xright ybot ytop ";

static int  pg_vport (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  xleft, xright, ybot, ytop;

   if (argc == 5){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); xleft=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); xright=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp)   != TCL_OK) return(TCL_ERROR); ybot=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp)   != TCL_OK) return(TCL_ERROR); ytop=(float)tmp;
      cpgsvp(xleft,xright,ybot,ytop);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_vport_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_vsize_hlp = "Change size and position of viewport (input in inches) ";
static char *pg_vsize_use = "USAGE: pgVsize xleft xright ybot ytop ";

static int  pg_vsize (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  xleft, xright, ybot, ytop;

   if (argc == 5){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); xleft=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); xright=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp)   != TCL_OK) return(TCL_ERROR); ybot=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp)   != TCL_OK) return(TCL_ERROR); ytop=(float)tmp;
      cpgvsiz(xleft,xright,ybot,ytop);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_vsize_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_vstand_hlp = "Define viewport to be the standard viewport ";
static char *pg_vstand_use = "USAGE: pgVstand ";

static int  pg_vstand (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1){
      cpgvstd();
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_vstand_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_window_hlp = "Change the window and adjust viewport ";
static char *pg_window_use = "USAGE: pgWindow x1 x2 y1 y2 ";

static int  pg_window (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x1, x2, y1, y2;
   if (argc == 5){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); x1=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); x2=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp)   != TCL_OK) return(TCL_ERROR); y1=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp)   != TCL_OK) return(TCL_ERROR); y2=(float)tmp;
      if (x1 == x2) {x2++; x1--;}
      if (y1 == y2) {y2++;  y1--;}

      cpgswin(x1, x2, y1, y2);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_window_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_wnad_hlp = "Change the window and adjust viewport ";
static char *pg_wnad_use = "USAGE: pgWnad x1 x2 y1 y2 ";

static int  pg_wnad (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x1, x2, y1, y2;
   if (argc == 5){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); x1=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); x2=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp)   != TCL_OK) return(TCL_ERROR); y1=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp)   != TCL_OK) return(TCL_ERROR); y2=(float)tmp;
      cpgwnad(x1, x2, y1, y2);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_wnad_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_draw_hlp = "Draw a line from current position to world-coordinates (x,y) ";
static char *pg_draw_use = "USAGE: pgDraw x y ";

static int  pg_draw (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x, y;
   if (argc == 3){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); x=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); y=(float)tmp;
      cpgdraw(x, y);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_draw_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_line_hlp = "Draw a polyline ";
static char *pg_line_use = "USAGE: pgLine xpts ypts ";

static int  pg_line (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int   nx, ny;
   float *xpts, *ypts;

   if (argc == 3){
      xpts = ftclList2Floats (interp, argv[1], &nx, 0); 
      if (xpts == (float *)0) {return (TCL_ERROR);}
      ypts = ftclList2Floats (interp, argv[2], &ny, 0); 
      if (ypts == (float *)0) {free(xpts); return (TCL_ERROR);}
      if (nx != ny) 
         {
         free (xpts); 
         free(ypts); 
         Tcl_SetResult(interp, "pgLine error: Arrays must be of equal length", TCL_VOLATILE);
         return (TCL_ERROR);
         }

      cpgline(nx, xpts, ypts);
      free (xpts); 
      free (ypts);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_line_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_move_hlp = "Move pen to world coordinates (x,y) ";
static char *pg_move_use = "USAGE: pgMove x y ";

static int  pg_move (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x, y;

   if (argc == 3){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); x=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); y=(float)tmp;
      cpgmove(x, y);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_move_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_point_hlp = "Draw graph markers (polymarkers) ";
static char *pg_point_use = "USAGE: pgPoint xpts ypts symbol ";

static int  pg_point (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int   symb;
   int   nx, ny;
   float *xpts, *ypts;

   if (argc == 4){
      if (Tcl_GetInt     (interp, argv[3], &symb) != TCL_OK) return(TCL_ERROR);
      xpts = ftclList2Floats (interp, argv[1], &nx, 0);
      if (xpts == (float *)0) {return (TCL_ERROR);}
      ypts = ftclList2Floats (interp, argv[2], &ny, 0); 
      if (ypts == (float *)0) {free(xpts); return (TCL_ERROR);}
      if (nx != ny) {
         free (xpts); free(ypts); 
         Tcl_SetResult(interp, "pgPoint error: Arrays must be of equal length", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      cpgpt(nx, xpts, ypts, symb);
      free (xpts); free (ypts);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_point_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_poly_hlp = "Shade the interior of a closed polygon in current window ";
static char *pg_poly_use = "USAGE: pgPoly xpts ypts ";

static int  pg_poly (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int   nx, ny;
   float *xpts, *ypts;

   if (argc == 3){
      xpts = ftclList2Floats (interp, argv[1], &nx, 0); 
      if (xpts == (float *)0) {return (TCL_ERROR);}
      ypts = ftclList2Floats (interp, argv[2], &ny, 0); 
      if (ypts == (float *)0) {free(xpts); return (TCL_ERROR);}
      if (nx != ny) {
         free(xpts); free(ypts); 
         Tcl_SetResult(interp, "pgPoly error: Arrays must be of equal length", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      cpgpoly(nx, xpts, ypts);
      free (xpts); free (ypts);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_poly_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_rect_hlp = "Draw a rectagle aligned with coordinate axes ";
static char *pg_rect_use = "USAGE: pgRect x1 x2 y1 y2 ";

static int  pg_rect (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x1, x2, y1, y2;

   if (argc == 5){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); x1=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); x2=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp)   != TCL_OK) return(TCL_ERROR); y1=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp)   != TCL_OK) return(TCL_ERROR); y2=(float)tmp;
      cpgrect(x1, x2, y1, y2);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_rect_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_label_hlp = "Write labels outside the viewport ";
static char *pg_label_use = "USAGE: pgLabel xlbl ylbl [toplbl]";

static int  pg_label (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   char xlbl[200];
   char ylbl[200];
   char tlbl[200];

   strcpy (tlbl, "");
 
   if ((argc != 3) && (argc != 4))
      {
      Tcl_SetResult(interp, pg_label_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   strcpy (xlbl, argv[1]);

   strcpy (ylbl, argv[2]);

   if (argc == 4)
      {
      strcpy (tlbl, argv[3]);
      }

   cpglab(xlbl, ylbl, tlbl);
   return (TCL_OK);
}

static char *pg_mtext_hlp = "Write text at position specified relative to viewport ";
static char *pg_mtext_use = "USAGE: pgMtext side disp coord fjust text ";

static int  pg_mtext (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  disp, coord, fjust;

   if (argc == 6){
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); disp=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp)   != TCL_OK) return(TCL_ERROR); coord=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp)   != TCL_OK) return(TCL_ERROR); fjust=(float)tmp;
      cpgmtxt(argv[1], disp,coord,fjust,argv[5]);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_mtext_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_ptext_hlp = "Primitive for drawing text ";
static char *pg_ptext_use = "USAGE: pgPtext x y angle fjust text ";

static int  pg_ptext (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x, y, angle, fjust;

   if (argc == 6){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); x=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); y=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp)   != TCL_OK) return(TCL_ERROR); angle=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp)   != TCL_OK) return(TCL_ERROR); fjust=(float)tmp;
      cpgptxt(x,y,angle,fjust,argv[5]);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_ptext_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_text_hlp = "Write text ";
static char *pg_text_use = "USAGE: pgText x y text ";

static int  pg_text (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x, y;

   if (argc == 4){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); x=(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); y=(float)tmp;
      cpgtext(x,y,argv[3]);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_text_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_scf_hlp = "Set the character font for subsequent text plotting ";
static char *pg_scf_use = "USAGE: pgScf if";

static int  pg_scf (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int i;

   if (argc == 2){
      if (Tcl_GetInt   (interp, argv[1], &i)   != TCL_OK) return(TCL_ERROR);
      cpgscf(i);
      return (TCL_OK);
   }else{
    Tcl_SetResult(interp, pg_scf_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_sch_hlp = "Set the character size ";
static char *pg_sch_use = "USAGE: pgSch size ";

static int  pg_sch (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float i;

   if (argc == 2){
      if (Tcl_GetDouble (interp, argv[1], &tmp) != TCL_OK) return(TCL_ERROR);
      i = (float)tmp;
      cpgsch(i);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_sch_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_sci_hlp = "Set the collor index for subsequent plotting ";
static char *pg_sci_use = "USAGE: pgSci ci ";

static int  pg_sci (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int i;

   if (argc == 2 ) {
      if (Tcl_GetInt   (interp, argv[1], &i)   != TCL_OK) return(TCL_ERROR);
      cpgsci(i);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_sci_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_scr_hlp = "Set color representation ";
static char *pg_scr_use = "USAGE: pgScr ci cr cg cb ";

static int  pg_scr (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int    i;
   double tmp;
   float  cr, cg, cb;

   if (argc == 5) {
      if (Tcl_GetInt   (interp, argv[1], &i)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[2], &tmp) != TCL_OK) return(TCL_ERROR); cr=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp) != TCL_OK) return(TCL_ERROR); cg=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp) != TCL_OK) return(TCL_ERROR); cb=(float)tmp;
      cpgscr(i, cr, cg, cb);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_scr_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_sfs_hlp = "Set the fill-area style attribute ";
static char *pg_sfs_use = "USAGE: pgSfs fs ";

static int  pg_sfs (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int i;

   if (argc == 2){
      if (Tcl_GetInt   (interp, argv[1], &i)   != TCL_OK) return(TCL_ERROR);
      cpgsfs(i);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_sfs_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_shls_hlp = "Set color representation ";
static char *pg_shls_use = "USAGE: pgShls ci ch cl cs ";

static int  pg_shls (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int    i;
   double tmp;
   float  ch, cl, cs;

   if (argc == 5) {
      if (Tcl_GetInt   (interp, argv[1], &i)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[2], &tmp) != TCL_OK) return(TCL_ERROR); ch=(float)tmp;
      if (Tcl_GetDouble(interp, argv[3], &tmp) != TCL_OK) return(TCL_ERROR); cl=(float)tmp;
      if (Tcl_GetDouble(interp, argv[4], &tmp) != TCL_OK) return(TCL_ERROR); cs=(float)tmp;
      cpgshls(i, ch, cl, cs);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_shls_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_shs_hlp = "Set the hatching style attribute ";
static char *pg_shs_use = "USAGE: pgShs <angle> <spacing> <phase> ";

static int  pg_shs (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double angle, spacing, phase;
   float  fangle, fspacing, fphase;

   if (argc == 4){
      if (Tcl_GetDouble(interp, argv[1], &angle) != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[2], &spacing) != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[3], &phase) != TCL_OK) return(TCL_ERROR);
      fangle = (float )angle;
      fspacing = (float )spacing;
      fphase = (float )phase;
      cpgshs(fangle, fspacing, fphase);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_shs_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_slct_hlp = "Select an open graphics device ";
static char *pg_slct_use = "USAGE: pgSlct <device> ";

static int  pg_slct (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int i;

   if (argc == 2){
      if (Tcl_GetInt   (interp, argv[1], &i)   != TCL_OK) return(TCL_ERROR);
      cpgslct(i);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_slct_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_sls_hlp = "Set the line style attribute ";
static char *pg_sls_use = "USAGE: pgSls ls ";

static int  pg_sls (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int i;

   if (argc == 2){
      if (Tcl_GetInt   (interp, argv[1], &i)   != TCL_OK) return(TCL_ERROR);
      cpgsls(i);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_sls_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_slw_hlp = "Set the line-width attribute ";
static char *pg_slw_use = "USAGE: pgSlw lw ";

static int  pg_slw (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int i;

   if (argc == 2){
      if (Tcl_GetInt   (interp, argv[1], &i)   != TCL_OK) return(TCL_ERROR);
      cpgslw(i);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_slw_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_bin_hlp = "Plot a histogram of NBIN values with X values along ordinate and DATA along abscissa ";
static char *pg_bin_use = "USAGE: pgBin x data [d:center | lowedge] ";

static int  pg_bin (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int   nx, nd;
   float *x, *data;
   int   center;

   if ((argc != 3) && (argc != 4))
      {
      Tcl_SetResult(interp, pg_bin_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   center = 1;
   if (argc == 4)
      {
      center = -1;
      if ( strcmp(argv[3], "center")  == 0)   center = 1;
      if ( strcmp(argv[3], "lowedge") == 0)   center = 0;
      }
   if (center == -1)
      {
      Tcl_SetResult(interp, pg_bin_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   x = ftclList2Floats (interp, argv[1], &nx, 0); 
   if (x == (float *)0) {return (TCL_ERROR);}
   data = ftclList2Floats (interp, argv[2], &nd, 0); 
   if (data == (float *)0) {free(x); return (TCL_ERROR);}
   if (nx != nd) {
      free (x); free(data); 
      Tcl_SetResult(interp, "pgBin error: Arrays must be of equal length", TCL_VOLATILE);
      return (TCL_ERROR);
      }

   cpgbin(nx, x, data, center);
   free (x); 
   free (data);
   return (TCL_OK);
}

static char *pg_cons_hlp = "Draw a contour map of an array (does not draw contours as a continuous line) ";
static char *pg_cons_use = "USAGE: pgCons a i1 i2 j1 j2 c tr ";

static int  pg_cons (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   float *a;
   float *c;
   float *tr;
   int i1, i2;
   int j1, j2;
   int idim, jdim, nc, nt;

   if (argc == 8){
      if (Tcl_GetInt   (interp, argv[2], &i1)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt   (interp, argv[3], &i2)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt   (interp, argv[4], &j1)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt   (interp, argv[5], &j2)   != TCL_OK) return(TCL_ERROR);

      a = ftclLol2FtnFloats (interp, argv[1], &idim, &jdim); 
      if (a == (float *)0) {return (TCL_ERROR);}

      c = ftclList2Floats (interp, argv[6], &nc, 0); 
      if (c == (float *)0) {free(a); return (TCL_ERROR);}

      tr = ftclList2Floats (interp, argv[7], &nt, 0); 
      if (tr == (float *)0) {free(a); free (c); return (TCL_ERROR);}
      if (nt != 6) {
         free (a); free(c); free(tr); 
         Tcl_SetResult(interp, "pgCons error: TR array must have 6 elements", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      
      cpgcons(a, idim, jdim, i1, i2, j1, j2, c, nc, tr);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_cons_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_cont_hlp = "Draw a contour map of an array ";
static char *pg_cont_use = "USAGE: pgCont a i1 i2 j1 j2 c tr [d:auto | current] ";

static int  pg_cont (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   float *a;
   float *c;
   float *tr;
   int i1, i2;
   int j1, j2;
   int idim, jdim, nc, nt;
   int ii;

   if ((argc != 8) && (argc != 9))
      {
      Tcl_SetResult(interp, pg_cont_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }
   if (Tcl_GetInt   (interp, argv[2], &i1)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[3], &i2)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[4], &j1)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[5], &j2)   != TCL_OK) return(TCL_ERROR);

   a = ftclLol2FtnFloats (interp, argv[1], &idim, &jdim); 
   if (a == (float *)0) {return (TCL_ERROR);}

   c = ftclList2Floats (interp, argv[6], &nc, 0); 
   if (c == (float *)0) {free(a); return (TCL_ERROR);}

   ii = 1;
   if (argc == 9)
      {
      ii = 0;
      if ( strcmp(argv[8], "auto")    == 0)   ii = 1;
      if ( strcmp(argv[8], "current") == 0)   ii = -1;
      }
   if (ii == 0)
      {
      Tcl_SetResult(interp, pg_cont_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   nc *= ii;

   tr = ftclList2Floats (interp, argv[7], &nt, 0); 
   if (tr == (float *)0) {free(a); free (c); return (TCL_ERROR);}
   if (nt != 6) {
      free (a); free(c); free(tr); 
      Tcl_SetResult(interp, "pgCont error: TR array must have 6 elements", TCL_VOLATILE);
      return (TCL_ERROR);
      }
      
   cpgcont(a, idim, jdim, i1, i2, j1, j2, c, nc, tr);
   free (a); free(c); free(tr); 
   return (TCL_OK);
}

static char *pg_conb_hlp = "Draw contour map of an array ignoring elements with value BLANK ";
static char *pg_conb_use = "USAGE: pgConb a i1 i2 j1 j2 c tr blank [d:auto | current] ";

static int  pg_conb (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   float *a;
   float *c;
   float *tr;
   double tmp;
   float blank;
   int i1, i2;
   int j1, j2;
   int idim, jdim, nc, nt;
   int ii;

   if ((argc != 9) && (argc != 10))
      {
      Tcl_SetResult(interp, pg_conb_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }
   if (Tcl_GetInt   (interp, argv[2], &i1)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[3], &i2)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[4], &j1)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[5], &j2)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble(interp, argv[8], &tmp) != TCL_OK) return(TCL_ERROR); blank=(float)tmp;

   a = ftclLol2FtnFloats (interp, argv[1], &idim, &jdim); 
   if (a == (float *)0) {return (TCL_ERROR);}

   c = ftclList2Floats (interp, argv[6], &nc, 0); 
   if (c == (float *)0) {free(a); return (TCL_ERROR);}

   ii = 1;
   if (argc == 10)
      {
      ii = 0;
      if ( strcmp(argv[9], "auto")    == 0)   ii = 1;
      if ( strcmp(argv[9], "current") == 0)   ii = -1;
      }
   if (ii == 0)
      {
      Tcl_SetResult(interp, pg_conb_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   nc *= ii;

   tr = ftclList2Floats (interp, argv[7], &nt, 0); 
   if (tr == (float *)0) {free(a); free (c); return (TCL_ERROR);}
   if (nt != 6) {
      free (a); free(c); free(tr); 
      Tcl_SetResult(interp, "pgConb error: TR array must have 6 elements", TCL_VOLATILE);
      return (TCL_ERROR);
      }
      
   cpgconb(a, idim, jdim, i1, i2, j1, j2, c, nc, tr, blank);
   free (a); free(c); free(tr); 
   return (TCL_OK);
}


static char *pg_conx_hlp = "Draw a contour map of an array using a user-supplied plotting routine ";
static char *pg_conx_use = "USAGE: pgConx a i1 i2 j1 j2 c xtrans ytrans [d:auto | current] ";

static int  pg_conx (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   float *a;
   float *c;
   int i1, i2;
   int j1, j2;
   int idim, jdim, nc;
   int ii;

   if ((argc != 9) && (argc != 10))
      {
      Tcl_SetResult(interp, pg_conx_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   if (Tcl_GetInt   (interp, argv[2], &i1)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[3], &i2)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[4], &j1)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[5], &j2)   != TCL_OK) return(TCL_ERROR);

   a = ftclLol2FtnFloats (interp, argv[1], &idim, &jdim); 
   if (a == (float *)0) {return (TCL_ERROR);}

   c = ftclList2Floats (interp, argv[6], &nc, 0); 
   if (c == (float *)0) {free(a); return (TCL_ERROR);}

   ii = 1;
   if (argc == 10)
      {
      ii = 0;
      if ( strcmp(argv[9], "auto")    == 0)   ii = 1;
      if ( strcmp(argv[9], "current") == 0)   ii = -1;
      }
   if (ii == 0)
      {
      Tcl_SetResult(interp, pg_conx_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   nc *= ii;

   pg_tcl_xtrans = (char *) alloca ((strlen(argv[7]) + 1));
   if (pg_tcl_xtrans == (char *)0)
      {
      Tcl_SetResult(interp,"pgConx internal allocation error", TCL_VOLATILE);
      return (TCL_ERROR);
      }

   pg_tcl_ytrans = (char *) alloca ((strlen(argv[8]) + 1));
   if (pg_tcl_ytrans == (char *)0)
      {
      Tcl_SetResult(interp,"pgConx internal allocation error", TCL_VOLATILE);
      return (TCL_ERROR);
      }

   pg_tcl_xtrans[0] = 0;
   pg_tcl_ytrans[0] = 0;
   strcpy (pg_tcl_xtrans, argv[7]);
   strcpy (pg_tcl_ytrans, argv[8]);
   pg_tcl_error = 0;

   pgplot_pgconx(a, &idim, &jdim, &i1, &i2, &j1, &j2, c, &nc, pg_tcl_plot);

   free (a); free(c);
   if (pg_tcl_error == 0){
      return (TCL_OK);
   }else{
      return (TCL_ERROR);
   }
}


static char *pg_errb_hlp = "plot error bars in specified direction ";
static char *pg_errb_use = "USAGE: pgErrb dir x y e [t, d:0.0] ";

static int  pg_errb (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   int    dir;
   float  *x, *y, *e;
   int     n1, n2,  n3;
   float  t;

   if ((argc != 5) && (argc != 6))
      {
      Tcl_SetResult(interp, pg_errb_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   t = 0.0;
   if (argc == 6) 
      {      
      if (Tcl_GetDouble(interp, argv[5], &tmp) != TCL_OK) return(TCL_ERROR); t=(float)tmp;
      }
   if (Tcl_GetInt(interp, argv[1], &dir) != TCL_OK) return(TCL_ERROR);
   x = ftclList2Floats (interp, argv[2], &n1, 0); if (x == (float *)0) {return (TCL_ERROR);}
   y = ftclList2Floats (interp, argv[3], &n2, 0); if (y == (float *)0) {free (x); return (TCL_ERROR);}
   e = ftclList2Floats (interp, argv[4], &n3, 0); if (e == (float *)0) {free (x); free(y); return (TCL_ERROR);}
   if (n1 != n2 && n1 != n3){
      free(x); free(y); free(e); 
      Tcl_SetResult(interp, "pgErrb error: Arrays must be of equal length", TCL_VOLATILE);
      return (TCL_ERROR);
      }
   cpgerrb(dir, n1, x, y, e, t);
   free(x); free(y); free(e); 
   return (TCL_OK);
}

static char *pg_errx_hlp = "Plot horizontal error bars ";
static char *pg_errx_use = "USAGE: pgErrx x1 x2 y [t, d:0.0] ";

static int  pg_errx (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  *x1, *x2, *y;
   int     n1, n2,  n3;
   float  t;

   if ((argc != 5) && (argc != 4))
      {
      Tcl_SetResult(interp, pg_errx_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   t = 0.0;
   if (argc == 5) 
      {      
      if (Tcl_GetDouble(interp, argv[4], &tmp) != TCL_OK) return(TCL_ERROR); t=(float)tmp;
      }
   x1 = ftclList2Floats (interp, argv[1], &n1, 0); if (x1 == (float *)0) {return (TCL_ERROR);}
   x2 = ftclList2Floats (interp, argv[2], &n2, 0); if (x2 == (float *)0) {free (x1); return (TCL_ERROR);}
   y  = ftclList2Floats (interp, argv[3], &n3, 0); if (y  == (float *)0) {free (x1); free(x2); return (TCL_ERROR);}
   if (n1 != n2 && n1 != n3){
      free(x1); free(x2); free(y); 
      Tcl_SetResult(interp, "pgErrx error: Arrays must be of equal length", TCL_VOLATILE);
      return (TCL_ERROR);
      }
   cpgerrx(n1, x1, x2, y, t);
   free(x1); free(x2); free(y);
   return (TCL_OK);
}

static char *pg_erry_hlp = "Plot vertical error bars ";
static char *pg_erry_use = "USAGE: pgErry x y1 y2 [t, d:0.0] ";

static int  pg_erry (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  *x, *y1, *y2;
   int     n1, n2,  n3;
   float  t;

   if ((argc != 5) && (argc != 4))
      {
      Tcl_SetResult(interp, pg_erry_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   t = 0.0;
   if (argc == 5)
      {
      if (Tcl_GetDouble(interp, argv[4], &tmp) != TCL_OK) return(TCL_ERROR); t=(float)tmp;
      }
   x  = ftclList2Floats (interp, argv[1], &n1, 0); if (x  == (float *)0) {return (TCL_ERROR);}
   y1 = ftclList2Floats (interp, argv[2], &n2, 0); if (y1 == (float *)0) {free(x); return (TCL_ERROR);}
   y2 = ftclList2Floats (interp, argv[3], &n3, 0); if (y2 == (float *)0) {free (y1); free(x); return (TCL_ERROR);}
   if (n1 != n2 && n1 != n3){
      free(y1); free(y2); free(x); 
      Tcl_SetResult(interp, "pgErry error: Arrays must be of equal length", TCL_VOLATILE);
      return (TCL_ERROR);
      }
   cpgerry(n1, x, y1, y2, t);
   free(x); free(y1); free(y2);
   return (TCL_OK);
}

static char *pg_funt_hlp = "Draw a curve define by parametric equation x=FX(t), y=FY(t) ";
static char *pg_funt_use = "USAGE: pgFunt fx fy n tmin tmax [d:current | new] ";

static int  pg_funt (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   int    n;
   float  tmin, tmax;
   int    pgflag;

   if ((argc != 7) && (argc != 6))
      {
      Tcl_SetResult(interp, pg_funt_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   pgflag= 1;
   if (argc == 7)
      {
      pgflag = -1;
      if ( strcmp(argv[6], "current") == 0)   pgflag = 1;
      if ( strcmp(argv[6], "new")     == 0)   pgflag = 0;
      }
   if (pgflag == -1)
      {
      Tcl_SetResult(interp, pg_funt_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   if (Tcl_GetInt   (interp, argv[3], &n)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble(interp, argv[4], &tmp) != TCL_OK) return(TCL_ERROR); tmin =(float)tmp;
   if (Tcl_GetDouble(interp, argv[5], &tmp) != TCL_OK) return(TCL_ERROR); tmax =(float)tmp;

   pg_tcl_xtrans = (char *) alloca ((strlen(argv[1]) + 1));
   if (pg_tcl_xtrans == (char *)0)
      {
      Tcl_SetResult(interp,"pgFunt internal allocation error", TCL_VOLATILE);
      return (TCL_ERROR);
      }

   pg_tcl_ytrans = (char *) alloca ((strlen(argv[2]) + 1));
   if (pg_tcl_ytrans == (char *)0)
      {
      Tcl_SetResult(interp,"pgFunt internal allocation error", TCL_VOLATILE);
      return (TCL_ERROR);
      }

   strcpy (pg_tcl_xtrans, argv[1]);
   strcpy (pg_tcl_ytrans, argv[2]);

   pg_tcl_error = 0;
   pgplot_pgfunt(pg_tcl_fxt, pg_tcl_fyt, &n, &tmin, &tmax, &pgflag);

   if (pg_tcl_error == 0)
      return (TCL_OK);
   else
      return (TCL_ERROR);
}

static char *pg_funx_hlp = "Draw a curve define by the user supplied equation y=FY(x) ";
static char *pg_funx_use = "USAGE: pgFunx fy n xmin xmax [d:current | new] ";

static int  pg_funx (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   int    n;
   float  xmin, xmax;
   int    pgflag;

   if ((argc != 5) && (argc != 6))
      {
      Tcl_SetResult(interp, pg_funx_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   pgflag= 1;
   if (argc == 6)
      {
      pgflag = -1;
      if ( strcmp(argv[5], "current") == 0)   pgflag = 1;
      if ( strcmp(argv[5], "new")     == 0)   pgflag = 0;
      }
   if (pgflag == -1)
      {
      Tcl_SetResult(interp, pg_funx_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   if (Tcl_GetInt   (interp, argv[2], &n)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble(interp, argv[3], &tmp) != TCL_OK) return(TCL_ERROR); xmin =(float)tmp;
   if (Tcl_GetDouble(interp, argv[4], &tmp) != TCL_OK) return(TCL_ERROR); xmax =(float)tmp;

   pg_tcl_ytrans = (char *) alloca ((strlen(argv[1]) + 1));
   if (pg_tcl_ytrans == (char *)0)
      {
      Tcl_SetResult(interp,"pgFunx internal allocation error", TCL_VOLATILE);
      return (TCL_ERROR);
      }

   strcpy (pg_tcl_ytrans, argv[1]);

   pg_tcl_error = 0;
   pgplot_pgfunx(pg_tcl_fy, &n, &xmin, &xmax, &pgflag);

   if (pg_tcl_error == 0)
      return (TCL_OK);
   else
      return (TCL_ERROR);
}

static char *pg_funy_hlp = "Draw a curve define by the user supplied equation x=FX(y) ";
static char *pg_funy_use = "USAGE: pgFuny fy n ymin ymax [d:current | new] ";

static int  pg_funy (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   int    n;
   float  ymin, ymax;
   int    pgflag;

   if ((argc != 5) && (argc != 6))
      {
      Tcl_SetResult(interp, pg_funy_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   pgflag= 1;
   if (argc == 6)
      {
      pgflag = -1;
      if ( strcmp(argv[5], "current") == 0)   pgflag = 1;
      if ( strcmp(argv[5], "new")     == 0)   pgflag = 0;
      }
   if (pgflag == -1)
      {
      Tcl_SetResult(interp, pg_funy_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   if (Tcl_GetInt   (interp, argv[2], &n)   != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble(interp, argv[3], &tmp) != TCL_OK) return(TCL_ERROR); ymin =(float)tmp;
   if (Tcl_GetDouble(interp, argv[4], &tmp) != TCL_OK) return(TCL_ERROR); ymax =(float)tmp;

   pg_tcl_xtrans = (char *) alloca ((strlen(argv[1]) + 1));
   if (pg_tcl_xtrans == (char *)0)
      {
      Tcl_SetResult(interp,"pgFuny internal allocation error", TCL_VOLATILE);
      return (TCL_ERROR);
      }

   strcpy (pg_tcl_xtrans, argv[1]);

   pg_tcl_error = 0;
   pgplot_pgfuny(pg_tcl_fx, &n, &ymin, &ymax, &pgflag);

   if (pg_tcl_error == 0)
      return (TCL_OK);
   else
      return (TCL_ERROR);
}

static char *pg_gray_hlp = "Draw gray scale map of array in current window ";
static char *pg_gray_use = "USAGE: pgGray a i1 i2 j1 j2 fg bg tr";

static int  pg_gray (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   float  *a;
   float  *tr;
   int    i1, i2;
   int    j1, j2;
   float  fg, bg;
   int    idim, jdim, nt;
   double tmp;

   if (argc == 9){
      if (Tcl_GetInt   (interp, argv[2], &i1)  != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt   (interp, argv[3], &i2)  != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt   (interp, argv[4], &j1)  != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt   (interp, argv[5], &j2)  != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetDouble(interp, argv[6], &tmp) != TCL_OK) return(TCL_ERROR); fg =(float)tmp;
      if (Tcl_GetDouble(interp, argv[7], &tmp) != TCL_OK) return(TCL_ERROR); bg =(float)tmp;

      a = ftclLol2FtnFloats (interp, argv[1], &idim, &jdim); 
      if (a == (float *)0) {return (TCL_ERROR);}

      tr = ftclList2Floats (interp, argv[8], &nt, 0); 
      if (tr == (float *)0) {free(a); return (TCL_ERROR);}
      if (nt != 6) {
         free (a); free(tr); 
         Tcl_SetResult(interp, "pgGray error: TR array must have 6 elements", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      
      cpggray(a, idim, jdim, i1, i2, j1, j2, fg, bg, tr);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_gray_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_hi2d_hlp = "Plot a series of cross-sections through a 2D data array ";
static char *pg_hi2d_use = "USAGE: pgHi2d data ix1 ix2 iy1 iy2 x ioff bias [d:center | lowedge] ";

static int  pg_hi2d (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   float  *data;
   int    ix1,ix2, iy1, iy2;
   float  *x;
   float  *ylims;
   int    ioff;
   float  bias;
   int    center;
   double tmp;
   int    nx;
   int    nxv, nyv;

   if ((argc != 9) && (argc != 10)) 
      {
      Tcl_SetResult(interp, pg_hi2d_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   center = 1;
   if (argc == 10)
      {
      center = -1;
      if ( strcmp(argv[9], "center")  == 0)   center = 1;
      if ( strcmp(argv[9], "lowedge") == 0)   center = 0;
      }
   if (center == -1)
      {
      Tcl_SetResult(interp, pg_hi2d_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   if (Tcl_GetInt   (interp, argv[2], &ix1)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[3], &ix2)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[4], &iy1)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[5], &iy2)  != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetInt   (interp, argv[7], &ioff) != TCL_OK) return(TCL_ERROR);
   if (Tcl_GetDouble(interp, argv[8], &tmp)  != TCL_OK) return(TCL_ERROR); bias =(float)tmp;

   data = ftclLol2FtnFloats (interp, argv[1], &nxv, &nyv); 
   if (data == (float *)0) {return (TCL_ERROR);}

   x = ftclList2Floats (interp, argv[6], &nx, 0); 
   if (x == (float *)0) {free(data); return (TCL_ERROR);}
   if (nx != (ix2-ix1+1)) {
      free (data); free(x); 
      Tcl_SetResult(interp, "pgHi2d error: X array must have (IX2-IX1+1) elements", TCL_VOLATILE);
      return (TCL_ERROR);
      }

   ylims = (float *)alloca ((ix2-ix1+1)*sizeof(float));
   if (ylims == (float *)0) {free(data); free(x); return (TCL_ERROR);}

   cpghi2d(data, nxv, nyv, ix1, ix2, iy1, iy2, x, ioff, bias, center, ylims);
   free(data); free(x);
   return (TCL_OK);
}

static char *pg_hist_hlp = "Draw a histogram of a variable in an array using NBIN bins ";
static char *pg_hist_use = "USAGE: pgHist data datmin datmax nbin [d:current | new] [divided | filled | outline]";

static int  pg_hist (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   int    nd;
   float  *data;
   float  dmin, dmax;
   int    nbin, pgflag, pgflag2;

   if ((argc != 6) && (argc != 5) && (argc != 7))
      {
      Tcl_SetResult(interp, pg_hist_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   pgflag = 1;
   pgflag2 = 0;
   if ( (argc == 6) || (argc == 7) )
     {
       pgflag = -1;
       if ( strcmp(argv[5], "current") == 0)   pgflag = 1;
       if ( strcmp(argv[5], "new")     == 0)   pgflag = 0;
       if (argc == 7)
	 {
	   pgflag2 = -1;
	   if ( strcmp(argv[6], "divided") == 0) pgflag2 = 0;
	   if ( strcmp(argv[6], "filled" ) == 0) pgflag2 = 2;
	   if ( strcmp(argv[6], "outline" ) == 0) pgflag2 = 4;
	 }
     }
   if ( (pgflag == -1) || (pgflag2 == -1) )
     {
       Tcl_SetResult(interp, pg_hist_use, TCL_VOLATILE);
       return (TCL_ERROR);
     }
   
   pgflag = pgflag + pgflag2;
   if (Tcl_GetDouble(interp, argv[2], &tmp)    != TCL_OK) return(TCL_ERROR); dmin =(float)tmp;
   if (Tcl_GetDouble(interp, argv[3], &tmp)    != TCL_OK) return(TCL_ERROR); dmax =(float)tmp;
   if (Tcl_GetInt   (interp, argv[4], &nbin)   != TCL_OK) return(TCL_ERROR);

   data = ftclList2Floats (interp, argv[1], &nd, 0); if (data == (float *)0) {return (TCL_ERROR);}

   cpghist(nd, data, dmin, dmax, nbin, pgflag);
   return (TCL_OK);
}

static char *pg_curse_hlp = "Read the cursor position and a character typed by user ";
static char *pg_curse_use = "USAGE: pgCurse x y ";

static int pg_curse (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x, y;
   char   ch;
   char   s[100];

   if (argc == 3){
      if (Tcl_GetDouble(interp, argv[1], &tmp)    != TCL_OK) return(TCL_ERROR); x =(float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)    != TCL_OK) return(TCL_ERROR); y =(float)tmp;
      cpgcurs(&x, &y, &ch);
      sprintf (s, "%f  %f  %c", x, y, ch);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_curse_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_lcur_hlp = "Enter a poly by use of the cursor ";
static char *pg_lcur_use = "USAGE: pgLcurse coord (coord = variable name) ";

static int  pg_lcur (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int  maxpt = 500;
   float *x;
   float *y;
   int   xalloc = 0, yalloc = 0;
   int   nx, ny;
   int   i;
   char  s[1000];
   char  tmp[100];   
   char  cmd[100];

   if (argc != 2)
      {
      Tcl_SetResult(interp, pg_lcur_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   sprintf (cmd, "llength $%s", argv[1]);
   if (Tcl_Eval(interp, cmd) == TCL_ERROR)
      {
      nx = 0;		/* new list to be created, no elements */
      x = (float *)alloca (maxpt*sizeof(float));
      if (x == (float *)0) {
         Tcl_SetResult(interp, "Internal pgLcur: alloca failure", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      ++xalloc;

      y = (float *)alloca (maxpt*sizeof(float));
      if (y == (float *)0) {
         Tcl_SetResult(interp, "Internal pgLcur: alloca failure", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      ++yalloc;
      }
   else
      {
      sprintf (cmd, "lindex $%s 0", argv[1]);	/* get first element --> x-cooordinates */
      if (Tcl_Eval(interp, cmd) == TCL_ERROR)
         {
         if (strlen(interp->result) > 0)
            {
            printf ("pgLcur error obtaining x-coordinates: %s\n", interp->result);
            return(TCL_ERROR);
            }
         }
      x = ftclList2Floats (interp, interp->result, &nx, maxpt); 
      if (x == (float *)0) 
         {
         return (TCL_ERROR);
         }

      sprintf (cmd, "lindex $%s 1", argv[1]);	/* get second element --> y-cooordinates */
      if (Tcl_Eval(interp, cmd) == TCL_ERROR)
         {
         if (strlen(interp->result) > 0)
            {
            printf ("pgLcur error obtaining y-coordinates: %s\n", interp->result);
            free (x); 
            return(TCL_ERROR);
            }
         }
      y = ftclList2Floats (interp, interp->result, &ny, maxpt); 
      if (y == (float *)0) 
         {
         free (x); 
         return (TCL_ERROR);
         }
      if (nx != ny) 
         {
         Tcl_SetResult(interp,"pgLcur error: X Y arrays must be equal length", TCL_VOLATILE);
         free (x); 
         free (y); 
         return (TCL_ERROR);
         }
      }

   cpglcur(maxpt, &nx, x, y);

   s[0] = 0;
   for (i=0; i<nx; i++) 
      {
      if (i == 0) 
          sprintf (tmp, "lappend %s {%f ",argv[1], x[i]);
      else
          sprintf (tmp, "%f ",   x[i]);
      strcat (s, tmp);
      }
   strcat (s, "} ");

   for (i=0; i<nx; i++) 
      {
      if (i == 0) 
          sprintf (tmp, " {%f ", y[i]);
      else
          sprintf (tmp, "%f ",   y[i]);
      strcat (s, tmp);
      }
   strcat (s, "} ");

   sprintf (cmd, "unset %s", argv[1]);  
   Tcl_Eval(interp, cmd);  /* do not worry about errors for this call */

   if ( nx == 0) sprintf (s, "lappend %s { } { }", argv[1]);
   if (Tcl_Eval(interp, s) == TCL_ERROR)
      {
      if (strlen(interp->result) > 0)
         {
         printf ("pgLcur error: %s\n", interp->result);
         free (x); 
         free (y); 
         return(TCL_ERROR);
         }
      }

   if (xalloc == 0) free (x);
   if (yalloc == 0) free (y);
   return (TCL_OK);
}

static char *pg_ncur_hlp = "Enter data points by use of cursor. Points are sorted in x-coordinate ";
static char *pg_ncur_use = "USAGE: pgNcurse coord [symbol, d=-1] (coord = variable name) ";

static int  pg_ncur (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int  maxpt = 500;
   float *x;
   float *y;
   int   xalloc = 0, yalloc = 0;
   int   nx, ny;
   int   i;
   char  s[1000];
   char  tmp[100];   
   char  cmd[100];
   int   symb;

   if ((argc != 2) && (argc != 3))
      {
      Tcl_SetResult(interp, pg_ncur_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   symb = -1;
   if (argc == 3)
      {
      if (Tcl_GetInt   (interp, argv[2], &symb)   != TCL_OK) return(TCL_ERROR);
      }

   sprintf (cmd, "llength $%s", argv[1]);
   if (Tcl_Eval(interp, cmd) == TCL_ERROR)
      {
      nx = 0;		/* new list to be created, no elements */
      x = (float *)alloca (maxpt*sizeof(float));
      if (x == (float *)0) {
         Tcl_SetResult(interp, "Internal pgNcur: alloca failure", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      ++xalloc;

      y = (float *)alloca (maxpt*sizeof(float));
      if (y == (float *)0) {
         Tcl_SetResult(interp, "Internal pgNcur: alloca failure", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      ++yalloc;
      }
   else
      {
      sprintf (cmd, "lindex $%s 0", argv[1]);	/* get first element --> x-cooordinates */
      if (Tcl_Eval(interp, cmd) == TCL_ERROR)
         {
         if (strlen(interp->result) > 0)
            {
            printf ("pgNcur error obtaining x-coordinates: %s\n", interp->result);
            return(TCL_ERROR);
            }
         }
      x = ftclList2Floats (interp, interp->result, &nx, maxpt); 
      if (x == (float *)0) 
         {
         return (TCL_ERROR);
         }

      sprintf (cmd, "lindex $%s 1", argv[1]);	/* get second element --> y-cooordinates */
      if (Tcl_Eval(interp, cmd) == TCL_ERROR)
         {
         if (strlen(interp->result) > 0)
            {
            printf ("pgNcur error obtaining y-coordinates: %s\n", interp->result);
            free (x); 
            return(TCL_ERROR);
            }
         }
      y = ftclList2Floats (interp, interp->result, &ny, maxpt); 
      if (y == (float *)0) 
         {
         free (x); 
         return (TCL_ERROR);
         }
      if (nx != ny) 
         {
         Tcl_SetResult(interp,"pgNcur error: X Y arrays must be equal length", TCL_VOLATILE);
         free (x); 
         free (y); 
         return (TCL_ERROR);
         }
      }

   cpgncur(maxpt, &nx, x, y, symb);

   s[0] = 0;
   for (i=0; i<nx; i++) 
      {
      if (i == 0) 
          sprintf (tmp, "lappend %s {%f ",argv[1], x[i]);
      else
          sprintf (tmp, "%f ",   x[i]);
      strcat (s, tmp);
      }
   strcat (s, "} ");

   for (i=0; i<nx; i++) 
      {
      if (i == 0) 
          sprintf (tmp, " {%f ", y[i]);
      else
          sprintf (tmp, "%f ",   y[i]);
      strcat (s, tmp);
      }
   strcat (s, "} ");

   sprintf (cmd, "unset %s", argv[1]);  
   Tcl_Eval(interp, cmd);  /* do not worry about errors for this call */

   if ( nx == 0) sprintf (s, "lappend %s { } { }", argv[1]);
   if (Tcl_Eval(interp, s) == TCL_ERROR)
      {
      if (strlen(interp->result) > 0)
         {
         printf ("pgNcur error: %s\n", interp->result);
         if (xalloc == 0) free (x); 
         if (yalloc == 0) free (y); 
         return(TCL_ERROR);
         }
      }

   if (xalloc == 0) free (x);
   if (yalloc == 0) free (y);
   return (TCL_OK);
}

static char *pg_olin_hlp = "Enter data point via a cursor. Points are returned in order entered. ";
static char *pg_olin_use = "USAGE: pgOlin coord [symbol, d=1] (coord=variable name) ";

static int  pg_olin (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int  maxpt = 500;
   float *x;
   float *y;
   int   xalloc = 0., yalloc = 0;
   int   nx, ny;
   int   i;
   char  s[1000];
   char  tmp[100];   
   char  cmd[100];
   int   symb;

   if ((argc != 2) && (argc != 3))
      {
      Tcl_SetResult(interp, pg_olin_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   symb = -1;
   if (argc == 3)
      {
      if (Tcl_GetInt   (interp, argv[2], &symb)   != TCL_OK) return(TCL_ERROR);
      }

   sprintf (cmd, "llength $%s", argv[1]);
   if (Tcl_Eval(interp, cmd) == TCL_ERROR)
      {
      nx = 0;		/* new list to be created, no elements */
      x = (float *)alloca (maxpt*sizeof(float));
      if (x == (float *)0) {
         Tcl_SetResult(interp, "Internal pgOlin: malloc failure", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      ++xalloc;

      y = (float *)alloca (maxpt*sizeof(float));
      if (y == (float *)0) {
         Tcl_SetResult(interp, "Internal pgOlin: malloc failure", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      ++yalloc;
      }
   else
      {
      sprintf (cmd, "lindex $%s 0", argv[1]);	/* get first element --> x-cooordinates */
      if (Tcl_Eval(interp, cmd) == TCL_ERROR)
         {
         if (strlen(interp->result) > 0)
            {
            printf ("pgOlin error obtaining x-coordinates: %s\n", interp->result);
            return(TCL_ERROR);
            }
         }
      x = ftclList2Floats (interp, interp->result, &nx, maxpt); 
      if (x == (float *)0) 
         {
         return (TCL_ERROR);
         }

      sprintf (cmd, "lindex $%s 1", argv[1]);	/* get second element --> y-cooordinates */
      if (Tcl_Eval(interp, cmd) == TCL_ERROR)
         {
         if (strlen(interp->result) > 0)
            {
            printf ("pgOlin error obtaining y-coordinates: %s\n", interp->result);
            free (x); 
            return(TCL_ERROR);
            }
         }
      y = ftclList2Floats (interp, interp->result, &ny, maxpt); 
      if (y == (float *)0) 
         {
         free (x); 
         return (TCL_ERROR);
         }
      if (nx != ny) 
         {
         Tcl_SetResult(interp,"pgOlin error: X Y arrays must be equal length", TCL_VOLATILE);
         free (x); 
         free (y); 
         return (TCL_ERROR);
         }
      }

   cpgolin(maxpt, &nx, x, y, symb);

   s[0] = 0;
   for (i=0; i<nx; i++) 
      {
      if (i == 0) 
          sprintf (tmp, "lappend %s {%f ",argv[1], x[i]);
      else
          sprintf (tmp, "%f ",   x[i]);
      strcat (s, tmp);
      }
   strcat (s, "} ");

   for (i=0; i<nx; i++) 
      {
      if (i == 0) 
          sprintf (tmp, " {%f ", y[i]);
      else
          sprintf (tmp, "%f ",   y[i]);
      strcat (s, tmp);
      }
   strcat (s, "} ");

   sprintf (cmd, "unset %s", argv[1]);  
   Tcl_Eval(interp, cmd);  /* do not worry about errors for this call */

   if ( nx == 0) sprintf (s, "lappend %s { } { }", argv[1]);
   if (Tcl_Eval(interp, s) == TCL_ERROR)
      {
      if (strlen(interp->result) > 0)
         {
         printf ("pgOlin error: %s\n", interp->result);
         if (xalloc == 0) free (x); 
         if (yalloc == 0) free (y); 
         return(TCL_ERROR);
         }
      }

   if (xalloc == 0) free (x);
   if (yalloc == 0) free (y);
   return (TCL_OK);
}

static char *pg_qcf_hlp = "Query the current character font ";
static char *pg_qcf_use = "USAGE: pgQcf ";

static int  pg_qcf (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int i;
   char s[50];

   if (argc == 1){
      s[0] = 0;
      cpgqcf(&i);
      sprintf (s, "%d", i);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qcf_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_qch_hlp = "Query current character size ";
static char *pg_qch_use = "USAGE: pgQch ";

static int  pg_qch (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   float size;
   char  s[50];

   if (argc == 1){
      s[0] = 0;
      cpgqch(&size);
      sprintf (s, "%f", size);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qch_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_qci_hlp = "Query current color index ";
static char *pg_qci_use = "USAGE: pgQci ";


static int  pg_qci (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int ci;
   char s[50];

   if (argc == 1){
      s[0] = 0;
      cpgqci(&ci);
      sprintf (s, "%d", ci);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qci_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}


static char *pg_qcol_hlp =
           "Query the range of color indices available on the current device ";
static char *pg_qcol_use = "USAGE: pgQcol ";


static int  pg_qcol (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int ci1, ci2;
   char s[50];

   if (argc == 1){
      s[0] = 0;
      cpgqcol(&ci1, &ci2);

      /* Build the results into a Tcl list */
      sprintf (s, "%d", ci1);
      Tcl_AppendElement(interp, s);
      sprintf (s, "%d", ci2);
      Tcl_AppendElement(interp, s);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qcol_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_qcr_hlp = "Not available ";
static char *pg_qcr_use = "USAGE:  ";

static int  pg_qcr (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   return (TCL_OK);
}

static char *pg_qfs_hlp = "Query the current fill-area style attribute ";
static char *pg_qfs_use = "USAGE: pgQfs ";

static int  pg_qfs (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int fs;
   char s[50];

   if (argc == 1){
      s[0] = 0;
      cpgqfs(&fs);
      sprintf (s, "%d", fs);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qfs_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_qhs_hlp = "Query the current hatching style (angle, spacing, phase) ";
static char *pg_qhs_use = "USAGE: pgQhs ";

static int  pg_qhs (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   float angle, sepn, phase;
   char s[50];

   if (argc == 1){
      s[0] = 0;
      cpgqhs(&angle, &sepn, &phase);
      sprintf (s, "%f", angle);
      Tcl_AppendElement(interp, s);
      sprintf (s, "%f", sepn);
      Tcl_AppendElement(interp, s);
      sprintf (s, "%f", phase);
      Tcl_AppendElement(interp, s);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qhs_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_qid_hlp = "Query the current plot id ";
static char *pg_qid_use = "USAGE: pgQid ";

static int  pg_qid (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int fs;
   char s[50];

   if (argc == 1){
      s[0] = 0;
      cpgqid(&fs);
      sprintf (s, "%d", fs);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qid_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_qinf_hlp = "Obtain miscelaneous information about PGPLOT environment ";
static char *pg_qinf_use = "USAGE: pgQinf item  ";

static int  pg_qinf (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   char s[100];
   int  l;

   if (argc == 2){
      s[0] = 0;
      l = 99;
      cpgqinf(argv[1], s, &l);
      if (l > 99) l = 99;
      s[l] = 0;
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qinf_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_qls_hlp = "Query the current line style attribute ";
static char *pg_qls_use = "USAGE: pgQls ";

static int  pg_qls (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int ls;
   char s[50];

   if (argc == 1){
      s[0] = 0;
      cpgqls(&ls);
      sprintf (s, "%d", ls);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qls_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}
static char *pg_qlw_hlp = "Query the current line width attribute ";
static char *pg_qlw_use = "USAGE: pgQlw";

static int  pg_qlw (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int lw;
   char s[50];

   if (argc == 1){
      s[0] = 0;
      cpgqlw(&lw);
      sprintf (s, "%d", lw);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qlw_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_qvp_hlp = "Inquiry to determine the current viewport setting ";
static char *pg_qvp_use = "USAGE: pgQvp units";

static int  pg_qvp (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int units;
   float x1, x2, y1, y2;
   char s[100];

   if (argc == 2){
      if (Tcl_GetInt   (interp, argv[1], &units)   != TCL_OK) return(TCL_ERROR);
      s[0] = 0;
      cpgqvp(units, &x1, &x2, &y1, &y2);
      sprintf (s, "{%f %f %f %f}", x1, x2, y1, y2);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qvp_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_qwin_hlp = "Inquiry to determine the current window setting ";
static char *pg_qwin_use = "USAGE: pgQwin ";

static int  pg_qwin (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   float x1, x2, y1, y2;
   char s[100];

   if (argc == 1){
      cpgqwin(&x1, &x2, &y1, &y2);
      sprintf (s, "{%f %f %f %f}", x1, x2, y1, y2);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_qwin_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_etxt_hlp = "Erases interactive dialog text from grphics window ";
static char *pg_etxt_use = "USAGE: pgEtxt";

static int  pg_etxt (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1){
      cpgetxt();
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_etxt_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_iden_hlp = "Write username, date and time at bottom of plot ";
static char *pg_iden_use = "USAGE: pgIden ";

static int  pg_iden (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1){
      cpgiden();
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_iden_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_ldev_hlp = "Lists all known to PGPLOT device types ";
static char *pg_ldev_use = "USAGE: pgLdev";

static int  pg_ldev (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   if (argc == 1){
      cpgldev();
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_ldev_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_numb_hlp = "Convert a number into a decimal character representation ";
static char *pg_numb_use = "USAGE: pgNumb mm pp form ";

static int  pg_numb (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   int mm, pp, form;
   char string[80];
   int nc;
   char s[100];

   if (argc == 4){
      if (Tcl_GetInt   (interp, argv[1], &mm)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt   (interp, argv[2], &pp)   != TCL_OK) return(TCL_ERROR);
      if (Tcl_GetInt   (interp, argv[3], &form) != TCL_OK) return(TCL_ERROR);
      nc = 79;
      cpgnumb(mm, pp, form, string, &nc);
      sprintf (s, "%s  %d", string, nc);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_numb_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_rnd_hlp = "Finds the smallest round number (=1,2 or 5 times a power of 10) larger than x ";
static char *pg_rnd_use = "USAGE: pgRnd x ";

static int  pg_rnd (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x, roundedNum;
   int nsub;
   char s[20];

   if (argc == 2){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); x = (float)tmp;
      roundedNum = cpgrnd(x, &nsub);
      sprintf (s, "%f", roundedNum);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_rnd_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}

static char *pg_rnge_hlp = "Choose plotting limits XLO and XHI that encompass the data range X1 to X2 ";
static char *pg_rnge_use = "USAGE: pgRnge x1 x2";

static int  pg_rnge (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   double tmp;
   float  x1, x2, xlo, xhi;
   char s[50];

   if (argc == 3){
      if (Tcl_GetDouble(interp, argv[1], &tmp)   != TCL_OK) return(TCL_ERROR); x1 = (float)tmp;
      if (Tcl_GetDouble(interp, argv[2], &tmp)   != TCL_OK) return(TCL_ERROR); x2 = (float)tmp;
      cpgrnge(x1, x2, &xlo, &xhi);
      sprintf (s, "%f  %f", xlo, xhi);
      Tcl_SetResult(interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }else {
    Tcl_SetResult(interp, pg_rnge_use, TCL_VOLATILE);
    return (TCL_ERROR);
   }
}


/****************************************************************************
**
** ROUTINE: tclPgArrow
**
**<AUTO EXTRACT>
** TCL VERB: pgArrow
**
**</AUTO>
*/
static char *g_pgArrow = "pgArrow";
static ftclArgvInfo g_pgArrowTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Draw an arrow from X1,Y1 to X2,Y2.\n"},
  {"<x1>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World coordinates of tail of arrow."},
  {"<y1>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World coordinates of tail of arrow."},
  {"<x2>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World coordinates of head of arrow."},
  {"<y2>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World coordinates of head of arrow."},
  {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int tclPgArrow
   (
   ClientData   a_clientData,   /* IN: */
   Tcl_Interp   *a_interp,      /* IN: */
   int          argc,           /* IN: */
   char         **argv          /* IN: */
   )
{
RET_CODE rstatus;
double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
float fx1, fx2, fy1, fy2;

/* Parse the command */
g_pgArrowTbl[1].dst = &x1;
g_pgArrowTbl[2].dst = &y1;
g_pgArrowTbl[3].dst = &x2;
g_pgArrowTbl[4].dst = &y2;

rstatus = shTclParseArgv(a_interp, &argc, argv, g_pgArrowTbl,
                         FTCL_ARGV_NO_LEFTOVERS, g_pgArrow);
if (rstatus == FTCL_ARGV_SUCCESS)
  {
  fx1 = (float )x1;
  fx2 = (float )x2;
  fy1 = (float )y1;
  fy2 = (float )y2;

  cpgarro(fx1, fy1, fx2, fy2);
  rstatus = TCL_OK;
  }

return(rstatus);
}

/****************************************************************************
**
** ROUTINE: tclPgBand
**
**<AUTO EXTRACT>
** TCL VERB: pgBand
**
**</AUTO>
*/
char g_pgBand[] = "pgBand";
ftclArgvInfo g_pgBandTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Read cursor position and character typed by user.\n\n" },
  {"<mode>", FTCL_ARGV_INT, NULL, NULL,
   "Specify what is drawn with respect to the cursor and the anchor point (0-7)."},
  {"<position>", FTCL_ARGV_INT, NULL, NULL,
   "If = 1 place cursor at entered point else leave cursor alone."},
  {"<xref>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World x-coordinate of the anchor point."},
  {"<yref>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World y-coordinate of the anchor point."},
  {"<x>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World x-coordinate of the cursor."},
  {"<y>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World y-coordinate of the cursor."},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgBand(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_mode = 0;
  int a_position = 0;
  double a_xref = 0.0;
  double a_yref = 0.0;
  double a_x = 0.0;
  double a_y = 0.0;
  float fxref, fyref, fx, fy;
  char scalar[128], buf[50];

  g_pgBandTbl[1].dst = &a_mode;
  g_pgBandTbl[2].dst = &a_position;
  g_pgBandTbl[3].dst = &a_xref;
  g_pgBandTbl[4].dst = &a_yref;
  g_pgBandTbl[5].dst = &a_x;
  g_pgBandTbl[6].dst = &a_y;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgBandTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgBand)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  fxref = (float )a_xref;
  fyref = (float )a_yref;
  fx = (float )a_x;
  fy = (float )a_y;

  cpgband(a_mode, a_position, fxref, fyref, &fx, &fy, scalar);

  /* Return the character typed and the position to the user. */
  sprintf(buf, "%f", fx);
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", fy);
  Tcl_AppendElement(interp, buf);
  Tcl_AppendElement(interp, scalar);
  return(TCL_OK); 
}

/****************************************************************************
**
** ROUTINE: tclPgCirc
**
**<AUTO EXTRACT>
** TCL VERB: pgCirc
**
**</AUTO>
*/
char g_pgCirc[] = "pgCirc";
ftclArgvInfo g_pgCircTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Draw a circle\n" },
  {"<xcent>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "world x-coordinate of the center of the circle."},
  {"<ycent>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "world y-coordinate of the center of the circle."},
  {"<radius>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "radius of circle (world coordinates)."},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgCirc(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_xcent = 0.0;
  double a_ycent = 0.0;
  double a_radius = 0.0;
  float fxcent, fycent, fradius;

  g_pgCircTbl[1].dst = &a_xcent;
  g_pgCircTbl[2].dst = &a_ycent;
  g_pgCircTbl[3].dst = &a_radius;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgCircTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgCirc)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  fxcent = (float )a_xcent;
  fycent = (float )a_ycent;
  fradius = (float )a_radius;

  cpgcirc(fxcent, fycent, fradius);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgConl
**
**<AUTO EXTRACT>
** TCL VERB: pgConl
**
**</AUTO>
*/
char g_pgConl[] = "pgConl";
ftclArgvInfo g_pgConlTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Label a contour map drawn with routine PGCONT.\n" },
  {"<a>", FTCL_ARGV_STRING, NULL, NULL,
   "Data array."},
  {"<i1>", FTCL_ARGV_INT, NULL, NULL,
   "start of range of first index to be contoured (inclusive)."},
  {"<i2>", FTCL_ARGV_INT, NULL, NULL,
   "end of range of first index to be contoured (inclusive)."},
  {"<j1>", FTCL_ARGV_INT, NULL, NULL,
   "start of range of second index to be contoured  (incluseive)."},
  {"<j2>", FTCL_ARGV_INT, NULL, NULL,
   "end  of range of second index to be contoured (inclusive)."},
  {"<c>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "level of the contour to be labelled "},
  {"<tr>", FTCL_ARGV_STRING, NULL, NULL,
   "array defining a transformation between the I,J rid of the array and the world coordinates."},
  {"<label>", FTCL_ARGV_STRING, NULL, NULL,
   "used to label the specified contour"},
  {"<intval>", FTCL_ARGV_INT, NULL, NULL,
   "spacing along the contour between labels, in grid cells"},
  {"<minint>", FTCL_ARGV_INT, NULL, NULL,
   "contours that cross less than MININT cells will not be labelled"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgConl(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_a = NULL;
  int a_i1 = 0;
  int a_i2 = 0;
  int a_j1 = 0;
  int a_j2 = 0;
  double a_c = 0.0;
  char *a_tr = NULL;
  char *a_label = NULL;
  int a_intval = 0;
  int a_minint = 0;
  int idim = 0;
  int jdim = 0;
  int nt;
  float *fa, fc, *ftr;

  g_pgConlTbl[1].dst = &a_a;
  g_pgConlTbl[2].dst = &a_i1;
  g_pgConlTbl[3].dst = &a_i2;
  g_pgConlTbl[4].dst = &a_j1;
  g_pgConlTbl[5].dst = &a_j2;
  g_pgConlTbl[6].dst = &a_c;
  g_pgConlTbl[7].dst = &a_tr;
  g_pgConlTbl[8].dst = &a_label;
  g_pgConlTbl[9].dst = &a_intval;
  g_pgConlTbl[10].dst = &a_minint;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgConlTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgConl)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  fa = ftclLol2FtnFloats (interp, a_a, &idim, &jdim); 
  if (fa == (float *)0) {return (TCL_ERROR);}

  ftr = ftclList2Floats (interp, a_tr, &nt, 0); 
  if (ftr == (float *)0) {free(fa); return (TCL_ERROR);}
  if (nt != 6) {
    free (fa); free(ftr); 
    Tcl_SetResult(interp, "pgConl error: tr array must have 6 elements", TCL_VOLATILE);
    return (TCL_ERROR);
  }
      
  fc = (float )a_c;

  cpgconl(fa, idim, jdim, a_i1, a_i2, a_j1, a_j2, fc, ftr, a_label,
	  a_intval, a_minint);
  free(fa);
  free(ftr);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgEras
**
**<AUTO EXTRACT>
** TCL VERB: pgEras
**
**</AUTO>
*/
char g_pgEras[] = "pgEras";
ftclArgvInfo g_pgErasTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Erase all graphics from the current page or panel.\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgEras(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgErasTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgEras)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgeras();
  return(TCL_OK);

}

/****************************************************************************
**
** ROUTINE: tclPgImag
**
**<AUTO EXTRACT>
** TCL VERB: pgImag
**
**</AUTO>
*/
char g_pgImag[] = "pgImag";
ftclArgvInfo g_pgImagTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Draw a color image of an array in current window\n" },
  {"<a>", FTCL_ARGV_STRING, NULL, NULL,
   "the array to be plotted. "},
  {"<i1>", FTCL_ARGV_INT, NULL, NULL,
   "lower bound of the first index to be plotted"},
  {"<i2>", FTCL_ARGV_INT, NULL, NULL,
   "upper bound of the first index to be plotted"},
  {"<j1>", FTCL_ARGV_INT, NULL, NULL,
   "lower bound of the second index to be plotted"},
  {"<j2>", FTCL_ARGV_INT, NULL, NULL,
   "upper  bound of the second index to be plotted"},
  {"<a1>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "the array value which is to appear with shade C1."},
  {"<a2>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "the array value which is to appear with shade C2."},
  {"<tr>", FTCL_ARGV_STRING, NULL, NULL,
   "transformation matrix between array grid and world coordinates."},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgImag(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_a = NULL;
  int a_i1 = 0;
  int a_i2 = 0;
  int a_j1 = 0;
  int a_j2 = 0;
  double a_a1 = 0.0;
  double a_a2 = 0.0;
  char *a_tr = NULL;
  int nt, idim, jdim;
  float *fa, fa1, fa2, *ftr;

  g_pgImagTbl[1].dst = &a_a;
  g_pgImagTbl[2].dst = &a_i1;
  g_pgImagTbl[3].dst = &a_i2;
  g_pgImagTbl[4].dst = &a_j1;
  g_pgImagTbl[5].dst = &a_j2;
  g_pgImagTbl[6].dst = &a_a1;
  g_pgImagTbl[7].dst = &a_a2;
  g_pgImagTbl[8].dst = &a_tr;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgImagTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgImag)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  fa = ftclLol2FtnFloats (interp, a_a, &idim, &jdim); 
  if (fa == (float *)0) {return (TCL_ERROR);}

  ftr = ftclList2Floats (interp, a_tr, &nt, 0); 
  if (ftr == (float *)0) {free(fa); return (TCL_ERROR);}
  if (nt != 6) {
    free (fa); free(ftr); 
    Tcl_SetResult(interp, "pgImag error: tr array must have 6 elements", TCL_VOLATILE);
    return (TCL_ERROR);
  }
      
  fa1 = (float )a_a1;
  fa2 = (float )a_a2;

  cpgimag(fa, idim, jdim, a_i1, a_i2, a_j1, a_j2, fa1, fa2, ftr); 
  free(fa);
  free(ftr);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgLen
**
**<AUTO EXTRACT>
** TCL VERB: pgLen
**
**</AUTO>
*/
char g_pgLen[] = "pgLen";
ftclArgvInfo g_pgLenTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Work out length of a string in x and y directions\n" },
  {"<units>", FTCL_ARGV_INT, NULL, NULL,
   "Desired coordinates for answer (0:normalized device coordinates\n1:inches\n2:mm\n3:absolute device coordinates\n4:world coordinates\n5:fraction of the current viewport."},
  {"<string>", FTCL_ARGV_STRING, NULL, NULL,
   "string of interest"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgLen(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_units = 0;
  char *a_string = NULL;
  float xl, yl;
  char buf[20];

  g_pgLenTbl[1].dst = &a_units;
  g_pgLenTbl[2].dst = &a_string;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgLenTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgLen)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpglen(a_units, a_string, &xl, &yl);

  sprintf(buf, "%f", xl); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", yl);
  Tcl_AppendElement(interp, buf);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgPanl
**
**<AUTO EXTRACT>
** TCL VERB: pgPanl
**
**</AUTO>
*/
char g_pgPanl[] = "pgPanl";
ftclArgvInfo g_pgPanlTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Start plotting in a different panel. \n" },
  {"<ix>", FTCL_ARGV_INT, NULL, NULL,
   "The horizontal index of the panel  (1 - number of horizontal panels)"},
  {"<iy>", FTCL_ARGV_INT, NULL, NULL,
   "The vertical  index of the panel  (1 - number of  vertical panels)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgPanl(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_ix = 0;
  int a_iy = 0;

  g_pgPanlTbl[1].dst = &a_ix;
  g_pgPanlTbl[2].dst = &a_iy;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgPanlTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgPanl)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgpanl(a_ix, a_iy);
  return(TCL_OK);
}


/****************************************************************************
**
** ROUTINE: tclPgPixl
**
**<AUTO EXTRACT>
** TCL VERB: pgPixl
**
**</AUTO>
*/
char g_pgPixl[] = "pgPixl";
ftclArgvInfo g_pgPixlTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Draw lots of solid-filled (tiny) rectangles aligned with the coordinate axes. \n" },
  {"<ia>", FTCL_ARGV_STRING, NULL, NULL,
   "the array to be plotted."},
  {"<i1>", FTCL_ARGV_INT, NULL, NULL,
   "the lower range bound of the first index to be plotted"},
  {"<i2>", FTCL_ARGV_INT, NULL, NULL,
   "the upper range bound of the first index to be plotted"},
  {"<j1>", FTCL_ARGV_INT, NULL, NULL,
   "the lower range bound of the second index to be plotted"},
  {"<j2>", FTCL_ARGV_INT, NULL, NULL,
   "the upper range bound of the second index to be plotted"},
  {"<x1>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "x world coordinate of one corner of the output region"},
  {"<y1>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "y world coordinate of one corner of the output region"},
  {"<x2>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "x world coordinates of the opposite corner of the output region"},
  {"<y2>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "y world coordinates of the opposite corner of the output region"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgPixl(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_ia = NULL;
  int a_i1 = 0;
  int a_i2 = 0;
  int a_j1 = 0;
  int a_j2 = 0;
  double a_x1 = 0.0;
  double a_y1 = 0.0;
  double a_x2 = 0.0;
  double a_y2 = 0.0;
  int idim = 0, jdim = 0, *ia;
  float fx1, fy1, fx2, fy2;

  g_pgPixlTbl[1].dst = &a_ia;
  g_pgPixlTbl[2].dst = &a_i1;
  g_pgPixlTbl[3].dst = &a_i2;
  g_pgPixlTbl[4].dst = &a_j1;
  g_pgPixlTbl[5].dst = &a_j2;
  g_pgPixlTbl[6].dst = &a_x1;
  g_pgPixlTbl[7].dst = &a_y1;
  g_pgPixlTbl[8].dst = &a_x2;
  g_pgPixlTbl[9].dst = &a_y2;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgPixlTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgPixl)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  ia = ftclLol2FtnInts (interp, a_ia, &idim, &jdim); 
  if (ia == (int *)0) {return (TCL_ERROR);}

  fx1 = (float )a_x1;
  fy1 = (float )a_y1;
  fx2 = (float )a_x2;
  fy2 = (float )a_y2;

  cpgpixl(ia, idim, jdim, a_i1, a_i2, a_j1, a_j2, fx1, fx2, fy1, fy2);

  free(ia);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgPnts
**
**<AUTO EXTRACT>
** TCL VERB: pgPnts
**
**</AUTO>
*/
char g_pgPnts[] = "pgPnts";
ftclArgvInfo g_pgPntsTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Draw Graph Markers. Unlike PGPT, this routine can draw a different  symbol at each point. \n" },
  {"<n>", FTCL_ARGV_INT, NULL, NULL,
   "number of points to mark"},
  {"<x>", FTCL_ARGV_STRING, NULL, NULL,
   " world x-coordinates of the points."},
  {"<y>", FTCL_ARGV_STRING, NULL, NULL,
   "world y-coordinates of the points."},
  {"<symbol>", FTCL_ARGV_STRING, NULL, NULL,
   "code number of the symbol to be plotted for each point"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgPnts(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_n = 0;
  char *a_x = NULL;
  char *a_y = NULL;
  char *a_symbol = NULL;
  float *fx, *fy;
  int *symbol, ns, nn;

  g_pgPntsTbl[1].dst = &a_n;
  g_pgPntsTbl[2].dst = &a_x;
  g_pgPntsTbl[3].dst = &a_y;
  g_pgPntsTbl[4].dst = &a_symbol;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgPntsTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgPnts)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  symbol = ftclList2Ints (interp, a_symbol, &ns, 0); 
  if (symbol == (int *)0) {return (TCL_ERROR);}
  fx = ftclList2Floats (interp, a_x, &nn, 0); 
  if (fx == (float *)0) {free(symbol); return (TCL_ERROR);}
  fy = ftclList2Floats (interp, a_y, &nn, 0); 
  if (fy == (float *)0) {free(symbol); free(fx); return (TCL_ERROR);}

  cpgpnts(a_n, fx, fy, symbol, ns);

  free(fx);
  free(fy);
  free(symbol);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgQtxt
**
**<AUTO EXTRACT>
** TCL VERB: pgQtxt
**
**</AUTO>
*/
char g_pgQtxt[] = "pgQtxt";
ftclArgvInfo g_pgQtxtTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Returns a bounding box for a text string.\n" },
  {"<x>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World x-coordinate"},
  {"<y>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "World y-coordinate"},
  {"<angle>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "Angle (in degrees) that the baseline is to make with the horizontal"},
  {"<fjust>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "Horizontal justification of the string (0.0 is left justified, 1.0 is right justified)"},
  {"<text>", FTCL_ARGV_STRING, NULL, NULL,
   "The character string to be plotted"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgQtxt(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  double a_x = 0.0;
  double a_y = 0.0;
  double a_angle = 0.0;
  double a_fjust = 0.0;
  char *a_text = NULL;
  float fx, fy, fangle, ffjust, xbox, ybox;
  char buf[20];

  g_pgQtxtTbl[1].dst = &a_x;
  g_pgQtxtTbl[2].dst = &a_y;
  g_pgQtxtTbl[3].dst = &a_angle;
  g_pgQtxtTbl[4].dst = &a_fjust;
  g_pgQtxtTbl[5].dst = &a_text;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgQtxtTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgQtxt)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  fx = (float )a_x;
  fy = (float )a_y;
  fangle = (float )a_angle;
  ffjust = (float )a_fjust;

  cpgqtxt(fx, fy, fangle, ffjust, a_text, &xbox, &ybox);
  sprintf(buf, "%f", xbox); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", ybox); 
  Tcl_AppendElement(interp, buf);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgQah
**
**<AUTO EXTRACT>
** TCL VERB: pgQah
**
**</AUTO>
*/
char g_pgQah[] = "pgQah";
ftclArgvInfo g_pgQahTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Inquire as to the arrow-head style\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgQah(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char buf[20];
  int fs;
  float angle, vent;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgQahTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgQah)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgqah(&fs, &angle, &vent);
  sprintf(buf, "%d", fs); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", angle); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", vent); 
  Tcl_AppendElement(interp, buf);
  return (TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgQcir
**
**<AUTO EXTRACT>
** TCL VERB: pgQcir
**
**</AUTO>
*/
char g_pgQcir[] = "pgQcir";
ftclArgvInfo g_pgQcirTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Query the color index range to be used for producing images with PGGRAY or PGIMAG\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgQcir(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int icilo, icihi;
  char buf[20];

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgQcirTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgQcir)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgqcir(&icilo, &icihi);
  sprintf(buf, "%d", icilo); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%d", icihi); 
  Tcl_AppendElement(interp, buf);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgQcr
**
**<AUTO EXTRACT>
** TCL VERB: pgQcr
**
**</AUTO>
*/
char g_pgQcr[] = "pgQcr";
ftclArgvInfo g_pgQcrTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Query the RGB colors associated with a color index.\n" },
  {"<ci>", FTCL_ARGV_INT, NULL, NULL,
   "Color index\n."},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgQcr(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int ci;
  float cr, cg, cb;
  char buf[20];

  g_pgQcrTbl[1].dst = &ci;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgQcrTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgQcr)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgqcr(ci, &cr, &cg, &cb);
  sprintf(buf, "%f", cr); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", cg); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", cb); 
  Tcl_AppendElement(interp, buf);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgQcs
**
**<AUTO EXTRACT>
** TCL VERB: pgQcs
**
**</AUTO>
*/
char g_pgQcs[] = "pgQcs";
ftclArgvInfo g_pgQcsTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the current PGPLOT character height in a variety of units.\n" },
  {"<units>", FTCL_ARGV_INT, NULL, NULL,
   "Used to specify the units of the output value:\n0 : normalized device coordinates\n1 : inches\n2 : mm\n3 : pixels"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgQcs(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_units = 0;
  float xch, ych;
  char buf[20];

  g_pgQcsTbl[1].dst = &a_units;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgQcsTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgQcs)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgqcs(a_units, &xch, &ych);
  sprintf(buf, "%f", xch); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", ych); 
  Tcl_AppendElement(interp, buf);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgQitf
**
**<AUTO EXTRACT>
** TCL VERB: pgQitf
**
**</AUTO>
*/
char g_pgQitf[] = "pgQitf";
ftclArgvInfo g_pgQitfTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the Image Transfer Function as set by default or by a previous call to PGSITF.\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgQitf(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int itf;
  char buf[20];

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgQitfTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgQitf)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgqitf(&itf);
  sprintf(buf, "%d", itf); 
  Tcl_AppendElement(interp, buf);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgQpos
**
**<AUTO EXTRACT>
** TCL VERB: pgQpos
**
**</AUTO>
*/
char g_pgQpos[] = "pgQpos";
ftclArgvInfo g_pgQposTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Query the current pen position in world C coordinates (X,Y).\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgQpos(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  float x, y;
  char buf[20];

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgQposTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgQpos)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgqpos(&x, &y);
  sprintf(buf, "%f", x); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", y); 
  Tcl_AppendElement(interp, buf);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgQtbg
**
**<AUTO EXTRACT>
** TCL VERB: pgQtbg
**
**</AUTO>
*/
char g_pgQtbg[] = "pgQtbg";
ftclArgvInfo g_pgQtbgTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Query the current Text Background Color Index (set by routine PGSTBG).\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgQtbg(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int tbci;
  char buf[20];

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgQtbgTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgQtbg)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgqtbg(&tbci);
  sprintf(buf, "%d", tbci); 
  Tcl_AppendElement(interp, buf);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgQvsz
**
**<AUTO EXTRACT>
** TCL VERB: pgQvsz
**
**</AUTO>
*/
char g_pgQvsz[] = "pgQvsz";
ftclArgvInfo g_pgQvszTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Return the window, in a variety of units, defined by the full device view surface (0 -> 1 in normalized device coordinates).\n" },
  {"<units>", FTCL_ARGV_INT, NULL, NULL,
   "0,1,2,3 for output in normalized device coords, inches, mm, or absolute device units (dots)"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgQvsz(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_units = 0;
  float x1, x2, y1, y2;
  char buf[50];

  g_pgQvszTbl[1].dst = &a_units;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgQvszTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgQvsz)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgqvsz(a_units, &x1, &x2, &y1, &y2);
  sprintf(buf, "%f", x1); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", x2); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", y1); 
  Tcl_AppendElement(interp, buf);
  sprintf(buf, "%f", y2); 
  Tcl_AppendElement(interp, buf);
  return(TCL_OK);

}

/****************************************************************************
**
** ROUTINE: tclPgSah
**
**<AUTO EXTRACT>
** TCL VERB: pgSah
**
**</AUTO>
*/
char g_pgSah[] = "pgSah";
ftclArgvInfo g_pgSahTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Set the style to be used for arrowheads drawn with routine PGARRO.\n" },
  {"<fs>", FTCL_ARGV_INT, NULL, NULL,
   "FS = 1 => filled; FS = 2 => outline."},
  {"<angle>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "the acute angle of the arrow point, in degrees; angles in the range 20.0 to 90.0 give reasonable results"},
  {"<vent>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "The fraction of the triangular arrow-head that is cut away from the back. 0.0 gives a triangular wedge arrow-head; 1.0 gives an open >. Values 0.3 to 0.7"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgSah(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_fs = 0;
  double a_angle = 0.0;
  double a_vent = 0.0;

  g_pgSahTbl[1].dst = &a_fs;
  g_pgSahTbl[2].dst = &a_angle;
  g_pgSahTbl[3].dst = &a_vent;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgSahTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgSah)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgsah(a_fs, a_angle, a_vent);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgSave
**
**<AUTO EXTRACT>
** TCL VERB: pgSave
**
**</AUTO>
*/
char g_pgSave[] = "pgSave";
ftclArgvInfo g_pgSaveTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Save the current PGPLOT attributes in a private storage area. Attributes saved are: character font, character height, color index, fill-area style, line style, line width, pen position, arrow-head style.\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgSave(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgSaveTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgSave)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgsave();
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgScir
**
**<AUTO EXTRACT>
** TCL VERB: pgScir
**
**</AUTO>
*/
char g_pgScir[] = "pgScir";
ftclArgvInfo g_pgScirTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Set the color index range to be used for producing images with PGGRAY or PGIMAG. \n" },
  {"<icilo>", FTCL_ARGV_INT, NULL, NULL,
   "The lowest color index to use for images"},
  {"<icihi>", FTCL_ARGV_INT, NULL, NULL,
   "The highest color index to use for images"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgScir(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_icilo = 0;
  int a_icihi = 0;

  g_pgScirTbl[1].dst = &a_icilo;
  g_pgScirTbl[2].dst = &a_icihi;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgScirTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgScir)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgscir(a_icilo, a_icihi);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgScrn
**
**<AUTO EXTRACT>
** TCL VERB: pgScrn
**
**</AUTO>
*/
char g_pgScrn[] = "pgScrn";
ftclArgvInfo g_pgScrnTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Set color representation: i.e., define the color to be associated with a color index.\n" },
  {"<ci>", FTCL_ARGV_INT, NULL, NULL,
   "the color index to be defined. index 0 applies to the background color."},
  {"<name>", FTCL_ARGV_STRING, NULL, NULL,
   "the name of the color to be associated with this color index."},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgScrn(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_ci = 0;
  char *a_name = NULL;
  int ier;

  g_pgScrnTbl[1].dst = &a_ci;
  g_pgScrnTbl[2].dst = &a_name;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgScrnTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgScrn)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgscrn(a_ci, a_name, &ier);
  if (ier != 0)
    {
    /* There was an error in the routine */
    Tcl_SetResult(interp, "Error setting the color representation (pgscrn)\n",
		  TCL_VOLATILE);
    return(TCL_ERROR);
    }

  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgSitf
**
**<AUTO EXTRACT>
** TCL VERB: pgSitf
**
**</AUTO>
*/
char g_pgSitf[] = "pgSitf";
ftclArgvInfo g_pgSitfTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Set the Image Transfer Function for subsequent images drawn by PGIMAG, PGGRAY, or PGWEDG.\n" },
  {"<itf>", FTCL_ARGV_INT, NULL, NULL,
   "type of transfer function: \n0 : linear\n1 : logarithmic\n2 : square-root"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgSitf(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_itf = 0;

  g_pgSitfTbl[1].dst = &a_itf;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgSitfTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgSitf)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgStbg
**
**<AUTO EXTRACT>
** TCL VERB: pgStbg
**
**</AUTO>
*/
char g_pgStbg[] = "pgStbg";
ftclArgvInfo g_pgStbgTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Set the Text Background Color Index for subsequent text. By default text does not obscure underlying graphics. If the text background color index is positive, text is opaque"},
  {"<tbci>", FTCL_ARGV_INT, NULL, NULL,
   "The color index to be used for the background for subsequent text plotting:\nTBCI < 0  => transparent (default)\n TBCI >= 0 => text will be drawn on an opaque background with color index TBCI."},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgStbg(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_tbci = 0;

  g_pgStbgTbl[1].dst = &a_tbci;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgStbgTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgStbg)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgstbg(a_tbci);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgSubp
**
**<AUTO EXTRACT>
** TCL VERB: pgSubp
**
**</AUTO>
*/
char g_pgSubp[] = "pgSubp";
ftclArgvInfo g_pgSubpTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "divides the physical surface of the plotting device (screen, window, or sheet of paper) into NXSUB x NYSUB `panels'. \n" },
  {"<nxsub>", FTCL_ARGV_INT, NULL, NULL,
   "The number of subdivisions of the view surface in X. If NXSUB > 0, PGPLOT uses the panels in row order; if <0, PGPLOT uses them in column order."},
  {"<nysub>", FTCL_ARGV_INT, NULL, NULL,
   "The number of subdivisions of the view surface in Y"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgSubp(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  int a_nxsub = 0;
  int a_nysub = 0;

  g_pgSubpTbl[1].dst = &a_nxsub;
  g_pgSubpTbl[2].dst = &a_nysub;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgSubpTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgSubp)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgsubp(a_nxsub, a_nysub);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgTbox
**
**<AUTO EXTRACT>
** TCL VERB: pgTbox
**
**</AUTO>
*/
char g_pgTbox[] = "pgTbox";
ftclArgvInfo g_pgTboxTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Draw a box and optionally label one or both axes with (DD) HH MM SS tyle numeric labels (useful for time or RA - DEC plots). \n" },
  {"<xopt>", FTCL_ARGV_STRING, NULL, NULL,
   "'Z', 'Y', 'H', 'D', 'F' or 'O'."},
  {"<xtick>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "X-axis major tick increment."},
  {"<nxsub>", FTCL_ARGV_INT, NULL, NULL,
   "Number of intervals for minor ticks on X-axis."},
  {"<yopt>", FTCL_ARGV_STRING, NULL, NULL,
   "'Z', 'Y', 'H', 'D', 'F' or 'O'."},
  {"<ytick>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "Y-axis major tick increment."},
  {"<nysub>", FTCL_ARGV_INT, NULL, NULL,
   "Number of intervals for minor ticks on Y-axis."},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgTbox(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;
  char *a_xopt = NULL;
  char *a_yopt = NULL;
  double a_xtick = 0.0;
  double a_ytick = 0.0;
  int a_nxsub = 0;
  int a_nysub = 0;

  g_pgTboxTbl[1].dst = &a_xopt;
  g_pgTboxTbl[2].dst = &a_xtick;
  g_pgTboxTbl[3].dst = &a_nxsub;
  g_pgTboxTbl[4].dst = &a_yopt;
  g_pgTboxTbl[5].dst = &a_ytick;
  g_pgTboxTbl[6].dst = &a_nysub;

  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgTboxTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgTbox)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgtbox(a_xopt, a_xtick, a_nxsub, a_yopt, a_ytick, a_nysub);
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgUnsa
**
**<AUTO EXTRACT>
** TCL VERB: pgUnsa
**
**</AUTO>
*/
char g_pgUnsa[] = "pgUnsa";
ftclArgvInfo g_pgUnsaTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Restore the PGPLOT attributes saved in the last call to pgSave\n" },
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclPgUnsa(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
  int rstat;


  if ((rstat = shTclParseArgv(interp, &argc, argv, g_pgUnsaTbl,
       FTCL_ARGV_NO_LEFTOVERS, g_pgUnsa)) != FTCL_ARGV_SUCCESS) {
    return(rstat);
  }

  cpgunsa();
  return(TCL_OK);
}

/****************************************************************************
**
** ROUTINE: tclPgWedge
**
**<AUTO EXTRACT>
** TCL VERB: pgWedge
**
**</AUTO>
*/
static char *g_pgWedge = "pgWedge";
static ftclArgvInfo g_pgWedgeTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Plot an annotated grey-scale or color wedge parallel to a given axis.\n"},
  {"<side>", FTCL_ARGV_STRING, NULL, NULL, "Two character string (B, L, T, R) and (I, G)"},
  {"<disp>", FTCL_ARGV_DOUBLE, NULL, NULL, "Displacement of wedge from viewport edge."},
  {"<width>", FTCL_ARGV_DOUBLE, NULL, NULL, "Width of wedge in units of character height"},
  {"<fg>", FTCL_ARGV_DOUBLE, NULL, NULL, "The value which is to appear with shade 1 (foreground)."},
  {"<bg>", FTCL_ARGV_DOUBLE, NULL, NULL, "The value which is to appear with shade 0 (background)."},
  {"<label>", FTCL_ARGV_STRING, NULL, NULL, "Units label"},
  {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int tclPgWedge
   (
   ClientData   a_clientData,   /* IN: */
   Tcl_Interp   *a_interp,      /* IN: */
   int          argc,           /* IN: */
   char         **argv          /* IN: */
   )
{
RET_CODE rstatus;
char *sidePtr = NULL;
char *labelPtr = NULL;
double disp = 0.0, width = 0.0, fg = 0.0, bg = 0.0;
float fdisp, fwidth, ffg, fbg;

/* Parse the command */
g_pgWedgeTbl[1].dst = &sidePtr;
g_pgWedgeTbl[2].dst = &disp;
g_pgWedgeTbl[3].dst = &width;
g_pgWedgeTbl[4].dst = &fg;
g_pgWedgeTbl[5].dst = &bg;
g_pgWedgeTbl[6].dst = &labelPtr;

rstatus = shTclParseArgv(a_interp, &argc, argv, g_pgWedgeTbl,
                         FTCL_ARGV_NO_LEFTOVERS, g_pgWedge);
if (rstatus == FTCL_ARGV_SUCCESS)
  {
  fdisp = disp;
  fwidth = width;
  ffg = fg;
  fbg = bg;

  cpgwedg(sidePtr, fdisp, fwidth, ffg, fbg, labelPtr);
  rstatus = TCL_OK;
  }
else
  {
  rstatus = TCL_ERROR;
  }

return(rstatus);
}

/****************************************************************************
**
** ROUTINE: tclPgVect
**
**<AUTO EXTRACT>
** TCL VERB: pgVect
**
**</AUTO>
*/
static char *g_pgVect = "pgVect";
static ftclArgvInfo g_pgVectTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Draw a vector map of two arrays.\n"},
  {"<a>", FTCL_ARGV_STRING, NULL, NULL,
   "horizontal component data array"},
  {"<b>", FTCL_ARGV_STRING, NULL, NULL,
   "vertical component data array"},
  {"<i1>", FTCL_ARGV_INT, NULL, NULL,
   "lower limit of range of first index to be mapped (inclusive)"},
  {"<i2>", FTCL_ARGV_INT, NULL, NULL,
   "upper limit of range of first index to be mapped (inclusive)"},
  {"<ji>", FTCL_ARGV_INT, NULL, NULL,
   "lower limit of range of second index to be mapped (inclusive)"},
  {"<j2>", FTCL_ARGV_INT, NULL, NULL,
   "upper limit of range of second index to be mapped (inclusive)"},
  {"<c>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "scale factor for vector lengths, \nif 0.0, longest vector will be smaller of\ntr(2)+tr(3) and tr(5)+tr(6)"},
  {"<nc>", FTCL_ARGV_INT, NULL, NULL,
   "vector positioning code\n<0 : vector head on coords\n>0 : vector base on coords\n=0 : cvector centered on coords"},
  {"<tr>", FTCL_ARGV_STRING, NULL, NULL,
   "array defining a transformation between the I,J grid of the array and the world coordinates"},
  {"<blank>", FTCL_ARGV_DOUBLE, NULL, NULL,
   "elements of arrays A or B that are exactly equal to this value are ignored (blanked)"},
  {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int tclPgVect
   (
   ClientData   a_clientData,   /* IN: */
   Tcl_Interp   *a_interp,      /* IN: */
   int          argc,           /* IN: */
   char         **argv          /* IN: */
   )
{
RET_CODE rstatus;

char *a_a, *a_b, *a_tr;
int a_i1, a_i2, a_j1, a_j2;
double a_c, a_blank;
int nt = 0, idim = 0, jdim = 0, ibdim = 0, jbdim= 0, a_nc = 0;
float fc, fblank;
float *fa=NULL;
float *fb=NULL;
float *ftr=NULL;


/* Parse the command */
g_pgVectTbl[1].dst = &a_a;
g_pgVectTbl[2].dst = &a_b;
g_pgVectTbl[3].dst = &a_i1;
g_pgVectTbl[4].dst = &a_i2;
g_pgVectTbl[5].dst = &a_j1;
g_pgVectTbl[6].dst = &a_j2;
g_pgVectTbl[7].dst = &a_c;
g_pgVectTbl[8].dst = &a_nc;
g_pgVectTbl[9].dst = &a_tr;
g_pgVectTbl[10].dst = &a_blank;

rstatus = shTclParseArgv(a_interp, &argc, argv, g_pgVectTbl, 
			 FTCL_ARGV_NO_LEFTOVERS, g_pgVect);
if (rstatus == FTCL_ARGV_SUCCESS)
  {
  fa = ftclLol2FtnFloats (a_interp, a_a, &idim, &jdim); 
  if (fa == (float *)0) {return (TCL_ERROR);}

  fb = ftclLol2FtnFloats (a_interp, a_b, &ibdim, &jbdim); 
  if (fb == (float *)0) {free(fa); return (TCL_ERROR);}

  if (ibdim < idim) idim = ibdim;  /* pass the smaller dimension */
  if (jbdim < jdim) jdim = jbdim;  /* pass the smaller dimension */

  ftr = ftclList2Floats (a_interp, a_tr, &nt, 0); 
  if (ftr == (float *)0) {free(fa); free(fb); return (TCL_ERROR);}
  if (nt != 6) {
    free (fa); free(fb); free(ftr); 
    Tcl_SetResult(a_interp, "pgVect error: tr array must have 6 elements", TCL_VOLATILE);
    return (TCL_ERROR);
  }

  fc = (float )a_c;
  fblank = (float )a_blank;

  cpgvect(fa, fb, idim, jdim, a_i1, a_i2, a_j1, a_j2, fc, a_nc, ftr, fblank);
  rstatus = TCL_OK;
  }
else
  {rstatus = TCL_ERROR;}

free(fa);
free(fb);
free(ftr);

return(rstatus);
}

static char *pg_print_hlp = "Set the printing command";
static char *pg_print_use = "USAGE: pgPrinter [PRINT_COMMAND] ";
static char *PGTCLPRINT   = "lpr";

static int  pg_print (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   char cmd[132];

   if ((argc != 1) && (argc != 2))
      {
      sprintf (cmd, "Usage: pgPrinter [PRINT_COMMAND, d: %s]", PGTCLPRINT);
      Tcl_SetResult(interp, cmd, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   cmd[0] = 0;
   if (argc == 1)
      {
      strcpy (cmd, PGTCLPRINT);
      }
   else
      {
      strcpy (cmd, argv[1]);
      }

   if (pg_tcl_print != NULL) shFree(pg_tcl_print);
   printf ("\n Using printer command : %s\n", cmd);
   pg_tcl_print = (char *)shMalloc ((strlen(cmd) + 1));
   if (pg_tcl_print == (char *)0)
      {
      Tcl_SetResult(interp,"pgPrinter internal allocation error", TCL_VOLATILE);
      return (TCL_ERROR);
      }
   strcpy (pg_tcl_print, cmd);

   return TCL_OK;
}

static char *pg_dump_hlp = "Create a bitmap dump of a window in postscript format ";
static char *pg_dump_use = "USAGE: pgDump [OUTPUT, (d: pgplot_xwd.ps)]";

static int  pg_dump (ClientData clientData, Tcl_Interp *interp, int argc, char **argv) 
{
   char *cmd;
   char *tmp_file = NULL;

   if ((argc != 1) && (argc != 2))
      {
      Tcl_SetResult(interp, pg_dump_use, TCL_VOLATILE);
      return (TCL_ERROR);
      }

   printf ("\n Creating a PostScript image dump of the X window.\n");

   if ((argc == 1) && (pg_tcl_print != NULL))
      {
      cmd = (char *)alloca ((strlen(pg_tcl_print) + 32));
      if (cmd == (char *)0)
         {
         Tcl_SetResult(interp,"pgDump internal allocation error", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      cmd[0] = 0;
      sprintf (cmd, "xwd -frame | xpr -device ps | %s", pg_tcl_print);
      printf (" Image will be processed using \n     '%s'\n", pg_tcl_print);
      }
   else
      {
      if (argc == 1)
         {
         tmp_file = tmpnam(NULL);
         }
      else
         {
         tmp_file = shEnvscan(argv[1]);
         }
      cmd = (char *)alloca ((strlen(tmp_file) + 38));
      if (cmd == (char *)0)
         {
	 if ((tmp_file != NULL) && (argc != 1)) shEnvfree(tmp_file);
         Tcl_SetResult(interp,"pgDump internal allocation error", TCL_VOLATILE);
         return (TCL_ERROR);
         }
      cmd[0] = 0;
      sprintf (cmd, "xwd -frame | xpr -device ps -output %s", tmp_file);
      printf (" Image will be stored in '%s'\n", tmp_file);
      if (pg_tcl_print == NULL) 
         printf ("\n To set the printer use the dervish command \n   pgPrinter [PRINT_COMMAND, d: %s]\n", 
                PGTCLPRINT);
      }

   system(cmd);
   if ((tmp_file != NULL) && (argc != 1)) shEnvfree(tmp_file);

   return TCL_OK;
}

static char *pg_GeomGet_hlp = 
  "Get the value of the geometry information and return it as a Tcl list \n of the form {width height x y}\n";
static char *pg_GeomGet_use =
          "USAGE: pgGeomGet";
/*============================================================================
**============================================================================
**
** ROUTINE: shTclPgGeomGet
**
** Description:
**      Return the values of the geometry string that will be used to create
**      the pgplot windows.  These values are returned as a Tcl list.
**
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** CALLS TO:
**      Tcl_SetResult
**      Tcl_AppendResult
**
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static int shTclPgGeomGet
   (
   ClientData   a_clientData,   /* IN: */
   Tcl_Interp   *a_interp,      /* IN: */
   int          a_argc,         /* IN: */
   char         **a_argv        /* IN: */
   )
{
int          rstatus = TCL_OK;
char         buf[20];
int          x, y;
unsigned int width, height;

/* There should be no arguments */
if (a_argc != 1)
   {
   /* Too many arguments on the command line */
   Tcl_SetResult(a_interp, "Syntax error - too many arguments\n",
                 TCL_VOLATILE);
   Tcl_AppendResult(a_interp, pg_GeomGet_use, (char *)(NULL));
   rstatus = TCL_ERROR;
   }
else
   {
   /* Get the geometry information */
   if (pgGeomGet(&width, &height, &x, &y) == 0)
      {
      /* No geometry string was set */
      Tcl_SetResult(a_interp,
		    "No geometry information set or in .Xdefaults file\n",
		    TCL_VOLATILE);
      rstatus = TCL_ERROR;
      }
   else
      {
      /* Now format it into a Tcl list and return it to the user. */
      sprintf(buf, "%d", width);
      Tcl_AppendElement(a_interp, buf);
      sprintf(buf, "%d", height);
      Tcl_AppendElement(a_interp, buf);
      sprintf(buf, "%d", x);
      Tcl_AppendElement(a_interp, buf);
      sprintf(buf, "%d", y);
      Tcl_AppendElement(a_interp, buf);
      }
   }

return(rstatus);
}

static char *pg_GeomSet_hlp = 
  "Set the value of the geometry information used to create pgplot windows.\n";
static char *pg_GeomSet_use =
          "USAGE: pgGeomSet <width> <height> <x> <y>";
/*============================================================================
**============================================================================
**
** ROUTINE: shTclPgGeomSet
**
** Description:
**      Set the values of the geometry string that will be used to create
**      the pgplot windows.
**
**
** RETURN VALUES:
**      TCL_OK
**      TCL_ERROR
**
** CALLS TO:
**      Tcl_SetResult
**      Tcl_AppendResult
**
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
static int shTclPgGeomSet
   (
   ClientData   a_clientData,   /* IN: */
   Tcl_Interp   *a_interp,      /* IN: */
   int          a_argc,         /* IN: */
   char         **a_argv        /* IN: */
   )
{
int          rstatus = TCL_OK;
int          x, y;
unsigned int width, height;

/* Parse the input parameters */
if (a_argc != 5)
   {
   /* Wrong number of arguments on the command line */
   Tcl_SetResult(a_interp, "Syntax error - wrong number of arguments\n",
                 TCL_VOLATILE);
   Tcl_AppendResult(a_interp, pg_GeomSet_use, (char *)(NULL));
   rstatus = TCL_ERROR;
   }
else
   {
   /* Now get the arguments */
   width = (unsigned int )atoi(a_argv[1]);
   height = (unsigned int )atoi(a_argv[2]);
   x = atoi(a_argv[3]);
   y = atoi(a_argv[4]);

   /* Set em. */
   pgGeomSet(width, height, x, y);
   }

return(rstatus);
}

/*****************************declare TCL verbs*******************************/

static char *pgtcl_facil = "shPlot";

void shTclPgplotDeclare (Tcl_Interp *interp) 
{
   int pgflags = FTCL_ARGV_NO_LEFTOVERS;

   if (shPg_shTclPgplotDeclare_called != 0)
      {
      /* We have already been called, just return */
      return;
      }

++shPg_shTclPgplotDeclare_called;

/*
 *	Metafile routines
 */
    shTclDeclare(interp, "pgGet", (Tcl_CmdProc *)shTclPgGet, 
	    (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_Get_hlp, pg_Get_use);

    shTclDeclare(interp, "pgSaveMf", (Tcl_CmdProc *)shTclPgSave, 
	    (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_Save_hlp, pg_Save_use);

/*
 *      Geometry routines
 */

    shTclDeclare(interp, "pgGeomGet", (Tcl_CmdProc *)shTclPgGeomGet, 
	    (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_GeomGet_hlp, pg_GeomGet_use);

    shTclDeclare(interp, "pgGeomSet", (Tcl_CmdProc *)shTclPgGeomSet, 
	    (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_GeomSet_hlp, pg_GeomSet_use);
/*
 *	Control routines
 */
    shTclDeclare(interp, "pgAsk", (Tcl_CmdProc *)pg_ask,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_ask_hlp, pg_ask_use);
    shTclDeclare(interp, "pgBbuf", (Tcl_CmdProc *)pg_bbuf,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_bbuf_hlp, pg_bbuf_use);
    shTclDeclare(interp, "pgBegin", (Tcl_CmdProc *)pg_begin,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_begin_hlp, pg_begin_use);
    shTclDeclare(interp, "pgOpen", (Tcl_CmdProc *)pg_open,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_open_hlp, pg_open_use);
    shTclDeclare(interp, "pgEbuf", (Tcl_CmdProc *)pg_ebuf,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_ebuf_hlp, pg_ebuf_use);
    shTclDeclare(interp, "pgEnd", (Tcl_CmdProc *)pg_end,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_end_hlp, pg_end_use);
    shTclDeclare(interp, "pgClose", (Tcl_CmdProc *)pg_close,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_close_hlp, pg_close_use);
    shTclDeclare(interp, "pgPage", (Tcl_CmdProc *)pg_page,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_page_hlp, pg_page_use);
    shTclDeclare(interp, "pgPaper", (Tcl_CmdProc *)pg_paper,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_paper_hlp, pg_paper_use);
    shTclDeclare(interp, "pgUpdt", (Tcl_CmdProc *)pg_updt,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_updt_hlp, pg_updt_use);
/*
 *	Windows and viewports
 */
    shTclDeclare(interp, "pgBox", (Tcl_CmdProc *)pg_box,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_box_hlp, pg_box_use);
    shTclDeclare(interp, "pgEnv", (Tcl_CmdProc *)pg_env,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_env_hlp, pg_env_use);
    shTclDeclare(interp, "pgVport", (Tcl_CmdProc *)pg_vport,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_vport_hlp, pg_vport_use);
    shTclDeclare(interp, "pgVsize", (Tcl_CmdProc *)pg_vsize,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_vsize_hlp, pg_vsize_use);
    shTclDeclare(interp, "pgVstand", (Tcl_CmdProc *)pg_vstand,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_vstand_hlp, pg_vstand_use);
    shTclDeclare(interp, "pgWindow", (Tcl_CmdProc *)pg_window,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_window_hlp, pg_window_use);
    shTclDeclare(interp, "pgWnad", (Tcl_CmdProc *)pg_wnad,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_wnad_hlp, pg_wnad_use);
/*
 *	Primitive drawing routines
 */
    shTclDeclare(interp, "pgDraw", (Tcl_CmdProc *)pg_draw,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_draw_hlp, pg_draw_use);
    shTclDeclare(interp, "pgLine", (Tcl_CmdProc *)pg_line,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_line_hlp, pg_line_use);
    shTclDeclare(interp, "pgMove", (Tcl_CmdProc *)pg_move,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_move_hlp, pg_move_use);
    shTclDeclare(interp, "pgPoint", (Tcl_CmdProc *)pg_point,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_point_hlp, pg_point_use);
    shTclDeclare(interp, "pgPoly", (Tcl_CmdProc *)pg_poly,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_poly_hlp, pg_poly_use);
    shTclDeclare(interp, "pgRect", (Tcl_CmdProc *)pg_rect,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_rect_hlp, pg_rect_use);
/*
 *	Text
 */
    shTclDeclare(interp, "pgLabel", (Tcl_CmdProc *)pg_label,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_label_hlp, pg_label_use);
    shTclDeclare(interp, "pgMtext", (Tcl_CmdProc *)pg_mtext,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_mtext_hlp, pg_mtext_use);
    shTclDeclare(interp, "pgPtext", (Tcl_CmdProc *)pg_ptext,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_ptext_hlp, pg_ptext_use);
    shTclDeclare(interp, "pgText", (Tcl_CmdProc *)pg_text,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_text_hlp, pg_text_use);
/*
 *	Attribute setting
 */
    shTclDeclare(interp, "pgScf", (Tcl_CmdProc *)pg_scf,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_scf_hlp, pg_scf_use);
    shTclDeclare(interp, "pgSch", (Tcl_CmdProc *)pg_sch,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_sch_hlp, pg_sch_use);
    shTclDeclare(interp, "pgSci", (Tcl_CmdProc *)pg_sci,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_sci_hlp, pg_sci_use);
    shTclDeclare(interp, "pgScr", (Tcl_CmdProc *)pg_scr,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_scr_hlp, pg_scr_use);
    shTclDeclare(interp, "pgSfs", (Tcl_CmdProc *)pg_sfs,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_sfs_hlp, pg_sfs_use);
    shTclDeclare(interp, "pgShls", (Tcl_CmdProc *)pg_shls,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_shls_hlp, pg_shls_use);
    shTclDeclare(interp, "pgShs", (Tcl_CmdProc *)pg_shs,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_shs_hlp, pg_shs_use);
    shTclDeclare(interp, "pgSlct", (Tcl_CmdProc *)pg_slct,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_slct_hlp, pg_slct_use);
    shTclDeclare(interp, "pgSls", (Tcl_CmdProc *)pg_sls,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_sls_hlp, pg_sls_use);
    shTclDeclare(interp, "pgSlw", (Tcl_CmdProc *)pg_slw,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_slw_hlp, pg_slw_use);
/*
 *	Higher-level drawing routines
 */
    shTclDeclare(interp, "pgBin", (Tcl_CmdProc *)pg_bin,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_bin_hlp, pg_bin_use);
    shTclDeclare(interp, "pgCons", (Tcl_CmdProc *)pg_cons,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_cons_hlp, pg_cons_use);
    shTclDeclare(interp, "pgConb", (Tcl_CmdProc *)pg_conb,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_conb_hlp, pg_conb_use);
    shTclDeclare(interp, "pgCont", (Tcl_CmdProc *)pg_cont,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_cont_hlp, pg_cont_use);
    shTclDeclare(interp, "pgConx", (Tcl_CmdProc *)pg_conx,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_conx_hlp, pg_conx_use);
    shTclDeclare(interp, "pgErrb", (Tcl_CmdProc *)pg_errb,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_errb_hlp, pg_errb_use);
    shTclDeclare(interp, "pgErrx", (Tcl_CmdProc *)pg_errx,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_errx_hlp, pg_errx_use);
    shTclDeclare(interp, "pgErry", (Tcl_CmdProc *)pg_erry,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_erry_hlp, pg_erry_use);
    shTclDeclare(interp, "pgFunt", (Tcl_CmdProc *)pg_funt,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_funt_hlp, pg_funt_use);
    shTclDeclare(interp, "pgFunx", (Tcl_CmdProc *)pg_funx,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_funx_hlp, pg_funx_use);
    shTclDeclare(interp, "pgFuny", (Tcl_CmdProc *)pg_funy,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_funy_hlp, pg_funy_use);
    shTclDeclare(interp, "pgGray", (Tcl_CmdProc *)pg_gray,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_gray_hlp, pg_gray_use);
    shTclDeclare(interp, "pgHi2d", (Tcl_CmdProc *)pg_hi2d,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_hi2d_hlp, pg_hi2d_use);
    shTclDeclare(interp, "pgHist", (Tcl_CmdProc *)pg_hist,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_hist_hlp, pg_hist_use);
/*
 *	Interactive graphics (cursor)
 */
    shTclDeclare(interp, "pgCurse", (Tcl_CmdProc *)pg_curse,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_curse_hlp, pg_curse_use);
    shTclDeclare(interp, "pgLcurse", (Tcl_CmdProc *)pg_lcur,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_lcur_hlp, pg_lcur_use);
    shTclDeclare(interp, "pgNcurse", (Tcl_CmdProc *)pg_ncur,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_ncur_hlp, pg_ncur_use);
    shTclDeclare(interp, "pgOlin", (Tcl_CmdProc *)pg_olin,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_olin_hlp, pg_olin_use);
/*
 *	Inquiry routines
 */
    shTclDeclare(interp, "pgQcf", (Tcl_CmdProc *)pg_qcf,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qcf_hlp, pg_qcf_use);
    shTclDeclare(interp, "pgQch", (Tcl_CmdProc *)pg_qch,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qch_hlp, pg_qch_use);
    shTclDeclare(interp, "pgQci", (Tcl_CmdProc *)pg_qci,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qci_hlp, pg_qci_use);
    shTclDeclare(interp, "pgQcol", (Tcl_CmdProc *)pg_qcol,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qcol_hlp, pg_qcol_use);
    shTclDeclare(interp, "pgQcr", (Tcl_CmdProc *)pg_qcr,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qcr_hlp, pg_qcr_use);
    shTclDeclare(interp, "pgQfs", (Tcl_CmdProc *)pg_qfs,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qfs_hlp, pg_qfs_use);
    shTclDeclare(interp, "pgQhs", (Tcl_CmdProc *)pg_qhs,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qhs_hlp, pg_qhs_use);
    shTclDeclare(interp, "pgQid", (Tcl_CmdProc *)pg_qid,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qid_hlp, pg_qid_use);
    shTclDeclare(interp, "pgQinf", (Tcl_CmdProc *)pg_qinf,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qinf_hlp, pg_qinf_use);
    shTclDeclare(interp, "pgQls", (Tcl_CmdProc *)pg_qls,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qls_hlp, pg_qls_use);
    shTclDeclare(interp, "pgQlw", (Tcl_CmdProc *)pg_qlw,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qlw_hlp, pg_qlw_use);
    shTclDeclare(interp, "pgQvp", (Tcl_CmdProc *)pg_qvp,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qvp_hlp, pg_qvp_use);
    shTclDeclare(interp, "pgQwin", (Tcl_CmdProc *)pg_qwin,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_qwin_hlp, pg_qwin_use);
/*
 *	Utility routines
 */
    shTclDeclare(interp, "pgEtxt", (Tcl_CmdProc *)pg_etxt,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_etxt_hlp, pg_etxt_use);
    shTclDeclare(interp, "pgIden", (Tcl_CmdProc *)pg_iden,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_iden_hlp, pg_iden_use);
    shTclDeclare(interp, "pgLdev", (Tcl_CmdProc *)pg_ldev,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_ldev_hlp, pg_ldev_use);
    shTclDeclare(interp, "pgNumb", (Tcl_CmdProc *)pg_numb,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_numb_hlp, pg_numb_use);
    shTclDeclare(interp, "pgRnd", (Tcl_CmdProc *)pg_rnd,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_rnd_hlp, pg_rnd_use);
    shTclDeclare(interp, "pgRnge", (Tcl_CmdProc *)pg_rnge,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_rnge_hlp, pg_rnge_use);
    shTclDeclare(interp, "pgPrinter", (Tcl_CmdProc *)pg_print,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_print_hlp, pg_print_use);
    shTclDeclare(interp, "pgDump", (Tcl_CmdProc *)pg_dump,  (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL, pgtcl_facil,
            pg_dump_hlp, pg_dump_use);
    shTclDeclare(interp, g_pgArrow,
		 (Tcl_CmdProc *) tclPgArrow,
		 (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
		 shTclGetArgInfo(interp, g_pgArrowTbl, pgflags, g_pgArrow),
		 shTclGetUsage(interp, g_pgArrowTbl, pgflags, g_pgArrow));
    shTclDeclare(interp, g_pgBand,
		 (Tcl_CmdProc *) tclPgBand,
		 (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
		 shTclGetArgInfo(interp, g_pgBandTbl, pgflags, g_pgBand),
		 shTclGetUsage(interp, g_pgBandTbl, pgflags, g_pgBand));
    shTclDeclare(interp, g_pgCirc,
		 (Tcl_CmdProc *) tclPgCirc,
		 (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
		 shTclGetArgInfo(interp, g_pgCircTbl, pgflags, g_pgCirc),
		 shTclGetUsage(interp, g_pgCircTbl, pgflags, g_pgCirc));
    shTclDeclare(interp, g_pgWedge,
		 (Tcl_CmdProc *) tclPgWedge,
		 (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
		 shTclGetArgInfo(interp, g_pgWedgeTbl, pgflags, g_pgWedge),
		 shTclGetUsage(interp, g_pgWedgeTbl, pgflags, g_pgWedge));
    shTclDeclare(interp, g_pgConl,
		 (Tcl_CmdProc *) tclPgConl,
		 (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
		 shTclGetArgInfo(interp, g_pgConlTbl, pgflags, g_pgConl),
		 shTclGetUsage(interp, g_pgConlTbl, pgflags, g_pgConl));
  shTclDeclare(interp, g_pgEras,
               (Tcl_CmdProc *) tclPgEras,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgErasTbl, pgflags, g_pgEras),
               shTclGetUsage(interp, g_pgErasTbl, pgflags, g_pgEras));
  shTclDeclare(interp, g_pgImag,
               (Tcl_CmdProc *) tclPgImag,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgImagTbl, pgflags, g_pgImag),
               shTclGetUsage(interp, g_pgImagTbl, pgflags, g_pgImag));
  shTclDeclare(interp, g_pgLen,
               (Tcl_CmdProc *) tclPgLen,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgLenTbl, pgflags, g_pgLen),
               shTclGetUsage(interp, g_pgLenTbl, pgflags, g_pgLen));
  shTclDeclare(interp, g_pgPanl,
               (Tcl_CmdProc *) tclPgPanl,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgPanlTbl, pgflags, g_pgPanl),
               shTclGetUsage(interp, g_pgPanlTbl, pgflags, g_pgPanl));
  shTclDeclare(interp, g_pgPixl,
               (Tcl_CmdProc *) tclPgPixl,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgPixlTbl, pgflags, g_pgPixl),
               shTclGetUsage(interp, g_pgPixlTbl, pgflags, g_pgPixl));
  shTclDeclare(interp, g_pgPnts,
               (Tcl_CmdProc *) tclPgPnts,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgPntsTbl, pgflags, g_pgPnts),
               shTclGetUsage(interp, g_pgPntsTbl, pgflags, g_pgPnts));
  shTclDeclare(interp, g_pgQtxt,
               (Tcl_CmdProc *) tclPgQtxt,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgQtxtTbl, pgflags, g_pgQtxt),
               shTclGetUsage(interp, g_pgQtxtTbl, pgflags, g_pgQtxt));
  shTclDeclare(interp, g_pgQah,
               (Tcl_CmdProc *) tclPgQah,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgQahTbl, pgflags, g_pgQah),
               shTclGetUsage(interp, g_pgQahTbl, pgflags, g_pgQah));
  shTclDeclare(interp, g_pgQcir,
               (Tcl_CmdProc *) tclPgQcir,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgQcirTbl, pgflags, g_pgQcir),
               shTclGetUsage(interp, g_pgQcirTbl, pgflags, g_pgQcir));
  shTclDeclare(interp, g_pgQcr,
               (Tcl_CmdProc *) tclPgQcr,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgQcrTbl, pgflags, g_pgQcr),
               shTclGetUsage(interp, g_pgQcrTbl, pgflags, g_pgQcr));
  shTclDeclare(interp, g_pgQcs,
               (Tcl_CmdProc *) tclPgQcs,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgQcsTbl, pgflags, g_pgQcs),
               shTclGetUsage(interp, g_pgQcsTbl, pgflags, g_pgQcs));
  shTclDeclare(interp, g_pgQitf,
               (Tcl_CmdProc *) tclPgQitf,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgQitfTbl, pgflags, g_pgQitf),
               shTclGetUsage(interp, g_pgQitfTbl, pgflags, g_pgQitf));
  shTclDeclare(interp, g_pgQpos,
               (Tcl_CmdProc *) tclPgQpos,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgQposTbl, pgflags, g_pgQpos),
               shTclGetUsage(interp, g_pgQposTbl, pgflags, g_pgQpos));
  shTclDeclare(interp, g_pgQtbg,
               (Tcl_CmdProc *) tclPgQtbg,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgQtbgTbl, pgflags, g_pgQtbg),
               shTclGetUsage(interp, g_pgQtbgTbl, pgflags, g_pgQtbg));
  shTclDeclare(interp, g_pgQvsz,
               (Tcl_CmdProc *) tclPgQvsz,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgQvszTbl, pgflags, g_pgQvsz),
               shTclGetUsage(interp, g_pgQvszTbl, pgflags, g_pgQvsz));
  shTclDeclare(interp, g_pgSah,
               (Tcl_CmdProc *) tclPgSah,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgSahTbl, pgflags, g_pgSah),
               shTclGetUsage(interp, g_pgSahTbl, pgflags, g_pgSah));
  shTclDeclare(interp, g_pgSave,
               (Tcl_CmdProc *) tclPgSave,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgSaveTbl, pgflags, g_pgSave),
               shTclGetUsage(interp, g_pgSaveTbl, pgflags, g_pgSave));
  shTclDeclare(interp, g_pgScir,
               (Tcl_CmdProc *) tclPgScir,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgScirTbl, pgflags, g_pgScir),
               shTclGetUsage(interp, g_pgScirTbl, pgflags, g_pgScir));
  shTclDeclare(interp, g_pgScrn,
               (Tcl_CmdProc *) tclPgScrn,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgScrnTbl, pgflags, g_pgScrn),
               shTclGetUsage(interp, g_pgScrnTbl, pgflags, g_pgScrn));
  shTclDeclare(interp, g_pgSitf,
               (Tcl_CmdProc *) tclPgSitf,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgSitfTbl, pgflags, g_pgSitf),
               shTclGetUsage(interp, g_pgSitfTbl, pgflags, g_pgSitf));
  shTclDeclare(interp, g_pgStbg,
               (Tcl_CmdProc *) tclPgStbg,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgStbgTbl, pgflags, g_pgStbg),
               shTclGetUsage(interp, g_pgStbgTbl, pgflags, g_pgStbg));
  shTclDeclare(interp, g_pgSubp,
               (Tcl_CmdProc *) tclPgSubp,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgSubpTbl, pgflags, g_pgSubp),
               shTclGetUsage(interp, g_pgSubpTbl, pgflags, g_pgSubp));
  shTclDeclare(interp, g_pgTbox,
               (Tcl_CmdProc *) tclPgTbox,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgTboxTbl, pgflags, g_pgTbox),
               shTclGetUsage(interp, g_pgTboxTbl, pgflags, g_pgTbox));
  shTclDeclare(interp, g_pgUnsa,
               (Tcl_CmdProc *) tclPgUnsa,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgUnsaTbl, pgflags, g_pgUnsa),
               shTclGetUsage(interp, g_pgUnsaTbl, pgflags, g_pgUnsa));
  shTclDeclare(interp, g_pgVect,
               (Tcl_CmdProc *) tclPgVect,
               (ClientData) 0, (Tcl_CmdDeleteProc *)0, pgtcl_facil,
               shTclGetArgInfo(interp, g_pgVectTbl, pgflags, g_pgVect),
               shTclGetUsage(interp, g_pgVectTbl, pgflags, g_pgVect));

}

#ifdef TCL_PGPLOT_TEST
main(argc, argv)
    int     argc;
    char  **argv;
{
    Tcl_Interp *interp;

    /*
     * If history is to be used, then set the eval procedure pointer that
     * Tcl_CommandLoop so that history will be recorded.  This reference
     * also brings in history from libtcl.a.
     */
     tclShellCmdEvalProc = Tcl_RecordAndEval;

    /*
     * Create a Tcl interpreter for the session, with all extended commands
     * initialized.  This can be replaced with Tcl_CreateInterp followed
     * by a subset of the extended command initializaton procedures if
     * desired.
     */
    interp = Tcl_CreateInterp();
    /*
     *   Initialize the PGPLOT commands
     */

    shTclPgplotDeclare (interp);

    /*
     * Create Fermi commands
     */
    ftclCreateCommands (interp);

    /*
     * Load the tcl startup code, this should pull in all of the tcl
     * procs, paths, command line processing, autoloads, packages, etc.
     * If Tcl was invoked interactively, Tcl_Startup will give it
     * a command loop.
     */
    Tcl_CommandLoop (interp, stdin, stdout, tclShellCmdEvalProc, 0);

    /*
     * Delete the interpreter (not neccessary under Unix, but we do
     * it if TCL_MEM_DEBUG is set to better enable us to catch memory
     * corruption problems)
     */

    exit(0);
}
#endif

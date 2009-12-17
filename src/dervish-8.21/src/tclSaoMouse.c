/*
 <AUTO>
 * FILE:
 *   tclSaoMouse.c
 *<HTML>
 *	DERVISH TCL mouse commands allow FSAOimage mouse button clicks to be
 *	translated into user defined DERVISH TCL commands.  This translation is
 *	done when FSAOimage is in <B>cursor</B> mode only.
 *<P>
 *	See the <A HREF="#mouseDefine">mouseDefine</A> command for a 
 *	description of the parameters that are passed to the user defined
 *	DERVISH TCL commands.
 *</HTML>
 </AUTO>
 *
 * ABSTRACT:
 *   This file contains all mouse related routines used in dervish for the
 *   interaction with (f)sao images.
 *
 * ENTRY POINT            SCOPE    DESCRIPTION
 * ----------------------------------------------------------------------------
 * p_shSaoMouseWaitGet    public   Get the value of g_mouseWait.wait
 * p_shSaoMouseKeyMap     public   Map mouse clicks to TCL commands
 * p_shSaoMouseKeyGet     public   Get mouse click info from SAO image     
 * shSaoMouseKeyAssign    local    Associate a TCL command w/ mouse button
 * shSaoMouseKeyList      local    List TCL commands associated w/ mouse buttons
 * tclSaoMouseWait        local    TCL command to wait for a mouse event
 * tclSaoMouseDefine      local    TCL command to assign action to mouse buttons
 * tclSaoMouseList        local    TCL command to list action assigned to mouse
 *                                    button
 * tclSaoMouseToggle      local    TCL command to toggle mouse verbosity
 * shTclSaoMouseDeclare   public   World's interface to SAO Mouse TCL commands
 *
 * AUTHORS:
 *   Gary Sergey
 *   Eileen Berman
 *
 * MODIFICATIONS:
 * July 1993  vijay  Extensive modifications: broke down cmdPgm.c to manageable
 *                   chunks.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>   /* For prototype of read() and friends */

#include "region.h"
#include "tcl.h"
#include "tk.h"
#include "shTclSao.h"
#include "shTclUtils.h"
#include "shTk.h"

/*
 * Function prototypes used in this source file only
 */
static void createMouseListMsg(char *a_buffer);
static void mouseCallWaiting(ClientData clientData);
static void shSaoMouseKeyAssign(const int, const char *);
static void shSaoMouseKeyList(const int);
static int  tclSaoMouseWait(ClientData, Tcl_Interp *, int, char **); 
static int  tclSaoMouseSave(ClientData, Tcl_Interp *, int, char **); 
static int  tclSaoMouseDefine(ClientData, Tcl_Interp *, int, char **); 
static int  tclSaoMouseList(ClientData, Tcl_Interp *, int, char **);
static int  tclSaoMouseToggle(ClientData, Tcl_Interp *, int, char **);

/*
 * Global variables and definitions
 */
extern SAOCMDHANDLE g_saoCmdHandle;
extern char g_saoTCLList[CMD_NUM_SAOBUTT][TCL_BUTT_FUNC_NAME_LEN];
extern int g_saoMouseVerbose;

static char g_tclMsgHdr[] = "Unknown mouse button - must be one of:\n";

/* The following must be long enough for the list of mouse commands and
   the above error message. */
static char g_listMouseMsg[CMD_NUM_SAOBUTT*CMD_LEN_SAOBUTT + 100] = "";

/*
 *
 * SAOIMAGE COMMAND BUTTON TYPES (ENUMERATION) AND CORRESPONDING ASCII TEXT
 *
 * NOTE: THE ORDER OF THESE DEFINITIONS SHOULD NOT BE CHANGED SINCE A TCL
 *       ARRAY IS DEFINED (IN ???.TCL) AND ASSUMES THIS ORDER !!!
 */
 
typedef enum
   {
   NO_BUTT,         /*     DO       */
   POINT_L,         /*     NOT      */
   POINT_M,         /*   MODIFY     */
   POINT_R,         /*     THE      */
   POINT_CL_L,      /*    ORDER     */
   POINT_CL_M,      /*      OF      */
   POINT_CL_R,      /*    THESE     */
   POINT_SH_L,
   POINT_SH_M,
   POINT_SH_R,
   POINT_CL_SH_L,
   POINT_CL_SH_M,
   POINT_CL_SH_R,
   CIRCLE_CL,       /*  ENUMERATED  */
   ELLIPSE_CL,      /*     TYPE     */
   RECTANGLE_CL     /* DEFINITIONS  */
   } CMD_SAOBUTT;
 
static char g_saobuttname[CMD_NUM_SAOBUTT][CMD_LEN_SAOBUTT] =
   {
   "NO_BUTT",       /*     DO       */
   "POINT_L",       /*     NOT      */
   "POINT_M",       /*   MODIFY     */
   "POINT_R",       /*     THE      */
   "POINT_CL_L",    /*    ORDER     */
   "POINT_CL_M",    /*      OF      */
   "POINT_CL_R",    /*    THESE     */
   "POINT_SH_L",
   "POINT_SH_M",
   "POINT_SH_R",
   "POINT_CL_SH_L",
   "POINT_CL_SH_M",
   "POINT_CL_SH_R",
   "CIRCLE_CL",     /*  ENUMERATED  */
   "ELLIPSE_CL",    /*     TYPE     */
   "RECTANGLE_CL"   /* DEFINITIONS  */
   };

/*----------------------------------------------------------------------------
**
** Structures used when the user is waiting for mouse command input.
*/
/* Structure to store the information we are waiting for. */
typedef struct {
   int           wait;         /* if TRUE, user is waiting */
   CMD_SAOBUTT   command;      /* mouse cmd user is waiting for */
   int           eval;         /* should dervish evaluate the cmd? */
   int           fsao;         /* # of fsao process to get input from */
   int           timeout;      /* how long to wait 0 = fo'evah */
   Tk_TimerToken timerToken;   /* used to cancel timeout */
} SHMOUSEWAIT;
static SHMOUSEWAIT g_mouseWait;

/* Structure to store the returned information when waiting for a command. 
   mDCommand must be the same size as buf in SaoImageProc. params must be
   the same size as params in p_shSaoMouseKeyMap. */
typedef struct {
   char mDCommand[BUFSIZ-1];         /* user command stored with mouseDefine */
   char params[CMD_SZ_IMCURVAL];     /* params returned from fsao */
   CMD_SAOBUTT command;              /* mouse command that was executed */
 } SHMOUSEGOT;
static SHMOUSEGOT g_mouseGot;


/*
 * ROUTINE:
 *   p_shSaoMouseWaitGet
 *
 * CALL:
 *   (int) p_shSaoMouseWaitGet(void)
 *
 * DESCRIPTION:
 *   Return the value of g_mouseWait.wait. Used in tkDervish.c.
 *
 * RETURN VALUES:
 *
 */
int
p_shSaoMouseWaitGet(void)
{
return(g_mouseWait.wait);
}

/*
 * ROUTINE: 
 *   p_shSaoMouseKeyMap
 *
 * CALL:
 *   (int) p_shSaoMouseKeyMap(
 *          CMD_SAOHANDLE *a_shl,   // MOD: internal handle structure
 *          int           a_saonum, // IN : SAO number
 *          char          *a_line   // OUT: TCL command. This parameter remains
 *         );                       //      unchanged, if no TCL command was
 *                                  //      defined.
 *
 * DESCRIPTION:
 *   Certain mouse clicks in SAOimage can be assigned TCL commands by
 *   the user (via MouseDefine).  This routine will convert the string
 *   returned from SAOimage into the appropriate TCL command (if previously
 *   defined by the user).  If no TCL command was defined, a_line remains
 *   unmodified.
 *
 * RETURN VALUES:
 *      0 - No TCL command defined for this mouse click.
 *      1 - TCL Command was located and copied to a_line.
 *
 * GLOBALS REFERENCED:
 *      g_saoTCLList
 */
int 
p_shSaoMouseKeyMap(CMD_SAOHANDLE *a_shl, int a_saonum, char *a_line)
{
   int             shape, mouse, control_key, shift_key, key;
   int             rstatus;
   char            params[CMD_SZ_IMCURVAL];
   char            saonumber[5];
   CMD_SAOBUTT     command;
 
   /*
    * Parse the string returned from saoimage to get the shape, mouse button,
    *  and (future use) key
    */
   memset((VOID *)params, 0, sizeof(params));

   /* ??? %80c */
   sscanf(a_shl->mousePos, "%d %d %d %d %d %80c", &shape, &mouse, &control_key, 
          &shift_key, &key, params);
 
   /*
    * Determine the command based on the shape and mouse button
    */
   switch((CMD_SAOSHAPES)shape)  {
      case S_CIRCLE:
         command = CIRCLE_CL;
         break;
      case S_POINT:
         if (control_key)       /* Control key and mouse button were pressed */
            {
            if (shift_key)	/* Control and Shift keys were pressed */
               {
               if (mouse == 2)
                  {command = POINT_CL_SH_M;}
               else if (mouse == 3)
                  {command = POINT_CL_SH_R;}
               else
                  {command = POINT_CL_SH_L;}
               }
            else
               {
               if (mouse == 2)
                  {command = POINT_CL_M;}
               else if (mouse == 3)
                  {command = POINT_CL_R;}
               else
                  {command = POINT_CL_L;}
               }
            }
         else
            {
            if (shift_key)	/* Shift key was also pressed */
               {
               if (mouse == 2)
                  {command = POINT_SH_M;}
               else if (mouse == 3)
                  {command = POINT_SH_R;}
               else
                  {command = POINT_SH_L;}
               }
            else
               {
               if (mouse == 2)        /* Only mouse button was pressed */
                  {command = POINT_M;}
               else if (mouse == 3)
                  {command = POINT_R;}
               else
                  {command = POINT_L;}
               }
            }
         break;
      case S_ELLIPSE:
         command = ELLIPSE_CL;
         break;
      case S_RECTANGLE:
         command = RECTANGLE_CL;
         break;
      case S_UNKNOWN:
         command = NO_BUTT;
         break;
      case S_UNSUPPORTED:
         command = NO_BUTT;
         break;
      default:
         command = NO_BUTT;
         break;
   }
 
   /*
    * Recall tcl command from tcl list (if one was defined) and add parameters
    * from saoimage - then return
    */
   if (g_saoTCLList[(int)command][0] != (char)0)  {
      strcpy(a_line, &g_saoTCLList[(int)command][0]);
      strcat(a_line, " ");
      strcat(a_line, a_shl->region); 	/* Add region identifier to line */
      strcat(a_line, " ");
      strcat(a_line, params);
      sprintf(saonumber, " %d", a_saonum); 
      strcat(a_line, saonumber);	/* Add sao number to line */

      /*??? Put 'Check if string is too big' code somewhere near here */
      rstatus = 1;
   }
   else
      rstatus = 0; /* No TCL command was associated with this mouse click */

   /* See if we are waiting for a command just like this one. Or if we
      are waiting for any possible command. */
   if (g_mouseWait.wait == TRUE)
      {
      /* Now save the mouseDefine command, the mouse arguments and which
         command occurred. */
      strcpy (g_mouseGot.mDCommand, &g_saoTCLList[(int)command][0]);
      strcpy (g_mouseGot.params, params);
      g_mouseGot.command = command;
      }

return(rstatus);
}

/*
 * ROUTINE:
 *   p_shSaoMouseKeyGet
 *
 * CALL:
 *   (RET_CODE) p_shSaoMouseKeyGet(
 *                    CMD_SAOHANDLE *a_shl  // MOD: Internal handle ptr
 *                   )
 *
 * DESCRIPTION:
 *   Get the mouse click information from sao image
 *
 * RETURN VALUES:
 *   SH_SUCCESS : on success
 *   SH_PIPE_READERR,
 *   SH_PIPE_CLOSE: otherwise
 *
 */
RET_CODE
p_shSaoMouseKeyGet(CMD_SAOHANDLE  *a_shl)
{
   RET_CODE		rstatus;
   char                *bufptr;
   int                 nbytes, bytes_xfer;
 
   /*
    * Get the mouse click data from the pipe
    */
   nbytes     = CMD_SZ_IMCURVAL;
   bufptr     = (char *)a_shl->mousePos;
   bytes_xfer = read(a_shl->rfd, bufptr, nbytes);
   if (bytes_xfer < 0)  {
       rstatus = SH_PIPE_READERR; 
       goto err_exit;
   }
   else if (bytes_xfer == 0)  {
       rstatus = SH_PIPE_CLOSE; 
       goto err_exit;
   }
 
   /*
    * If we didn't get all the data, try one more time
    */
   if (bytes_xfer < nbytes)  {
       nbytes -= bytes_xfer;
       bufptr += bytes_xfer;
       bytes_xfer = read(a_shl->rfd, bufptr, nbytes);
       if (bytes_xfer < nbytes)  {
           rstatus = SH_PIPE_READERR; 
           goto err_exit;
       }
   }
 
   /* 
    * Null terminate just in case
    */
   a_shl->mousePos[CMD_SZ_IMCURVAL-1] = (char)0;  

   return(SH_SUCCESS);
 
   err_exit:
   return(rstatus);
}

/*
 * ROUTINE:
 *   p_shSaoMouseKeyAssign
 * 
 * CALL:
 *   (static void) shSaoMouseKeyAssign(
 *                   const int  a_button, // IN: SAO Image button pressed
 *                   const char *a_tcl    // IN: TCL cmd to associate w/ button
 *                 )
 *
 * DESCRIPTION:
 *   Associates a TCL command with a specific SAO image mouse button
 *
 * RETURN VALUE:
 *   None
 *
 * GLOBALS REFERENCED:
 *   g_saoTCLList
 */
static void
shSaoMouseKeyAssign(const int a_button, const char *a_tcl)
{
   if ((a_button > 0) && (a_button < CMD_NUM_SAOBUTT))
       strcpy(&g_saoTCLList[a_button][0], a_tcl);
}

/*
 * ROUTINE:
 *   shSaoMouseKeyList
 *
 * CALL:
 *   (static void) shSaoMouseKeyList(
 *                   const int a_butt   // IN: SAO image button list (0 for all)
 *                 )
 * DESCRIPTION:
 *   List all TCL commands assigned to SAO image mouse buttons
 *
 * RETURN VALUE:
 *   None
 *
 * GLOBALS REFERENCED:
 *   g_saoTCLList
 *   g_saobuttname
 */
static void
shSaoMouseKeyList(const int a_butt)
{
   int             i;
  
   printf("\nSAOimage mouse button        Associated TCL command\n");
   printf("---------------------------------------------------\n");
   if ((a_butt > 0) && (a_butt < CMD_NUM_SAOBUTT))
      printf("%-28s %s\n", &g_saobuttname[a_butt][0], &g_saoTCLList[a_butt][0]);
   else
      for (i=1; i < CMD_NUM_SAOBUTT; i++)  {
         if (g_saoTCLList[i][0] != 0)
            {printf("%-28s %s\n", &g_saobuttname[i][0], &g_saoTCLList[i][0]);}
      }

   return;
}
/*
 * ROUTINE:
 *   createMouseListMsg
 *
 * CALL:
 *   (void) createMouseListMsg(char *buffer);
 *
 * DESCRIPTION:
 *   Build the mouse list error message with all the allowable mouse commands.
 *
 * RETURN VALUES:
 *   none
 *
 * GLOBALS REFERENCED:
 *   g_saobuttname
 */
static void createMouseListMsg(char *a_buffer)
{
   int i;

   /* Copy in the error message first and then all the mouse commands */
   sprintf(a_buffer, "%s", g_tclMsgHdr);
   for (i = 1; i < CMD_NUM_SAOBUTT; i++)  {
      strcat(a_buffer, &g_saobuttname[i][0]);
      strcat(a_buffer, "\n");
   }
}

/*============================================================================
**============================================================================
**
** ROUTINE: p_shSaoMouseCheckWait
**
** DESCRIPTION:
**	This function is called to check if the mouse command we received
**      is the one that we are waiting for.
**
** RETURN VALUES:
**	
** CALLS TO:
**
** GLOBALS REFERENCED:
**	
**
**============================================================================
*/
int p_shSaoMouseCheckWait(int saoNum)
{
int rstatus;

/* See if we are waiting for anything */
if (g_mouseWait.wait == TRUE)
   {
   /* See if this command came from the fsao process we want */
   if ((g_mouseWait.fsao == saoNum) || (g_mouseWait.fsao == ANYFSAO))
      {
      /* Compare the command we just got with the one we are waiting for.  If
         the one we are waiting for is NO_BUTT, then we are waiting for
         anything */
      if ((g_mouseWait.command == g_mouseGot.command) ||
          (g_mouseWait.command == NO_BUTT))
         {
         /* Folks - we have a winner !! */
         g_mouseWait.wait = FALSE;               /* we are no longer waiting */
         Tk_DeleteTimerHandler(g_mouseWait.timerToken);  /* cancel the timer */
         p_shTk_CreateFileHandlerOnly();         /* restart command line use */
         if (g_mouseWait.eval == TRUE)
            {rstatus = TRUE;}                /* continue on as normal */
         else
            {rstatus = FALSE;}                /* do not pass go. */
         }
      else
         {
         /* This is not the command you are looking for, continue on. */
         rstatus = TRUE;
         }
      }
   else
      {
      /* This is not the fsao output you are looking for, continue on. */
      rstatus = TRUE;
      }
   }
else
   {
   /* We are not waiting for anything, even the bus. Let the calling routine
      proceed as normal. */
   rstatus = TRUE;
   }

return(rstatus);
}

/*============================================================================
**============================================================================
**
** ROUTINE: mouseCallWaiting
**
** DESCRIPTION:
**	This function is called once the tk timer has expired.
**
** RETURN VALUES:
**	
** CALLS TO:
**
** GLOBALS REFERENCED:
**	
**
**============================================================================
*/
static void mouseCallWaiting
   (
   ClientData clientData           /* IN : */
   )   
{
/* We are no longer waiting for a mouse command */
g_mouseWait.wait = FALSE;

/* Startup command line use again */
p_shTk_CreateFileHandlerOnly();

/* Tell the user that we timed out */
printf("Error: Time out occured while waiting for mouse command!\n");

}

static char *tclSaoMouseWaitHelp =
              "Wait for the arrival of information from a mouse click in fSAO";
static char *tclSaoMouseWaitUse = 
              "USAGE: mouseWait [button] [-t milliseconds] [-n] [-s fsao]";
/*
 * ROUTINE:
 *   tclSaoMouseWait
 *
 * CALL:
 *   (static int) tclSaoMouseWait(ClientData clientData, Tcl_Interp *interp,
 *                                int argc, char *argv[])
 *
 <AUTO>
 * TCL VERB:
 *   mouseWait
 *
 * DESCRIPTION:
 <HTML>
 * Wait for the arrival of a mouse command from fSAO.  The mouse command output
 * is generated whenever a supported mouse button is pressed when the window
 * focus is on an fSAO process.  No input will be accepted from the command line
 * or from a sourced file while the wait is in effect. If no arguments are
 * entered on the command line, Dervish will wait for any mouse button output from
 * any existing fSAO process forever.   When used in conjunction with the
 * <A HREF="#mouseSave">mouseSave</A> command, a specified mouse
 * button can be assigned a particular TCL command (with  
 * <A HREF="#mouseDefine">mouseDefine</A> which is only in effect
 * for the time that Dervish is waiting for mouse button output.  For example, 
 * consider the following procedure:
 </HTML>
 * 	proc mouseButton {} {
 *	  set oldMouse [mouseSave POINT_CL_L]
 *	  mouseDefine POINT_CL_L echo
 *	  mouseWait POINT_CL_L
 *	  mouseDefine POINT_CL_L $oldMouse
 *	}
 *
 * SYNTAX:
 *	mouseWait [button] [-t milliseconds] [-e] [-s fsao]
 *
 *	  [button]     Mouse button to wait for.  The available buttons are:
 *		POINT_L       - Point Mode, Left mouse button
 *		POINT_M       - Point Mode, Middle mouse button
 *		POINT_R       - Point Mode, Right mouse button
 *		POINT_CL_L    - Point Mode, Ctrl Left mouse button
 *		POINT_CL_M    - Point Mode, Ctrl Middle mouse button
 *		POINT_CL_R    - Point Mode, Ctrl Right mouse button
 *		POINT_SH_L    - Point Mode, Shift Left mouse button
 *		POINT_SH_M    - Point Mode, Shift Middle mouse button
 *		POINT_SH_R    - Point Mode, Shift Right mouse button
 *		POINT_CL_SH_L - Point Mode, Ctrl Shift Left mouse button
 *		POINT_CL_SH_M - Point Mode, Ctrl Shift Middle mouse button
 *		POINT_CL_SH_R - Point Mode, Ctrl Shift Right mouse butto
 *		CIRCLE_CL     - Circle Mode, Ctrl (any) mouse button
 *		ELLIPSE_CL    - Ellipse Mode, Ctrl (any) mouse button
 *		RECTANGLE_CL  - Rectangle Mode, Ctrl (any) mouse button
 </AUTO>
 *
 * RETURN VALUES:
 *   TCL_OK
 *   TCL_ERROR
 *
 * GLOBALS REFERENCED:
 *   g_saobuttname
 */
static int
tclSaoMouseWait(ClientData clientData, Tcl_Interp *interp, int argc, 
                char *argv[])
{
int   rstatus = TCL_OK;
int   j, i = 1;
char  dash[1];
int   maxParams = 7;
char  *mouseCmd = g_saobuttname[0];

dash[0] = '-';

/* Here we go again */
g_mouseWait.wait = FALSE;
g_mouseWait.command = NO_BUTT;            /* Wait for anything */
g_mouseWait.eval = TRUE;                  /* The default */
g_mouseWait.fsao = ANYFSAO;               /* wait for any fsao mouse input */
g_mouseWait.timeout = 0;                  /* Wait forever */

if (argc > maxParams)
   {
   /* There are more parameters than allowed. */
   Tcl_SetResult(interp, "Too many parameters", TCL_VOLATILE);
   Tcl_AppendResult(interp, tclSaoMouseWaitUse);
   rstatus = TCL_ERROR;
   }
else
   {
   /* Parse the arguments */
   while ((i < argc) && (rstatus != TCL_ERROR))
      {
      if (argv[i][0] == dash[0])	/* is an option */
         {
         j = 1;
switch_here:
         switch (argv[i][j]) 
            {
            case 't':
               /* A timeout value is requested - the number of secs should be
                  the next parameter */
               g_mouseWait.timeout = atoi(argv[i+1]);
               ++i;               /* Skip the timeout value too */
               break;
            case 'n':
               /* We should not evaluate the mouse command when it arrives */
               g_mouseWait.eval = FALSE;
               /* Check if any other switches were grouped with this one. */
               ++j;
               if (argv[i][j] != '\0')
                  {goto switch_here;}
               break;
	    case 's':
               /* Only look for mouse input from a specific fsao process */
               g_mouseWait.fsao = atoi(argv[i+1]) - 1;
               if (g_mouseWait.fsao < 0)
                  {
                  /* Invalid number, must be greater than 0 */
                  rstatus = TCL_ERROR;
                  Tcl_SetResult(interp,
                                "ERROR : Invalid SAO number - must be > zero.",
                                TCL_VOLATILE);
                  }
               else
                  {
                  if ((g_mouseWait.fsao > CMD_SAO_MAX) ||
                      (g_saoCmdHandle.saoHandle[g_mouseWait.fsao].state
		       == SAO_UNUSED))
                     {
                     rstatus = TCL_ERROR;
                     Tcl_SetResult(interp,
                                 "ERROR : No fSAO associated with SAO number.",
                                 TCL_VOLATILE);
                     }
                  }
               ++i;               /* skip the fsao number arg */
               break;
            default:
               /* Invalid option argument */
               Tcl_SetResult(interp, "Invalid parameter\n", TCL_VOLATILE);
               Tcl_AppendResult(interp, tclSaoMouseWaitUse, (char *)NULL);
               rstatus = TCL_ERROR;
            }
         }
      else
         {
         /* This must be the mouse command to wait for */
         mouseCmd = argv[i];
         }
      i++;					/* Go to next parameter */
      }
   }


/* Only continue on if there were no errors */
if (rstatus != TCL_ERROR)
   {
   /* If an fsao number was not entered, make sure there is at least 1 fsao
      process out there */
   if (g_mouseWait.fsao == ANYFSAO)
      {
      i = -1;                                /* start looking at beginning */
      if (p_shSaoIndexCheck(&i) != SH_SUCCESS)
	 {
	 rstatus = TCL_ERROR;             /* could not find any fsao process */
         Tcl_SetResult(interp, "No fSAO associated with this process",
                       TCL_VOLATILE);
         }
      }   
   }

/* Only continue on if there were no errors */
if (rstatus != TCL_ERROR)
   {
   /* Make sure the mouse command matches one of the allowed buttons */
   for (i = 0; i < CMD_NUM_SAOBUTT; i++)
      {
      if (!strcmp(mouseCmd, &g_saobuttname[i][0]))
          break;
      }
   if (i < CMD_NUM_SAOBUTT)
      {
      /* Found a match - and i = the enum value for the ascii button name.
         Set the global variable. */
      g_mouseWait.command = (CMD_SAOBUTT )i;

      /* Create the tk timer handler to do a timeout. */
      if (g_mouseWait.timeout > 0)
         {g_mouseWait.timerToken = Tk_CreateTimerHandler(g_mouseWait.timeout,
                                                    mouseCallWaiting,
                                                    clientData);}
      
      /* Stop tk from listening to the user's input on file descriptor 0.
         The user has to wait until we are done. */
         p_shTk_DeleteFileHandler();

      /* Inform the mouse input processor that we are waiting for a mouse
         command */
      g_mouseWait.wait = TRUE;

      /* Call the event processor over and over until either a timeout occurs
         or the requested event occurs. */
      while (g_mouseWait.wait == TRUE)
	 {
         Tk_DoOneEvent(0);
         }
      }
   else
      {
      /* Could not find the entered command on the approved list */
      createMouseListMsg(g_listMouseMsg);
      Tcl_SetResult(interp, g_listMouseMsg, TCL_VOLATILE);
      rstatus = TCL_ERROR;
      }
   }

return (rstatus);
}

static char *tclSaoMouseSaveHelp =
       "Return an action defined in a previous mouseDefine call";
static char *tclSaoMouseSaveUse = "USAGE: mouseSave <button>";
/*
 * ROUTINE:
 *   tclSaoMouseSave
 *
 * CALL:
 *   (static int) tclSaoMouseSave(ClientData clientData, Tcl_Interp *interp,
 *                                int argc, char *argv[])
 *
 * RETURN VALUES:
 *   TCL_OK
 *   TCL_ERROR
 *
 * GLOBALS REFERENCED:
 *   g_saoTCLList
 *   g_listMouseMsg
 *
 <AUTO>
 * TCL VERB:
 *   mouseSave
 *
 * DESCRIPTION:
 *	Return the TCL command that was associated with a particular mouse
 *	button in a previous mouseDefine command.
 *
 * SYNTAX:
 *   mouseSave <button>
 *
 *	<button>	Mouse button to return.  The available buttons are:
 *		POINT_L       - Point Mode, Left mouse button
 *		POINT_M       - Point Mode, Middle mouse button
 *		POINT_R       - Point Mode, Right mouse button
 *		POINT_CL_L    - Point Mode, Ctrl Left mouse button
 *		POINT_CL_M    - Point Mode, Ctrl Middle mouse button
 *		POINT_CL_R    - Point Mode, Ctrl Right mouse button
 *		POINT_SH_L    - Point Mode, Shift Left mouse button
 *		POINT_SH_M    - Point Mode, Shift Middle mouse button
 *		POINT_SH_R    - Point Mode, Shift Right mouse button
 *		POINT_CL_SH_L - Point Mode, Ctrl Shift Left mouse button
 *		POINT_CL_SH_M - Point Mode, Ctrl Shift Middle mouse button
 *		POINT_CL_SH_R - Point Mode, Ctrl Shift Right mouse button
 *		CIRCLE_CL     - Circle Mode, Ctrl (any) mouse button
 *		ELLIPSE_CL    - Ellipse Mode, Ctrl (any) mouse button
 *		RECTANGLE_CL  - Rectangle Mode, Ctrl (any) mouse button
 </AUTO>
 */
static int
tclSaoMouseSave(ClientData clientData, Tcl_Interp *interp, int argc, 
                char *argv[])
{
   int  i;
   int  rstatus = TCL_OK;
 
   if (argc != 2)  {
      Tcl_SetResult(interp, tclSaoMouseSaveUse, TCL_VOLATILE);
      rstatus = TCL_ERROR;
   }
   else  {
      for (i = 1; i < CMD_NUM_SAOBUTT; i++)  {
         if (!strcmp(argv[1], &g_saobuttname[i][0]))
             break;
      }

      if (i < CMD_NUM_SAOBUTT)
       Tcl_SetResult(interp, g_saoTCLList[i], TCL_VOLATILE);
      else  {
         /* Could not find entered mouse button in the approved list. */
         createMouseListMsg(g_listMouseMsg);
         Tcl_SetResult(interp, g_listMouseMsg, TCL_VOLATILE);
         rstatus = TCL_ERROR;
      }
   }

   return(rstatus);
}

static char *tclSaoMouseDefineHelp =
       "Define actions resulting from FSAO cursor mouse clicks";
static char *tclSaoMouseDefineUse =
       "mouseDefine          USAGE: mouseDefine <button> <TCL function>";
/*
 * ROUTINE:
 *   tclSaoMouseDefine
 *
 * CALL:
 *   (static int) tclSaoMouseDefine(ClientData clientData, Tcl_Interp *interp,
 *                                  int argc, char *argv[])
 *
 * ROUTINE DESCRIPTION:
 *   Routine for mouseDefine TCL command.  Uses shSaoMouseKeyAssign to
 *   assign a TCL command to a particular SAOimage mouse click.
 *
 * RETURN VALUES:
 *   TCL_OK
 *   TCL_ERROR
 *
 * GLOBALS REFERENCED:
 *   g_saobuttname
 *
 <AUTO>
 * TCL VERB:
 *   mouseDefine
 *
 * DESCRIPTION:
 * 	Assign a TCL command to be executed upon selection of the designated
 *	mouse button from a Fermi enhanced SAOimage (FSAOimage) program. The
 *	user supplied TCL command will be invoked with parameters based
 *	on the type of cursor used.  These are listed below:
 *
 * PARAMETERS PASSED TO USER SUPPLIED TCL COMMAND:
 *	Cursor Type  |  Parameters
 *	-------------+----------------------------------------------x
 *	point	     |	regName  X  Y  fsao#      
 *	circle	     |	regName  X  Y  radius  fsao#      
 *	ellipse	     |	regName  X  Y  rotAngle  width  height  fsao#      
 *	rectangle    |	regName  X  Y  rotAngle  width  height  fsao#      
 *
 *	Where,
 *		regName  - Handle to region displayed by FSAOimage
 *		X        - Center X coordinate
 *		Y        - Center Y coordinate
 *		fsao#    - FSAOimage number
 *		radius   - Radius of circle (in pixels)
 *		rotAngle - Rotation angle
 *		width    - Width (in pixels)
 *		height   - Height (in pixels)
 *
 * TCL SYNTAX:
 *   mouseDefine <button> <command>
 *	<button> Mouse button to assign.  The available buttons are:
 *		POINT_L       - Point Mode, Left mouse button
 *		POINT_M       - Point Mode, Middle mouse button
 *		POINT_R       - Point Mode, Right mouse button
 *		POINT_CL_L    - Point Mode, Ctrl Left mouse button
 *		POINT_CL_M    - Point Mode, Ctrl Middle mouse button
 *		POINT_CL_R    - Point Mode, Ctrl Right mouse button
 *		POINT_SH_L    - Point Mode, Shift Left mouse button
 *		POINT_SH_M    - Point Mode, Shift Middle mouse button
 *		POINT_SH_R    - Point Mode, Shift Right mouse button
 *		POINT_CL_SH_L - Point Mode, Ctrl Shift Left mouse button
 *		POINT_CL_SH_M - Point Mode, Ctrl Shift Middle mouse button
 *		POINT_CL_SH_R - Point Mode, Ctrl Shift Right mouse button
 *		CIRCLE_CL     - Circle Mode, Ctrl (any) mouse button
 *		ELLIPSE_CL    - Ellipse Mode, Ctrl (any) mouse button
 *		RECTANGLE_CL  - Rectangle Mode, Ctrl (any) mouse button
 </AUTO>
 */
static int
tclSaoMouseDefine(ClientData clientData, Tcl_Interp *interp, int argc, 
                  char *argv[])
{
   int  i;
   int  rstatus = TCL_OK;
 
   if (argc != 3)  {
      Tcl_SetResult(interp, tclSaoMouseDefineUse, TCL_VOLATILE);
      rstatus = TCL_ERROR;
   }
   else  {
      for (i = 1; i < CMD_NUM_SAOBUTT; i++)  {
         if (!strcmp(argv[1], &g_saobuttname[i][0]))
             break;
      }

      if (i < CMD_NUM_SAOBUTT)
         shSaoMouseKeyAssign(i, argv[2]);
      else  {
         /* Could not find entered mouse button in the approved list. */
         createMouseListMsg(g_listMouseMsg);
         Tcl_SetResult(interp, g_listMouseMsg, TCL_VOLATILE);
         rstatus = TCL_ERROR;
      }
   }

   return(rstatus);
}

static char *tclSaoMouseListHelp = "List actions assigned to mouse buttons";
static char *tclSaoMouseListUse = "mouseList";
/*
 * ROUTINE:
 *   tclSaoMouseList
 *
 * CALL:
 *   tclSaoMouseList(ClientData clientData, Tcl_Interp *interp, int argc,
 *                   char *argv[])
 * 
 <AUTO>
 * TCL VERB:
 *   mouseList
 *
 * DESCRIPTION:
 *	List the TCL commands assigned to FSAOimage mouse buttons.  The
 *	assignments are made using the mouseDefine command.
 *
 * TCL SYNTAX:
 *   mouseList
 </AUTO>
 */
static int
tclSaoMouseList(ClientData clientData, Tcl_Interp *interp, int argc,
                char *argv[])
{
   if (argc > 1)  {
       Tcl_AppendResult(interp, "usage: ", tclSaoMouseListUse, (char *)0);
       return TCL_ERROR;
   }

   shSaoMouseKeyList(0);

   return TCL_OK;
}

static char *tclSaoMouseToggleHelp =
       "Toggles mouse button verbosity. Used mainly for debugging purposes.";
static char *tclSaoMouseToggleUse = "mouseVerbose (or mouseQuiet)";
/*
 * ROUTINE:
 *   tclSaoMouseToggle
 *
 * CALL:
 *   (static int) tclSaoMouseToggle(ClientData clientData, Tcl_Interp *interp,
 *                                  int argc, char *argv[])
 *
 * ROUTINE DESCRIPTION:
 *   TCL routine to toggle the mouse button verbose switch.  When the
 *   switch is enabled, a message will be printed to stdout whenever
 *   one of the assignable mouse buttons in SAOimage is clicked.  This
 *   is generally used for debug purposes.
 *
 * RETURN VALUES:
 *   TCL_OK
 *   TCL_ERROR
 *
 * GLOBALS REFERENCED:
 *   g_saoMouseVerbose
 *
 <AUTO>
 * TCL VERB:
 *   mouseVerbose
 *
 * DESCRIPTION:
 *	Print a message to stdout each time one of the assignable FSAOimage
 *	mouse buttons is selected.  The message will be printed regardless of
 *	whether or not a TCL function is assigned to that button.  This
 *	feature is generally used for debug purposes and can be disabled with
 *	the mouseQuiet command.
 *
 * TCL SYNTAX:
 *   mouseVerbose
 </AUTO>
 <AUTO>
 * TCL VERB:
 *   mouseQuiet
 *
 * DESCRIPTION:
 *	Disable the mouseVerbose command.  Messages will no longer be sent to
 * 	stdout whenever one of the assignable FSAOimage mouse buttons is
 *	selected.  This command has no effect on TCL commands assigned to
 *	mouse buttons - they will still be invoked when selected.
 *
 * TCL SYNTAX:
 *   mouseQuiet
 </AUTO>
 */
static int
tclSaoMouseToggle(ClientData clientData, Tcl_Interp *interp, int argc,
                  char *argv[])
{
   if (argc > 1)  {
       Tcl_AppendResult(interp, "usage: ", tclSaoMouseToggleUse, (char *)0);
       return TCL_ERROR;
   }

   g_saoMouseVerbose = (clientData ? 1 : 0);

   return TCL_OK;
}

/*
 * ROUTINE:
 *   shTclSaoMouseDeclare
 *
 * CALL:
 *   (void) shTclSaoMouseDeclare(
 *               Tcl_Interp *a_interp  // IN: TCL interpreter
 *          )
 *
 * DESCRIPTION:
 *   The world's access to SAO mouse functions.
 *
 * RETURN VALUE:
 *   Nothing
 */
void
shTclSaoMouseDeclare(Tcl_Interp *a_interp)
{
   char *helpFacil = "shMouse";

   shTclDeclare(a_interp, "mouseDefine", (Tcl_CmdProc *) tclSaoMouseDefine,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                tclSaoMouseDefineHelp, tclSaoMouseDefineUse);

   shTclDeclare(a_interp, "mouseList", (Tcl_CmdProc *) tclSaoMouseList,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                tclSaoMouseListHelp, tclSaoMouseListUse);

   shTclDeclare(a_interp, "mouseVerbose", (Tcl_CmdProc *) tclSaoMouseToggle,
                (ClientData) 1, (Tcl_CmdDeleteProc *) 0, helpFacil,
                tclSaoMouseToggleHelp, tclSaoMouseToggleUse);

   shTclDeclare(a_interp, "mouseQuiet", (Tcl_CmdProc *) tclSaoMouseToggle,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                tclSaoMouseToggleHelp, tclSaoMouseToggleUse);

   shTclDeclare(a_interp, "mouseWait", (Tcl_CmdProc *) tclSaoMouseWait,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                tclSaoMouseWaitHelp, tclSaoMouseWaitUse);

   shTclDeclare(a_interp, "mouseSave", (Tcl_CmdProc *) tclSaoMouseSave,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                tclSaoMouseSaveHelp, tclSaoMouseSaveUse);

   return;
}



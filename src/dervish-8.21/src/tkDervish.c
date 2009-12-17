/*
 * FILE:
 *   tkDervish.c
 *
 * ABSTRACT:
 *   This file contains routines to initialize Tk/Wish and Tcl. Additionally,
 *   it also contains the user-level function shTk_MainLoop() which is
 *   the DERVISH event loop.
 *
 * MODIFICATION:  
 *   09/15/93  Wei Peng
 *   To facilitate use of dervish without the capability of using tk window, 
 *   conditions are added such that, if no DISPLAY is set, dervish will start
 *   but with a warning message. The event loop used then is Tcl_Startup.
 * 
 *  (This has been modified by Chih-Hao Huang on 11/14/94. Please see
 *   the notes below.)
 *
 *
 *   10/18/93 Vijay
 *   dervish -f file will now exit when input from file is exhausted.
 *
 *
 *   11/14/94 Chih-Hao Huang
 *
 *   Many changes have been made to bring up dervish with Ftcl2.0
 *   which is based on TclX7.3a, which, in terms, is based on Tcl7.3,
 *   and Ftk2.0, which is based on TkX3.6a, which is part of TclX7.3a
 *   and is based on Tk3.6.
 *
 *   The concept of "Tcl_AppInit()" is first employed to be consistent
 *   with new generation of Tcl products. Tcl_AppInit() is defined
 *   in "shAppInit.c". It is supposed to do the application (dervish in
 *   this case) specific initilization (such as finding and running
 *   the startup scripts).
 *
 *   A cleaning up effort has been made to straighten out the issue
 *   about the supporting environment for running dervish. The issues is,
 *   for many practical reasons, dervish should be able to run as much as
 *   as possible even in one of the following limited environments:
 *
 *   [1] No X support. (So that Tk is impossible to use.) There are
 *       several different situations:
 *       [1a] The machine that is running dervish has no X support at all.
 *       [1b] The machine that is running dervish has X support but the
 *            output device has no X support (such as VT100, ..etc.)
 *       [1c] The DISPLAY variable was not set or was not set correctly.
 *   [2] dervish image is running alone with no supporting products nor
 *       supporting scripts. People may copy the dervish image alone and
 *       have it run in a bare environment. In this situation, even the
 *       dervish is running in very limited fashion, it should not crash.
 *
 *   Note: Even in [2], it is still possible that the environment does
 *       have X support.
 *
 *   In the past, to know whether the dependent products, such as FTCL,
 *   FTK, etc., were properly set up is thought the testing of the
 *   existence of the environmental variables such as FTCL_DIR,
 *   FTK_DIR, ..., etc. However, if we take fact in to account that
 *   Tcl related products may be hard wired at compile time, these
 *   environmental variable may not be reliable. That is, (hard wired)
 *   Tcl products may run just fine without any product related
 *   environmental variables. Therefore, the logic of determining the
 *   the running environment is different and is as follows:
 *
 *   [1] dervish will first try to create a Tk Main Window. If this
 *       attemp succeeds, it means the X support is fine on the host
 *       that is running dervish. Then, Tk_MainLoop() will be the
 *       command loop of dervish. Otherwise, including the case that
 *       DISPLAY variable is set wrong or unset, dervish assumes the
 *       X windowing capability is impossible. Then it tricks
 *       Tk_MainLoop() by increasing "tk_NumMainWindows".
 *
 *   [2] Even though a Tk Main Window may be created successfully, the
 *       user may not want to use or see it at all. Therefore, by
 *       default, once the Tk Main Window is created, unless the user
 *       specify "-useTk" switch in the command line when invoking
 *       dervish, the main window will be "withdrawn" immediately from
 *       the window manager. This main window become "invisible" and
 *       the purpose of its existence is to allow dervish to use
 *       Tk_MainLoop() as the command loop. If the user does specify
 *       "-useTk" switch, the main window will NOT be withdrawn.
 *
 *   [3] Whenever an attempt to initialize the dependent product fails,
 *       a warning message is generate and the dervish is allowed to
 *       proceeed, yet with limited abilities, which is supposed to be
 *       understandable to the users when they see the warning
 *       messages.
 *
 *
 * ENTRY POINT            SCOPE    DESCRIPTION
 * ----------------------------------------------------------------------------
 * shMainTcl_Init         public   Initialize Tcl, Tk/Wish
 * shMainTcl_PromptReset  public   Reset prompt for TCL command line editor
 * shTk_MainLoop          public   DERVISH event loop
 * StdinProc              local    Event procedure called by Tk when I/O is
 *                                    detected on file descriptor 0
 * StructureProc          local    Interaction with Tk/Wish
 * DelayedMap             local    Interaction with Tk/Wish
 * 
 * Vijay K. Gurbani
 * Fermi National Accelerator Laboratory, Batavia, Illinois.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright 1990-1992 Regents of the University of California.
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
#include <sys/types.h>
#include <unistd.h>     /* For prototype of isatty() */
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#ifdef SDSS_IRIX5
/* This is REALLY ugly but we need to include resource.h.  However resource.h
   itself includes sys/time.h and counts on it to define the struct timeval.
   However since we have _POSIX_SOURCE defined, timeval does not get defined
   in sys/time.h and thus resource.h does not get what it wants so the file
   does not compile.  Therefore we are forced to define our own timeval
   struct.  I do not believe that we use it from resource .h so it should be
   safe. This was pulled verbatim from time.h on IRIX. (EFB) */
struct timeval {
        long    tv_sec;         /* seconds */
        long    tv_usec;        /* and microseconds */
};
#endif
#include <sys/resource.h>  /* To be able to fix the stack size */ 

#include "region.h"
#include "shTclSao.h"
#include "shTk.h"
#include "tcl.h"
#include "tclExtend.h"
#include "tk.h"
#include "shCErrStack.h"		/* For Tcl_CreateTrace () */
#include "prvt/errStackP.h"	/* For Tcl_CreateTrace () */
#include "shTclErrStack.h"

#include "tkInt.h"

#include "region.h"      /* For definition of PIXDATATYPE */
#include "dervish_msg_c.h"

/*
 * Prototype for function called on exiting dervish
 */
extern void shCleanUp(void);

/*
 * Global variables used by the main program:
 */
static Tk_Window w;		/* The main window for the application.  If
				 * NULL then the application no longer
				 * exists. */
static int argc;                /* Command line argument count */
static char **argv;             /* Command line argument vector */

static CMD_HANDLE  *g_shHndl;   /* Save initial handle so can get at by mouse
                                   functions calling functions here. */

#define CONTINUE_PROMPT	">> ";
#define GETCHAR_TIMEOUT 1	/* Timeout value for getchar (1/10 sec) */
#define NOCONTINUE 0            /* Exit after sourcing fileName or executing
				   command */

/*
 * Command-line options:
 */

int synchronize = 0;
int useTk = 0;
int noTk = 0;
static int shContinue = NOCONTINUE;

static char *fileName = NULL;
static char *commandName = NULL;
char *name = NULL;
char *display = NULL;
char *geometry = NULL;
char *cmdLinePrompt = NULL;

Tk_ArgvInfo argTable[] = {
    {"-file", TK_ARGV_STRING, (char *) NULL, (char *) &fileName,
	"File from which to read commands"},
    {"-continue", TK_ARGV_CONSTANT, (char *) 1, (char *) &shContinue,
	"Do not exit after sourcing fileName or executing command"},
    {"-geometry", TK_ARGV_STRING, (char *) NULL, (char *) &geometry,
	"Initial geometry for window"},
    {"-display", TK_ARGV_STRING, (char *) NULL, (char *) &display,
	"Display to use"},
    {"-name", TK_ARGV_STRING, (char *) NULL, (char *) &name,
	"Name to use for application"},
    {"-sync", TK_ARGV_CONSTANT, (char *) 1, (char *) &synchronize,
	"Use synchronous mode for display server"},
    {"-useTk", TK_ARGV_CONSTANT, (char *) 1, (char *) &useTk,
        "Use the Tk/Wish capabilities with DERVISH"},
    {"-noTk", TK_ARGV_CONSTANT, (char*) 1, (char *) &noTk,
        "Do not use the Tk/Wish capabilities with DERVISH"}, 
    {"-prompt", TK_ARGV_STRING, (char *) NULL, (char *) &cmdLinePrompt,
        "Set the main DERVISH prompt to given string"},
    {"-command", TK_ARGV_STRING, (char *) NULL, (char *) &commandName,
        "Execute the following command and then exit;\n"
        "            The command to be executed must be quoted, i.e.\n"
        "            -command \"echo Running an interactive command\""},
    {(char *) NULL, TK_ARGV_END, (char *) NULL, (char *) NULL,
	(char *) NULL}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void DelayedMap(ClientData clientData);
static void StdinProc(ClientData clientData, int mask);
static void StructureProc (ClientData clientData, XEvent *eventPtr);

void
shMainTcl_PromptReset(CMD_HANDLE *sh_handle, char *prompt)
{
strncpy(sh_handle->prompt, prompt, CMD_PROMPT_LEN);
sh_handle->prompt[CMD_PROMPT_LEN] = (char) 0;  /* Null terminate just in case */
}


RET_CODE 
shMainTcl_Init(CMD_HANDLE *sh_handle, int *ac, char **av, char *prompt)
{
   RET_CODE status;
   Tcl_Interp    *l_interp;
   struct rlimit rlp;
   char  *tcl_interactive_temp;
   
   sh_handle->interp = Tcl_CreateInterp();
   l_interp = sh_handle->interp;

#ifdef TCL_MEM_DEBUG
   Tcl_InitMemory(l_interp);
#endif
#define DERVISH_STACK  67108864   /* This is 64Mb */  
   /*
    *  Set the stack size limit to the min of 64Mb and the hard limit
    *  cf man pages for setrlimit and ulimit (or limit) for more information
    *  This value is set for this process and all its children.
    */

   getrlimit( RLIMIT_STACK, &rlp);  /* We are getting the hard limit */
   rlp.rlim_cur= (rlp.rlim_max>DERVISH_STACK) ? DERVISH_STACK : rlp.rlim_max;
   setrlimit( RLIMIT_STACK, &rlp);  /* Set the soft limit */

   /*
    * Parse command-line arguments.
    */
   if (Tk_ParseArgv(l_interp, (Tk_Window) NULL, ac, av, argTable, 0)
          != TCL_OK) {
	fprintf(stderr, "%s\n", l_interp->result);
	return SH_GENERIC_ERROR;
   }

   /*
    * If both -file and -command were present on the command line, flag that
    * as an error.
    */
   if (fileName && commandName)  {
       fprintf(stderr, "%s: please specify either -file or -command\n", *av);
       return SH_GENERIC_ERROR;
   }

   /*
    * If a display was specified, put it into DISPLAY
    * environment variable so that it will be available for
    * any sub-processes created by us.
    */

   if (display != NULL)
   {
      Tcl_SetVar2(l_interp, "env", "DISPLAY", display, TCL_GLOBAL_ONLY);
   }

   /*
    * Set the "tcl_interactive" variable
    */

   tcl_interactive_temp = (( !isatty(0) ||
			   ((shContinue == NOCONTINUE) &&
			    ((fileName != NULL) || (commandName != NULL)))
			   ) ? "0" : "1");

   Tcl_SetVar(l_interp, "tcl_interactive", tcl_interactive_temp,
	      TCL_GLOBAL_ONLY);

   /*
    * Initialize the command buffer and populate the sh_handle structure.
    */

   Tcl_DStringInit(&sh_handle->TclBuf);
   status = tclSaoInit(l_interp);

   if (status != SH_SUCCESS)  {
       fprintf(stderr, "tclSaoInit() failed. Return status = %d\n", status);
       return SH_GENERIC_ERROR;
   }

   /*
    * Initialize the ftcl history, then declare the ftcl interrupt handlers.
    */
   ftclCmd_HistInit(&(sh_handle->history), l_interp);
   ftclCmd_INTDec(2);

   /*
    * Create a Tcl trace for the error stack.
    *
    *   o  The error stack is automatically cleared on the first push to it.
    *      But, this is permitted only from the top level of the interpreter.
    */

   Tcl_CreateTrace (l_interp, 1, shErrStackTclTrace, (ClientData)(0));
 
   /*
    * Save the following values. Will need them in shTk_MainLoop()
    */
   strncpy(sh_handle->prompt, prompt, CMD_PROMPT_LEN);
   sh_handle->prompt[CMD_PROMPT_LEN] = (char) 0;  /* Null terminate just in case */
   argc = *ac;
   argv = av;

   /*
    * Before returning, declare a callback function that gets called
    * before exiting dervish. Any cleanup activities should be put in
    * this function (like killing fsao or pgplot processes upon the
    * exit of dervish)
    */
   atexit(shCleanUp);
   return SH_SUCCESS;
}


/* These tk routines have been factored off so they can be called
   from the mouse commands to reinstate the stdin file handler after waiting
   for a mouse command */
void p_shTk_CreateFileHandlerOnly(void)
{
   Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) g_shHndl);
}


static void p_shTk_CreateFileHandler(void)
{
   if ( ((fileName == NULL) && (commandName == NULL)) ||
       shContinue != NOCONTINUE ) {

        /*
         * Commands will come from standard input (0).  Set up a handler
         * to receive those characters and print a prompt if the input
         * device is a terminal. Commands can also come from anywhere
         * else, like a named pipe. In that case, include all file
         * descriptors of interest here.
         */
   
        /* Modification: if window is not creatable, proceed with no Tk
         * capability. Following two lines are not used then. Tcl_Startup
         * itself generates a prompt and executes commands. 
         * Wei Peng. 09/15/93
         */

        ftclCmd_LineStart(&(g_shHndl->lineHandle), g_shHndl->prompt, 
                          &(g_shHndl->history), GETCHAR_TIMEOUT);
        Tk_CreateFileHandler(0, TK_READABLE, StdinProc, (ClientData) g_shHndl);
      }
}


/* Disable stdin handling by calling Tk_CreateFileHandler with a mask of 0.
   See "Tcl and the Tk Toolkit" (Ousterhout), p. 398. */
void p_shTk_DeleteFileHandler(void)
{
   Tk_CreateFileHandler(0, 0, StdinProc, (ClientData) g_shHndl);
}


RET_CODE 
shTk_Init(CMD_HANDLE *a_shHndl)
{
   char *p,
        *args,
        buf[20];
   Tk_3DBorder border;
   Tcl_Interp	*interp;


   interp = a_shHndl->interp;
   g_shHndl = a_shHndl;               /* save so can get at later when called
                                         from mouse functions */
   if (name == NULL) {
       if (fileName != NULL) {
           p = fileName;
       } else {
           p = argv[0];
       }
       name = strrchr(p, '/');
       if (name != NULL) {
           name++;
       } else {
           name = p;
       }
   }

   /*
    * If prompt given on command line, use that instead of the default one.
    */
   if (cmdLinePrompt != NULL)
      {
      strncpy(a_shHndl->prompt, cmdLinePrompt, CMD_PROMPT_LEN);
      a_shHndl->prompt[CMD_PROMPT_LEN] = (char) 0;  /* Null terminate just in case */
      }

   /*
    * Initialize the Tk application and arrange to map the main window
    * after the startup script has been executed, if any.  This way
    * the script can withdraw the window so it isn't ever mapped
    * at all.  If the environment variable 'TK_LIBRARY' does not exist,
    * do not try to create a window. Inform the user that tk will not be used
    * and skip window creation, set w = NULL.
    */

   Tcl_ResetResult(interp);     /* So that only tk error msg are in interp->result */
   if ( noTk == 0 )             /* noTk has not been set */
     {
       w = Tk_CreateMainWindow(interp, display, name, "Dervish");

       /* Remove the top level main window that no one seems to like */

       if ((w != NULL) && (!useTk))
	 { char withdrawMainWindow[] = "wm withdraw .";
	   if (Tcl_Eval(interp, withdrawMainWindow) != TCL_OK)
	     {
	       fprintf(stderr, "Error: Can't close the main window\n");
	     }
	 }

       /* Start the window up as an icon */
       if ((w != NULL) && (useTk))
	 { char iconifyMainWindow[] = "wm iconify .";
	 if (Tcl_Eval(interp, iconifyMainWindow) != TCL_OK)
	   {
	     fprintf(stderr, "Error: Can't iconify the main window\n");
	   }
	 }

       if (w == NULL)
	 {
	   fprintf(stderr, "Warning: %s\n", interp->result);
	   fprintf(stderr, "Warning: cannot use Tk capabilities\n");
	 }
     };

   /*
    * Initialized all packages and application specific commands.
    * This includes Extended Tcl initialization.
    */

	/* current version of Tcl_AppInit() can never fail */

   if (Tcl_AppInit (interp) == TCL_ERROR) {
     fprintf(stderr,"Error: Tcl_AppInit() failed\n");
     TclX_ErrorExit (interp, 255);
   }

   if(w!=NULL) {

       Tk_SetClass(w, "Tk");
       Tk_CreateEventHandler(w, StructureNotifyMask, StructureProc,
                        (ClientData) NULL);
       Tk_DoWhenIdle(DelayedMap, (ClientData) &useTk);
       if (synchronize)
           XSynchronize(Tk_Display(w), True);

       Tk_GeometryRequest(w, 200, 200);
       border = Tk_Get3DBorder(interp, w, "#ffe4c4");
   
     if (border == NULL) {
         Tcl_SetResult(interp, (char *) NULL, TCL_STATIC); 
         Tk_SetWindowBackground(w, WhitePixelOfScreen(Tk_Screen(w)));
        } else {
          Tk_SetBackgroundFromBorder(w, border);
       }

      XSetForeground(Tk_Display(w), DefaultGCOfScreen(Tk_Screen(w)),
      BlackPixelOfScreen(Tk_Screen(w)));

   }
   else
   {
      tk_NumMainWindows++;
   }

   /*
    * Make command-line arguments available in the Tcl variables "argc"
    * and "argv".  Also set the "geometry" variable from the geometry
    * specified on the command line.
    */
 
   args = Tcl_Merge(argc-1, argv+1);
   Tcl_SetVar(interp, "argv", args, TCL_GLOBAL_ONLY);
   ckfree(args);
   sprintf(buf, "%d", argc-1);
   Tcl_SetVar(interp, "argc", buf, TCL_GLOBAL_ONLY);
   if (geometry != NULL)
       {Tcl_SetVar(interp, "geometry", geometry, TCL_GLOBAL_ONLY);}

   /* In case you're wondering - there used to be a call to
   ** p_shTk_CreateFileHandler here, but it has been moved to
   ** shTk_MainLoop so that users can call their own startup
   ** routines (after this routine) without the prompt getting
   ** printed prematurely.                GPS - 1/18/96
   */

   return (SH_SUCCESS);
}

/* Modified by Chih-Hao */

/* A short prototype for ftcl_CommandLoop() */

extern void ftcl_CommandLoop(Tcl_Interp *interp, FILE *in, FILE *out,
	int (*evalProc)(), unsigned options);

void shTk_MainLoop(CMD_HANDLE *a_shHndl)
{
   Tcl_Interp	*interp;
   int  result;
   char *msg;


   interp = a_shHndl->interp;

   if (fileName != NULL) {
       /*
        * If a -file option is provided, source the file named.
        *
        *   o   Tk_DoOneEvent is called to provide the "feel" of Tk_MainLoop.
        *       Some X Window work gets accomplished with this call.  Without
        *       it, a straight call to Tcl_Eval can behave differently than
        *       the same command line ("source filename") issued from the
        *       keyboard (or any X event) and executed with Tk_MainLoop.
        */
       char tmpBuf[80];

       Tk_DoOneEvent(TK_DONT_WAIT);

       if (access(fileName, F_OK) != 0) {
	  snprintf(tmpBuf, sizeof(tmpBuf), "No such file: %s", fileName);
	  Tcl_SetVar(interp, "errorInfo", tmpBuf, TCL_GLOBAL_ONLY);
	  goto error;
       }

       result = Tcl_EvalFile(interp, fileName);
       if (result != TCL_OK) {
           goto error;
       }
       /*
	* Only exit if the -continue flag was not specified
	*/
       if (shContinue == NOCONTINUE) {
       /*
        * We could easily use the system call exit(2) to quit out now. But
        * in this politically correct world we are living in, since we are
        * using Tcl, might as well use Tcl to exit
        */
	 sprintf(tmpBuf, "exit 0");
	 result = Tcl_Eval(interp, tmpBuf);
	 if (result != TCL_OK)
           goto error;
       }
     }

   if (commandName != NULL) {
       /*
        * If a -command option is provided, execute the command.
        */
       char tmpBuf[2000];

       if (strlen(commandName) <= sizeof(tmpBuf)-1)  {

           Tk_DoOneEvent(TK_DONT_WAIT);
           sprintf(tmpBuf, "%s", commandName);
           result = Tcl_Eval(interp, tmpBuf);
	   if (shContinue != NOCONTINUE) {
	      if(result != TCL_OK) goto error;
	   } else {
	      /*
	       * Only exit if the -continue flag was not specified; if it
	       * is return the low order 7 bits of the return code from
	       * TCL, and set bit 8 in case of error
	       */
	      int code;			/* return code for unix */

	      if(result != TCL_OK) {
		 fprintf(stderr,"errorInfo: %s\n",
			 Tcl_GetVar(interp, "errorInfo", 0));
	      }

	      code = atoi(interp->result);
	      exit((code & 0x7f) + (result == TCL_OK ? 0 : 0x80));
	   }
       } else  {
           Tcl_SetVar(interp, "errorInfo", 
                         "Internal buffer too small to hold command."
                         " Please notify\nsdss-bugs@sdss.fnal.gov about"
                         " this error message.\n", 0);
           goto error;
       }
     }
   /* Call routine to prepare to handle commands from standard input */
   p_shTk_CreateFileHandler();

   /*
    * Loop infinitely, waiting for commands to execute.  When there
    * are no windows left, Tk_MainLoop returns and we clean up and
    * exit.
    */

   Tk_MainLoop();
 
   Tcl_DeleteInterp(a_shHndl->interp);
   Tcl_DStringFree(&a_shHndl->TclBuf);    
   exit(0); 

error:
   msg = Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY);
   if (msg == NULL)
       msg = interp->result;

   fprintf(stderr, "%s\n", msg);
   {
      char cmd[] = "destroy .";		/* Tcl_Eval modifies arg 2 (!) */
      Tcl_Eval(interp, cmd);
   }
   
   exit(1);
}
/*
 *----------------------------------------------------------------------
 *
 * StdinProc --
 *
 *	This procedure is invoked by the event dispatcher whenever
 *	standard input becomes readable.  It grabs the next line of
 *	input characters, adds them to a command being assembled, and
 *	executes the command if it's complete.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Could be almost arbitrary, depending on the command that's
 *	typed.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
static void
StdinProc(ClientData clientData, int mask)
{
    CMD		l_line;
    CMD_HANDLE  *shHandle;
    int		lineEntered, result;
    Tcl_Interp  *interp;
    char	*promptPtr;
    int		lchar;
    int		lpCnt = 0;

    shHandle = (CMD_HANDLE *)clientData;
    interp   = shHandle->interp;

    /* Get single character input from stdin */
    ftclCmd_GetChar(&shHandle->lineHandle);
    /* Now process the entered character */
    lineEntered = ftclCmd_ProcChar(&shHandle->lineHandle, l_line);

    /* Loop to get any queued up characters */
    while(!lineEntered)
       {
       lchar = ftclCmd_GetChar(&shHandle->lineHandle);
       if (lchar <= 0)
          {break;}
       lineEntered = ftclCmd_ProcChar(&shHandle->lineHandle, l_line);
       if (lpCnt++ > 100) {break;}	/* Don't loop too long */
       }
 
    /* Assemble/execute the TCL command if line entry is complete */ 
    if (lineEntered == 1)
       {
       strcat(l_line, "\n");
       Tcl_DStringAppend(&shHandle->TclBuf, l_line, -1);
       if ( Tcl_CommandComplete(Tcl_DStringValue(&shHandle->TclBuf)))
          {
	  result = Tcl_RecordAndEval(interp, Tcl_DStringValue(&shHandle->TclBuf) , 0);
	  Tcl_DStringFree(&shHandle->TclBuf);
          if (result != TCL_OK)
             {
             printf("Error");
             if (*interp->result != 0) 
                {printf(": %s\n", interp->result);}
             else
                {printf("\n");}
             }
          else
             {
             if (*interp->result != 0) 
                {printf("%s\n", interp->result);}
             }
          promptPtr = shHandle->prompt;
          }
       else
          {
          promptPtr = CONTINUE_PROMPT;
          }

       /* Do not do anything with the command line if we are waiting for
          mouse input to arrive */
       if (p_shSaoMouseWaitGet() == FALSE)
          ftclCmd_LineStart(&shHandle->lineHandle, promptPtr, 
                            &shHandle->history, GETCHAR_TIMEOUT);
       }
   else if (lineEntered < 0)
      {
      /*???*/
      Tcl_DeleteInterp(interp);
      Tcl_DStringFree(&shHandle->TclBuf);
      exit(0);
      }
}


/*
 *----------------------------------------------------------------------
 *
 * StructureProc --
 *
 *	This procedure is invoked whenever a structure-related event
 *	occurs on the main window.  If the window is deleted, the
 *	procedure modifies "w" to record that fact.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Variable "w" may get set to NULL.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
StructureProc(ClientData clientData, XEvent *eventPtr)
{
    if (eventPtr->type == DestroyNotify) {
	w = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DelayedMap --
 *
 *	This procedure is invoked by the event dispatcher once the
 *	startup script has been processed.  It waits for all other
 *	pending idle handlers to be processed (so that all the
 *	geometry information will be correct), then maps the
 *	application's main window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The main window gets mapped.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
DelayedMap(ClientData flag)
{
   int *tk;

   while (Tk_DoOneEvent(TK_IDLE_EVENTS) != 0) {
	/* Empty loop body. */
   }
   if (w == NULL)
       return;

   tk = (int *) flag;
   if (*tk)
       Tk_MapWindow(w);
}

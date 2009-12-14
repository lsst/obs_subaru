/*****************************************************************************
 ******************************************************************************
 **
 ** FILE:
 **	photo_template.c
 **
 ** ABSTRACT:
 **	This is the main program for the PHOTO product.
 **
 **	This product is built on top of DERVISH and FTCL.
 **
 ** ENVIRONMENT:
 **	ANSI C.
 **
 ******************************************************************************
 ******************************************************************************
 */
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include "dervish.h"
#include "tclMath.h"
#include "astrotools.h"
#include "phSpanUtil.h"
#include "phSignals.h"
#include "phMeschach.h"			/* for phMeschachLoad() */

void phTclDervishDeclare(Tcl_Interp *interp);
void phTclVerbsDeclare(Tcl_Interp *interp);

#define	L_PHPROMPT	"photo> "   /* Command line prompt for PHOTO */
/*
 * declare variables/functions for the signal catching facilities
 */
DECLARE_CATCH;

/*
 * This is an shMalloc callback function. If you set the C variable
 * g_Serial_threshold to a number, this function will be called when
 * that block is allocated.
 *
 * If write_malloc_trace is true, a file called shMalloc.dat will be written
 * recording that call to malloc; if the threshold is ~0 this file will
 * be _large_, containing all calls to malloc (the same file can be written
 * for shFree --- see p_free_trace)
 *
 * I expect that g_Serial_threshold and write_malloc_trace will be set
 * using a debugger (you may have to set the scope of the variable with
 * something like shGarbage.g_Serial_threshold).
 */
static volatile int write_malloc_trace = 0;
static FILE *shMalloc_fil = NULL;

static void
p_malloc_trace(unsigned long long thresh,
	       const SH_MEMORY *mem)
{
   static volatile int frequency = 0;	/* frequency of breaking */

   if(write_malloc_trace) {
      if(shMalloc_fil == NULL) {
	 if((shMalloc_fil = fopen("shMalloc.dat","w")) == NULL) {
	    shFatal("Can't open shMalloc.dat");
	 }
      }
      fprintf(shMalloc_fil,"shMalloc %llu %lu  %lu %s %d %ld %ld\n",
	      mem->serial_number, shMemBytesInUse(), shMemBytesInPool(),
	      mem->file, mem->line,
	      (long)mem->user_bytes, (long)mem->actual_bytes);
   }

   if(frequency > 0) {
      shMemSerialCB(thresh + frequency, p_malloc_trace);
   }
}

/*
 * And here's another, which can be used to check the heap for
 * corruption at any desired granularity (set by the variable frequency)
 *
 * Note that it resets the callback trigger
 */
static void
p_malloc_check(unsigned long long thresh,
	       const SH_MEMORY *mem)	/* NOTUSED */
{
   static volatile int abort_on_error = 1; /* abort on first error? */
   static volatile int check_allocated = 1; /* check allocated blocks? */
   static volatile int check_free = 1;	/* check free blocks? */
   static volatile int frequency = 10;	/* frequency of checks */

   shAssert(mem != NULL);		/* use it for something */

   if(frequency > 0) {
#if DERVISH_VERSION >= 6 && DERVISH_MINOR_VERSION >= 8
      if(p_shMemCheck(check_allocated, check_free, abort_on_error)) {
	 shError("Continuing after memory error was detected");
      }
#endif
      shMemSerialCB(thresh + frequency, p_malloc_check);
   }
}

/*
 * Here's a callback function for shFree. See comments above
 * p_malloc_trace
 */
static void
p_free_trace(unsigned long long thresh,	/* NOTUSED */
	     const SH_MEMORY *mem)
{
   if(write_malloc_trace) {
      if(shMalloc_fil == NULL) {
	 if((shMalloc_fil = fopen("shMalloc.dat","w")) == NULL) {
 	    shFatal("Can't open shMalloc.dat");
	 }
      }
      fprintf(shMalloc_fil,"shFree %llu %lu  %lu %s %d %ld %ld\n",
	      mem->serial_number, shMemBytesInUse(), shMemBytesInPool(),
	      mem->file, mem->line,
	      (long)mem->user_bytes, (long)mem->actual_bytes);
   }
}

/*****************************************************************************/
/*
 * Source a file given by an environment variable
 */
static int
source(Tcl_Interp *interp, char *file)
{
   char cmd[200];
   char *script = getenv(file);
   
   if(script != NULL) {
      if(*script != '\0') {
	 sprintf(cmd, "source %s",script);
	 printf("Executing commands in %s: ",script);
	 Tcl_Eval(interp, cmd);
	 putchar('\n');
	 if(*interp->result != '\0') {
	    printf("--> %s\n", interp->result);
	 }
	 fflush(stdout);
      }
      return(0);
   } else {
      return(-1);
   }
}

/*****************************************************************************/

static jmp_buf jmp_env;
static int initialised_setjmp = 0;	/* did we call setjmp() ? */

void
p_phSignalTrap(int sig)
{
   signal(sig, p_phSignalTrap);		/* re-install signal handler */
   
   if(initialised_setjmp) {
      longjmp(jmp_env, sig);
   } else {
      shError("trap: you must call setjmp before trapping signals");
   }
}

/*****************************************************************************/

int
main(int argc, char *argv[])
{
   SHMAIN_HANDLE	handle;
   volatile int check_malloc = 0;
   volatile int free_threshold = 0;
   volatile int serial_threshold = 0;
   volatile int allow_break = 0;
   int i;

   if(allow_break) {			/* somewhere to set a breakpoint */
      printf("%d\n",allow_break);	/* NOTREACHED */
   }
   
   if(check_malloc) {
      shMemSerialCB(serial_threshold,p_malloc_check); /* call p_malloc_check if
							 we set the C global
							 g_Serial_threshold */
   } else {
      shMemSerialCB(serial_threshold,p_malloc_trace); /* call p_malloc_trace if
							 we set the C global
							 g_Serial_threshold */
   }
   shMemSerialFreeCB(free_threshold,p_free_trace); /* call p_free_trace() if we
						      set the C global
						      g_Serial_free_threshold*/
   /*
    ** INITIALIZE TO BEGIN ACCEPTING TCL COMMANDS FROM TERMINAL AND FSAOIMAGE 
    */
   if (shMainTcl_Init(&handle, &argc, argv, L_PHPROMPT) != SH_SUCCESS)
     return 1;
   
   /*
    ** DECLARE TCL COMMANDS
    */
   ftclCreateCommands(handle.interp);   /* Fermi enhancements to TCL   */
   ftclCreateHelp (handle.interp);	/* Define help command         */
   ftclMsgDefine (handle.interp);	/* Define help for tcl verbs   */

   /*
    * Now load definitions that should be in Dervish -- they should be in
    * the next release
    */
   phTclDervishDeclare(handle.interp);
   ftclParseInit(handle.interp);	/* Ftcl command parsing */

   /* 
     NOTE: shMainTcl_Declare is a comprehensive declaration routine for
     Dervish.  Included are TCL command declarations for region, peek, and
     FSAOimage.  If all commands are not desired, one can customize by
     replacing the call to this routine with individual calls to only the
     desired TCL command declaration routines. It runs the initialisation
     scripts, so must go after all of the initialisation routines have
     been called
     */
   shMainTcl_Declare(&handle);	/* Required Dervish TCL commands */ 
   atVerbsDeclare(handle.interp);	/* must come after loading Dervish */

   /* This should be called after shMainTcl_Declare */
   phTclVerbsDeclare(handle.interp);

   initSpanObjmask();			/* initialise SPANMASKs etc. */
   phMeschachLoad();			/* force loader to load meschach.o */

   /* 
    * here, we call PHOTO-specific initialization, using a TCL scripts
    * defined by environment variables
    */
   if(source(handle.interp, "PHOTO_STARTUP") < 0) {
      fprintf(stderr,
	      "Warning: $PHOTO_STARTUP was not set, and thus not executed.\n");
      fprintf(stderr,
	      "         Be prepared to see different behaviors.\n");
      fflush(stderr);
   }
   (void)source(handle.interp, "PHOTO_USER");
/*
 * handle longjmps from signal handlers
 */
   if((i = setjmp(jmp_env)) == 0) {	/* initial call */
      initialised_setjmp = 1;
   } else {				/* OK, a longjmp return */
      memset(&handle.lineHandle, '\0', sizeof(handle.lineHandle));
      sprintf(handle.TclBuf.string,"error \"trapped a %s\"",Tcl_SignalId(i));
      ungetc('\n', stdin);
   }
   /*
    *             DO NOT PUT ANYTHING BELOW THIS LINE.
    *
    * We will now enter the main loop of Tk. There is no successfull return
    * from this loop. Anything added below this line will never be reached,
    * NEVER!!
    */
   shTk_MainLoop(&handle);

   return 0;
}

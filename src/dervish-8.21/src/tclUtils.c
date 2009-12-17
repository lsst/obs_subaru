/*
 * TCL support for dervish utility functions
 *
 * ENTRY POINT		SCOPE
 * tclUtilsDeclare	public	declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL	 	TCL + Fermi extension
 *
 * Robert Lupton
 * Eileen Berman
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>			/* For timerXXX commands: gettimeofday*/
#include <sys/types.h>			/* For timerXXX commands: times       */
#include <sys/times.h>			/* For timerXXX commands: times       */
#include <sys/param.h>			/* For timerXXX commands: times       */
#include <alloca.h>

#if defined(__osf__) || defined(DARWIN)
#define HZ CLK_TCK
#endif

#include "tcl.h"
#include "shCAssert.h"
#include "shCUtils.h"
#include "shCGarbage.h"
#include "shCDiskio.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shTclHandle.h"
#include "shTclUtils.h"		/* needed for shTclDeclare prototype */
#include "shCUtils.h"		/* needed for SH_BIG_ENDIAN */
#include "shCSchema.h"                     /* prototype of shTypeGetFromName etc */
#include "shTclVerbs.h"
#include "shTclParseArgv.h"

#define BSIZE 1000

char *g_word = NULL;
int  g_groupLen = 0;

static char *module = "shUtils";	/* name of this set of code */

/************** nameGetFromType *****************************/
/* return an ascii string value based on the passed in type.  NOTE:
   the structure associated with the type must have a defined Dervish
   schema. */

static char* shTclNameGetFromType_use =
         "Usage: nameGetFromType <integer type> ";
static char* shTclNameGetFromType_help= 
         "Return the ascii name associated with an integer type.";
static char* shTclNameGetFromType_cmd = "nameGetFromType";

static int shTclNameGetFromType (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,       /*  TCL Interpreter structure */
   int          a_argc,          /*  TCL argument count */
   char         **a_argv         /*  TCL argument */
   )   
 
{
int  rstatus = TCL_OK;
int  len, i;
TYPE intType;
char *asciiName;

/* Verify the passed parameters */
if (a_argc != 2)
   {
   /* Wrong number of parameters given */
   rstatus = TCL_ERROR;
   Tcl_SetResult(a_interp, "Invalid number of parameters!\n", TCL_VOLATILE);
   Tcl_AppendResult(a_interp, shTclNameGetFromType_use, (char *)NULL);
   }
else
   {
   /* Assume the second parameter is the integer type - be sure it's a number */
   len = strlen(a_argv[1]);
   for (i = 0 ; i < len ; ++i)
      {
      if (! isdigit(a_argv[1][i]))
         {
	 /* One of the entered characters is not a numeric digit. */
	 rstatus = TCL_ERROR;
	 Tcl_SetResult(a_interp, "The type parameter must be numeric.\n",
		       TCL_VOLATILE);
	 break;
	 }
      }
   
   if (rstatus == TCL_OK)
      {
      intType = atoi(a_argv[1]);               /* Convert to an integer */
      asciiName = shNameGetFromType(intType);

      /* Return the ascii name to the user */
      Tcl_SetResult(a_interp, asciiName, TCL_VOLATILE);
      }
   }
return(rstatus);
}

/************** typeGetFromName *****************************/
/* return a integer type value based on the passed in name.  NOTE:
   the structure associated with the name must have a defined Dervish
   schema. */

static char* shTclTypeGetFromName_use = "Usage: typeGetFromName <name> ";
static char* shTclTypeGetFromName_help= 
         "Return the integer value associated with a named type (ex: REGION)";
static char* shTclTypeGetFromName_cmd = "typeGetFromName";

static int shTclTypeGetFromName (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,       /*  TCL Interpreter structure */
   int          a_argc,          /*  TCL argument count */
   char         **a_argv         /*  TCL argument */
   )   
 
{
int  rstatus;
TYPE intType;
char asciiType[10]; /* 10 sounds long enough to handle all size of int 
		       expressed as a string */

/* Verify the passed parameters */
if (a_argc != 2)
   {
   /* Wrong number of parameters given */
   rstatus = TCL_ERROR;
   Tcl_SetResult(a_interp, "Invalid number of parameters!\n", TCL_VOLATILE);
   Tcl_AppendResult(a_interp, shTclTypeGetFromName_use, (char *)NULL);
   }
else
   {
   /* Convert the ascii name to an integer type. */
   intType = shTypeGetFromName(a_argv[1]);

   /* Return the type value to the user */
   sprintf(asciiType, "%d", intType);
   Tcl_SetResult(a_interp, asciiType, TCL_VOLATILE);
   rstatus = TCL_OK;
   }

return(rstatus);
}

/*****************************************************************************/

static char *tclVerboseSet_use =
  "USAGE: verboseSet level";
static char *tclVerboseSet_hlp =
  "set the verbosity level (used by shDebug) to <level>";

static int
tclVerboseSet(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   char buff[BSIZE];
   
   if (argc != 2) {
      Tcl_SetResult(interp,tclVerboseSet_use,TCL_STATIC);
      return(TCL_ERROR);
   }
   sprintf(buff,"%d",shVerboseSet(atoi(argv[1])));
   Tcl_SetResult(interp,buff,TCL_VOLATILE);

   return(TCL_OK);
}
/****************************************************************************/
/*
 * Now some verbs to make/delete THINGSs
 *
 * return a handle to a new thing
 */
#define tclThingNew_hlp \
  "create a new thing of the specified type pointing to address addr (default: NULL)\n"

static ftclArgvInfo thingNewArgTable[] = {
    { NULL, FTCL_ARGV_HELP, NULL, NULL, tclThingNew_hlp },
    {"<type>",     FTCL_ARGV_STRING,  NULL, NULL, "Type of thing to create"},
    {"<addr>",     FTCL_ARGV_INT,  NULL, NULL, "Point the thing to this address"},
    {NULL,     FTCL_ARGV_END,  NULL, NULL, NULL}
   };

char g_thingNew[] = "thingNew";
    
static int
tclThingNew(
	   ClientData clientData,
	   Tcl_Interp *interp,
	   int argc,
	   char **argv
	   )
{
   int rstatus;
   int addr;				/* desired address */
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   TYPE type;				/* desired type */
   char* type_s = NULL;
   
   thingNewArgTable[1].dst = &type_s;
   thingNewArgTable[2].dst = &addr;

   if((rstatus = shTclParseArgv(interp,&argc,argv,thingNewArgTable,
				FTCL_ARGV_NO_LEFTOVERS, g_thingNew))
      == FTCL_ARGV_SUCCESS) {
      
      type = (TYPE)(isdigit(*type_s) ?
		    (TYPE)atoi(type_s) : shTypeGetFromName(type_s));
      
   } else {
      return(rstatus);
   }
/*
 * ok, get a handle for our new thing
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   handle.ptr = shThingNew((void *)addr,type);
   handle.type = shTypeGetFromName("THING");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new thing handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a thing
 */
static char *tclThingDel_use =
  "USAGE: thingDel thing";
static char *tclThingDel_hlp =
  "Delete a thing; it must contain a NULL pointer";

static int
tclThingDel(
	   ClientData clientData,
	   Tcl_Interp *interp,
	   int argc,
	   char **argv
	   )
{
   HANDLE realHandle;
   HANDLE *handle = &realHandle;
   void   *userPtr;
   char *thing;
  
   
   if(argc == 2) {
      thing = argv[1];
      if(shTclHandleExprEval(interp,thing,handle,&userPtr) != TCL_OK) {
	 return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclThingDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(handle->type != shTypeGetFromName("THING")) {
      Tcl_AppendResult(interp,"handle ",thing," is a ",
		       shNameGetFromType(handle->type)," not a THING",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   
   if(((THING *)handle->ptr)->ptr != NULL) {
      Tcl_SetResult(interp,"Thing doesn't contain a NULL pointer",TCL_STATIC);
      return(TCL_ERROR);
   }

   shThingDel((THING *)handle->ptr);
   if(p_shTclHandleDel(interp,thing) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/****************************************************************************/
/*
 *  sizeof: return the size of a given schema
 */

static char* tclShSizeof_use = "USAGE: sizeof SCHEMA";
static char* tclShSizeof_hlp = " return the size of a given schema";

static int
tclShSizeof(
	   ClientData clientData,
	   Tcl_Interp *interp,
	   int argc,
	   char **argv
	   )
{

    char result[10];
    SCHEMA* schema;

    
    if(argc != 2) 
    {
      Tcl_SetResult(interp,tclShSizeof_use,TCL_STATIC);
      return(TCL_ERROR);
    }
   schema = (SCHEMA*) shSchemaGet(argv[1]);
   if(schema == NULL)
   {
      Tcl_SetResult(interp, "Unknown schema", TCL_STATIC);
      return TCL_ERROR;
   }
   sprintf(result, "%d", schema->size);
   Tcl_SetResult(interp, result, TCL_VOLATILE);
   return TCL_OK;
}


/*****************************************************************************/
/*
 * shMalloc some space
 */
#define tclShMalloc_hlp "shMalloc some space, and return it in a handle\n"

static ftclArgvInfo shMallocArgTable[] = {
    { NULL, FTCL_ARGV_HELP, NULL, NULL, tclShMalloc_hlp },
    {"<len>", FTCL_ARGV_INT, NULL, NULL, "Length of space (in bytes) to shMalloc"},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };

char g_shMalloc[] = "shMalloc";

static int
tclShMalloc(
	   ClientData clientData,
	   Tcl_Interp *interp,
	   int argc,
	   char **argv
	   )
{
   int rstatus;
   int  nbyte;				/* desired address */
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   char *opts = "nbyte";
   
   shMallocArgTable[1].dst = &nbyte;

   if((rstatus = shTclParseArgv(interp, &argc, argv,shMallocArgTable,
				FTCL_ARGV_NO_LEFTOVERS, g_shMalloc))!=
      FTCL_ARGV_SUCCESS)
    {
      return(rstatus);
   }
/*
 * ok, get some space and get a handle to it
 */
   handle.ptr = shMalloc(nbyte);
   memset(handle.ptr,'\0',nbyte);
   handle.type = shTypeGetFromName("PTR");

   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new thing handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * shCalloc some space
 */
#define tclShCalloc_hlp "shCalloc some space, and return it in a handle\n"

static ftclArgvInfo shCallocArgTable[] = {
    { NULL, FTCL_ARGV_HELP, NULL, NULL, tclShCalloc_hlp },
    {"<num_elem>", FTCL_ARGV_INT, NULL, NULL, "Number of elements to shCalloc"},
    {"<elem_size>", FTCL_ARGV_INT, NULL, NULL, "Size of each elements (in bytes) to shCalloc"},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };

char g_shCalloc[] = "shCalloc";

static int
tclShCalloc(
	   ClientData clientData,
	   Tcl_Interp *interp,
	   int argc,
	   char **argv
	   )
{
   int rstatus;
   int num_elem,elem_size;	/* desired array size */
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   
   shCallocArgTable[1].dst = &num_elem;
   shCallocArgTable[2].dst = &elem_size;

   if((rstatus = shTclParseArgv(interp, &argc, argv,shCallocArgTable,
				FTCL_ARGV_NO_LEFTOVERS, g_shCalloc))!=
      FTCL_ARGV_SUCCESS)
    {
      return(rstatus);
   }
/*
 * ok, get some space and get a handle to it
 */
   handle.ptr = shCalloc(num_elem,elem_size);
   handle.type = shTypeGetFromName("PTR");

   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   
   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new thing handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * free some shMalloced space
 */
static char *tclShFree_use =
  "USAGE: shFree handle";
static char *tclShFree_hlp =
  "free some shMalloced space, associated with the handle";

static int
tclShFree(
	   ClientData clientData,
	   Tcl_Interp *interp,
	   int argc,
	   char **argv
	   )
{
   HANDLE realHandle;
   HANDLE *handle = &realHandle;
   void   *userPtr;
   char *addr;
   
   
   if(argc==2) {
      addr = argv[1];
      if(shTclHandleExprEval(interp,addr,handle,&userPtr) != TCL_OK) {
	 return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclShFree_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(handle->type != shTypeGetFromName("PTR")) {
      Tcl_AppendResult(interp,"handle ",addr," is a ",
		       shNameGetFromType(handle->type)," not a PTR",
		       (char *)NULL);
      return(TCL_ERROR);
   }
   
   shFree(handle->ptr);
   if(p_shTclHandleDel(interp,addr) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/******************************************************************************/
/*
 * Timer globals.
 */

/*
 * The following definition must be defined for ObjectCenter v1_1_0 compiler.
 * It does not appear to be in the new version either although we should 
 * revisit this when it becomes necessary.  As it is the value is defined
 * the same on the sun and sgi. EFB
 */
#ifdef __cplusplus
extern "C"
{
typedef long clock_t;
}
#endif  /* ifdef cpluplus */


struct tclTimerPoint
   {
   time_t	         walltimeval;		/* time  ()                   */
   struct tms		 cpu;			/* times ()                   */
   clock_t		 elapsed;   		/* times ()          [unused] */
   };

static struct
   {
   struct tclTimerPoint	 start;			/* Starting time information  */
   struct tclTimerPoint	 lap;			/* Lap      time information  */
   }		 tclTimerData;

/******************************************************************************/

static char	*tclTimerStart_use = "USAGE: timerStart";
static char	*tclTimerStart_hlp = "Reset and start elapsed and CPU timer";

static int	 tclTimerStart
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv []		/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 tclTimerStart
 *
 * DESCRIPTION:
 *
 *	Tcl command to reset the timer start times.
 *
 *	NOTE:	This command is akin to the reset and start buttons on a stop-
 *		watch.
 *
 * RETURN VALUES:
 *
 *   TCL_OK
 *
 *   TCL_ERROR
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:	None
 *--
 ******************************************************************************/

   {	/* tclTimerStart */

   /*
    * Declarations.
    */

                  int	lcl_status = TCL_OK;

   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Parse out the arguments.
    */

   if (argc != 1)
      {
      if (argc != 1)
         {
         lcl_status = TCL_ERROR;
         Tcl_SetResult (interp, "Syntax error (too many arguments)\n", TCL_VOLATILE);
         }
      Tcl_AppendResult (interp, tclTimerStart_use, (char *)(NULL));
      }
   else
     {
   /*
    * Perform the work.
    *
    *   o   Current CPU times are gotten last, since as little of this routine's
    *       processing should affect those times.
    */
   /* 
    * The result of times used to be case to an (int) before being assigned
    * to tclTimerData.start.elapsed.  Since both the return value of times
    * and elapsed are clock_t, this does not seem necessary (and is even
    * potentially harmfull in an environnement where clock_t is a long and
    * the long is longer than the int !.
    * So it has been removed!! (PGC)
    */

       time(&tclTimerData.start.walltimeval); 
       tclTimerData.start.elapsed = times (&tclTimerData.start.cpu); 
     };
   /*
    * All done.
    */

   return (lcl_status);

   }	/* tclTimerStart */

/******************************************************************************/

static char	*tclTimerLap_use   = "USAGE: timerLap";
static char	*tclTimerLap_hlp   = "Return elapsed and CPU times (seconds) since timerStart";

static int	 tclTimerLap
   (
   ClientData	 clientData,		/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,		/* INOUT: Tcl interpreter structure   */
   int		 argc,			/* IN:    Tcl argument count          */
   char		*argv []		/* IN:    Tcl arguments               */
   )

/*++
 * ROUTINE:	 tclTimerLap
 *
 * DESCRIPTION:
 *
 *	Tcl command to return timer information since the last tclTimerStart.
 *
 *	NOTE:	This command is akin to the lap button on a stopwatch.
 *
 * RETURN VALUES:
 *
 *   TCL_OK
 *
 *   TCL_ERROR
 *
 * SIDE EFFECTS:	None
 *
 * SYSTEM CONCERNS:
 *
 *   o	In practice, the resolution of system clocks (under UNIX) is usually 100
 *	Hz.  Thus, the division of CPU time between user and system modes should
 *	be taken with the understanding that they can be quite skewed.  Consider
 *	the following two cases:
 *
 *	---|----------|----------|----------|---   clock ticks
 *	    +-------+---+      +--+--------+
 *	    |///////|\\\|      |//|\\\\\\\\|
 *	    +-------+---+      +--+--------+
 *
 *	      +---+                          +---+
 *	where |///| represents user mode and |\\\| represents system mode.
 *	      +---+                          +---+
 *
 *	When the overall CPU time is large compared to the clock granularity,
 *	the user and system mode times can still be considerably skewed.  This
 *	can occur because the user and system times may be broken up into small
 *	chunks, on the order of the course clock ticks.  As the drawing above
 *	shows, it is possible to have the time from the last clock tick counted
 *	towards the smaller user of that clock tick duration. These inaccuracies
 *	can extend over more than one clock tick.  Unfortunately, they are also
 *	additive (and one cannot rely on them "balancing" themselves out over
 *	the long term).
 *
 *   o	An invalid elapsed wall time is possible if the system has its time
 *	(not time zone or Daylight Savings Time state) changed between the
 *	start (tclTimerStart) and lap (tclTimerLap) times.
 *--
 ******************************************************************************/

   {	/* tclTimerLap */

   /*
    * Declarations.
    */

                  int	 lcl_status = TCL_OK;
   unsigned long  int	 lcl_utime;
   unsigned long  int	 lcl_stime;
   unsigned long  int	 lcl_cutime;
   unsigned long  int	 lcl_cstime;
   unsigned long  int	 lcl_wallSec;

   /*
    *   o   When detecting integer overflow, the size of the datum can be up to
    *       a longword (4 bytes).
    */

   unsigned long  int	 lcl_ovfMax;

   unsigned long  int	 lcl_ovfMaxs [9];

   /*
    *   o   The result is an Extended Tcl keyed list.
    */

       char  lcl_result [132 + 1];		/* Must hold formatted string */
						/* from lcl_resultFmt + NULL. */
static char *lcl_resultFmt = "{ELAPSED %.03f} {CPU {{OVERALL %.03f} {UTIME %.03f} {STIME %.03f} {CUTIME %.03f} {CSTIME %.03f}}}";

   /*
    * Have to init this in the code as opposed to automatically in the 
    * definition because of the lousy ObjectCenter v1_1_0 EFB
    * This array used to be containing only:
    *   lcl_ovfMaxs[0] = 0;
    *   lcl_ovfMaxs[1] = 0x000000FF;
    *   lcl_ovfMaxs[2] = 0x0000FFFF;
    *   lcl_ovfMaxs[3] = 0xFFFFFFFF;     inexact ??
    * 
    */
    lcl_ovfMaxs[0] = 0;
    lcl_ovfMaxs[1] = 0x000000FF;
    lcl_ovfMaxs[2] = 0x0000FFFF;
    lcl_ovfMaxs[3] = 0x00FFFFFF;
    lcl_ovfMaxs[4] = 0xFFFFFFFF;
#if (LONG_MAX > 2147483647)
    lcl_ovfMaxs[5] = 0x000000FFFFFFFFFF;
    lcl_ovfMaxs[6] = 0x0000FFFFFFFFFFFF;
    lcl_ovfMaxs[7] = 0x00FFFFFFFFFFFFFF;
    lcl_ovfMaxs[8] = 0xFFFFFFFFFFFFFFFF;
#else
    lcl_ovfMaxs[5] = 0;
    lcl_ovfMaxs[6] = 0;
    lcl_ovfMaxs[7] = 0;
    lcl_ovfMaxs[8] = 0;
#endif
   /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*
    * Current times are gotten prior to parsing to prevent parsing times from
    * being counted.
    *
    *   o   Current CPU times are gotten first, since as little of this rou-
    *       tine's processing should affect those times.
    */
   /* 
    * The result of times used to be case to an (int) before being assigned
    * to tclTimerData.lap.elapsed.  Since both the return value of times
    * and elapsed are clock_t, this does not seem necessary (and is even
    * potentially harmfull in an environnement where clock_t is a long and
    * the long is longer than the int !.
    * So it has been removed!! (PGC)
    */

   tclTimerData.lap.elapsed   = times (&tclTimerData.lap.cpu);
   time (&tclTimerData.lap.walltimeval);

   /*
    * Parse out the arguments.
    */

   if (argc != 1)
      {
      if (argc != 1)
         {
         lcl_status = TCL_ERROR;
         Tcl_SetResult (interp, "Syntax error (too many arguments)\n", TCL_VOLATILE);
         }
      Tcl_AppendResult (interp, tclTimerLap_use, (char *)(NULL));
      }
   else
     {
   /*
    * Perform the work.
    *
    *   o   There is some protection built against CPU and seconds counter over-
    *       flows. If the ending value is less than the starting value, it's
    *       assumed that the counter overflowed only ONCE.  This should be
    *       more than adequate for for most applications.
    *
    *       NOTE:  The overflow protection code can only handle values that are
    *              contained up to one longword (4 bytes or 8 bytes).
    *
    *   o   For the overall CPU time, each member CPU time is first cast to a
    *       floating point value prior to addition to prevent integer overflow.
    *       This is a tad slower, but safer.
    *
    *       NOTE:  No protection is offered against changes to the clock which
    *              set the "absolute" (Greenwich) lap time back.
    */

       lcl_ovfMax  =             (sizeof (tclTimerData.lap.cpu.tms_utime)  <= sizeof(lcl_utime) )
                   ? lcl_ovfMaxs [sizeof (tclTimerData.lap.cpu.tms_utime)] : 0;

       lcl_utime   = (tclTimerData.lap.cpu.tms_utime  >=               tclTimerData.start.cpu.tms_utime)
                   ? (tclTimerData.lap.cpu.tms_utime   -               tclTimerData.start.cpu.tms_utime)
                   : (tclTimerData.lap.cpu.tms_utime   + (lcl_ovfMax - tclTimerData.start.cpu.tms_utime)  + 1);
       lcl_stime   = (tclTimerData.lap.cpu.tms_stime  >=               tclTimerData.start.cpu.tms_stime)
                   ? (tclTimerData.lap.cpu.tms_stime   -               tclTimerData.start.cpu.tms_stime)
                   : (tclTimerData.lap.cpu.tms_stime   + (lcl_ovfMax - tclTimerData.start.cpu.tms_stime)  + 1);
       lcl_cutime  = (tclTimerData.lap.cpu.tms_cutime >=               tclTimerData.start.cpu.tms_cutime)
                   ? (tclTimerData.lap.cpu.tms_cutime  -               tclTimerData.start.cpu.tms_cutime)
                   : (tclTimerData.lap.cpu.tms_cutime  + (lcl_ovfMax - tclTimerData.start.cpu.tms_cutime) + 1);
       lcl_cstime  = (tclTimerData.lap.cpu.tms_cstime >=               tclTimerData.start.cpu.tms_cstime)
                   ? (tclTimerData.lap.cpu.tms_cstime  -               tclTimerData.start.cpu.tms_cstime)
		   : (tclTimerData.lap.cpu.tms_cstime  + (lcl_ovfMax - tclTimerData.start.cpu.tms_cstime) + 1);

       lcl_ovfMax  =             (sizeof (tclTimerData.lap.walltimeval)  <= sizeof(lcl_wallSec) )
                   ? lcl_ovfMaxs [sizeof (tclTimerData.lap.walltimeval)] : 0;

       lcl_wallSec = (tclTimerData.lap.walltimeval    >=               tclTimerData.start.walltimeval)
                   ? (tclTimerData.lap.walltimeval     -               tclTimerData.start.walltimeval)
                   : (tclTimerData.lap.walltimeval     + (lcl_ovfMax - tclTimerData.start.walltimeval) + 1);

       sprintf (lcl_result, lcl_resultFmt,
		(double)(lcl_wallSec),
		((double)(lcl_utime) + (double)(lcl_stime) + (double)(lcl_cutime) + (double)(lcl_cstime)) / (double)(HZ),
		(double)(lcl_utime)  / (double)(HZ),
		(double)(lcl_stime)  / (double)(HZ),
		(double)(lcl_cutime) / (double)(HZ),
		(double)(lcl_cstime) / (double)(HZ)
		);

       Tcl_SetResult (interp, lcl_result, TCL_VOLATILE);

       /*
	* All done.
	*/
     };

   return (lcl_status);

   }	/* tclTimerLap */

/******************************************************************************/

/*============================================================================
**============================================================================
**
** ROUTINE: shStrtok
**
** DESCRIPTION:
**	Given a word, separate the word into groups of characters. Groups
**      consist of all \n characters or all characters that are not \n.  So
**      for example the word :  aaa\nbbb\nccc\n\n, would consist of 6
**      different groups, aaa, \n, bbb, \n, ccc, and \n\n.  Return a pointer
**      to the beginning of the group and the number of characters in the
**      group.  If the routine is called with the first parameter = NULL,
**      continue searching from where we stopped during the last call.
**
** RETURN VALUES:
**      pointer to next group - success
**      NULL  - end of word.
**
** CALLS TO:
**
** GLOBALS REFERENCED:
**      g_word, g_groupLen
**
**============================================================================
*/
char *shStrtok
   (
   char *word,
   int  *groupLen
   )
{
char *group;

if (word == NULL)
   {
   /* continue searching through the word we have saved */
   g_word += g_groupLen;
   }
else
   {
   /*  This is the first time through here, save word so we can get at it
       for subsequent calls */
   g_word = word;
   }

/* Advance through the word until the end of the current group */
group = g_word;
*groupLen = 0;
while (group[0] != '\0')
   {
   ++*groupLen;

   /* Only continue on if the first and second characters are either both
      \n's or neither are */
   if (((group[0] == '\n') && (group[1] == '\n')) ||
       ((group[0] != '\n') && (group[1] != '\n')))
      {
      /* Point to next character */
      ++group;
      }
   else
      {
      /* We found the end of a group */
      break;
      }
   }

g_groupLen = *groupLen;            /* Save for next pass thru */
return(g_word);
}

/*============================================================================
**============================================================================
**
** ROUTINE: shTclDeclare
**
** DESCRIPTION:
**	It creates a tcl command and declares the help message and facility
**	to ftclHelp.
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	Tcl_CreateCommand
**	ftclHelpDefine
**	alloca
**	sprintf
**
** GLOBALS REFERENCED:
**
** NOTA BENE
**      No error checking is done on the parameters.
**      In particuler a_helpText and a_CmdPrototype should not
**      be null. 
**
**============================================================================
*/
int shTclDeclare
   (
   Tcl_Interp        *a_interp,
   char              *a_cmdName,
   Tcl_CmdProc       *a_cmdProc,
   ClientData        a_data,
   Tcl_CmdDeleteProc *a_deleteProc,
   char              *a_helpFacility,
   char              *a_helpText,
   char              *a_cmdPrototype
   )   
{
#define MAXLINE  79
#define INDENTSIZE 22

   char *lcl_helpPtr, *helpTextPtr, *lcl_PrototypePtr;
   int lineTotalSoFar = 0, i;

/* The help text will be formatted to look like the following -
** 
**	Usage text
**
**	                    All lines of help text which all
**	                    are indented the same.
**
** NOTE: the name of the tcl extension is add by the call to
**       ftclHelpDefine.
*/

/* Add on enough to cover the fact that the help text is indented */
   char *lcl_help  = (char *)(alloca(strlen(a_helpText) + 
	                             strlen(a_cmdPrototype) + 4000));

/* We will keep local pointers that run through the two help strings
** so we do not have to keep counting from the beginning. 
*/
   helpTextPtr = a_helpText;
   lcl_helpPtr = lcl_help;

/* Copy the usage string to the local buffer */
   for ( lcl_PrototypePtr = a_cmdPrototype ; *lcl_PrototypePtr != '\0' ;
	 ++lcl_PrototypePtr)
      {
      *lcl_helpPtr++ = *lcl_PrototypePtr;
      }
   *lcl_helpPtr++ = '\n';

/* Now copy the help string remembering to start each line with the
** correct amount of indentation
*/
   for ( lineTotalSoFar = 0 ; *helpTextPtr != '\0' ; )
     {
     if (lineTotalSoFar == 0)
       {
       /* Add the help text indentation at the beginning of each line */
       for (i = 0 ; i < INDENTSIZE ; ++i)
	 {
	 *lcl_helpPtr++ = ' ';
	 ++lineTotalSoFar;
	 }
       }

     if ((*helpTextPtr != '\n') && (lineTotalSoFar < MAXLINE))
       {
       *lcl_helpPtr++ = *helpTextPtr++;
       ++lineTotalSoFar;
       }
     else if (*helpTextPtr == '\n')
       {
       *lcl_helpPtr++ = *helpTextPtr++;
       lineTotalSoFar = 0;       
       }
     else
       {
       /* Now we have reached the end of a line.  However we do not want
	  to break words in the middle so, back up to the beginning of
	  the current word. */
       if (*helpTextPtr != ' ')
	 {
	 for ( ; *helpTextPtr != ' ' ; --lcl_helpPtr)
	   {
	   --helpTextPtr;
	   }
	 }
       *lcl_helpPtr++ = '\n';
       ++helpTextPtr;            /* Skip over the space */
       lineTotalSoFar = 0;
       }
     }

/* Now we must remove the last bunch of indentation if that is what we ended
** with.
*/
   if (lineTotalSoFar == INDENTSIZE)
     {
     lcl_helpPtr = lcl_helpPtr - INDENTSIZE;
     }
   *lcl_helpPtr = '\0';
/*
**	define the help string for the command
*/
   ftclHelpDefine (a_interp, a_helpFacility, a_cmdName, lcl_help);
/*
**	create the tcl command
*/
   Tcl_CreateCommand(a_interp, 
                     a_cmdName, 
                     a_cmdProc, 
                     a_data, 
                     a_deleteProc);

   return (TCL_OK);
}

/* Print the endedness of the current machine. */

static char *tclEndian_hlp="Return either BIG_ENDIAN or LITTLE_ENDIAN";
static char *tclEndian_use="shEndian";

static int	 tclEndian
   (
   ClientData	 clientData,	/* IN:    Tcl parameter (not used)    */
   Tcl_Interp	*interp,	/* INOUT: Tcl interpreter structure   */
   int		 argc,		/* IN:    Tcl argument count          */
   char		*argv []	/* IN:    Tcl arguments               */
   )
{
  if (shEndedness() == SH_BIG_ENDIAN)
    {
      Tcl_SetResult(interp, "BIG_ENDIAN", TCL_VOLATILE);
    }
  else
    {
      Tcl_SetResult(interp, "LITTLE_ENDIAN", TCL_VOLATILE);
    }
  return(TCL_OK);
}

/*****************************************************************************/
/*
 * Return the local time as a string
 */
static int want_utc = 0;

static char *tclCtime_use =
  "USAGE: ctime [-utc]";
#define tclCtime_hlp \
  "Return the current time as a string (with the \\n stripped);\n"\
"usually return the current time, but with the -utc flag, return UTC"

static ftclArgvInfo ctime_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclCtime_hlp},
   {"-utc", FTCL_ARGV_CONSTANT, (void *)1, &want_utc, "return the UTC time"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclCtime(
	 ClientData clientDatag,
	 Tcl_Interp *interp,
	 int ac,
	 char **av
	 )
{
   char buff[26 + 4];			/* ANSI says 26 chars are needed;
					   we may append " UTC" */
   time_t t;
   
   shErrStackClear();

   want_utc = 0;

   if(shTclParseArgv(interp,&ac,av,ctime_opts,
		     FTCL_ARGV_NO_LEFTOVERS, "ctime") != FTCL_ARGV_SUCCESS) {
      return(TCL_ERROR);
   }
/*
 * work
 */
   (void)time(&t);
   if(want_utc) {
      struct tm *tmp = gmtime(&t);
      if(tmp == NULL) {
	 Tcl_SetResult(interp, "UTC is not available", TCL_VOLATILE);
	 return(TCL_ERROR);
      }
      strcpy(buff,asctime(tmp));
   } else {
      strcpy(buff,ctime(&t));
   }
   shAssert(buff[24] == '\n');
   buff[24] = '\0';			/* strip newline */
   if(want_utc) {
      strcat(buff," UTC");
   }

   Tcl_SetResult(interp, buff, TCL_VOLATILE);

   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
shTclUtilsDeclare(Tcl_Interp *interp) 
{
   int flags = FTCL_ARGV_NO_LEFTOVERS;

    shTclDeclare(interp, shTclTypeGetFromName_cmd, shTclTypeGetFromName,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               module,
               shTclTypeGetFromName_help,
               shTclTypeGetFromName_use);

    shTclDeclare(interp, shTclNameGetFromType_cmd, shTclNameGetFromType,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               module,
               shTclNameGetFromType_help,
               shTclNameGetFromType_use);

   shTclDeclare(interp,g_thingNew, 
		(Tcl_CmdProc *)tclThingNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module,
		shTclGetArgInfo(interp, thingNewArgTable, flags, g_thingNew),
		shTclGetUsage(interp, thingNewArgTable, flags, g_thingNew));

   shTclDeclare(interp,"thingDel", 
		(Tcl_CmdProc *)tclThingDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclThingDel_hlp, tclThingDel_use);

   shTclDeclare(interp,"shMalloc", 
		(Tcl_CmdProc *)tclShMalloc, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		"shMemory",
		shTclGetArgInfo(interp, shMallocArgTable, flags, g_shMalloc),
		shTclGetUsage(interp, shMallocArgTable, flags, g_shMalloc));

   shTclDeclare(interp,"shCalloc", 
		(Tcl_CmdProc *)tclShCalloc, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		"shMemory",
		shTclGetArgInfo(interp, shCallocArgTable, flags, g_shCalloc),
		shTclGetUsage(interp, shCallocArgTable, flags, g_shCalloc));

  shTclDeclare(interp,"sizeof", 
		(Tcl_CmdProc *)tclShSizeof, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclShSizeof_hlp, tclShSizeof_use);

   shTclDeclare(interp,"shFree", 
		(Tcl_CmdProc *)tclShFree, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		"shMemory", tclShFree_hlp, tclShFree_use);

   shTclDeclare(interp,"verboseSet", 
		(Tcl_CmdProc *)tclVerboseSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclVerboseSet_hlp, tclVerboseSet_use);

   shTclDeclare(interp,"timerStart",
		(Tcl_CmdProc *)tclTimerStart,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclTimerStart_hlp, tclTimerStart_use);

   shTclDeclare(interp,"timerLap",
		(Tcl_CmdProc *)tclTimerLap,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclTimerLap_hlp, tclTimerLap_use);

   shTclDeclare(interp,"shEndian",
		(Tcl_CmdProc *)tclEndian,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL,
		module, tclEndian_hlp, tclEndian_use);

   shTclDeclare(interp,"ctime",
                (Tcl_CmdProc *)tclCtime,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclCtime_hlp,
                tclCtime_use);
} 

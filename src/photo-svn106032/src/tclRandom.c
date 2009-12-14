/*
 * Export the random number generator Set/Unset functions to TCL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>			/* for getpid() */
#include "dervish.h"
#include "phUtils.h"
#include "phRandom.h"

static char *module = "phTclRandom";    /* name of this set of code */

/*****************************************************************************/
/*
 * Setup the random number generator
 */
static char *tclRandomNew_use =
  "USAGE: phRandomNew file [-seed ###] [-type 1|2]";
#define tclRandomNew_hlp \
  "Set up the random number generator, by reading random numbers "\
"from a file and returning a RANDOM. If the <name> begins with a digit, "\
"<file> will be treated as the number of random numbers desired, "\
"and they will be calculated rather than read; in this case the -type "\
"flag may be used to specify the random number generator; 1 (the default) "\
"uses a single linear-congruent generator, while 2 uses a pair of such "\
"generators with non-commensurate periods\n" \
"The random numbers will be seeded with <seed> if supplied, otherwise we "\
"will choose one for you"

static ftclArgvInfo randomNew_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRandomNew_hlp},
   {"<file>", FTCL_ARGV_STRING, NULL, NULL, "file name"},
   {"-seed", FTCL_ARGV_INT, NULL, NULL, "seed for random numbers"},
   {"-type", FTCL_ARGV_INT, NULL, NULL, "type of generator"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRandomNew(
	      ClientData clientData,
	      Tcl_Interp *interp,
	      int argc,
	      char **argv
	      )
{
   int i;
   char buff[40];
   HANDLE hand;				/* handle to return */
   char name[HANDLE_NAMELEN];
   RANDOM *rand;			/* the created RANDOM */
   char *fileStr = NULL;		/* file name */
   int seed = 0;			/* seed for random numbers */
   int type = 0;			/* type of generator */
   
   shErrStackClear();

   seed = type = -1;
   i = 1;
   randomNew_opts[i++].dst = &fileStr;
   randomNew_opts[i++].dst = &seed;
   randomNew_opts[i++].dst = &type;
   shAssert(randomNew_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&argc,argv,randomNew_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process the arguments
 */
   if(seed == -1) {			/* need a seed */
      seed = abs(getpid() ^ time(NULL));
   }
   if(type != -1) {			/* specify a type of random generator*/
      if(isdigit(fileStr[0])) {		/* OK, we _are_ calculating them */
	 sprintf(buff,"%s:%d",fileStr,type);
	 fileStr = buff;
      } else {
	 Tcl_SetResult(interp,
		  "phRandomNew: you can't both choose a -type and read a file",
		    						   TCL_STATIC);
	 return(TCL_ERROR);
      }
   }
/*
 * Work
 */
   if((rand = phRandomNew(fileStr,seed)) == NULL) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * And return the answer
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      phRandomDel(rand);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   hand.ptr = rand;
   hand.type = shTypeGetFromName("RANDOM");
   
   if(p_shTclHandleAddrBind(interp, hand, name) != TCL_OK) {
      Tcl_SetResult(interp, "can't bind to new RANDOM handle", TCL_STATIC);
      phRandomDel(rand);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp, name, TCL_VOLATILE);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete the random number generator
 */
static char *tclRandomDel_use =
  "USAGE: phRandomDel handle";
#define tclRandomDel_hlp \
  "Un-setup the random number generator"

static ftclArgvInfo randomDel_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRandomDel_hlp},
   {"<random>", FTCL_ARGV_STRING, NULL, NULL, "handle to random numbers"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRandomDel(
	      ClientData clientData,
	      Tcl_Interp *interp,
	      int argc,
	      char **argv
	      )
{
   int i;
   HANDLE hand;
   void *vptr;				/* used by shTclHandleExprEval */
   char *randomStr = NULL;		/* handle to random numbers */
   
   shErrStackClear();

   i = 1;
   randomDel_opts[i++].dst = &randomStr;
   shAssert(randomDel_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&argc,argv,randomDel_opts) != TCL_OK) {

      return(TCL_ERROR);
   }
/*
 * Process the arguments
 */
   if(shTclHandleExprEval(interp,randomStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(hand.type != shTypeGetFromName("RANDOM")) {
      Tcl_SetResult(interp,"phRandomDel: argument is not a RANDOM",TCL_STATIC);
      return(TCL_ERROR);
   }

   phRandomDel(hand.ptr);
   p_shTclHandleDel(interp,randomStr);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Set the random number generator's seed
 */
static char *tclPhRandomSeedSet_use =
  "USAGE: PhRandomSeedSet <rand> <seed>";
#define tclPhRandomSeedSet_hlp \
  "Set the RANDOM <rand>'s <seed>, and return the old value"

static ftclArgvInfo phRandomSeedSet_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPhRandomSeedSet_hlp},
   {"<random>", FTCL_ARGV_STRING, NULL, NULL, "handle to random numbers"},
   {"<seed>", FTCL_ARGV_INT, NULL, NULL, "desired seed"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclPhRandomSeedSet(
		   ClientData clientDatag,
		   Tcl_Interp *interp,
		   int ac,
		   char **av
		   )
{
   int i;
   HANDLE hand;
   char buff[40];
   void *vptr;				/* used by shTclHandleExprEval */
   char *randomStr = NULL;		/* handle to random numbers */
   int seed = 0;			/* desired seed */

   shErrStackClear();

   i = 1;
   phRandomSeedSet_opts[i++].dst = &randomStr;
   phRandomSeedSet_opts[i++].dst = &seed;
   shAssert(phRandomSeedSet_opts[i].type == FTCL_ARGV_END);

   if(get_FtclOpts(interp,&ac,av,phRandomSeedSet_opts) != TCL_OK) {

      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,randomStr,&hand,&vptr) != TCL_OK) {
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("RANDOM")) {
      Tcl_SetResult(interp,"phRandomSeedSet: argument is not a RANDOM",
		    TCL_STATIC);
      return(TCL_ERROR);
   }

   sprintf(buff,"%ld",(long)phRandomSeedSet(hand.ptr,seed));
   Tcl_SetResult(interp,buff,TCL_VOLATILE);

   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Return a random number
 */
static char *tclRandom_use =
  "USAGE: random";
#define tclRandom_hlp \
  "Return a random 32-bit number. This is designed SOLELY for testing the" \
"C-callable random number code"

static ftclArgvInfo random_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRandom_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRandom(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   char buff[40];
   shErrStackClear();

   if(get_FtclOpts(interp,&argc,argv,random_opts) != TCL_OK) {
      return(TCL_ERROR);
   }

   if(!phRandomIsInitialised()) {
      Tcl_SetResult(interp,"phRandom: Random numbers are not initialised",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   
   sprintf(buff,"%d",phRandom());
   Tcl_SetResult(interp, buff, TCL_VOLATILE);
   
   return(TCL_OK);
}

/************************************************************************************************************/

static char *tclPhRandomUniform_use =
  "USAGE: phRandomUniform";
#define tclPhRandomUniform_hlp \
  "Return a uniform number on [0,1]. DO NOT USE IN PRODUCTION CODE"

static ftclArgvInfo phRandomUniform_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclPhRandomUniform_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define phRandomUniform_name "phRandomUniform"

static int
tclPhRandomUniform(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac,
		   char **av)
{
   char buff[40];
   int a_i;

   shErrStackClear();

   a_i = 1;
   shAssert(phRandomUniform_opts[a_i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, phRandomUniform_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     phRandomUniform_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   if(!phRandomIsInitialised()) {
      Tcl_SetResult(interp,"phRandom: Random numbers are not initialised",
		    TCL_STATIC);
      return(TCL_ERROR);
   }
   
   sprintf(buff, "%.20g", phRandomUniformdev());
   Tcl_SetResult(interp, buff, TCL_VOLATILE);
   
   return(TCL_OK);
}



/*****************************************************************************/
/*
 * Are phRandom etc. initialised?
 */
static char *tclRandomIsInitialised_use =
  "USAGE: random";
#define tclRandomIsInitialised_hlp \
  "Return a random 32-bit number. This is designed SOLELY for testing the" \
"C-callable random number code"

static ftclArgvInfo randomIsInit_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclRandomIsInitialised_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

static int
tclRandomIsInitialised(
	  ClientData clientData,
	  Tcl_Interp *interp,
	  int argc,
	  char **argv
	  )
{
   char buff[40];
   shErrStackClear();

   if(get_FtclOpts(interp,&argc,argv,randomIsInit_opts) != TCL_OK) {
      return(TCL_ERROR);
   }

   sprintf(buff,"%d",phRandomIsInitialised());

   Tcl_SetResult(interp, buff, TCL_VOLATILE);
   
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
phTclRandomDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"phRandomNew",
		(Tcl_CmdProc *)tclRandomNew, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRandomNew_hlp,
		tclRandomNew_use);

   shTclDeclare(interp,"phRandomDel",
		(Tcl_CmdProc *)tclRandomDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRandomDel_hlp,
		tclRandomDel_use);

   shTclDeclare(interp,"phRandom",
		(Tcl_CmdProc *)tclRandom, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRandom_hlp,
		tclRandom_use);

   shTclDeclare(interp,"phRandomIsInitialised",
		(Tcl_CmdProc *)tclRandomIsInitialised, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclRandomIsInitialised_hlp,
		tclRandomIsInitialised_use);

   shTclDeclare(interp,"phRandomSeedSet",
		(Tcl_CmdProc *)tclPhRandomSeedSet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPhRandomSeedSet_hlp,
		tclPhRandomSeedSet_use);

   shTclDeclare(interp,phRandomUniform_name,
		(Tcl_CmdProc *)tclPhRandomUniform, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclPhRandomUniform_hlp,
		tclPhRandomUniform_use);
}

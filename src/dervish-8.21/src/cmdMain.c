/*****************************************************************************
 ******************************************************************************
 **
 ** FILE:
 **	cmdMain.c
 **
 ** ABSTRACT:
 **	This file contains the high level routines that make up the Dervish
 **	main TCL program template (dervish_template.c).  It is the intent
 **	of these routines to encapsulate the lower layers of the Dervish TCL
 **	command line editing code, in order to minimize chances of the 
 **	template needing changes in the future.
 **
 **   ENTRY POINT	    SCOPE   DESCRIPTION
 **   -------------------------------------------------------------------------
 **   shMainTcl_Declare	    public  Declare all TCL commands for Dervish.
 **
 ** ENVIRONMENT:
 **	ANSI C.
 **
 ******************************************************************************
 ******************************************************************************
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include "ftcl.h"
#include "tcl.h"
#include "tclExtend.h"
#include "dervish_msg_c.h"    /* Murmur message file for Dervish */
#include "region.h"         /* For definition of PIXDATATYPE */
#include "shCSchema.h"         /* For prototype of shSchemaLoadFromDervish */
#include "shTclRegSupport.h"
#include "shTclHandle.h"
#include "shCSao.h"
#include "shCErrStack.h"
#include "shCAssert.h"
#include "shTclErrStack.h"
#include "shTclParseArgv.h"
#include "shTclUtils.h"
#include "shTclVerbs.h"

extern RET_CODE shTk_Init(CMD_HANDLE *a_shHandle);
int shTclParseArg(
    ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]);


/*============================================================================  


 **============================================================================
 **
 ** LOCAL TCL COMMANDS
 **
 **============================================================================
 */
static int
cmdEcho
   (
   ClientData clientData,
   Tcl_Interp *interp,
   int argc,
   char *argv[]
   )
{
   int i;
   
   for (i = 1; ; i++) {
      if (argv[i] == NULL) {
	 if (i != argc) {
	  echoError:
	    sprintf(interp->result,
		    "argument list wasn't properly NULL-terminated in \"%s\" command",
		    argv[0]);
	 }
	 break;
      }
      if (i >= argc) {
	 goto echoError;
      }
      fputs(argv[i], stdout);
      if (i < (argc-1)) {
	 printf(" ");
      }
   }
   printf("\n");
   return TCL_OK;
}

static int
cmdHelp(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{
   char  command[132];
   char  filename[80];
   FILE	*fp;
   if (argc == 1){
      sprintf(command,"www $DERVISH_HELP/www/dervish.home.html");
   }
   else{
      strcpy(filename,(char *)getenv("DERVISH_HELP"));
      strcpy(&filename[strlen(filename)],"/");
      strncpy(&filename[strlen(filename)],argv[1],60);
      strcpy(&filename[strlen(filename)],".html");
      if((fp=fopen(filename,"r")) != (FILE *) NULL){
	 fclose(fp);
      }
      else{
	 strcpy(filename,"$DERVISH_HELP/www/dervish.home.html");
      }
      sprintf(command,"www %s",filename);
   }
   system(command);
   
   return TCL_OK;
}

static char *tclDervishName_use =
  "USAGE: dervishName";
#define tclDervishName_hlp \
  "Return the CVS Name string, as compiled into the binary"

static ftclArgvInfo dervishName_opts[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, tclDervishName_hlp},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
};

#define dervishName_name "dervishName"

static int
tclDervishName(ClientData clientData,
	       Tcl_Interp *interp,
	       int ac,
	       char **av)
{
   int i;
   static char *name = "$Name: v8_21 $";

   shErrStackClear();

   i = 1;
   shAssert(dervishName_opts[i].type == FTCL_ARGV_END);

   if(shTclParseArgv(interp, &ac, av, dervishName_opts,
		     FTCL_ARGV_NO_LEFTOVERS,
		     dervishName_name) != FTCL_ARGV_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * work
 */
   Tcl_SetResult(interp, name, TCL_STATIC);

   return(TCL_OK);
}


/*============================================================================  
 **============================================================================
 **
 ** ROUTINE: shMainTcl_Declare 
 */
RET_CODE shMainTcl_Declare
  (
   CMD_HANDLE    *a_shHandle	    /* MOD: Dervish main handle structure */
   )   
/*
 ** DESCRIPTION:
 **	Declare all necessary TCL commands needed for the Dervish product.
 **
 ** RETURN VALUES:
 **	SH_SUCCESS	-   Successful completion (NOTE: This routine has no
 **			    failure mode at this time).
 **
 **============================================================================
 */
{
   
   Tcl_Interp  *l_interp;
   
   ftclParseInit(0);

   /*
    * Invoke initialization routines that only belong here because of Dervish's
    * progression of initialization of packages.  These initializations are
    * here since modifications to dervish_template.c are avoided, as that would
    * force users to update their code whenever a new package was introduced
    * or some change was made.  Though the following initializations have
    * nothing to do with Dervish's Tcl interface, shMainTcl_Declare () is the
    * first place in dervish_template.c's main () that makes sense to place
    * miscellaneous initializations.
    */
  
   (void)shSchemaLoadFromDervish();	/* load the DERVISH schema definitions */

   p_shAlignConstruct ();

   /*
    * Initialize Tcl stuff.
    */

   l_interp = a_shHandle->interp;

     shTclRegSupportInit(l_interp);
     shTclHandleInit(l_interp);
     shTclMaskDeclare(l_interp);
     shTclRegionDeclare(l_interp);
     shTclRegUtilDeclare(l_interp);
     shTclHdrDeclare(l_interp);
     shTclDemoDeclare(l_interp);
     shTclDiskioDeclare(l_interp);
     shTclFitsIoDeclare(l_interp);
     shTclTblColSchemaConversionDeclare(l_interp);
     shTclSchemaTransDeclare(l_interp);
   p_shTclFitsDeclare(l_interp);
   p_shTclTblDeclare(l_interp);
     shTclHandleDeclare(l_interp);
     shTclErrStackDeclare(l_interp);
     shTclPgplotDeclare(l_interp);
     shTclUtilsDeclare(l_interp);
     shTclRegPrintDeclare(l_interp);
     shTclLutDeclare(l_interp);
     shTclHgDeclare(l_interp, a_shHandle);
   p_shTclAlignDeclare(l_interp);
   p_shTclArrayDeclare(l_interp);
     shTclContribDeclare(l_interp);
     shTclChainDeclare(l_interp);
     shTclVectorDeclare(l_interp);
     shTclMemDeclare(l_interp);
     shTclMaskUtilDeclare(l_interp);
     shTclVectorChainDeclare(l_interp);
     shTclSEvalDeclare(l_interp);
     shTclParamIncDeclare(l_interp);
     shTclSaoPanDeclare(l_interp);
     shTclDs9Declare(l_interp);
     
   tclSaoDeclare(l_interp);

   Tcl_CreateCommand(l_interp, "echo", cmdEcho, (ClientData) "echo",
		     (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(l_interp, "www", cmdHelp, (ClientData) 0,
		     (Tcl_CmdDeleteProc *) NULL);
   Tcl_CreateCommand(l_interp, "shTclParseArg", shTclParseArg, (ClientData) 0,
		     (Tcl_CmdDeleteProc *) NULL);
   shTclDeclare(l_interp,dervishName_name,
		(Tcl_CmdProc *)tclDervishName, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		"dervish", tclDervishName_hlp,
		tclDervishName_use);

   if (shTk_Init(a_shHandle) != SH_SUCCESS) {
       fprintf(stderr, "shTk_Init() failed\n");
       TclX_ErrorExit(a_shHandle->interp, 255);
   }

   return(SH_SUCCESS);
}

#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "phAnalysis.h"
#include "phObjc.h"

static char *module = "tclAnalysis";    /* name of this set of code */

static char *tclPhotReadCatalog_use =
  "USAGE: readCatalog catfile [offsetx offsety] ";
static char *tclPhotReadCatalog_hlp =
  "Read Catalog and returns a CHAIN of OBJC ";

static int
tclPhotReadCatalog(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   char *opts = "catfile -x=0.0 -y=0.0 ";

   char catfile[BUFSIZ];
   float offsetx,offsety;
   CHAIN *objlist;
   
   shErrStackClear();

   ftclParseSave("photReadCatalog");
   if(ftclFullParseArg(opts,argc,argv) != 0) 
     {
       strcpy(catfile,ftclGetStr("catfile"));
       offsetx = ftclGetDouble("x");
       offsety = ftclGetDouble("y");

     } 
   else 
     {
      ftclParseRestore("photReadCatalog");
       Tcl_SetResult(interp,tclPhotReadCatalog_use,TCL_STATIC);
       return(TCL_ERROR);
     }

/* Call ! */
 
   objlist=phReadCatalog(catfile,offsetx,offsety);

/* get a handle for our new List of OBJC */
 
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new list of objc handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   handle.ptr = objlist;
   handle.type = shTypeGetFromName("CHAIN");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new chain of objc handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}


static char *tclObjcListMatch_use =
  "USAGE:objcListMatch objclistA objclistB radius";
static char *tclObjcListMatch_hlp =
  "match 2 lists of objcs and returns 4 lists; matchA matchB unmatchA unmatchB ";

static int
tclObjcListMatch(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   CHAIN *listA = NULL,*listB = NULL,*listJ,*listK,*listL,*listM;

   char listAname[HANDLE_NAMELEN],listBname[HANDLE_NAMELEN];
   char listJname[HANDLE_NAMELEN],listKname[HANDLE_NAMELEN];
   char listLname[HANDLE_NAMELEN],listMname[HANDLE_NAMELEN];
   char *opts = "objclistA objclistB radius";
   float radius;
   char answer[BUFSIZ];

   shErrStackClear();

   ftclParseSave("photObjcListMatch");
   if(ftclFullParseArg(opts,argc,argv) != 0) 
     {
      strcpy(listAname,ftclGetStr("objclistA"));
      if (shTclAddrGetFromName(interp, listAname , (void **) &listA, "CHAIN") 
                 != TCL_OK) {
         Tcl_SetResult(interp, "can't get address for given OBJC list",
                 TCL_STATIC);
         return(TCL_ERROR);
      }
      if (shChainTypeGet(listA) != shTypeGetFromName("OBJC")) {
         Tcl_SetResult(interp, "listA is not a chain of OBJC",
                 TCL_STATIC);
         return(TCL_ERROR);
      }

      strcpy(listBname,ftclGetStr("objclistB"));
      if (shTclAddrGetFromName(interp, listBname , (void **) &listB, "CHAIN") 
                 != TCL_OK) {
         Tcl_SetResult(interp, "can't get address for given OBJC list",
                 TCL_STATIC);
         return(TCL_ERROR);
       }
      if (shChainTypeGet(listB) != shTypeGetFromName("OBJC")) {
	Tcl_SetResult(interp, "listB is not a chain of OBJC",
		      TCL_STATIC);
	return(TCL_ERROR);
      }
      radius = ftclGetDouble("radius");
    } 
   else 
     {
       ftclParseRestore("photObjcListMatch");
       Tcl_SetResult(interp,tclObjcListMatch_use,TCL_STATIC);
       return(TCL_ERROR);
     }


/* Call ! */ 
   if (phObjcListMatchSlow(listA,listB,radius,&listJ,&listK,&listL,&listM)!=
       SH_SUCCESS)

     {
       /* TENUKI !! */
       return(TCL_ERROR);
     }
 
/* get a handle for our new 4 Chains of OBJC */

   if(shTclHandleNew(interp,listJname,"CHAIN",(void*)listJ) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp,"Can't bind to new chain of objc handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(shTclHandleNew(interp,listKname,"CHAIN",(void*)listK) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp,"Can't bind to new chain of objc handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(shTclHandleNew(interp,listLname,"CHAIN",(void*)listL) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp,"Can't bind to new chain of objc handle",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(shTclHandleNew(interp,listMname,"CHAIN",(void*)listM) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      Tcl_SetResult(interp,"Can't bind to new chain of objc handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,"",0);
   sprintf(answer,"matched from listA %s , from listB %s ",
	   listJname,listKname);
   Tcl_AppendElement(interp,answer);
   sprintf(answer,"unmatched from listA %s , from listB %s",
	   listLname,listMname);
   Tcl_AppendElement(interp,answer);
   return(TCL_OK);
}




static char *tclObjcChainSortXc_use =
  "USAGE:objcChainSortXc objcchain ";
static char *tclObjcChainSortXc_hlp =
  "sort OBJC chain members with xc";

static int
tclObjcChainSortC0Xc(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *objcHandle;
   CHAIN *objcchain;
   char *objcchainstr;
   char *opts = "objcchain";
   int color;

   /* future versions should add this as an argument */
   color = 0;

   shErrStackClear();
   ftclParseSave("objcChainSortC0Xc");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      objcchainstr = ftclGetStr("objcchain");
      if (p_shTclHandleAddrGet(interp, objcchainstr , &objcHandle) != TCL_OK) {
         Tcl_SetResult(interp, "can't get handle for given CHAIN of OBJC", 
		       TCL_STATIC);
         return(TCL_ERROR);
      }
    }
   else
     {
       ftclParseRestore("objcChainSortXc");
       Tcl_SetResult(interp, tclObjcChainSortXc_use, TCL_STATIC);
       return(TCL_ERROR);
     }
   
      objcchain = objcHandle->ptr;

      if (objcchain->type != shTypeGetFromName("CHAIN")) 
	{
	  Tcl_SetResult(interp, "given argument is not a CHAIN", TCL_STATIC);
	  return(TCL_ERROR);
	}
	 objcchain=phObjcChainMSortC0Xc(objcchain, color);
      Tcl_SetResult(interp, " ",TCL_STATIC);
      return(TCL_OK);
 }

void
phTclAnalysisDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"readCatalog",
                (Tcl_CmdProc *)tclPhotReadCatalog,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclPhotReadCatalog_hlp, tclPhotReadCatalog_use);
   shTclDeclare(interp,"objcListMatch",
                (Tcl_CmdProc *)tclObjcListMatch,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclObjcListMatch_hlp, tclObjcListMatch_use);
   shTclDeclare(interp,"objcChainSortXc",
                (Tcl_CmdProc *)tclObjcChainSortC0Xc,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclObjcChainSortXc_hlp, tclObjcChainSortXc_use);
}

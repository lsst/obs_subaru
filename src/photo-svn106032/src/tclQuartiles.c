/*
 * TCL support for QUARTILES type in photo.
 *
 * ENTRY POINT		SCOPE
 * tclQuartilesDeclare	public	Declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL		TCL + XTCL + Fermilab extensions
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dervish.h"
#include "phQuartiles.h"

static char *module = "phTclStructsFacil";    /* name of this set of code */

/*****************************************************************************/
/*
 * return a handle to a new QUARTILES
 */
static char *tclQuartilesNew_use =
  "USAGE: quartilesNew nrow ncol";
static char *tclQuartilesNew_hlp =
  "create a new QUARTILES";

static int
tclQuartilesNew(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   int nrow, ncol;
   char *opts = "nrow ncol";
   
   shErrStackClear();
   
   ftclParseSave("quartilesNew");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      nrow = ftclGetInt("nrow");
      ncol = ftclGetInt("ncol");
   } else {
      Tcl_SetResult(interp,tclQuartilesNew_use,TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new QUARTILES
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = phQuartilesNew(nrow, ncol);
   handle.type = shTypeGetFromName("QUARTILES");

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      Tcl_SetResult(interp,"Can't bind to new quartiles handle",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Delete a QUARTILES
 */
static char *tclQuartilesDel_use =
  "USAGE: quartilesDel quartiles";
static char *tclQuartilesDel_hlp =
  "Delete a QUARTILES";

static int
tclQuartilesDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   HANDLE *handle;
   char *quartiles;
   char *opts = "quartiles";

   shErrStackClear();

   ftclParseSave("quartilesDel");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      quartiles = ftclGetStr("quartiles");
      if(p_shTclHandleAddrGet(interp,quartiles,&handle) != TCL_OK) {
         return(TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclQuartilesDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   phQuartilesDel(handle->ptr);
   (void) p_shTclHandleDel(interp,quartiles);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Convert a quartiles TBLCOL to part of a QUARTILES
 */
static char *tclTblcolToQuartiles_use =
  "USAGE: tblcolToQuartiles tblcol quartiles row";
static char *tclTblcolToQuartiles_hlp =
  "Convert a quartiles TBLCOL to part of a QUARTILES";

static int
tclTblcolToQuartiles(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   QUARTILES *quartiles = NULL;
   TBLCOL *tblcol = NULL;
   char *string;
   char *opts = "tblcol quartiles row";
   int row;

   shErrStackClear();

   ftclParseSave("tblcolToQuartiles");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      string = ftclGetStr("tblcol");
      if(shTclAddrGetFromName(interp,string,(void**)&tblcol,"TBLCOL")!=TCL_OK) {
         return(TCL_ERROR);
      }
      string = ftclGetStr("quartiles");
      if(shTclAddrGetFromName(interp,string,(void **)&quartiles,"QUARTILES")
         != TCL_OK) {
         return(TCL_ERROR);
      }
      row = ftclGetInt("row");
   } else {
      Tcl_SetResult(interp,tclTblcolToQuartiles_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   /* Check to see that the structs are the right size */
   if (tblcol->rowCnt!=quartiles->nc) {
      Tcl_SetResult(interp,"Tblcol nr does not match quartiles nc",TCL_STATIC);
      return(TCL_ERROR);
   }
   if (row>(quartiles->nr)) {
      Tcl_SetResult(interp,"Too few rows in quartiles",TCL_STATIC);
      return(TCL_ERROR);
   }
   if (phTblcolToQuartiles(tblcol,quartiles,row)!=SH_SUCCESS) {
      Tcl_SetResult(interp,"Error in phTblcolToQuartiles - check tscal",
		TCL_STATIC);
      return TCL_ERROR;
   }

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}



/****************************************************************************
 *
 * Convert "quartile-ish" structures stored in FITS files by the DA 
 * into a PHOTO-type QUARTILES structure.  This procedure takes a CHAIN
 * of "DA-style" structures, with each structure representing one
 * column of data on the chip, and coverts them all to a single row of one
 * PHOTO-style QUARTILES structure.  The user must already have
 * created a QUARTILES structure, large enough to hold the data,
 * and pass a handle of that structure to this function.
 *
 * This routine assumes that the structures read from FITS files
 * have been given the name "DAQUART".  If not, it will fail.
 *
 * This routine performs NO scaling, nor checks of scaling value.
 * 
 * If DEBUG is #define'd, then the routine not only creates a handle for 
 * the new QUARTILES, but also handles for each REGION inside it, and return
 * four handle names: 
 *      quartiles  25'th-region  50'th-region  75'th-region
 *
 *
 */
static char *tclDAQuartToQuartiles_use =
  "USAGE: DAQuartToQuartiles chain_of_DAQuart quartiles rownumber";
static char *tclDAQuartToQuartiles_hlp =
  "Convert a chain of DA-style quartile-ish structures to one row of a PHOTO-style QUARTILES";

#define DEBUG

static int
tclDAQuartToQuartiles(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
   int i, ncol, row, col;
   void *daquart_ptr;
   char *dastr, *quartstr;
   char *opts = "chain_of_DAQuart quartiles rownumber";
   S32 q1, q2, q3;
   TYPE type;
   CHAIN *chain_ptr;
   QUARTILES *quartile_ptr = NULL;
   const SCHEMA *schema_ptr;
   SCHEMA_ELEM elem;
   HANDLE *chainHandle, *quartHandle;
#ifdef DEBUG
   char *regname[3];
   char reg1[HANDLE_NAMELEN], reg2[HANDLE_NAMELEN], reg3[HANDLE_NAMELEN];
   HANDLE regHandle[3];
   char bigresult[200];
#endif

   shErrStackClear();

   ftclParseSave("DAQuartToQuartiles");
   if(ftclFullParseArg(opts,argc,argv) != 0) {
      dastr = ftclGetStr("chain_of_DAQuart");
      if (p_shTclHandleAddrGet(interp, dastr, &chainHandle) != TCL_OK) {
         return(TCL_ERROR);
      }
      if (chainHandle->type != shTypeGetFromName("CHAIN")) {
         Tcl_SetResult(interp, "tclDAQuartToQuartiles: first arg is not a CHAIN",
                  TCL_STATIC);
	 return(TCL_ERROR);
      }
      chain_ptr = (CHAIN *) chainHandle->ptr;
      if (shChainTypeGet(chain_ptr) != shTypeGetFromName("DAQUART")) {
         Tcl_SetResult(interp, "tclDAQuartToQuartiles: CHAIN is not of type DAQUART",
                  TCL_STATIC);
	 return(TCL_ERROR);
      
      }
      quartstr = ftclGetStr("quartiles");
      if (p_shTclHandleAddrGet(interp, quartstr, &quartHandle) != TCL_OK) {
         return(TCL_ERROR);
      }
      if (quartHandle->type != shTypeGetFromName("QUARTILES")) {
         Tcl_SetResult(interp, "tclDAQuartToQuartiles: second arg is not a QUARTILES",
                  TCL_STATIC);
	 return(TCL_ERROR);
      }
      quartile_ptr = quartHandle->ptr;
      row = ftclGetInt("rownumber");
   } else {
      Tcl_SetResult(interp,tclDAQuartToQuartiles_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   /* we'll need this to decode the "DAQUART" structure */
   schema_ptr = shSchemaGet("DAQUART");

   ncol = shChainSize(chain_ptr);
   shAssert(quartile_ptr->nr > row);
   shAssert(quartile_ptr->nc == ncol);

   /*
    * now, we want to set only the "row"'th row of each of the quartiles' 
    * regions
    */
   for (col = 0; col < ncol; col++) {
      daquart_ptr = (void *) shChainElementGetByPos(chain_ptr, col);

      /* find the "q1" field of this structure, which holds 25'th percentile */
      for (i = 0; i < schema_ptr->nelem; i++) {
         elem = schema_ptr->elems[i];
         if (strcmp(elem.name, "q1") == 0) {
            break;
         }
      }
      shAssert(strcmp(elem.name, "q1") == 0);
      shAssert(strcmp(elem.type, "INT") == 0);
      q1 = *((S32 *) shElemGet(daquart_ptr, &elem, &type));

      /* find the "q2" field of this structure, which holds 50'th percentile */
      for (i = 0; i < schema_ptr->nelem; i++) {
         elem = schema_ptr->elems[i];
         if (strcmp(elem.name, "q2") == 0) {
            break;
         }
      }
      shAssert(strcmp(elem.name, "q2") == 0);
      shAssert(strcmp(elem.type, "INT") == 0);
      q2 = *((S32 *) shElemGet(daquart_ptr, &elem, &type));

      /* find the "q3" field of this structure, which holds 75'th percentile */
      for (i = 0; i < schema_ptr->nelem; i++) {
         elem = schema_ptr->elems[i];
         if (strcmp(elem.name, "q3") == 0) {
            break;
         }
      }
      shAssert(strcmp(elem.name, "q3") == 0);
      shAssert(strcmp(elem.type, "INT") == 0);
      q3 = *((S32 *) shElemGet(daquart_ptr, &elem, &type));

      /* now set the appropriate pixels in the quartiles' REGIONs */
      shRegPixSetWithDbl(quartile_ptr->data[0], row, col, q1);
      shRegPixSetWithDbl(quartile_ptr->data[1], row, col, q2);
      shRegPixSetWithDbl(quartile_ptr->data[2], row, col, q3);
   }

#ifdef DEBUG
   /* now bind each REGION structure to a new handle and return it */
  regname[0] = reg1; regname[1] = reg2; regname[2] = reg3;
  for (i = 0; i < 3; i++) {
   if (p_shTclHandleNew(interp, regname[i]) != TCL_OK) {
      phQuartilesDel(quartile_ptr);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   regHandle[i].ptr = quartile_ptr->data[i];
   regHandle[i].type = shTypeGetFromName("REGION");
   if (p_shTclHandleAddrBind(interp, regHandle[i], regname[i]) != TCL_OK) {
      phQuartilesDel(quartile_ptr);
      Tcl_SetResult(interp, "can't bind to new REGION handle", TCL_STATIC);
      return(TCL_ERROR);
   }
  }
  sprintf(bigresult, "quartile regions in %s %s %s", 
                           regname[0], regname[1], regname[2]);
  Tcl_SetResult(interp, bigresult, TCL_VOLATILE);
#else
   Tcl_SetResult(interp, "", TCL_STATIC);
#endif
   return(TCL_OK);
}
   














/****************************************************************************
 * Declare my new tcl verbs to tcl
 */
void
phTclQuartilesDeclare(Tcl_Interp *interp)
{
   shTclDeclare(interp,"quartilesNew",
                (Tcl_CmdProc *)tclQuartilesNew,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclQuartilesNew_hlp, tclQuartilesNew_use);

   shTclDeclare(interp,"quartilesDel",
                (Tcl_CmdProc *)tclQuartilesDel,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclQuartilesDel_hlp, tclQuartilesDel_use);

   shTclDeclare(interp,"tblcolToQuartiles",
                (Tcl_CmdProc *)tclTblcolToQuartiles,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclTblcolToQuartiles_hlp, tclTblcolToQuartiles_use);

   shTclDeclare(interp,"DAQuartToQuartiles",
                (Tcl_CmdProc *)tclDAQuartToQuartiles,
                (ClientData) 0,
                (Tcl_CmdDeleteProc *)NULL,
                module, tclDAQuartToQuartiles_hlp, tclDAQuartToQuartiles_use);


}



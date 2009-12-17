/*****************************************************************************
******************************************************************************
**
** FILE:
**	tclHdr.c
**
** ABSTRACT:
**	This file contains routines that manipulate header information
**	at the TCL layer.
**
** ENTRY POINT		        SCOPE   DESCRIPTION
** -------------------------------------------------------------------------
** shTclHdrAddrGetFromExpr      public  Returns header address
** shTclHdrDeclare              public  World's interface to the private
**                                         routines below
** tclHdrInsWithLogical         private Insert a logical value in header
** tclHdrInsWithDbl             private Insert a double value in header
** tclHdrInsWithInt             private Insert an integer value in header
** tclHdrInsWithAscii           private Insert an ASCII string in header
** tclHdrDelByKeyword           private Delete the line in header 
**                                        containing the given keyword
** tclHdrFree                   private Free header vector elements
** tclHdrPrint                  private Echo the header lines.
** tclHdrGetAsAscii             private Get an ASCII value from header
**                                        identified by given keyword
** tclHdrGetAsDbl               private Get a double value from a header
**                                        identified by the given keyword
** tclHdrGetAsInt               private Get an integer value from a header
**                                        identified by the given keyword
** tclHdrGetAsLogical           private Get a logical value from a header
**                                        identified by the given keyword
** tclHdrGetLine                private Get a line from a header identified
**                                        by the given keyword
** tclHdrGetLineCont            private Get a line from a header identified
**                                        by the given line number
** tclHdrGetLineno              private Get the line number from a header
**                                        containing the given keyword
** tclHdrGetLineTotal           private Get the total number of lines in the
**                                        header.
** tclHdrDelByLine              private Delete the indicated line number from a
**                                        header
** tclHdrReplaceLine            private Replace the given line after the given
**                                        line number in a header
** tclHdrInsertLine             private Insert the given line at the given
**                                        line number in a header
** tclHdrMakeLineWithAscii      private Create a line of ASCII value suitable
**                                        for insertion in a header
** tclHdrMakeLineWithDbl        private Create a line of double value suitable
**                                        for insertion in a header
** tclHdrMakeLineWithInt        private Create a line of integer value suitable
**                                        for insertion in a header
** tclHdrMakeLineWithLogical    private Create a line of logical value suitable
**                                         for insertion in a header
** tclHdrCopy                   private Copy a header from one to another
*
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	TCL
**
** AUTHORS:
**	Creation date:  May 7, 1993
**		Vijay Gurbani
**
** MODIFICATIONS:
** 05/26/1993  VKG    Deleted function tclRegHdrInit
** 07/01/1993  EFB    Added function tclRegHdrInsertLine
** 01/17/1994  THN    Converted from REGION-specific to generic header code.
******************************************************************************
******************************************************************************
*/
#include <stdlib.h>      /* For atoi() and friends prototype */
#include <memory.h>      /* For memset() prototype */

#include <string.h>
#include "libfits.h"
#include "ftcl.h"
#include "shTclUtils.h"	/* For shTclDeclare() */
#include "shCHdr.h"
#include "shTclHdr.h"
#include "shCFitsIo.h"
#include "shTclVerbs.h"
#include "shTclHandle.h"

#define DESTBUFSIZ   20

static char *tclHelpFacil = "shHdr";
static char errBuf[512];

/*============================================================================
**============================================================================
**
** ROUTINE: shTclHdrAddrGetFromExpr
**
** DESCRIPTION:
**	Returns a header address given a handle expression
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	shTclHandleExprEval
**	Tcl_AppendResult
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclHdrAddrGetFromExpr
   (
   Tcl_Interp *a_interp,
   char       *a_hdrExpr,
   HDR     **a_hdr
   )   
{
   HANDLE  hand;
   void   *handPtr;
/*
** Find header address
*/
   if (shTclHandleExprEval(a_interp,a_hdrExpr,&hand,&handPtr) != TCL_OK) {
      Tcl_AppendResult(a_interp, "\n-----> Unknown header: ", 
                                  a_hdrExpr, (char *) NULL);
      return(TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("HDR")) {
      Tcl_AppendResult(a_interp,a_hdrExpr," is a ",
		   shNameGetFromType(hand.type)," not a header (HDR)",(char *)NULL);
      return(TCL_ERROR);
   }
/*
**	Return header address
*/
   *a_hdr = (HDR *)hand.ptr;
   return(TCL_OK);
}

static char *tclHdrInsWithLogicalHelp =
       "Insert a LOGICAL (1 or 0) value in a header";
static char *tclHdrInsWithLogicalUse = 
       "USAGE: hdrInsWithLogical <header> <keyword> <value> [comment]";
static int tclHdrInsWithLogical(ClientData clientData, Tcl_Interp *a_interp, 
                                   int a_argc, char **a_argv)
{
   int    rc,
          value;
   char   *comment;
   HDR    *hdr;

   /*
    * Comments are optional.
    */
   comment = NULL;

   if (a_argc < 4)  {
       Tcl_SetResult(a_interp, tclHdrInsWithLogicalUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   /*
    * NOTE: Should we be checking the validity of a_argv[3]?
    */
   rc = Tcl_GetBoolean(a_interp, a_argv[3], &value);
   if (rc != TCL_OK)
       return rc;

   if (a_argc >= 5)
       comment = a_argv[4];

   if (shHdrInsertLogical(hdr, a_argv[2], value, comment) != SH_SUCCESS)  {
       memset(errBuf, 0, sizeof(errBuf));
       sprintf(errBuf, 
               "%s: Unable to insert Logical value of keyword %s and value %d in header", a_argv[0], a_argv[2], value);
       Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
       return TCL_ERROR;
   }

   return TCL_OK;
}

static char *tclHdrInsWithDblHelp =
       "Insert a Double value in a header";
static char *tclHdrInsWithDblUse =
       "USAGE: hdrInsWithDbl <header> <keyword> <value> [comment]";
static int tclHdrInsWithDbl(ClientData clientData, Tcl_Interp *a_interp, 
                               int a_argc, char **a_argv)
{
   int    rc;
   double value;
   char   *comment;
   HDR    *hdr;

   /*
    * Comments are optional
    */
   comment = NULL;

   if (a_argc < 4)  {
       Tcl_SetResult(a_interp, tclHdrInsWithDblUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   rc = Tcl_GetDouble(a_interp, a_argv[3], &value);  
   if (rc != TCL_OK)
       return rc;

   if (a_argc >= 5)
       comment = a_argv[4];

   if (shHdrInsertDbl(hdr, a_argv[2], value, comment) != SH_SUCCESS)  {
       memset(errBuf, 0, sizeof(errBuf));
       sprintf(errBuf, 
               "%s: Unable to insert Double value of keyword %s and value %f in header", a_argv[0], a_argv[2], value);
       Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
       return TCL_ERROR;
   }

   return TCL_OK;
}

static char *tclHdrInsWithIntHelp = 
       "Insert an Integer value in a header";
static char *tclHdrInsWithIntUse =
       "USAGE: hdrInsWithInt <header> <keyword> <value> [comment]";
static int tclHdrInsWithInt(ClientData clientData, Tcl_Interp *a_interp, 
                               int a_argc, char **a_argv)
{
   int    rc,
          value;
   char   *comment;
   HDR    *hdr;

   /*
    * Comments are optional
    */
   comment = NULL;

   if (a_argc < 4)  {
       Tcl_SetResult(a_interp, tclHdrInsWithIntUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   rc = Tcl_GetInt(a_interp, a_argv[3], &value);  
   if (rc != TCL_OK)
       return rc;

   if (a_argc >= 5)
       comment = a_argv[4];

   if (shHdrInsertInt(hdr, a_argv[2], value, comment) != SH_SUCCESS)  {
       memset(errBuf, 0, sizeof(errBuf));
       sprintf(errBuf, 
               "%s: Unable to insert Integer value of keyword %s and value %din header", a_argv[0], a_argv[2], value);
       Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
       return TCL_ERROR;
   }

   return TCL_OK;
}

static char *tclHdrInsWithAsciiHelp =
       "Insert an ASCII string in a header";
static char *tclHdrInsWithAsciiUse =
       "USAGE: hdrInsWithAscii <header> <keyword> <value> [comment]";
static int tclHdrInsWithAscii(ClientData clientData, Tcl_Interp *a_interp, 
                                 int a_argc, char **a_argv)
{
   int    rc;
   char   *comment;
   HDR    *hdr;

   /*
    * Comments are optional 
    */
   comment = NULL;

   if (a_argc < 4)  {
       Tcl_SetResult(a_interp, tclHdrInsWithAsciiUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   if (a_argc >= 5)
       comment = a_argv[4];

   if (shHdrInsertAscii(hdr, a_argv[2], a_argv[3], comment) != SH_SUCCESS)  {
       memset(errBuf, 0, sizeof(errBuf));
       sprintf(errBuf, 
               "%s: Unable to insert alphanumeric value of keyword %s and value %s in header", a_argv[0], a_argv[2], a_argv[3]);
       Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
       return TCL_ERROR;
   }

   return TCL_OK;
}

static char *tclHdrDelByKeywordHelp = 
       "Delete a line identified by keyword from the header";
static char *tclHdrDelByKeywordUse =
       "USAGE: hdrDelByKeyword <header> <keyword>";
static int tclHdrDelByKeyword(ClientData clientData, Tcl_Interp *a_interp, 
                                 int a_argc, char **a_argv)
{
   HDR    *hdr;
   int    rc;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrDelByKeywordUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   if (shHdrDelByKeyword(hdr, a_argv[2]) != SH_SUCCESS)  {
       Tcl_AppendResult(a_interp, a_argv[0], 
         ": key ", a_argv[2], " not in header", (char *)0);
       return TCL_ERROR;
   }

   return TCL_OK;
}

static char *tclHdrFreeHelp = 
    "Free all memory associtated with a header, but not the hdr struct itself";
static char *tclHdrFreeUse  = "USAGE: hdrFree <header>";

static int tclHdrFree(ClientData clientData, Tcl_Interp *a_interp, int a_argc,
                         char **a_argv)
{
   HDR    *hdr;
   int    rc;
   
   if (a_argc != 2)  {
       Tcl_SetResult(a_interp, tclHdrFreeUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   p_shHdrFreeForVec(hdr);

   return TCL_OK;
}

static char *tclHdrPrintHelp =
       "Echo the lines in the header";
static char *tclHdrPrintUse = 
       "USAGE: hdrPrint <header>";

static int tclHdrPrint(ClientData clientData, Tcl_Interp *a_interp, 
		        int a_argc, char **a_argv)
{
   HDR      *hdr;
   char     *hdrExpr;
   int      rc;

   if(a_argc == 2) {
      hdrExpr = a_argv[1];		/* Get header expression */

      /* Get the pointer to the header from the hedaer name. */
      rc = shTclHdrAddrGetFromExpr(a_interp, hdrExpr, &hdr);

      if (rc == TCL_OK) {
         p_shHdrPrint(hdr);
         }
      }
   else {
       Tcl_SetResult(a_interp, tclHdrPrintUse, TCL_VOLATILE);
       rc = TCL_ERROR;
   }

   return rc;

}

static char *tclHdrGetAsAsciiHelp =
       "Get an ASCII string identified by the keyword from a header";
static char *tclHdrGetAsAsciiUse = 
       "USAGE: hdrGetAsAscii <header> <keyword>";

static int tclHdrGetAsAscii(ClientData clientData, Tcl_Interp *a_interp, 
                               int a_argc, char **a_argv)
{
   static   char dest[HDRLINESIZE+1];
   HDR      *hdr;
   int      rc;
   RET_CODE retCode;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrGetAsAsciiUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;
  
   retCode = shHdrGetAscii(hdr, a_argv[2], dest);
   memset(errBuf, 0, sizeof(errBuf));
   if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: header is NULL", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else if (retCode == SH_SUCCESS) {
      Tcl_SetResult(a_interp, dest, TCL_VOLATILE);
      rc = TCL_OK;
   }
   else {
      sprintf(errBuf, "%s: could not get requested value of keyword %s", a_argv[0], a_argv[2]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrGetAsDblHelp =
       "Get a double value identified by the keyword from a header";
static char *tclHdrGetAsDblUse = 
       "USAGE: hdrGetAsDbl <header> <keyword>";

static int tclHdrGetAsDbl(ClientData clientData, Tcl_Interp *a_interp, 
                           int a_argc, char **a_argv)
{
   static   char destBuf[DESTBUFSIZ];
   double   dest;
   HDR      *hdr;
   int      rc;
   RET_CODE retCode;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrGetAsDblUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;
  
   retCode = shHdrGetDbl(hdr, a_argv[2], &dest);
   memset(errBuf, 0, sizeof(errBuf));
   if (retCode == SH_SUCCESS) {
      sprintf(destBuf, "%-.17g", dest);
      if(strchr(destBuf, '.') == NULL && strchr(destBuf, 'e') == NULL) {
	 strcat(destBuf, ".0");
      }
      Tcl_SetResult(a_interp, destBuf, TCL_VOLATILE);
      rc = TCL_OK;
   }
   else if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: header is NULL", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else {
      sprintf(errBuf, "%s: could not get requested value of keyword %s", a_argv[0], a_argv[2]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrGetAsIntHelp =
       "Get an integer value identified by the keyword from a header";
static char *tclHdrGetAsIntUse = 
       "USAGE: hdrGetAsInt <header> <keyword>";

static int tclHdrGetAsInt(ClientData clientData, Tcl_Interp *a_interp, 
			     int a_argc, char **a_argv)
{
   static   char destBuf[DESTBUFSIZ];
   int      dest;
   HDR      *hdr;
   int      rc;
   RET_CODE retCode;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrGetAsIntUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   retCode = shHdrGetInt(hdr, a_argv[2], &dest);
   memset(errBuf, 0, sizeof(errBuf));
   if (retCode == SH_SUCCESS) {
      sprintf(destBuf, "%d", dest);
      Tcl_SetResult(a_interp, destBuf, TCL_VOLATILE);
      rc = TCL_OK;
   }
   else if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: header is NULL", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else {
      sprintf(errBuf, "%s: could not get requested value of keyword %s", a_argv[0], a_argv[2]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrGetAsLogicalHelp =
       "Get a logical (0 or 1) value identified by keyword from header";
static char *tclHdrGetAsLogicalUse = 
       "USAGE: hdrGetAsLogical <header> <keyword>";

static int tclHdrGetAsLogical(ClientData clientData, Tcl_Interp *a_interp, 
				 int a_argc, char **a_argv)
{
   static   char destBuf[DESTBUFSIZ];
   int      dest;
   HDR      *hdr;
   int      rc;
   RET_CODE retCode;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrGetAsLogicalUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   retCode = shHdrGetLogical(hdr, a_argv[2], &dest);
   memset(errBuf, 0, sizeof(errBuf));
   if (retCode == SH_SUCCESS) {
      sprintf(destBuf, "%d", dest);
      Tcl_SetResult(a_interp, destBuf, TCL_VOLATILE);
      rc = TCL_OK;
   }
   else if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: header is NULL", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else {
      sprintf(errBuf, "%s: could not get requested value of keyword %s", a_argv[0], a_argv[2]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrGetLineHelp =
       "Get a line identified by keyword from a header";
static char *tclHdrGetLineUse = "USAGE: hdrGetLine <header> <keyword>";

static int tclHdrGetLine(ClientData clientData, Tcl_Interp *a_interp, int a_argc,
			    char **a_argv)
{
   char     dest[HDRLINESIZE+1];
   HDR      *hdr;
   int      rc;
   RET_CODE retCode;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrGetLineUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   retCode = shHdrGetLine(hdr, a_argv[2], dest);
   memset(errBuf, 0, sizeof(errBuf));
   if (retCode == SH_SUCCESS) {
      Tcl_SetResult(a_interp, dest, TCL_VOLATILE);
      rc = TCL_OK;
   }
   else if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: header is NULL", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else {
      sprintf(errBuf, "%s: no such line found in header with keyword %s", a_argv[0], a_argv[2]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrGetLineContHelp =
       "Get a line identified by line number from a header";
static char *tclHdrGetLineContUse = "USAGE: hdrGetLineCont <header> <lineNo>";

static int tclHdrGetLineCont(ClientData clientData, Tcl_Interp *a_interp, int a_argc,
			    char **a_argv)
{
   char     dest[HDRLINESIZE+1];
   HDR      *hdr;
   int      rc;
   int      ln;
   RET_CODE retCode;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrGetLineContUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   if ((rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr)) != TCL_OK) return rc;
   if (Tcl_GetInt(a_interp, a_argv[2], &ln) != TCL_OK) return TCL_ERROR;
   retCode = shHdrGetLineCont(hdr, ln, dest);
   memset(errBuf, 0, sizeof(errBuf));
   if (retCode == SH_SUCCESS) {
      if (dest[0] == '\0') {
        sprintf(errBuf, "%s: header line number %d is empty", a_argv[0], ln);
        Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
        rc = TCL_ERROR;
      } else {
        Tcl_SetResult(a_interp, dest, TCL_VOLATILE);
        rc = TCL_OK;
      }
   }
   else if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: header is NULL", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else {
      sprintf(errBuf, "%s: no such line found in header with line number %d", a_argv[0], ln);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrGetLinenoHelp =
       "Get the header line number identified by keyword";
static char *tclHdrGetLinenoUse = 
       "USAGE: hdrGetLineno <header> <keyword>";

static int tclHdrGetLineno(ClientData clientData, Tcl_Interp *a_interp, 
			      int a_argc, char **a_argv)
{
   static   char destBuf[DESTBUFSIZ];
   int      dest;
   HDR      *hdr;
   int      rc;
   RET_CODE retCode;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrGetLinenoUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   retCode = shHdrGetLineno(hdr, a_argv[2], &dest);
   memset(errBuf, 0, sizeof(errBuf));
   if (retCode == SH_SUCCESS) {
      sprintf(destBuf, "%d", dest);
      Tcl_SetResult(a_interp, destBuf, TCL_VOLATILE);
      rc = TCL_OK;
   }
   else if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: header is NULL", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else {
      sprintf(errBuf, "%s: no such line found in header with keyword %s", a_argv[0], a_argv[2]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrGetLineTotalHelp =
       "Get the total number of lines in the header";
static char *tclHdrGetLineTotalUse = 
       "USAGE: hdrGetLineTotal <header>";

static int tclHdrGetLineTotal(ClientData clientData, Tcl_Interp *a_interp, 
			         int a_argc, char **a_argv)
{
   HDR      *hdr;
   char     *hdrExpr;
   RET_CODE retCode;
   int      rc;
   int      lineNumber;

   if(a_argc == 2) {
      hdrExpr = a_argv[1];		/* Get header expression */

      /* Get the pointer to the header from the header expression. */
      rc = shTclHdrAddrGetFromExpr(a_interp, hdrExpr, &hdr);

      if (rc == TCL_OK) {
         retCode = shHdrGetLineTotal(hdr, &lineNumber);
         if (retCode == SH_SUCCESS) {
            sprintf(errBuf, "%d", lineNumber);
            rc = TCL_OK;
         }
         else {
            sprintf(errBuf, "%s: unable to find total number of lines in %s", a_argv[0], hdrExpr);
            rc = TCL_ERROR;
         }
         Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      }
   }
   else {
       Tcl_SetResult(a_interp, tclHdrGetLineTotalUse, TCL_VOLATILE);
       rc = TCL_ERROR;
   }

   return rc;

}

static char *tclHdrDelByLineHelp =
       "Delete the header line number identified by lineNumber";
static char *tclHdrDelByLineUse =
       "USAGE: hdrDelByLine <header> <lineNumber>";

static int tclHdrDelByLine(ClientData clientData, Tcl_Interp *a_interp, 
			      int a_argc, char **a_argv)
{
   HDR      *hdr;
   int      rc,
	    lineNum;
   RET_CODE retCode;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrDelByLineUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   rc = Tcl_GetInt(a_interp, a_argv[2], &lineNum);  
   if (rc != TCL_OK)
       return rc;

   retCode = shHdrDelByLine(hdr, lineNum);
   memset(errBuf, 0, sizeof(errBuf));
   if (retCode == SH_SUCCESS)
      rc = TCL_OK;
   else if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: header is NULL", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else {
      sprintf(errBuf, "%s: illegal line number (%d) specified", a_argv[0], lineNum);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrReplaceLineHelp =
       "Replace the contents of lineNumber with lineContents in header";
static char *tclHdrReplaceLineUse =
       "USAGE: hdrReplaceLine <header> <lineNumber> <lineContents>";

static int tclHdrReplaceLine(ClientData clientData, Tcl_Interp *a_interp, 
				int a_argc, char **a_argv)
{
   HDR      *hdr;
   int      lineNum,
	    rc;
   RET_CODE retCode;

   memset(errBuf, 0, sizeof(errBuf));

   if (a_argc != 4)  {
       Tcl_SetResult(a_interp, tclHdrReplaceLineUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &hdr);
   if (rc != TCL_OK)
       return rc;

   rc = Tcl_GetInt(a_interp, a_argv[2], &lineNum);  
   if (rc != TCL_OK)  {
       sprintf(errBuf, "%s: Tcl_GetInt() failed for %s", a_argv[0],
               a_argv[2]);
       Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
       return rc;
   }

   retCode = shHdrReplaceLine(hdr, lineNum, a_argv[3]);
   if (retCode == SH_SUCCESS)
      rc = TCL_OK;
   else if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: header is NULL", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else {
      sprintf(errBuf, "%s: unable to replace line number %d", a_argv[0], lineNum);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrInsertLineHelp =
       "Insert lineContents into the lineNumber'th line in the header";
static char *tclHdrInsertLineUse =
       "USAGE: hdrInsertLine <header> <lineNumber> <lineContents>";

static int tclHdrInsertLine(ClientData clientData, Tcl_Interp *a_interp, 
				int a_argc, char **a_argv)
{
   HDR      *hdr;
   int lineNumber,
	    rc;
   char     *lineContents;
   char     *hdrExpr;
   RET_CODE retCode;
   char     *dumPtr;

   memset(errBuf, 0, sizeof(errBuf));

   if(a_argc == 4) {
      hdrExpr = a_argv[1];		/* Get header expression */

      lineNumber = (int)strtol(a_argv[2], &dumPtr,0); /* Get line number */
      if (dumPtr == a_argv[2]) {
         Tcl_SetResult(a_interp, tclHdrInsertLineUse, TCL_VOLATILE);
         rc = TCL_ERROR;
      } else {
	lineContents = a_argv[3];	/* Get line contents */

	/* We must decrement the line number as the internal world works on
	   indices from 0 - n and the external world works on indices from
	   1 - m.  If the index was originally 0, keep it there. */
	if (lineNumber != 0) {
	  --lineNumber;
	}

	/* Get the pointer to the header from the header expression. */
	rc = shTclHdrAddrGetFromExpr(a_interp, hdrExpr, &hdr);
	if (rc == TCL_OK) {
	  retCode = shHdrInsertLine(hdr, lineNumber, lineContents);
	  if (retCode == SH_SUCCESS)
            rc = TCL_OK;
	  else {
            sprintf(errBuf, "%s: unable to insert line at line number %d", a_argv[0], lineNumber);
            Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
            rc = TCL_ERROR;
	  }
	}
      } /* end of else from if (dumPtr == a_argv[2]) */
   }
   else {
       Tcl_SetResult(a_interp, tclHdrInsertLineUse, TCL_VOLATILE);
       rc = TCL_ERROR;
   }

   return rc;
}

static char *tclHdrMakeLineWithAsciiHelp =
       "Make line containing ASCII value suitable for inclusion in a header";
static char *tclHdrMakeLineWithAsciiUse =
       "USAGE: hdrMakeLineWithAscii <keyword> <value> [comment]";

static int tclHdrMakeLineWithAscii(ClientData clientData, Tcl_Interp *a_interp,
				      int a_argc, char **a_argv)
{
   static char dest[HDRLINESIZE+1];
   char   *comment;

   /*
    * Comments are optional
    */
   comment = NULL;

   if (a_argc < 3)  {
       Tcl_SetResult(a_interp, tclHdrMakeLineWithAsciiUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   if (a_argc >= 4)
       comment = a_argv[3];

   shHdrMakeLineWithAscii(a_argv[1], a_argv[2], comment, dest);
   Tcl_SetResult(a_interp, dest, TCL_VOLATILE);

   return TCL_OK;
} 

static char *tclHdrMakeLineWithDblHelp =
       "Make line containing a double value suitable for inclusion in a header";
static char *tclHdrMakeLineWithDblUse =
       "USAGE: hdrMakeLineWithDbl <keyword> <double> [comment]";

static int tclHdrMakeLineWithDbl(ClientData clientData, Tcl_Interp *a_interp,
				    int a_argc, char **a_argv)
{
   static char dest[HDRLINESIZE+1];
   char   *comment;
   double value;
   int    rc;

   /*
    * Comments are optional
    */
   comment = NULL;

   if (a_argc < 3)  {
       Tcl_SetResult(a_interp, tclHdrMakeLineWithDblUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   if (a_argc >= 4)
       comment = a_argv[3];

	   
   rc = Tcl_GetDouble(a_interp, a_argv[2], &value);
   if (rc != TCL_OK)
       return rc;

   shHdrMakeLineWithDbl(a_argv[1], value, comment, dest);
   Tcl_SetResult(a_interp, dest, TCL_VOLATILE);

   return TCL_OK;
}

static char *tclHdrMakeLineWithIntHelp =
       "Make line containing an integer value suitable for inclusion in a header";
static char *tclHdrMakeLineWithIntUse =
       "USAGE: hdrMakeLineWithInt <keyword> <integer> [comment]";

static int tclHdrMakeLineWithInt(ClientData clientData, Tcl_Interp *a_interp,
				    int a_argc, char **a_argv)
{
   static char dest[HDRLINESIZE+1];
   char   *comment;
   int    value,
	  rc;

   /*
    * Comments are optional
    */
   comment = NULL;

   if (a_argc < 3)  {
       Tcl_SetResult(a_interp, tclHdrMakeLineWithIntUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   if (a_argc >= 4)
       comment = a_argv[3];


   rc = Tcl_GetInt(a_interp, a_argv[2], &value);
   if (rc != TCL_OK)
       return rc;

   shHdrMakeLineWithInt(a_argv[1], value, comment, dest);
   Tcl_SetResult(a_interp, dest, TCL_VOLATILE);

   return TCL_OK;
}

static char *tclHdrMakeLineWithLogicalHelp =
       "Make line containing a logical value suitable for inclusion in a header";
static char *tclHdrMakeLineWithLogicalUse =
       "USAGE: hdrMakeLineWithLogical <keyword> <logical> [comment]";

static int tclHdrMakeLineWithLogical(ClientData clientData, Tcl_Interp *a_interp,
					int a_argc, char **a_argv)
{
   static char dest[HDRLINESIZE+1];
   char   *comment;
   int    value,
	  rc;

   /*
    * Comments are optional
    */
   comment = NULL;

   if (a_argc < 3)  {
       Tcl_SetResult(a_interp, tclHdrMakeLineWithLogicalUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   if (a_argc >= 4)
       comment = a_argv[3];

   rc = Tcl_GetBoolean(a_interp, a_argv[2], &value);
   if (rc != TCL_OK)
       return rc;

   shHdrMakeLineWithLogical(a_argv[1], value, comment, dest);
   Tcl_SetResult(a_interp, dest, TCL_VOLATILE);

   return TCL_OK;
}

static char *tclHdrCopyHelp = "Copy a header";
static char *tclHdrCopyUse  = 
       "USAGE: hdrCopy <srcHeader> <destHeader>";

static int tclHdrCopy(ClientData clientData, Tcl_Interp *a_interp, int a_argc,
			 char **a_argv)
{
   HDR      *s_hdr,
	    *d_hdr;
   int      rc;
   RET_CODE retCode;

   if (a_argc != 3)  {
       Tcl_SetResult(a_interp, tclHdrCopyUse, TCL_VOLATILE);
       return TCL_ERROR;
   }

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[1], &s_hdr);
   if (rc != TCL_OK)
       return rc;

   rc = shTclHdrAddrGetFromExpr(a_interp, a_argv[2], &d_hdr);
   if (rc != TCL_OK)
       return rc;

   /*
    * Make sure that source and destination headers are distinct
    */
   if (s_hdr == d_hdr)  {
       Tcl_AppendResult(a_interp,
	 a_argv[0], ": destHeader must be different than srcHeader",
	 (char *)0);
       return TCL_ERROR;
   }

   retCode = shHdrCopy(s_hdr, d_hdr);
   memset(errBuf, 0, sizeof(errBuf));
   if (retCode == SH_SUCCESS) 
      rc = TCL_OK;
   else if (retCode == SH_HEADER_IS_NULL) {
      sprintf(errBuf, "%s: error - NULL header %s",
              a_argv[0], a_argv[1]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }
   else {
      sprintf(errBuf, "%s: unknown error", a_argv[0]);
      Tcl_SetResult(a_interp, errBuf, TCL_VOLATILE);
      rc = TCL_ERROR;
   }

   return rc;
}

void shTclHdrDeclare(Tcl_Interp *a_interp)
{
   /*
    * This is the publicly visible module that 'exports' all the above
    * routines as TCL expressions.
    */
   shTclDeclare(a_interp, "hdrPrint",
               (Tcl_CmdProc *) tclHdrPrint,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrPrintHelp, tclHdrPrintUse);

   shTclDeclare(a_interp, "hdrGetLineTotal",
	       (Tcl_CmdProc *) tclHdrGetLineTotal,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
	       tclHelpFacil, tclHdrGetLineTotalHelp, 
	       tclHdrGetLineTotalUse);

   shTclDeclare(a_interp, "hdrInsertLine",
	       (Tcl_CmdProc *) tclHdrInsertLine,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
	       tclHelpFacil, tclHdrInsertLineHelp, 
	       tclHdrInsertLineUse);

   shTclDeclare(a_interp, "hdrInsWithLogical",
	       (Tcl_CmdProc *) tclHdrInsWithLogical,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
	       tclHelpFacil, tclHdrInsWithLogicalHelp, 
	       tclHdrInsWithLogicalUse);

   shTclDeclare(a_interp, "hdrInsWithDbl",
	       (Tcl_CmdProc *) tclHdrInsWithDbl,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
	       tclHelpFacil, tclHdrInsWithDblHelp, tclHdrInsWithDblUse);
  
   shTclDeclare(a_interp, "hdrInsWithInt",
	       (Tcl_CmdProc *) tclHdrInsWithInt,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
	       tclHelpFacil, tclHdrInsWithIntHelp, tclHdrInsWithIntUse);

   shTclDeclare(a_interp, "hdrInsWithAscii",
	       (Tcl_CmdProc *) tclHdrInsWithAscii,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
	       tclHelpFacil, tclHdrInsWithAsciiHelp, 
	       tclHdrInsWithAsciiUse);

   shTclDeclare(a_interp, "hdrDelByKeyword",
	       (Tcl_CmdProc *) tclHdrDelByKeyword,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
	       tclHelpFacil, tclHdrDelByKeywordHelp, 
	       tclHdrDelByKeywordUse);

   shTclDeclare(a_interp, "hdrFree",
	       (Tcl_CmdProc *) tclHdrFree,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
	       tclHelpFacil, tclHdrFreeHelp, tclHdrFreeUse);

   shTclDeclare(a_interp, "hdrGetAsAscii",
	       (Tcl_CmdProc *) tclHdrGetAsAscii,
	       (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
	       tclHelpFacil, tclHdrGetAsAsciiHelp, tclHdrGetAsAsciiUse);

   shTclDeclare(a_interp, "hdrGetAsDbl",
               (Tcl_CmdProc *) tclHdrGetAsDbl,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrGetAsDblHelp, tclHdrGetAsDblUse);

   shTclDeclare(a_interp, "hdrGetAsInt",
               (Tcl_CmdProc *) tclHdrGetAsInt,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrGetAsIntHelp, tclHdrGetAsIntUse);

   shTclDeclare(a_interp, "hdrGetAsLogical",
               (Tcl_CmdProc *) tclHdrGetAsLogical,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrGetAsLogicalHelp, 
               tclHdrGetAsLogicalUse);

   shTclDeclare(a_interp, "hdrGetLine",
               (Tcl_CmdProc *) tclHdrGetLine,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrGetLineHelp, tclHdrGetLineUse);

   shTclDeclare(a_interp, "hdrGetLineCont",
               (Tcl_CmdProc *) tclHdrGetLineCont,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrGetLineContHelp, tclHdrGetLineContUse);

   shTclDeclare(a_interp, "hdrGetLineno",
               (Tcl_CmdProc *) tclHdrGetLineno,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrGetLinenoHelp, tclHdrGetLinenoUse);

   shTclDeclare(a_interp, "hdrDelByLine",
               (Tcl_CmdProc *) tclHdrDelByLine,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrDelByLineHelp, tclHdrDelByLineUse);

   shTclDeclare(a_interp, "hdrReplaceLine",
               (Tcl_CmdProc *) tclHdrReplaceLine,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrReplaceLineHelp, tclHdrReplaceLineUse);

   shTclDeclare(a_interp, "hdrMakeLineWithAscii",
               (Tcl_CmdProc *) tclHdrMakeLineWithAscii,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrMakeLineWithAsciiHelp,
               tclHdrMakeLineWithAsciiUse);

   shTclDeclare(a_interp, "hdrMakeLineWithDbl",
               (Tcl_CmdProc *) tclHdrMakeLineWithDbl,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrMakeLineWithDblHelp,
               tclHdrMakeLineWithDblUse);

   shTclDeclare(a_interp, "hdrMakeLineWithInt",
               (Tcl_CmdProc *) tclHdrMakeLineWithInt,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrMakeLineWithIntHelp,
               tclHdrMakeLineWithIntUse);

   shTclDeclare(a_interp, "hdrMakeLineWithLogical",
               (Tcl_CmdProc *) tclHdrMakeLineWithLogical,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrMakeLineWithLogicalHelp,
               tclHdrMakeLineWithLogicalUse);

   shTclDeclare(a_interp, "hdrCopy",
               (Tcl_CmdProc *) tclHdrCopy,
               (ClientData) 0, (Tcl_CmdDeleteProc *) 0,
               tclHelpFacil, tclHdrCopyHelp, tclHdrCopyUse);

   return;
}

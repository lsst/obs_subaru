/****************************************************************************
*****************************************************************************
**<AUTO>
** FILE:
**	tclRegUtils.c
**
**<HTML>
** <P>These region utilities will operate on regions whose
** pixels are of any supported type.  Procedures which
** take more than one region as parameters require that all
** regions have the same number of rows and columns.
**
** <H2>Arithmetic</H2>
**
** Arithmetic operations are performed "as if" all operations were
** converted to a type which has an exact representation for all the
** values of each type of pixel.
**
** <P>Before a result is stored back into a pixel the following conversions
** are performed: 1) If the result is smaller (largest) than the
** smallest (largest) value representable in a pixel, it is set to that
** small (large) value. 2)If any of the operands were a floating-point
** type, and the result is an integer type, the result is rounded down after
** 0.5 (-0.5) has been added to positive (negative) values.
**
** <H2>Rows and Columns</H2>
**
** <P>ROW and COLUMN operands which locate specific pixels in a region
** may be specified as either floating point or decimal numbers. 0.0,
** 0, 0.5, or 0.9 all refer to the row or column 0.
**</HTML>
**</AUTO>
** ABSTRACT:
**	This file contains software making TCL verbs from the
**	pure C primitives in regUtils.c. Region Programmer's
**	tools are used from tclRegSupport.c
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shTclRegUtilDeclare	Public  Declare all the extensions in this file to TCL 
**
**
** ENVIRONMENT:
**	ANSI C.
**	tclRegSupport	tcl Support routines in this package.
**	utils		Vanilla C utilities in this package.
**
** REQUIRED PRODUCTS:
**	FTCL	 	TCL + Fermi extension
**
**
** AUTHORS:     Creation date: April 28, 1992
**              Heidi Newberg
**		Don Petravick
**              Gary Sergey
**
*****************************************************************************
*****************************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <alloca.h>
#include "tcl.h"
#include "ftcl.h"
#include "region.h"
#include "shTclRegSupport.h"
#include "dervish_msg_c.h"
#include "shCRegUtils.h"
#include "shTclHandle.h"
#include "shTclParseArgv.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shCAssert.h"
#include "shTclUtils.h"
#include "shTclVerbs.h"
/*============================================================================  
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/
#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif


/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/
static char *shtclRegUtilsFacil = "shRegion";

/******************************************************************************
******************************************************************************/


static int samesize3 (REGION const *reg1, REGION const *reg2, REGION const *reg3) {

  if (reg1->nrow == reg2->nrow &&
      reg1->nrow == reg3->nrow &&
      reg1->ncol == reg2->ncol &&
      reg1->ncol == reg3->ncol) return 1;
  return 0;

}

static int samesize2 (REGION const *reg1, REGION const *reg2) {

  if (reg1->nrow == reg2->nrow &&
      reg1->ncol == reg2->ncol) return 1;
  return 0;

}

static int samerows2 (REGION const *reg1, REGION const *reg2) {

  if (reg1->nrow == reg2->nrow ) return 1;
  return 0;

}

static int samecols2 (REGION const *reg1, REGION const *reg2) {

  if (reg1->ncol == reg2->ncol ) return 1;
  return 0;

}

/*============================================================================
**============================================================================
**
**<AUTO EXTRACT>
** TCL VERB: regTypeSwitch
**
** DESCRIPTION:
**<HTML>
** Switch the pixel type of the entered region.  The type is
** switched to either the entered type (if allowed) or to the
** logically correct type.  The following type switches are allowed.
** <DIR>
** <LI>signed 8-bit data to unsigned 8-bit data and vice versa
** <LI>signed 16-bit data to unsigned 16-bit data and vice versa
** <LI>signed 32-bit data to unsigned 32-bit data and vice versa
** </DIR>
** For example, if an unsigned 8-bit region is entered and no
** type is entered, the region will have it's type switched to
** signed 8-bit.  In all cases the pixels remain untouched.
**
** <P>Regions with pixel types equal to floating point, cannot be
** switched to any other type.
**</HTML>
** ARGUMENTS:
**	<region>	Handle to region.
**	[newtype]	Type to switch pixels to.
**	
**</AUTO>	
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**============================================================================
*/
static char *shtclRegTypeSwitch_hlp =
                 "Switch the pixel type of the region to the specified type";
static char *shtclRegTypeSwitch_use =
                 "USAGE: regTypeSwitch <region> [newtype]";

static int shTclRegTypeSwitch
   (
   ClientData clientData,
   Tcl_Interp *interp,
   int argc,
   char **argv
   )
{
int typeGiven = FALSE;                    /* Assume no type entered on line */
int numArgs = 2;                          /* Minimum number of arguments */
int tStatus;
char *regName, *typeName;
REGION *regPtr;
PIXDATATYPE pixType;

/* Parse the input arguments.  They should be a region and possible a 
 * type to switch the region to.  If no type is entered, switch the region
 * pixel type to the only allowed value.  If the region has floating point
 * pixels, no switching is allowed at all.
 */ 

if (argc < numArgs)
  {
  Tcl_SetResult(interp, shtclRegTypeSwitch_use, TCL_VOLATILE);
  tStatus = TCL_ERROR;
  }
else
  {
  /* The region should be the first parameter */
  regName = argv[1];

  /* If argc > numArgs, a type was entered on the command line, else we
     should just assume we are to do the only possible pixel type switch
     which depends on the current value of the pixel type in the region. */
  if (argc > numArgs)
    {
    /* Get the entered type */
    typeName = argv[2];
    typeGiven = TRUE;

    /* Convert the ascii name to one of the supported type enums */
    if ((tStatus = shTclRegTypeGetAsEnum(interp, typeName, &pixType)) !=
        TCL_OK)
      {goto exit;}               /* We were not passed a supported type */
    }
  
  /* Check if this is valid region by trying to get the pointer to it */
  if ((tStatus = shTclRegAddrGetFromName(interp, regName, &regPtr))
                                                             == TCL_OK)
    {
    /* If no desired type was entered, we need to figure out which type
       to switch to. */
    if (!typeGiven)
      {
      switch (regPtr->type)
        {
        case TYPE_U8:
          pixType = TYPE_S8;
          break;
        case TYPE_S8:
          pixType = TYPE_U8;
          break;
        case TYPE_U16:
          pixType = TYPE_S16;
          break;
        case TYPE_S16:
          pixType = TYPE_U16;
          break;
        case TYPE_U32:
          pixType = TYPE_S32;
          break;
        case TYPE_S32:
          pixType = TYPE_U32;
          break;
        case TYPE_FL32:
          tStatus = TCL_ERROR;
          Tcl_SetResult(interp, 
         "ERROR : Region is of type FL32 and cannot be switched to anything!!",
                        TCL_VOLATILE);
          break;
        default:
	  /* We should never reach here */
          shFatal("In shTclRegTypeSwitch: The file pixel type is unknown!!");
        }
      }

    if (tStatus == TCL_OK)
      {
      /* Do the switch based on the current pixel type of the region and the
         desired new pixel type. */
      if (shRegTypeSwitch(regPtr, pixType) != SH_SUCCESS)
	{
        /* There was an error - text will be on the error stack */
        tStatus = TCL_ERROR;
	}
      }
    }  
  }

exit:
if (tStatus != TCL_OK) 
  {
  shTclInterpAppendWithErrStack(interp); /* Append errors from stack if any */
  }
return (tStatus);
}
/*============================================================================
**============================================================================
**
**<AUTO EXTRACT>
** TCL VERB: regSetWithDbl
**
** DESCRIPTION:
**<HTML>
** Sets all pixels in REGION to the value implied by DOUBLE.
** REGION may have pixels of any type.
** Type conversions are performed as described in
** <A HREF=tclRegUtils.html#Arithmetic>Arithmetic </A>.
**</HTML>
** ARGUMENTS:
**	<region>	Handle to region.
**	<double>	Floating point value to set all pixels to.
**
**</AUTO>
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**============================================================================
*/
static char *shtclRegSetWithDbl_hlp = "Set all pixels in a region using input as scaler";
static char *shtclRegSetWithDbl_use = "USAGE: regSetWithDbl <region> <double>";

static int shTclRegSetWithDbl(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   double constant;
   int status;
   REGION *reg;

   if (argc != 3) {
      Tcl_SetResult (interp, shtclRegSetWithDbl_use, TCL_STATIC);
      return (TCL_ERROR);
   }

   if (shTclRegAddrGetFromName(interp, argv[1], &reg) != TCL_OK) return (TCL_ERROR);
   if ((status=Tcl_GetDouble(interp, argv[2], &constant)) != TCL_OK) return(status);

   shRegSetWithDbl(constant, reg);

   return (TCL_OK);
}

/*============================================================================
**============================================================================
**
**<AUTO EXTRACT>
** TCL VERB: regSubtract
**
** DESCRIPTION:
**<HTML>
** This procedure subtracts REGION2  from REGION1 pixel-by-pixel.
** If regOut is present, store the result there; else store the
** result in REGION1. The regions may have pixels of any type.
** Type conversions are performed as described in
** <A HREF=tclRegUtils.html#Arithmetic>Arithmetic </A>.
**</HTML>
** ARGUMENTS:
**	<region1>	Required handle to region holding minuend pixels
**	<region2>	Required handle to region holding subtrahend pixels.
**	[-regOut]	Optional Handle to region to receive the
**			resulting pixels. If Not specified, REGION1 is used.
**
**</AUTO>
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**
**============================================================================
*/
static ftclArgvInfo regSubArgTable[] = {
  {NULL,                 FTCL_ARGV_HELP,   NULL, NULL,
   "Subtract pixels in subtrahend region from those in the minuend region\n"},
  {"<minuendRegion>",    FTCL_ARGV_STRING, NULL, NULL, "Region to subtract from"},
  {"<subtrahendRegion>", FTCL_ARGV_STRING, NULL, NULL, "Region with values to subtract"},
  {"-regOut",            FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {(char *)NULL,         FTCL_ARGV_END,    NULL, NULL, ""}
};
static char g_regSubtract[] = "regSubtract";

static int shTclRegSub(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *subtrahend, *minuend;
   char   *subtrahendName= NULL, *minuendName = NULL, *resultName = NULL;
   REGION *result = NULL;
   int status;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   /*
    * Fill in the destination arguments in the arg table
    */
   regSubArgTable[1].dst = &minuendName;
   regSubArgTable[2].dst = &subtrahendName;
   regSubArgTable[3].dst = &resultName;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regSubArgTable, 
				flags, g_regSubtract)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}
   else
      {
      /* 
       * Everything looks ok, parsing went fine 
       */
      status = TCL_ERROR;         /* Assume an error will occur */
      if (shTclRegAddrGetFromName(interp, minuendName, &minuend) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, subtrahendName, &subtrahend)
	  != TCL_OK)
	 {goto sh_exit;}

      if (resultName != NULL) {
         if (shTclRegAddrGetFromName(interp, resultName, &result) != TCL_OK)
	    {goto sh_exit;}
      }
      else {
         /* Use minuend region to store result if output region was not
	    entered */
         result = minuend;
      }

      if ( ! samesize3 (subtrahend, minuend, result)) {
         shErrStackPush("regions are of differing sizes, consider operating on subregions");
	 goto sh_exit;
      }

      shRegSub(minuend, subtrahend, result);
      status = TCL_OK;
   }

sh_exit:
  if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
  }
  return (status);
}

/*============================================================================
**============================================================================
**
**<AUTO EXTRACT>
** TCL VERB: regXOR
**
** DESCRIPTION:
**<HTML>
** This procedure does a pixel-by-pixel exclusive-OR of REGION2 with REGION1.
** If regOut is present, store the result there; else store the
** result in REGION1. The regions may have pixels of any integer type.
**</HTML>
** ARGUMENTS:
**	<region1>	Required handle to region holding pixels
**	<region2>	Required handle to region holding pixels.
**	[-regOut]	Optional Handle to region to receive the
**			resulting pixels. If Not specified, REGION1 is used.
**
**</AUTO>
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**
**============================================================================
*/
static ftclArgvInfo regXORArgTable[] = {
  {NULL,                 FTCL_ARGV_HELP,   NULL, NULL,
   "Exclusive-OR pixels in region1 with those in region2\n"},
  {"<region1>",          FTCL_ARGV_STRING, NULL, NULL, "Region"},
  {"<region2>",          FTCL_ARGV_STRING, NULL, NULL, "Region"},
  {"-regOut",            FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {(char *)NULL,         FTCL_ARGV_END,    NULL, NULL, ""}
};
static char g_regXOR[] = "regXOR";

static int shTclRegXOR(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *region2, *region1;
   char   *region2Name= NULL, *region1Name = NULL, *resultName = NULL;
   REGION *result = NULL;
   int status;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   /*
    * Fill in the destination arguments in the arg table
    */
   regXORArgTable[1].dst = &region1Name;
   regXORArgTable[2].dst = &region2Name;
   regXORArgTable[3].dst = &resultName;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regXORArgTable, 
				flags, g_regXOR)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}
   else
      {
      /* 
       * Everything looks ok, parsing went fine 
       */
      status = TCL_ERROR;         /* Assume an error will occur */
      if (shTclRegAddrGetFromName(interp, region1Name, &region1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, region2Name, &region2)
	  != TCL_OK)
	 {goto sh_exit;}

      if (resultName != NULL) {
         if (shTclRegAddrGetFromName(interp, resultName, &result) != TCL_OK)
	    {goto sh_exit;}
      }
      else {
         /* Use region1 region to store result if output region was not
	    entered */
         result = region1;
      }

      if ( ! samesize3 (region2, region1, result)) {
         shErrStackPush("regions are of differing sizes, consider operating on subregions");
	 goto sh_exit;
      }

      shRegXor(region1, region2, result);
      status = TCL_OK;
   }

sh_exit:
  if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
  }
  return (status);
}


/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	A a constant to each pixel in a region
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regAddWithDblArgTable[] = {
  {NULL,            FTCL_ARGV_HELP,   NULL, NULL,
   "Add a constant to each pixel in a region\n"},
  {"<regionIn>",    FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<constant>",    FTCL_ARGV_DOUBLE, NULL, NULL, "Constant to add to region pixels"},
  {"-regOut",       FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {(char *)NULL,    FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regAddWithDbl[]="regAddWithDbl";

static int shTclRegAddWithDbl(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   double constant;
   REGION *regIn;
   REGION *result = NULL;
   char *regionInName = NULL, *regOutName = NULL;
   int status;
   int flags = FTCL_ARGV_NO_LEFTOVERS;

   regAddWithDblArgTable[1].dst = &regionInName;
   regAddWithDblArgTable[2].dst = &constant;
   regAddWithDblArgTable[3].dst = &regOutName;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regAddWithDblArgTable, 
				flags, g_regAddWithDbl)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */
      if (shTclRegAddrGetFromName(interp, regionInName, &regIn) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
	    {goto sh_exit;}
      }
      else {
         /* Use region to store result if output region was not entered */
         result = regIn;
      }

      if ( ! samesize2 (regIn, result)) {
         shErrStackPush("regions are of differing sizes, consider operating on subregions");
         goto sh_exit;
      }

      shRegAddWithDbl(constant, regIn, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}


/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Add two regions, pixel-by-pixel
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regAddArgTable[] = {
  {NULL,            FTCL_ARGV_HELP,   NULL, NULL,
   "Add two regions, pixel by pixel\n"},
  {"<Addend1>",     FTCL_ARGV_STRING, NULL, NULL, "Contains initial pixels"},
  {"<Addend2>",     FTCL_ARGV_STRING, NULL, NULL, "Contains pixels to add to first region"},
  {"-regOut",       FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {(char *)NULL,    FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regAdd[] = "regAdd";

static int shTclUtilRegionAdd(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{

   REGION *addend1, *addend2;
   REGION *result = NULL;
   char *addend1Name = NULL, *addend2Name = NULL, *regOutName = NULL;
   int status;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regAddArgTable[1].dst = &addend1Name;
   regAddArgTable[2].dst = &addend2Name;
   regAddArgTable[3].dst = &regOutName;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regAddArgTable, 
				flags, g_regAdd)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */
      if (shTclRegAddrGetFromName(interp, addend1Name, &addend1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, addend2Name, &addend2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
	 if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
	    {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = addend1;
      }

      if ( ! samesize3 (addend1, addend2, result)) {
         shErrStackPush("regions are of differing sizes, consider operating on subregions");
	 goto sh_exit;
      }

      shRegAdd(addend1, addend2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}
/*============================================================================ 
**============================================================================
**
**
** DESCRIPTION:
**      Add a row vector to a region, pixel-by-pixel, row-by-row
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regAddRowArgTable[] = {
  {NULL,            FTCL_ARGV_HELP,   NULL, NULL,
   "Add a row vector to a region\n"},
  {"<Addend1>",     FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<RowVector>",   FTCL_ARGV_STRING, NULL, NULL, "Region that contains the row vector"},
  {"-regOut",       FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {(char *)NULL,    FTCL_ARGV_END,    NULL, NULL, ""}
};
static char g_regAddRow[] = "regAddRow";

static int shTclRegAddRow(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{

   REGION *addend1, *addend2;
   REGION *result = NULL;
   char *addend1Name = NULL, *addend2Name = NULL, *regOutName = NULL;
   int status;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regAddRowArgTable[1].dst = &addend1Name;
   regAddRowArgTable[2].dst = &addend2Name;
   regAddRowArgTable[3].dst = &regOutName;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regAddRowArgTable, 
				flags, g_regAddRow)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, addend1Name, &addend1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, addend2Name, &addend2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
	    {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = addend1;
      }

      if ( ! samesize2 (addend1, result)) {
         shErrStackPush("Input and reult regions are of differing sizes");
	 goto sh_exit;
      }

      if ( ! samecols2 (addend1, addend2)) {
         shErrStackPush("region and row have different number of columns");
	 goto sh_exit;
      } 

      if ( addend2->nrow != 1) {
         shErrStackPush("Second region is not a single row vector");
	 goto sh_exit;
      }

      shRegAddRow(addend1, addend2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}
/*============================================================================ **============================================================================
**
**
** DESCRIPTION:
**      Add a column vector to a region, pixel-by-pixel, column-by-column
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regAddColArgTable[] = {
  {NULL,            FTCL_ARGV_HELP,   NULL, NULL,
   "Add a column vector to a region\n"},
  {"<Addend1>",     FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<ColVector>",   FTCL_ARGV_STRING, NULL, NULL, "Region that contains the column vector"},
  {"-regOut",       FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {(char *)NULL,    FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regAddCol[] = "regAddCol";

static int shTclRegAddCol(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{

   REGION *addend1, *addend2;
   REGION *result = NULL;
   char *addend1Name = NULL, *addend2Name = NULL, *regOutName = NULL;
   int status;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regAddColArgTable[1].dst = &addend1Name;
   regAddColArgTable[2].dst = &addend2Name;
   regAddColArgTable[3].dst = &regOutName;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regAddColArgTable, 
				flags, g_regAddCol)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, addend1Name, &addend1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, addend2Name, &addend2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
	    {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = addend1;
      }

      if ( ! samesize2 (addend1, result)) {
         shErrStackPush("Input and result regions are of differing sizes");
	 goto sh_exit;
      }

      if ( ! samerows2 (addend1, addend2)) {
         shErrStackPush("region and column have different number of rows");
	 goto sh_exit;
      }

      if ( addend2->ncol != 1) {
         shErrStackPush("Second region is not a single column vector");
	 goto sh_exit;
      }

      shRegAddCol(addend1, addend2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}

/*============================================================================ **============================================================================
**
**
** DESCRIPTION:
**      Subtract a row vector from a region, pixel-by-pixel, row-by-row
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regSubRowArgTable[] = {
  {NULL,            FTCL_ARGV_HELP,   NULL, NULL,
   "Subtract a row vector from a region\n"},
  {"<Addend1>",     FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<RowVector>",   FTCL_ARGV_STRING, NULL, NULL, "Region that contains the row vector"},
  {"-regOut",       FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {(char *)NULL,    FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regSubRow[] = "regSubRow";

static int shTclRegSubRow(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{

   REGION *addend1, *addend2;
   REGION *result = NULL;
   char *addend1Name = NULL, *addend2Name = NULL, *regOutName = NULL;
   int status;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regSubRowArgTable[1].dst = &addend1Name;
   regSubRowArgTable[2].dst = &addend2Name;
   regSubRowArgTable[3].dst = &regOutName;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regSubRowArgTable, 
				flags, g_regSubRow)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, addend1Name, &addend1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, addend2Name, &addend2) != TCL_OK)
	 {goto sh_exit;}
      
      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
            {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = addend1;
      }

      if ( ! samesize2 (addend1, result)) {
         shErrStackPush("Input and reult regions are of differing sizes");
         goto sh_exit;
      }

      if ( ! samecols2 (addend1, addend2)) {
         shErrStackPush("region and row have different number of columns");
	 goto sh_exit;
      }

      if ( addend2->nrow != 1) {
         shErrStackPush("Second region is not a single row vector");
	 goto sh_exit;
      }

      shRegSubRow(addend1, addend2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}

/*============================================================================ **============================================================================
**
**
** DESCRIPTION:
**      Subtract a column vector from a region, pixel-by-pixel, column-by-column
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regSubColArgTable[] = {
  {NULL,            FTCL_ARGV_HELP,   NULL, NULL,
   "Subtract a column vector from a region\n"},
  {"<Addend1>",     FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<ColVector>",   FTCL_ARGV_STRING, NULL, NULL, "Region that holds the column vector"},
  {"-regOut",       FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {(char *)NULL,    FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regSubCol[] = "regSubCol";

static int shTclRegSubCol(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{

   REGION *addend1, *addend2;
   REGION *result = NULL;
   char *addend1Name = NULL, *addend2Name = NULL, *regOutName = NULL;
   int status;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regSubColArgTable[1].dst = &addend1Name;
   regSubColArgTable[2].dst = &addend2Name;
   regSubColArgTable[3].dst = &regOutName;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regSubColArgTable, 
				flags, g_regSubCol)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, addend1Name, &addend1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, addend2Name, &addend2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
            {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = addend1;
      }

      if ( ! samesize2 (addend1, result)) {
         shErrStackPush("Input and reult regions are of differing sizes");
	 goto sh_exit;
      }

      if ( ! samerows2 (addend1, addend2)) {
         shErrStackPush("region and column have different number of rows");
	 goto sh_exit;
      }

      if ( addend2->ncol != 1) {
         shErrStackPush("Second region is not a single column vector");
	 goto sh_exit;
      }

      shRegSubCol(addend1, addend2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}
/*============================================================================ **============================================================================
**
**
** DESCRIPTION:
**	Set one pixel in a region
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static char *shtclRegPixSetWithDbl_hlp     = "Set value of one pixel in a region ";
static char *shtclRegPixSetWithDbl_use     = "USAGE: regPixSetWithDbl region row col value";

static int shTclRegPixSetWithDbl(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   int status;
   REGION *reg;
   int r, c; 
   double value;
     
   if (argc != 5) {
      Tcl_SetResult (interp, shtclRegPixSetWithDbl_use, TCL_STATIC);
      return (TCL_ERROR);
   }

   if (shTclRegAddrGetFromName(interp, argv[1], &reg) != TCL_OK) return (TCL_ERROR);
   if ((status=shTclRowColStrGetAsInt(interp, argv[2], &r)) != TCL_OK) return (status);
   if ((status=shTclRowColStrGetAsInt(interp, argv[3], &c)) != TCL_OK) return (status);
   if ((status=Tcl_GetDouble(interp, argv[4], &value)) != TCL_OK) return (status);

   if (r<reg->nrow && c<reg->ncol  && r>=0 && c>=0) {
      shRegPixSetWithDbl (reg, r, c, value);
   }else{
      Tcl_SetResult (interp, "regPixSetWithDbl: row or col is out of bounds", TCL_STATIC);
      return (TCL_ERROR);
   }
   return (TCL_OK);
}


/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Get one pixel from a region. The ascii representation is
**	formatted in a way appropriate to the type of pixel in
**	a region.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static char *shtclRegPixGet_hlp     = "Get One pixel from a region ";
static char *shtclRegPixGet_use     = "USAGE: regPixGet region row col";

static int shTclRegPixGet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   int status;
   REGION *reg;
   int r, c; 

   int typesIKnow =	 TYPE_U8 | TYPE_S8| TYPE_U16| TYPE_S16| TYPE_U32| TYPE_S32 |TYPE_FL32;
   char s[30];

   if (argc != 4) {
      Tcl_SetResult (interp, shtclRegPixGet_use, TCL_STATIC);
      return (TCL_ERROR);
   }

   if (shTclRegAddrGetFromName(interp, argv[1], &reg) != TCL_OK) return (TCL_ERROR);
   if ((status=shTclRowColStrGetAsInt(interp, argv[2], &r)) != TCL_OK) return (status);
   if ((status=shTclRowColStrGetAsInt(interp, argv[3], &c)) != TCL_OK) return (status);
   if ((status=shTclRegTypeCheck(interp, reg, typesIKnow)) != TCL_OK) return(TCL_ERROR);
   if (r < reg->nrow && c< reg->ncol && r>=0 && c >=0) {
      switch (reg->type) {
	   case TYPE_U8:   sprintf (s, "%u", reg->rows_u8[r][c]);	break;
	   case TYPE_S8:   sprintf (s, "%d", reg->rows_s8[r][c]);	break;
	   case TYPE_U16:  sprintf (s, "%u", reg->rows[r][c]);		break;
	   case TYPE_S16:  sprintf (s, "%d", reg->rows_s16[r][c]);	break;
	   case TYPE_U32:  sprintf (s, "%u", reg->rows_u32[r][c]);	break;
	   case TYPE_S32:  sprintf (s, "%d", reg->rows_s32[r][c]);	break;
	   case TYPE_FL32: sprintf (s, "%f", reg->rows_fl32[r][c]);	break;
	   default:        strcpy  (s, "Unsupported pixel type");
	}
      Tcl_SetResult (interp, s, TCL_VOLATILE);         
   }else{
      Tcl_SetResult (interp, "regPixGet: row or col is out of bounds", TCL_STATIC);
      return (TCL_ERROR);
   }
   return (TCL_OK);
}

static char *shtclRegComp_hlp       = "Compare two regions pixel by pixel ";
static char *shtclRegComp_use       = "USAGE: regComp region1 region2";

static int shTclRegComp(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   int diffRow, diffCol;
   REGION *reg1, *reg2;
   char s[100];

   if (argc != 3) {	
     Tcl_SetResult (interp, shtclRegComp_use, TCL_STATIC);
     return (TCL_ERROR);
   }
   if (shTclRegAddrGetFromName(interp, argv[1], &reg1) != TCL_OK) return (TCL_ERROR);
   if (shTclRegAddrGetFromName(interp, argv[2], &reg2) != TCL_OK) return (TCL_ERROR);
   if ( ! samesize2 (reg1, reg2)) {
      Tcl_SetResult (interp, "regions are of differing sizes", 
		TCL_STATIC);
      return (TCL_ERROR);
   }

   if (shRegComp (reg1, reg2, &diffRow, &diffCol) == 0) {
      Tcl_SetResult (interp, "same", TCL_STATIC);
      return (TCL_OK);	
   }else{
      sprintf (s, "{row %d} {col %d}", diffRow, diffCol); 
      Tcl_SetResult (interp, s, TCL_VOLATILE);
      return (TCL_OK);
   }
}

/*============================================================================ 
**============================================================================
**
**
** DESCRIPTION:
**	Flip a region by rotating 90 degrees
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static char *shtclRegFlip_hlp    = "Flip a region by rotating CW 90 degrees ";
static char *shtclRegFlip_use    = "USAGE: regFlip region";

static int shTclRegFlip(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   	REGION *reg;

 	if (argc != 2) {	
      Tcl_SetResult (interp, shtclRegFlip_use, TCL_STATIC);
      return (TCL_ERROR);
   	}
   	if (shTclRegAddrGetFromName(interp, argv[1], &reg) != TCL_OK) return (TCL_ERROR);
   	reg=shRegFlip(reg);

   	/* Associate region with TCL region name */
	if (shTclRegNameSetWithAddr(interp, reg, argv[1]) != TCL_OK) return (TCL_ERROR);

   return (TCL_OK);
}
/*============================================================================ 
**============================================================================
**
**
** DESCRIPTION:
**	Flip a region by rows. 
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static char *shtclRegRowFlip_hlp    = "Flip a region by rows ";
static char *shtclRegRowFlip_use    = "USAGE: regRowFlip region";

static int shTclRegRowFlip(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *reg;

   if (argc != 2) {	
      Tcl_SetResult (interp, shtclRegRowFlip_use, TCL_STATIC);
      return (TCL_ERROR);
   }
   if (shTclRegAddrGetFromName(interp, argv[1], &reg) != TCL_OK) return (TCL_ERROR);
   shRegRowFlip(reg);

   return (TCL_OK);
}

/*============================================================================ 
**============================================================================
**
**
** DESCRIPTION:
**	Flip a region by columns. 
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static char *shtclRegColFlip_hlp    = "Flip a region by columns ";
static char *shtclRegColFlip_use    = "USAGE: regColFlip region";

static int shTclRegColFlip(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *reg;

   if (argc != 2) {	
      Tcl_SetResult (interp, shtclRegColFlip_use, TCL_STATIC);
      return (TCL_ERROR);
   }
   if (shTclRegAddrGetFromName(interp, argv[1], &reg) != TCL_OK) return (TCL_ERROR);
   shRegColFlip(reg);

   return (TCL_OK);
}

/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	If  an output region is not specified, make mult1
**	the output region.
**	Multiply each pixel in a mult1 by the constant
**	Place the result in the output region.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regMultiplyWithDblArgTable[] = {
  {NULL,               FTCL_ARGV_HELP,   NULL, NULL,
   "For each pix in regionIn: multiply by constant, result in regionOut\n"},
  {"<RegionIn>",       FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<scale constant>", FTCL_ARGV_DOUBLE, NULL, NULL, "The scale constant"},
  {"-regOut",          FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {(char *)NULL,       FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regMultiplyWithDbl[] = "regMultiplyWithDbl";

static int shTclRegMultWithDbl(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   double constant;
   REGION  *mult;
   REGION *result = NULL;
   char *multName = NULL, *regOutName = NULL;
   int status;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regMultiplyWithDblArgTable[1].dst = &multName;
   regMultiplyWithDblArgTable[2].dst = &constant;
   regMultiplyWithDblArgTable[3].dst = &regOutName;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv,
				regMultiplyWithDblArgTable, 
				flags, g_regMultiplyWithDbl))
       != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */
 
      if (shTclRegAddrGetFromName(interp, multName, &mult) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
	    {goto sh_exit;}
      }
      else {
         /* Use region to store result if output region was not entered */
         result = mult;
      }

      if ( ! samesize2 (mult, result)) {
	 shErrStackPush("regions are of differing sizes, consider operating on subregions");
	 goto sh_exit;
      }

      shRegMultWithDbl(constant, mult, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}


/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Multiply two regions, pixel-by-pixel.
**	Divide the result by scale
**	Store the result in the output vector.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regMultiplyArgTable[] = {
  {NULL,          FTCL_ARGV_HELP,   NULL, NULL,
   "Multiply two regions, pixel by pixel, divide result by scale\n"},
  {"<reg1>",      FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<reg2>",      FTCL_ARGV_STRING, NULL, NULL, "Second region"},
  {"-regOut",     FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {"-scale",      FTCL_ARGV_DOUBLE, NULL, NULL, "Scale value"},
  {(char *)NULL,  FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regMultiply[] = "regMultiply";

static int shTclRegMult(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *mult1, *mult2;
   REGION *result  = NULL;
   char *mult1Name = NULL, *mult2Name = NULL, *regOutName = NULL;
   int status;
   double scale    = 1.0;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regMultiplyArgTable[1].dst = &mult1Name;
   regMultiplyArgTable[2].dst = &mult2Name;
   regMultiplyArgTable[3].dst = &regOutName;
   regMultiplyArgTable[4].dst = &scale;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regMultiplyArgTable, 
				flags, g_regMultiply)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, mult1Name, &mult1) != TCL_OK)
	 {goto sh_exit;}
      
      if (shTclRegAddrGetFromName(interp, mult2Name, &mult2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
            {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = mult1;
      }

      if ( ! samesize3 (mult1, mult2, result)) {
         shErrStackPush("regions are of differing sizes, consider operating on subregions");
	 goto sh_exit;
      }

      shRegMult(scale, mult1, mult2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}

/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Multiply a region by a row vector, pixel-by-pixel, row-by-row.
**	Divide the result by scale
**	Store the result in the output vector.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regMultWithRowArgTable[] = {
  {NULL,          FTCL_ARGV_HELP,   NULL, NULL,
   "Multiply a region by a row, pixel by pixel, row-by-row, divide result by scale\n"},
  {"<region>",    FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<row>",       FTCL_ARGV_STRING, NULL, NULL, "Row"},
  {"-regOut",     FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {"-scale",      FTCL_ARGV_DOUBLE, NULL, NULL, "Scale value"},
  {(char *)NULL,  FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regMultWithRow[] = "regMultWithRow";

static int shTclRegMultWithRow(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *mult1, *mult2;
   REGION *result  = NULL;
   char *mult1Name = NULL, *mult2Name = NULL, *regOutName = NULL;
   int status;
   double scale    = 1.0;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regMultWithRowArgTable[1].dst = &mult1Name;
   regMultWithRowArgTable[2].dst = &mult2Name;
   regMultWithRowArgTable[3].dst = &regOutName;
   regMultWithRowArgTable[4].dst = &scale;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regMultWithRowArgTable, 
				flags, g_regMultWithRow)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, mult1Name, &mult1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, mult2Name, &mult2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
            {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = mult1;
      }

      if ( ! samesize2 (mult1, result)) {
         shErrStackPush("input and output regions are of differing sizes");
	 goto sh_exit;
      }

      if ( ! samecols2 (mult1, mult2)) {
         shErrStackPush("region and row have different number of columns");
	 goto sh_exit;
      }

      if (mult2->nrow != 1) {
         shErrStackPush("Second region is not a single row vector");
	 goto sh_exit;
      }

      shRegMultWithRow(scale, mult1, mult2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}
/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Multiply a region by a column vector, pixel-by-pixel, column-by-column.
**	Divide the result by scale
**	Store the result in the output vector.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regMultWithColArgTable[] = {
  {NULL,          FTCL_ARGV_HELP,   NULL, NULL,
   "Multiply a region by a column, pixel by pixel, column-by-column, divide result by scale\n"},
  {"<region>",    FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<Column>",    FTCL_ARGV_STRING, NULL, NULL, "Column"},
  {"-regOut",     FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {"-scale",      FTCL_ARGV_DOUBLE, NULL, NULL, "Scale value"},
  {(char *)NULL,  FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regMultWithCol[] = "regMultWithCol";

static int shTclRegMultWithCol(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *mult1, *mult2;
   REGION *result  = NULL;
   char *mult1Name = NULL, *mult2Name = NULL, *regOutName = NULL;
   int status;
   double scale    = 1.0;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regMultWithColArgTable[1].dst = &mult1Name;
   regMultWithColArgTable[2].dst = &mult2Name;
   regMultWithColArgTable[3].dst = &regOutName;
   regMultWithColArgTable[4].dst = &scale;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regMultWithColArgTable, 
				flags, g_regMultWithCol)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, mult1Name, &mult1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, mult2Name, &mult2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
            {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = mult1;
      }

      if ( ! samesize2 (mult1, result)) {
         shErrStackPush("input and output regions are of differing sizes");
	 goto sh_exit;
      }

      if ( ! samerows2 (mult1, mult2)) {
         shErrStackPush("region and column have different number of rows");
	 goto sh_exit;
      }

      if (mult2->ncol != 1) {
         shErrStackPush("Second region is not a single column vector");
	 goto sh_exit;
      }

      shRegMultWithCol(scale, mult1, mult2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}

/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	If  an output region is not specified, make regIn
**	the output region.
**	Divide each pixel in a regIn by the constant
**	Place the result in the output region.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
/*============================================================================  
**============================================================================
**
**
** DESCRIPTION:
**	Divide two regions, pixel-by-pixel.
**	Divide the result by scale
**	Store the result in the output vector.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regDivideArgTable[] = {
  {NULL,             FTCL_ARGV_HELP,   NULL, NULL,
   "Divide two regions, pixel by pixel, divide result by scale\n"},
  {"<dividendReg>",  FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<divisorReg>",   FTCL_ARGV_STRING, NULL, NULL, "Divide initial region by this region"},
  {"-regOut",        FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {"-scale",         FTCL_ARGV_DOUBLE, NULL, NULL, "Scale value"},
  {"-divZero",         FTCL_ARGV_DOUBLE, NULL, NULL, "If divide by zero, replace with value"},
  {(char *)NULL,     FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regDivide[] = "regDivide";

static int shTclRegDiv(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *div1, *div2;
   REGION *result   = NULL;
   char *div1Name = NULL, *div2Name = NULL, *regOutName = NULL;
   int status;
   double scale	    = 1.0;
   double divZero	= -123454321;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regDivideArgTable[1].dst = &div1Name;
   regDivideArgTable[2].dst = &div2Name;
   regDivideArgTable[3].dst = &regOutName;
   regDivideArgTable[4].dst = &scale;
   regDivideArgTable[5].dst = &divZero;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regDivideArgTable, 
				flags, g_regDivide)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, div1Name, &div1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, div2Name, &div2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
            {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = div1;
      }

      if ( ! samesize3 (div1, div2, result)) {
         shErrStackPush("regions are of differing sizes, consider operating on subregions");
	 goto sh_exit;
      }

      if ( divZero == -123454321) {
        shRegDiv(scale, div1, div2, result);
        status = TCL_OK;
      } else {
        shRegDivZero(divZero, scale, div1, div2, result);
        status = TCL_OK;
      }
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}
/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Divide a region by a row vector, pixel-by-pixel, column-by-column.
**	Divide the result by scale
**	Store the result in the output vector.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regDivByRowArgTable[] = {
  {NULL,             FTCL_ARGV_HELP,   NULL, NULL,
   "Divide a region by a row vector, pixel by pixel, column by column, divide result by scale\n"},
  {"<dividendReg>",  FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<divisorReg>",   FTCL_ARGV_STRING, NULL, NULL, "Divide initial region by this region"},
  {"-regOut",        FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {"-scale",         FTCL_ARGV_DOUBLE, NULL, NULL, "Scale value"},
  {(char *)NULL,     FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regDivByRow[] = "regDivByRow";

static int shTclRegDivByRow(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *div1, *div2;
   REGION *result   = NULL;
   char *div1Name = NULL, *div2Name = NULL, *regOutName = NULL;
   int status;
   double scale	    = 1.0;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regDivByRowArgTable[1].dst = &div1Name;
   regDivByRowArgTable[2].dst = &div2Name;
   regDivByRowArgTable[3].dst = &regOutName;
   regDivByRowArgTable[4].dst = &scale;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regDivByRowArgTable, 
				flags, g_regDivByRow)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, div1Name, &div1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, div2Name, &div2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
            {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = div1;
      }

      if ( ! samesize2 (div1, result)) {
         shErrStackPush("Dividend and result regions are of different sizes");
	 goto sh_exit;
      }

      if ( ! samecols2 (div1, div2)) {
         shErrStackPush("Dividend and Divisor have different number of columns");
	 goto sh_exit;
      }

      if (div2->nrow != 1) {
         shErrStackPush("Divisor is not a single row vector");
	 goto sh_exit;
      }

      shRegDivByRow(scale, div1, div2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}
/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Divide a region by a column vector, pixel-by-pixel, row-by-row.
**	Divide the result by scale
**	Store the result in the output vector.
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static ftclArgvInfo regDivByColArgTable[] = {
  {NULL,             FTCL_ARGV_HELP,   NULL, NULL,
   "Divide a region by a column vector, pixel by pixel, row by row, divide result by scale\n"},
  {"<dividendReg>",  FTCL_ARGV_STRING, NULL, NULL, "Initial region"},
  {"<divisorReg>",   FTCL_ARGV_STRING, NULL, NULL, "Divide initial region by this region"},
  {"-regOut",        FTCL_ARGV_STRING, NULL, NULL, "Holds result"},
  {"-scale",         FTCL_ARGV_DOUBLE, NULL, NULL, "Sclae value"},
  {(char *)NULL,     FTCL_ARGV_END,    NULL, NULL, (char *)NULL}
};
static char g_regDivByCol[] = "regDivByCol";

static int shTclRegDivByCol(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *div1, *div2;
   REGION *result   = NULL;
   char *div1Name = NULL, *div2Name = NULL, *regOutName = NULL;
   int status;
   double scale	    = 1.0;
   int flags      = FTCL_ARGV_NO_LEFTOVERS;

   regDivByColArgTable[1].dst = &div1Name;
   regDivByColArgTable[2].dst = &div2Name;
   regDivByColArgTable[3].dst = &regOutName;
   regDivByColArgTable[4].dst = &scale;

   /*
    * Parse arguments
    */
   if ((status = shTclParseArgv(interp, &argc, argv, regDivByColArgTable, 
				flags, g_regDivByCol)) != FTCL_ARGV_SUCCESS)
      {goto sh_exit;}

   else {
      /*
       * Everything looks ok, parsing went fine
       */
      status = TCL_ERROR;         /* Assume an error will occur */

      if (shTclRegAddrGetFromName(interp, div1Name, &div1) != TCL_OK)
	 {goto sh_exit;}

      if (shTclRegAddrGetFromName(interp, div2Name, &div2) != TCL_OK)
	 {goto sh_exit;}

      if (regOutName != NULL) {
         if (shTclRegAddrGetFromName(interp, regOutName, &result) != TCL_OK)
            {goto sh_exit;}
      }
      else {
         /* Use first region to store result if output region was not
	    entered */
         result = div1;
      }

      if ( ! samesize2 (div1, result)) {
         shErrStackPush("Dividend and result regions are of different sizes");
	 goto sh_exit;
      }

      if ( ! samerows2 (div1, div2)) {
         shErrStackPush("Dividend and Divisor have different number of rows");
	 goto sh_exit;
      }

      if (div2->ncol != 1) {
         shErrStackPush("Divisor is not a single column vector");
	 goto sh_exit;
      }

      shRegDivByCol(scale, div1, div2, result);
      status = TCL_OK;
   }

sh_exit:
   if (status != TCL_OK) {
     shTclInterpAppendWithErrStack(interp);
   }
   return (status);
}

/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	Copy one region to another pixel-by-pixel
**
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**
** CALLS TO:
**	utils.c
**	tclRegSupport.c
**============================================================================
*/
static char *shtclRegPixCopy_hlp    = 
                                    "Copy one region to another pixel-by-pixel";
static char *shtclRegPixCopy_use    = "USAGE: regPixCopy regIn regOut";

static int shTclRegPixCopy(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *regIn;
   REGION *regOut;

   if (argc != 3) {	
      Tcl_SetResult (interp, shtclRegPixCopy_use, TCL_STATIC);
      return (TCL_ERROR);
   }
   if (shTclRegAddrGetFromName(interp, argv[1], &regIn)  != TCL_OK) return (TCL_ERROR);
   if (shTclRegAddrGetFromName(interp, argv[2], &regOut) != TCL_OK) return (TCL_ERROR);

   /* Make sure the regions are of the same size */
   if (p_samesize2(regIn, regOut) == 1)
     shRegPixCopy(regIn, regOut);
   else
      {
      /* Error - regions are not the same size. */
      Tcl_SetResult(interp,
		    "shTclRegPixCopy - Both regions must be the same size\n",
		    TCL_STATIC);
      return (TCL_ERROR);
      }

   return (TCL_OK);
}

/*============================================================================
**============================================================================
**<AUTO EXTRACT>
** TCL VERB: regInfoGet
**
** DESCRIPTION:
**   Return information about a region in the form of a keyed list.
**   This procedure returns information about the public fields of
**   a region and the public state information implied by the private
**   parts of the region structure in the form of a keyed list.
**
**</AUTO>
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**============================================================================
*/
#define LCL_BUFFER_SIZE 1000
static char *shtclRegInfoGet_hlp = "Return information about a region in the form of a keyed list";
static char *shtclRegInfoGet_use = "USAGE: regInfoGet <region>";
static int shTclRegInfoGet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *reg;
   REGINFO *regInfo;
   RET_CODE rc;
   static char s[LCL_BUFFER_SIZE];
   static char ctype[20];

   if (argc != 2) {	
      Tcl_SetResult (interp, shtclRegInfoGet_use, TCL_STATIC);
      return (TCL_ERROR);
   }
   if (shTclRegAddrGetFromName(interp, argv[1], &reg) != TCL_OK) return (TCL_ERROR);

   rc = shRegInfoGet(reg, &regInfo);

   if (rc == SH_SUCCESS) {
     sprintf (s, "nrow %d ", reg->nrow); Tcl_AppendElement (interp, s);
     sprintf (s, "ncol %d ", reg->ncol); Tcl_AppendElement (interp, s);
     shTclRegTypeGetAsAscii (interp, reg->type, ctype); 
     sprintf (s, "type %s ", ctype);     Tcl_AppendElement (interp, s);
     sprintf (s, "name %s ", reg->name); Tcl_AppendElement (interp, s);
     sprintf (s, "row0 %d ", reg->row0); Tcl_AppendElement (interp, s);
     sprintf (s, "col0 %d ", reg->col0); Tcl_AppendElement (interp, s);
     sprintf (s, "modCntr %d ", regInfo->modCntr); Tcl_AppendElement (interp, s);
     sprintf (s, "isSubReg %d ", regInfo->isSubReg); Tcl_AppendElement (interp, s);
     sprintf (s, "isPhysical %d ", regInfo->isPhysical); Tcl_AppendElement (interp, s);
     sprintf (s, "physicalRegNum %d ", regInfo->physicalRegNum); Tcl_AppendElement (interp, s);
     sprintf (s, "pxAreContiguous %d ", regInfo->pxAreContiguous); Tcl_AppendElement (interp, s);
     sprintf (s, "hasHeader %d ", regInfo->hasHeader); Tcl_AppendElement (interp, s);
     sprintf (s, "headerModCntr %d ", regInfo->headerModCntr); Tcl_AppendElement (interp, s);
     sprintf (s, "nSubReg %d ", regInfo->nSubReg); Tcl_AppendElement (interp, s);
     sprintf (s, "hasMask %d ", regInfo->hasMask); Tcl_AppendElement (interp, s);
     return (TCL_OK);
   } else {
      Tcl_SetResult (interp, "Call to shRegInfoGet failed", TCL_STATIC);
      return (TCL_ERROR);
   }
}
#undef LCL_BUFFER_SIZE
/*============================================================================
**============================================================================
**<AUTO EXTRACT>
** TCL VERB: subRegGet
**
** DESCRIPTION:
**   Return a list of handles to subregions parented in this region.
**   Return an empty list if this region parents no subregions.
**
**   For each subregion, if a handle to the subregion exists, use that handle.
**   If no handle points to the subregion, create one.
**
**   This has the _awful_ side effect that the user will have to delete any 
**   new handles when finished.  A side effect of having a handle is that 
**   the underlying memory cannot be completely free-ed.
**
**</AUTO>
** RETURN VALUES:
**	TCL_OK      -   Successful completion.
**	TCL_ERROR   -   Failed miserably. Interp->result is set.
**	
**============================================================================
*/
#define LCL_BUFFER_SIZE 1000
static char *shtclSubRegGet_hlp        = "Return a list of handles to a region's sub-regions.";
static char *shtclSubRegGet_use        = "USAGE: subRegGet <region>";
static int shSubRegGet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   REGION *reg;
   REGINFO *regInfo;
   RET_CODE rc;
   int i;
   char name[HANDLE_NAMELEN+1];
   static char *list;

   if (argc != 2) {	
      Tcl_SetResult (interp, shtclSubRegGet_use, TCL_STATIC);
      return (TCL_ERROR);
   }
   if (shTclRegAddrGetFromName(interp, argv[1], &reg) != TCL_OK) return (TCL_ERROR);

   rc = shRegInfoGet(reg, &regInfo);

/* Build list of regions to be returned. Do NOT use Tcl_Append Element
 * As the interp is passed down to "handle name get" and so forth, which
 * could (and probably does) overwrites interp->result. Malloc
 * too much, just because I don't want to worry about the computation
 * of name lengths, which was buggy. N.B. we have had regions with hundreds
 * of sub regions. ALways malloc at least a byte to hold the null for empty list.
 */
   if (rc == SH_SUCCESS) {
     list = (char *)alloca ((unsigned int )(2+((HANDLE_NAMELEN+2)*regInfo->nSubReg))); list[0]=0;
     for (i = 0; i<regInfo->nSubReg; i++) {
        if (p_shTclHandleNameGet(interp,regInfo->subRegs[i]) == TCL_OK) {
	   strcpy(name, interp->result);
        }else{
          shTclHandleNew (interp, name,"REGION", regInfo->subRegs[i]);
        }
        strcat (list, name ); strcat (list, " ");
     }
     Tcl_SetResult(interp, list, TCL_VOLATILE);
     return (TCL_OK);
   } else {
      Tcl_SetResult (interp, "Call to shRegInfoGet failed", TCL_STATIC);
      return (TCL_ERROR);
   }
}
#undef LCL_BUFFER_SIZE


/*************** Declaration of TCL verbs in this module *****************/

void shTclRegUtilDeclare(Tcl_Interp *interp) 
{
int flags = 0;

   shTclDeclare (interp, "regTypeSwitch", 
                 (Tcl_CmdProc *)shTclRegTypeSwitch, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,shtclRegTypeSwitch_hlp,
                 shtclRegTypeSwitch_use);

   shTclDeclare (interp, "regSetWithDbl", 
                 (Tcl_CmdProc *)shTclRegSetWithDbl, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclRegSetWithDbl_hlp,
                 shtclRegSetWithDbl_use);

   shTclDeclare (interp, g_regSubtract, 
                 (Tcl_CmdProc *)shTclRegSub, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regSubArgTable, flags, g_regSubtract),
                 shTclGetUsage(interp, regSubArgTable, flags, g_regSubtract));

   shTclDeclare (interp, g_regXOR,
                 (Tcl_CmdProc *)shTclRegXOR,
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regXORArgTable, flags, g_regXOR),
                 shTclGetUsage(interp, regXORArgTable, flags, g_regXOR));

   shTclDeclare (interp, g_regAddWithDbl, 
                 (Tcl_CmdProc *)shTclRegAddWithDbl, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regAddWithDblArgTable, flags, g_regAddWithDbl),
                 shTclGetUsage(interp, regAddWithDblArgTable, flags, g_regAddWithDbl));

   shTclDeclare (interp, g_regAdd, 
                 (Tcl_CmdProc *)shTclUtilRegionAdd, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regAddArgTable, flags, g_regAdd),
                 shTclGetUsage(interp, regAddArgTable, flags, g_regAdd));

   shTclDeclare (interp, g_regAddRow, 
                 (Tcl_CmdProc *)shTclRegAddRow, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regAddRowArgTable, flags, g_regAddRow),
                 shTclGetUsage(interp, regAddRowArgTable, flags, g_regAddRow));

   shTclDeclare (interp, g_regAddCol, 
                 (Tcl_CmdProc *)shTclRegAddCol, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regAddColArgTable, flags, g_regAddCol),
                 shTclGetUsage(interp, regAddColArgTable, flags, g_regAddCol));

   shTclDeclare (interp, g_regSubRow, 
                 (Tcl_CmdProc *)shTclRegSubRow, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regSubRowArgTable, flags, g_regSubRow),
                 shTclGetUsage(interp, regSubRowArgTable, flags, g_regSubRow));

   shTclDeclare (interp, g_regSubCol, 
                 (Tcl_CmdProc *)shTclRegSubCol, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regSubColArgTable, flags, g_regSubCol),
                 shTclGetUsage(interp, regSubColArgTable, flags, g_regSubCol));

   shTclDeclare (interp, "regPixSetWithDbl", 
                 (Tcl_CmdProc *)shTclRegPixSetWithDbl, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclRegPixSetWithDbl_hlp,
                 shtclRegPixSetWithDbl_use);

   shTclDeclare (interp, "regPixGet", 
                 (Tcl_CmdProc *)shTclRegPixGet, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclRegPixGet_hlp,
                 shtclRegPixGet_use);

   shTclDeclare (interp, "regComp", 
                 (Tcl_CmdProc *)shTclRegComp, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclRegComp_hlp,
                 shtclRegComp_use);

   shTclDeclare (interp, "regFlip", 
                 (Tcl_CmdProc *)shTclRegFlip, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclRegFlip_hlp,
                 shtclRegFlip_use);

   shTclDeclare (interp, "regRowFlip", 
                 (Tcl_CmdProc *)shTclRegRowFlip, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclRegRowFlip_hlp,
                 shtclRegRowFlip_use);

   shTclDeclare (interp, "regPixCopy", 
                 (Tcl_CmdProc *)shTclRegPixCopy, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclRegPixCopy_hlp,
                 shtclRegPixCopy_use);

   shTclDeclare (interp, "regColFlip", 
                 (Tcl_CmdProc *)shTclRegColFlip, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclRegColFlip_hlp,
                 shtclRegColFlip_use);

   shTclDeclare (interp, g_regMultiplyWithDbl, 
                 (Tcl_CmdProc *)shTclRegMultWithDbl,
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regMultiplyWithDblArgTable, flags, g_regMultiplyWithDbl),
                 shTclGetUsage(interp, regMultiplyWithDblArgTable, flags, g_regMultiplyWithDbl));

   shTclDeclare (interp, g_regMultiply, 
                 (Tcl_CmdProc *)shTclRegMult, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regMultiplyArgTable, flags, g_regMultiply),
                 shTclGetUsage(interp, regMultiplyArgTable, flags, g_regMultiply));

   shTclDeclare (interp, g_regMultWithRow, 
                 (Tcl_CmdProc *)shTclRegMultWithRow, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regMultWithRowArgTable, flags, g_regMultWithRow),
                 shTclGetUsage(interp, regMultWithRowArgTable, flags, g_regMultWithRow));

   shTclDeclare (interp, g_regMultWithCol, 
                 (Tcl_CmdProc *)shTclRegMultWithCol, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regMultWithColArgTable, flags, g_regMultWithCol),
                 shTclGetUsage(interp, regMultWithColArgTable, flags, g_regMultWithCol));

   shTclDeclare (interp, g_regDivide, 
                 (Tcl_CmdProc *)shTclRegDiv, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regDivideArgTable, flags, g_regDivide),
                 shTclGetUsage(interp, regDivideArgTable, flags, g_regDivide));

   shTclDeclare (interp, g_regDivByRow, 
                 (Tcl_CmdProc *)shTclRegDivByRow, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regDivByRowArgTable, flags, g_regDivByRow),
                 shTclGetUsage(interp, regDivByRowArgTable, flags, g_regDivByRow));

   shTclDeclare (interp, g_regDivByCol, 
                 (Tcl_CmdProc *)shTclRegDivByCol, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil ,
		 shTclGetArgInfo(interp, regDivByColArgTable, flags, g_regDivByCol),
                 shTclGetUsage(interp, regDivByColArgTable, flags, g_regDivByCol));

   shTclDeclare (interp, "regInfoGet", 
                 (Tcl_CmdProc *)shTclRegInfoGet, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclRegInfoGet_hlp,
                 shtclRegInfoGet_use);

   shTclDeclare (interp, "subRegGet", 
                 (Tcl_CmdProc *)shSubRegGet, 
                 (ClientData) 0,
                 (Tcl_CmdDeleteProc *)NULL, 
                 shtclRegUtilsFacil , shtclSubRegGet_hlp,
                 shtclSubRegGet_use);

} 







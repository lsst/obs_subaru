/*****************************************************************************
******************************************************************************
**
** FILE:
**	tclRegSupport.c
**
** ABSTRACT:
**	This file contains routines that support the definition of TCL verbs
**	for regions.
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shTclRegAddrGetFromName	public  Returns region address address
** shTclRegNameGet		public	Returns new region name for address
** shTclRegNameSetWithAddr	public	Assign new address to old region  name
** shTclRegNameDel		public	Delete region name
** shTclRegTypeCheck		public	Check if region type match given ones
** shTclRegTypeGetAsEnum	public	Convert ascii type name to PIXDATATYPE
** shTclRegTypeGetAsAscii	public	Convert PIXDATATYPE to ascii type name 
** shTclRowColStrGetAsInt	public	Convert row or col string to integer
** shTclRowColStrGetAsFloat	public	Convert row or col string to float
** shTclRegSupportInit		public	Initialize TCL for region support
**
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	ftcl
**
** AUTHORS:
**	Creation date:  May 7, 1993
**	Penelope Constanta-Fanourakis
**
******************************************************************************
******************************************************************************
*/
/*============================================================================

**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/

/*----------------------------------------------------------------------------
**
** LOCAL DEFINITIONS
*/

/*----------------------------------------------------------------------------
**
** GLOBAL VARIABLES
*/

#include <stdlib.h>
#include <string.h>
#include "shTclRegSupport.h"
#include "tclExtend.h"
#include "shTclHandle.h"
#include "shCErrStack.h"


/*============================================================================x
**============================================================================
**
** ROUTINE: shTclRegAddrGetFromName
**
** DESCRIPTION:
**	Returns a region address given a region name
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	Tcl_HandleXlate	
**	Tcl_AppendResult
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRegAddrGetFromName
   (
   Tcl_Interp *a_interp,
   char const *a_regionName,
   REGION     **a_region
   )   
{
   HANDLE hand;
   void   *handPtr; /*???*/
   char   *l_regionName= (char *)a_regionName;
/*
** Find region address
*/
   if (shTclHandleExprEval(a_interp, l_regionName, &hand, &handPtr) != TCL_OK) {
      return (TCL_ERROR);
   }
   if(hand.type != shTypeGetFromName("REGION")) {
      Tcl_AppendResult(a_interp,l_regionName," is a ",
		   shNameGetFromType(hand.type)," not a REGION",(char *)NULL);
      return(TCL_ERROR);
   }
/*
**	Return region address
*/
   *a_region = (REGION *)hand.ptr;
   return(TCL_OK);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegNameGet
**
** DESCRIPTION:
**	Returns a new region name for the given region address
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	Tcl_HandleAlloc
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRegNameGet
   (
   Tcl_Interp   *a_interp,
   char         *a_regionName
   )   
{
   return(p_shTclHandleNew(a_interp,a_regionName));
}

/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegNameSetWithAddr
**
** DESCRIPTION:
**	Associates given address with an existing region name
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	Tcl_HandleXlate
**	Tcl_AppendResult
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRegNameSetWithAddr
   (
   Tcl_Interp   *a_interp,
   REGION       *a_region,
   char   const *a_regionName
   )   
{
   HANDLE hand;

   hand.type = shTypeGetFromName("REGION");
   hand.ptr = a_region;
   return(p_shTclHandleAddrBind(a_interp,hand,a_regionName));
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegNameDel
**
** DESCRIPTION:
**	Deletes a region handle without deleting the region
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	Tcl_HandleXlate
**	Tcl_AppendResult
**	Tcl_HandleFree
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRegNameDel
   (
   Tcl_Interp *a_interp,
   char const *a_regionName
   )   
{
   if(p_shTclHandleDel(a_interp,a_regionName) != TCL_OK) {
      Tcl_AppendResult(a_interp,"\n-----> Unknown region: ", 
                       a_regionName, (char *) NULL);
      return(TCL_ERROR);
   }
   return(TCL_OK);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegTypeCheck
**
** DESCRIPTION:
**	It checks if the type of the region corresponds to one in the
**	specified list.
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRegTypeCheck
   (
   Tcl_Interp   *a_interp,
   REGION       *a_region,
   int          a_typeMask
   )   
{
/*
**	If region type matches given mask return TCL_OK else TCL_ERROR
*/
   if (a_region->type && a_typeMask){
      return(TCL_OK);
      }
   else{
      Tcl_AppendResult(a_interp, "\n-----> Unknown region type ", 
                                  (char *) NULL);
      return(TCL_ERROR);
      }
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegTypeGetAsEnum
**
** DESCRIPTION:
**	Convert pixel ascii type to corresponding PIXDATATYPE enum
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRegTypeGetAsEnum
   (
   Tcl_Interp  *a_interp,
   char const  *a_typeName,
   PIXDATATYPE *a_type
   )   
{
/*
**	Compare existing types with given string
*/
   if (strcmp (a_typeName, "U8") == 0) {
      *a_type = TYPE_U8;
      return(TCL_OK);
   }

   if (strcmp (a_typeName, "S8") == 0) {
      *a_type = TYPE_S8;
      return(TCL_OK);
   }

   if (strcmp (a_typeName, "U16") == 0) {
      *a_type = TYPE_U16;
      return(TCL_OK);
   }

   if (strcmp (a_typeName, "S16") == 0) {
      *a_type = TYPE_S16;
      return(TCL_OK);
   }

   if (strcmp (a_typeName, "U32") == 0) {
      *a_type = TYPE_U32;
      return(TCL_OK);
   }

   if (strcmp (a_typeName, "S32") == 0) {
      *a_type = TYPE_S32;
      return(TCL_OK);
   }

   if (strcmp (a_typeName, "FL32") == 0) {
      *a_type = TYPE_FL32;
      return(TCL_OK);
   }
/*
**	At this point given type does not match any type. Return error
*/
   Tcl_AppendResult(a_interp, "\n-----> Unknown region pixel type: ", 
                    a_typeName, " (U8,S8,U16,S16,U32,S32,or FL32 required)\n", 
                    (char *) NULL);
   return (TCL_ERROR);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegTypeGetAsAscii
**
** DESCRIPTION:
**	Convert PIXDATATYPE to the corresponding ascii name type
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRegTypeGetAsAscii
   (
   Tcl_Interp  *a_interp,
   PIXDATATYPE a_type,
   char        *a_typeName
   )   
{
   switch (a_type) {
      case TYPE_U8:
                strcpy (a_typeName, "U8");
                return(TCL_OK);

      case TYPE_S8:
                strcpy (a_typeName, "S8");
                return(TCL_OK);

      case TYPE_U16:
                strcpy (a_typeName, "U16");
                return(TCL_OK);

      case TYPE_S16:
                strcpy (a_typeName, "S16");
                return(TCL_OK);

      case TYPE_U32:
                strcpy (a_typeName, "U32");
                return(TCL_OK);

      case TYPE_S32:
                strcpy (a_typeName, "S32");
                return(TCL_OK);

      case TYPE_FL32:
                strcpy (a_typeName, "FL32");
                return(TCL_OK);

      default:
                Tcl_AppendResult(a_interp, "\n-----> Unknown region pixel type", 
                                 (char *) NULL);
                return (TCL_ERROR);
   }
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRowColStrGetAsInt
**
** DESCRIPTION:
**	Convert a row or col value string to integer.
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	Tcl_GetInt
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRowColStrGetAsInt
   (
   Tcl_Interp *a_interp,
   char       *a_RowOrCol,
   int        *a_val
   )   
{
   int    rstat;
   float  floater = 0;

   rstat = Tcl_GetFloat(a_interp, a_RowOrCol, &floater);
   *a_val = (int)floater;
   return (rstat);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRowColStrGetAsFloat
**
** DESCRIPTION:
**	Convert a row or col value string to Float.
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	Tcl_GetDouble
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRowColStrGetAsFloat
   (
   Tcl_Interp *a_interp,
   char       *a_RowOrCol,
   float      *a_val
   )   
{
   double dval;
   
   if (Tcl_GetDouble(a_interp, a_RowOrCol, &dval) != TCL_OK) {
      return (TCL_ERROR);
      }
   *a_val = (float)dval;

   return (TCL_OK);
}


/*============================================================================  
**============================================================================
**
** ROUTINE: shTclRegSupportInit
**
** DESCRIPTION:
**	Initialization routine for tcl help support.
**
** RETURN VALUES:
**	TCL_OK		success
**	TCL_ERROR	error
**
** CALLS TO:
**	Tcl_
**
** GLOBALS REFERENCED:
**
**============================================================================
*/
int shTclRegSupportInit
   (
   Tcl_Interp *a_interp
   )   
{
/*
**	Initialize the ftclHelp routine support
*/
   if (ftclCreateHelp(a_interp) != TCL_OK) {
      Tcl_AppendResult(a_interp, "\n-----> Cannot initialize TCL HELP", 
                                  (char *) NULL);
   }
   return (TCL_OK);
}

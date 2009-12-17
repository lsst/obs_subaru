#ifndef SHTCLREGSUPPORT_H
#define SHTCLREGSUPPORT_H
/*****************************************************************************
******************************************************************************
**
** FILE:
**      shTclRegSupport.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the dervish TCL region support routines 
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: May 07, 1993
**
** CVS file: $Source: /cvs/sdss/dervish/include/shTclRegSupport.h,v $
** Version: $Revision: 1.18 $    Last Modified: $Date: 2002/08/05 19:17:10 $
**
******************************************************************************
******************************************************************************
*/

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/
#include "ftcl.h"
#include "region.h"

#define MAXTCLREGNAMELEN   255	/* Maximum length of TCL region name */

/*----------------------------------------------------------------------------
**
** STRUCTURE DEFINITIONS
*/

/* 
** The user MUST ENSURE that help and prototype are static
*/

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

/* 
** I have a name, please give me the corresponding address
*/
int shTclRegAddrGetFromName (Tcl_Interp *interp, 
                                char const *RegionName, 
                                REGION     **region); 

/*
** Please give me a new region name
*/
int shTclRegNameGet	(Tcl_Interp   *interp, 
                                char         *regionName);

/*
** Please associate this address with the exisiting name
*/
int shTclRegNameSetWithAddr(Tcl_Interp   *interp, 
                                   REGION       *region, 
                                   char   const *regionName);

/*
** Please delete this region name
*/
int shTclRegNameDel  (Tcl_Interp *interp, 
                             char const *regionName);

/*
** Check to see if this region is one of the types I have specified
*/
int shTclRegTypeCheck (Tcl_Interp   *interp, 
                           REGION       *region, 
                           int          AllowedTypes);

/*
** Name of region pixel to the corresponding enum
*/
int shTclRegTypeGetAsEnum  (Tcl_Interp  *interp, 
                          char const  *typeName,
                          PIXDATATYPE *type);

/*
** Name of region pixel to the corresponding enum
*/
int shTclRegTypeGetAsAscii (Tcl_Interp  *interp, 
                          PIXDATATYPE type,
                          char        *typeName);

/*
** row or col to int
*/
int shTclRowColStrGetAsInt  (Tcl_Interp *interp,  
                             char       *RoworCol, 
                             int        *val);

/*
** row or col to float
*/
int shTclRowColStrGetAsFloat  (Tcl_Interp *interp,  
                               char       *RoworCol, 
                               float      *val);

int shTclRegSupportInit (Tcl_Interp *interp);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif

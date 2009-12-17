#ifndef SHTCLVERBS_H
#define SHTCLVERBS_H

/*
 * All dervish TCL verb prototypes
 */
/*****************************************************************************
******************************************************************************
**
** FILE:
**      shTclVerbs.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the routines to implement dervish TCL verbs.
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: May 07, 1993
**
******************************************************************************
******************************************************************************
*/

#include "shTclSao.h"

/*----------------------------------------------------------------------------
**
** prototypes
*/

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

RET_CODE shMainTcl_Declare(CMD_HANDLE *);
void   shTclDemoDeclare(Tcl_Interp *a_interp);
void   shTclDiskioDeclare(Tcl_Interp *a_interp);
void   shTclFitsIoDeclare(Tcl_Interp *a_interp);
void p_shTclFitsDeclare(Tcl_Interp *a_interp);
void p_shTclTblDeclare(Tcl_Interp *a_interp);
void   shTclHandleDeclare(Tcl_Interp *a_interp);
void   shTclRegUtilDeclare(Tcl_Interp *a_interp);
void   shTclRegionDeclare(Tcl_Interp *a_interp);
void   shTclMaskDeclare(Tcl_Interp *a_interp);
void   shTclHdrDeclare(Tcl_Interp *a_interp);
void   shTclRegSciDeclare(Tcl_Interp *a_interp);
void   shTclPgplotDeclare(Tcl_Interp *interp);
void   shTclUtilsDeclare(Tcl_Interp *a_interp);
void   shTclContribDeclare(Tcl_Interp *interp);
void   shTclErrStackDeclare(Tcl_Interp *interp);
void   shTclRegPrintDeclare(Tcl_Interp *interp);
void   shTclLutDeclare(Tcl_Interp *interp);
void   shTclTreeDeclare(Tcl_Interp *interp);
void   shTclLinkDeclare(Tcl_Interp *interp);
void   shTclTblColSchemaConversionDeclare(Tcl_Interp *interp);
void   shTclSchemaTransDeclare(Tcl_Interp *interp);
void   shTclHgDeclare(Tcl_Interp *interp, CMD_HANDLE *);
void p_shTclAlignDeclare(Tcl_Interp *interp);
void p_shTclArrayDeclare(Tcl_Interp *interp);
void   shTclChainDeclare(Tcl_Interp *interp);
void   shTclMaskUtilDeclare(Tcl_Interp *interp);
void   shTclParamIncDeclare(Tcl_Interp *interp);
void   shTclSaoPanDeclare(Tcl_Interp *interp);
/*
 * Uncomment this when Steve Kent's chain contributions are rolled into
 * dervish
 *
 *       void   shTclChainOpsDeclare(Tcl_Interp *interp);
 *
 * - vijay
 */
void   tclSaoDeclare(Tcl_Interp *);
void   shTclSaoDisplayDeclare(Tcl_Interp *);
void   shTclSaoMouseDeclare(Tcl_Interp *);
void   shTclSaoRegionDeclare(Tcl_Interp *);
void   shTclSaoDrawDeclare(Tcl_Interp *);
void   shTclSaoMaskDisplayDeclare(Tcl_Interp *);
void   shTclRegSimDeclare(Tcl_Interp *);
void   shTclVectorDeclare(Tcl_Interp *);
void   shTclMemDeclare(Tcl_Interp *);
void   shTclVectorChainDeclare(Tcl_Interp *);
void   shTclSEvalDeclare(Tcl_Interp *);
void   shTclDs9Declare(Tcl_Interp *);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif


#ifndef DERVISH_H
#define DERVISH_H
/*
 * These two symbols give the version number of the _next_ release,
 * and should be a pair of numbers
 * 
 * They should NOT be used to report the dervish version (that comes from cvs
 * as $Name: v8_21 $), but is useful in derived products such as photo, when
 * wondering whether a certain feature is present in dervish, e.g.
#if DERVISH_VERSION >= 6 && DERVISH_MINOR_VERSION > 8
   code that uses shMemInconsistencyCB
#endif
 */
#define DERVISH_VERSION 7
#define DERVISH_MINOR_VERSION 8

/*****************************************************************************
******************************************************************************
**
** FILE:
**      dervish.h
**
** ABSTRACT:
**      This file is a higher level include file that in turn includes all
**      lower level include files.
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
#include "tclExtend.h"
#include "ftcl.h"
#include "dervish_msg_c.h"
#include "region.h"
#include "shC.h"
#include "shChain.h"
#include "shCUtils.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shCArray.h"
#include "shCSchemaCnv.h"
#include "shCSchemaTrans.h"
#include "shCSchemaSupport.h"
#include "shCHdr.h"
#include "shCFitsIo.h"
#include "shCFits.h"
#include "shCTbl.h"
#include "shCMaskUtils.h"
#include "shCRegUtils.h"
#include "shCRegPrint.h"
#include "shCHash.h"
#include "shCAssert.h"
#include "shCEnvscan.h"
#include "shTclRegSupport.h"
#include "shCSao.h"
#include "shTclParseArgv.h"
#include "shTclVerbs.h"
#include "shTclUtils.h"
#include "shTk.h"  
#include "shTclHandle.h"
#include "shCGarbage.h"
#include "shCHg.h"
#include "shCLut.h"
#include "shCVecExpr.h"
#include "shCVecChain.h"
#include "shCSchema.h"
#include "imtool.h"
#include "shCAlign.h"
#include "shCDemo.h"
#include "shCDiskio.h"
#include "shCHist.h"
#include "shCLink.h"
#include "shCList.h"
#include "shCMask.h"
#include "shCMemory.h"
#include "shCRegPhysical.h"
#include "shTclAlign.h"
#include "shTclArray.h"
#include "shTclFits.h"
#include "shTclHdr.h"
#include "shTclSao.h"
#include "shTclSupport.h"
#include "shTclTree.h"
#endif




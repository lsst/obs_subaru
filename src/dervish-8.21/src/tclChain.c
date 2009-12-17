/*
 * tclChain.c - TCL extensions for Dervish Chains.
 *
 * Vijay K. Gurbani
 * (C) 1994 All Rights Reserved
 * Fermi National Accelerator Laboratory
 * Batavia, Illinois USA
 *
 * ----------------------------------------------------------------------------
 *
 * FILE:
 *   tclChain.c
 *
 * ABSTRACT:
 *   This file contains routines to provide TCL extensions for Dervish 
 *   Chains. For a complete description of chains, please refer to the
 *   file shChain.c
 */

#include <stdlib.h>
#include <string.h>

#include "ftcl.h"
#include "shTclErrStack.h"
#include "shCSchema.h"
#include "shChain.h"
#include "shTclHandle.h"
#include "shTclParseArgv.h"
#include "shTclUtils.h"
#include "shCGarbage.h"
#include "shCVecExpr.h"
#include "prvt/shGarbage_p.h"

/*
 * Local TCL function prototypes. These functions are called in response
 * to the TCL extensions entered by the user 
 */
static int tclChainNew(ClientData, Tcl_Interp *, int, char **);
static int tclChainDel(ClientData, Tcl_Interp *, int, char **);
#if 0
static int tclChainDestroy(ClientData, Tcl_Interp *, int, char **);
#endif
static int tclChainElementAddByPos(ClientData, Tcl_Interp *, int, char **);
static int tclChainElementTypeGetByPos(ClientData, Tcl_Interp *, int, char **);
static int tclChainElementGetByPos(ClientData, Tcl_Interp *, int, char **);
static int tclChainElementRemByPos(ClientData, Tcl_Interp *, int, char **);
static int doTheJob(char *, ftclArgvInfo *, char *, 
                    void *(*)(CHAIN *, unsigned int), ClientData,
                    Tcl_Interp *, int, char **);
static int tclChainTypeSet(ClientData, Tcl_Interp *, int, char **);
static int tclChainTypeGet(ClientData, Tcl_Interp *, int, char **);
static int tclChainSize(ClientData, Tcl_Interp *, int, char **);
static int tclChainJoin(ClientData, Tcl_Interp *, int, char **);
static int tclChainCopy(ClientData, Tcl_Interp *, int, char **);
static int tclChainTypeDefine(ClientData, Tcl_Interp *, int, char **);
static int tclChainCursorNew(ClientData, Tcl_Interp *, int, char **);
static int tclChainCursorDel(ClientData, Tcl_Interp *, int, char **);
static int tclChainCursorCount(ClientData, Tcl_Interp *, int, char **);
static int tclChainCursorSet(ClientData, Tcl_Interp *, int, char **);
static int tclChainElementAddByCursor(ClientData, Tcl_Interp *, int, char **);
static int tclChainElementTypeGetByCursor(ClientData, Tcl_Interp *, int, 
                                          char **);
static int tclChainElementRemByCursor(ClientData, Tcl_Interp *, int, char **);
static int tclChainWalk(ClientData, Tcl_Interp *, int, char **);
static int tclChainSort(ClientData, Tcl_Interp *, int, char **);
static int tclChainCompare(ClientData, Tcl_Interp *, int, char **);
static int tclChainCursorCopy(ClientData, Tcl_Interp *, int, char **);

/*
 * ROUTINE: tclChainNew()
 *
 * DESCRIPTION:
 *   tclChainNew() creates a new chain object and returns a handle bound to it.
 *   This routine is called in response to the TCL command "chainNew"
 *
 * CALL:
 *   (static int) tclChainNew(ClientData a_clientData, Tcl_Interp *a_interp,
 *                int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainNew = "chainNew";
static ftclArgvInfo g_chainNewTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Create a chain of specified type. One can also create\n"
    "a heterogeneous chain by setting TYPE to GENERIC\n" },
   {"<type>", FTCL_ARGV_STRING, NULL, NULL,
    "Type of chain (MASK, REGION, STAR, GENERIC, etc.)" },
   {"[n]", FTCL_ARGV_INT, NULL, NULL,
    "Number of elements to pre-allocate and install on chain" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int tclChainNew(ClientData a_clientData, Tcl_Interp *a_interp,
                       int a_argc, char **a_argv)
{
   HANDLE handle;
   char   handleBuf[HANDLE_NAMELEN],
          *pType;
   int    retStatus,
          flags;
   int    pNum=0;

   pType = NULL;
   flags = FTCL_ARGV_NO_LEFTOVERS;
   g_chainNewTbl[1].dst  = &pType;
   g_chainNewTbl[2].dst  = &pNum;

   if ((retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
              g_chainNewTbl, flags, g_chainNew)) != FTCL_ARGV_SUCCESS) {
       return(retStatus);
   }

   if (pNum > 0) {
     handle.ptr  = shChainNewBlock(pType, pNum);
   } else {
     handle.ptr  = shChainNew(pType);
   }
   handle.type = shTypeGetFromName("CHAIN");

   if (p_shTclHandleNew(a_interp, handleBuf) != TCL_OK)  {
       Tcl_SetResult(a_interp, "chainNew: cannot obtain a handle for chain",
                     TCL_VOLATILE);
       return TCL_ERROR;
   }

   if (p_shTclHandleAddrBind(a_interp, handle, handleBuf) != TCL_OK)  {
       Tcl_SetResult(a_interp, 
          "chainNew: cannot bind CHAIN address to the new chain handle",
                     TCL_VOLATILE);
       return TCL_ERROR;
   }

   Tcl_SetResult(a_interp, handleBuf, TCL_VOLATILE);

   return TCL_OK;
}

#if 0
/*
 * **** NOTE ****
 *
 * This proc is not available, as it is unable to call shChainDestroy
 * as there's no way to get the address of the type's destructor. There
 * is a version written in tcl in shTools.tcl
 *
 * **** END NOTE ****
 *
 * ROUTINE: tclChainDestroy()
 *
 * DESCRIPTION:
 *   tclChainDestroy() deletes an existing chain and all objects on the chain.
 *   Contrast this with tclChainDel() which only deletes the chain and all
 *   it's elements, leaving the objects intact.
 *
 * CALL:
 *   (static int) tclChainDestroy(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Pointer to a memory area passed by TCL to tclChainDel()
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS: 
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise
 *   a_interp->result will contain the reason for this error.
 */
static char *g_chainDestroy = "chainDestroy";
static ftclArgvInfo g_chainDestroyTbl[] = {
  { NULL, FTCL_ARGV_HELP, NULL, NULL, 
    "Delete a chain and all the objects contained on it"},
  { "<chain>", FTCL_ARGV_STRING, NULL, NULL, 
    "Handle bound to the chain to be deleted"},
  { "<deleteProc>", FTCL_ARGV_STRING, NULL, NULL, 
    "TCL procedure to delete each object on the chain"},
  { NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};
static int
tclChainDestroy(ClientData a_clientData, Tcl_Interp *a_interp, int a_argc,
                char **a_argv)
{
   HANDLE    hChain;
   CHAIN    *pChain;
   char     *pChainHandle, *pTCLProc;
   char      tmpBuf[80];
   int       flags, retStatus;
   unsigned  int i;
   void     *pTemp;

   pChainHandle = NULL;
   flags = FTCL_ARGV_NO_LEFTOVERS;
   g_chainDestroyTbl[1].dst = &pChainHandle;
   g_chainDestroyTbl[2].dst = &pTCLProc;

   if ((retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
            g_chainDestroyTbl, flags, g_chainDestroy)) != FTCL_ARGV_SUCCESS)
        return retStatus;

   if (shTclHandleExprEval(a_interp, pChainHandle, &hChain, &pTemp)!= TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, "chainDestroy: handle ", pChainHandle, 
          " is a ", shNameGetFromType(hChain.type), ", not a CHAIN", 
          (char *) NULL);
       return TCL_ERROR;
   }

   pChain = (CHAIN *)hChain.ptr;
   if (pChain->type == shTypeGetFromName("GENERIC"))  {
       Tcl_AppendResult(a_interp, "chainDestroy: ", pChainHandle, 
          " is a GENERIC chain. Cannot destroy such a chain (yet).",
          (char *) NULL);
       return TCL_ERROR;
   }

   /*
    * Now, go through the chain deleting each object on it using the TCL
    * procedure passed in. Unfortunately, we cannot call the C level API
    * shChainDestroy() since at the TCL level, we do not have a pointer to
    * the function that will destroy each object. All we have is a TCL 
    * procedure name. 
    *
    * When an object is added to a chain, it's reference count is bumped up.
    * shChainDestroy() decrements the reference count appropriately. But in 
    * this case, since we are not calling shChainDestroy(), we have to 
    * manually decrement the reference count of each object. Note that we
    * call the TCL procedure to destroy the object only if it's reference
    * count is 1. 
    *
    * Also , remember, objects on the chain should have a handle
    * bound to them. That handle is used as a parameter to the TCL procedure.
    * If the object on the chain does not have a handle bound to it, then
    * bind a transient handle to it.
    */
   retStatus = TCL_OK;
   for (i = 0; i < shChainSize(pChain); i++)  {
        pTemp = shChainElementGetByPos(pChain, i);

        if (p_shTclHandleNameGet(a_interp, pTemp) == TCL_ERROR)  {
            /*
             * Object does not have a handle bound to it. Create a new handle
             * and bind it to the object
             */
            char   handleBuf[HANDLE_NAMELEN];
            HANDLE hndl;

            if (p_shTclHandleNew(a_interp, handleBuf) != TCL_OK)  {
                Tcl_AppendResult(a_interp, 
                   "chainDestroy: error creating a new handle", (char *) NULL);
                retStatus = TCL_ERROR;
                break;
            }

            hndl.ptr  = pTemp;
            hndl.type = shChainElementTypeGetByPos(pChain, i);

            if (p_shTclHandleAddrBind(a_interp, hndl, handleBuf) == TCL_OK)
                Tcl_SetResult(a_interp, handleBuf, TCL_VOLATILE);
            else  {
                Tcl_AppendResult(a_interp, 
                   "chainDestroy: error binding address to a transient handle",
                   (char *) NULL);
                retStatus = TCL_ERROR;
                break;
            }
        }

        shMemRefCntrDecr(pTemp);
        if (p_shMemRefCntrGet(pTemp) == 1)  {
            /*
             * If a reference count of an object is 1, in all probability that
             * is because it has a handle bound to it. So it's safe to call
             * the object's destructor, which will destroy the object and the
             * handle bound to it.
             */
            sprintf(tmpBuf, "%s %s", pTCLProc, a_interp->result);
            if (Tcl_Eval(a_interp, tmpBuf) == TCL_ERROR)  {
                Tcl_AppendResult(a_interp, "chainDestroy: error executing the"
                   " following command: ", tmpBuf, "\n", (char *) NULL);
                retStatus = TCL_ERROR;
                break;
            }
        }
   }

   if (retStatus == TCL_OK)  {
       Tcl_ResetResult(a_interp);

       /*
        * May have to write the following function:
        p_shChainShallowDel(pChain);
       */
       if (p_shTclHandleDel(a_interp, pChainHandle) != TCL_OK)  {
           retStatus = TCL_ERROR;
           Tcl_AppendResult(a_interp, "chainDestroy: cannot delete handle \"",
              pChainHandle, "\"\n", (char *) NULL);
       }
   }

   return retStatus;
}
#endif

/*
 * ROUTINE: tclChainDel()
 *
 * DESCRIPTION:
 *   tclChainDel() deletes an existing chain. Note that it deletes the CHAIN
 *   itself and all the CHAIN_ELEMs on it. It DOES NOT delete each element on
 *   a CHAIN_ELEM. It is the user's responsibility to get a handle to each
 *   element on the chain before calling this TCL expression.
 *
 * CALL:
 *   (static int) tclChainDel(ClientData a_clientData, Tcl_Interp *a_interp,
 *                int a_argc, char **a_argv)
 *   a_clientData - Pointer to a memory area passed by TCL to tclChainDel()
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainDel = "chainDel";
static ftclArgvInfo g_chainDelTbl[] = {
       {NULL, FTCL_ARGV_HELP, NULL, NULL,
        "Delete a CHAIN and all elements on it\n" },
       {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
        "Handle bound to the chain to be deleted" },
       {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

static int
tclChainDel(ClientData a_clientData, Tcl_Interp *a_interp,
            int a_argc, char **a_argv)
{
   HANDLE hChain;
   char   *pHandleName;
   int    flags,
          retStatus;
   void   *pUnused;

   pHandleName = NULL;
   flags = FTCL_ARGV_NO_LEFTOVERS;
   g_chainDelTbl[1].dst  = &pHandleName;

   if ((retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, g_chainDelTbl, 
        flags, g_chainDel)) != FTCL_ARGV_SUCCESS) {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pHandleName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainDel: handle ", pHandleName, " is a ", 
          shNameGetFromType(hChain.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }

   shChainDel((CHAIN *)hChain.ptr);
  
   if (p_shTclHandleDel(a_interp, pHandleName) != TCL_OK)  {
       Tcl_AppendResult(a_interp, "chainDel: cannot delete handle name ", 
          pHandleName, (char *)NULL);
       return TCL_ERROR;
   }
  
   return TCL_OK;
}

/*
 * ROUTINE: tclChainElementAddByPos()
 *
 * DESCRIPTION:
 *   tclChainElementAddByPos() inserts an element on a chain. It accepts
 *   5 command line arguments: <CHAIN> <ELEMENT> <TYPE> [n] [how]; i.e.
 *   <ELEMENT> is inserted on <CHAIN> relative to position [n] and specified
 *   by [how]. The last two arguments can be defaulted. Typically, [n] is an
 *   integer signifying the position in the chain to add the element at. It
 *   can between 1 and number of elements on the chain (inclusive). For ease
 *   of use, it can be one of the defined constants HEAD or TAIL. Default is
 *   TAIL.
 *   [HOW] specifies how to add the element, i.e. should it be added before
 *   (BEFORE) the nth element of after (AFTER).
 *
 * CALL:
 *   (static int) tclChainElementAddByPos(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainElementAddByPos = "chainElementAddByPos";
static ftclArgvInfo g_chainElementAddByPosTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Insert an element on a chain\n" },
   {"<CHAIN>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle of the chain" },
   {"<ELEMENT>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle of the element to be added" },
   {"[n]", FTCL_ARGV_STRING, NULL, NULL,
    "New element is added in relation to element at this position" },
   {"[how]", FTCL_ARGV_STRING, NULL, NULL,
    "How to add the new element in relation to the nth element" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainElementAddByPos(ClientData a_clientData, Tcl_Interp *a_interp,
                        int a_argc, char **a_argv)
{
   char *pChainName, *pElementName, *pWhere, *pHow, *sp;
   int  flags;
   int  rstatus;
   TYPE type;
   RET_CODE retStatus;
   CHAIN_ADD_FLAGS how;
   HANDLE hChain, hElement;
   CHAIN *pChain;
   void  *pUnused;
   unsigned int where;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   pChainName = pElementName = NULL;
   pWhere = "TAIL";
   pHow = "AFTER";

   g_chainElementAddByPosTbl[1].dst  = &pChainName;
   g_chainElementAddByPosTbl[2].dst  = &pElementName;
   g_chainElementAddByPosTbl[3].dst  = &pWhere;
   g_chainElementAddByPosTbl[4].dst  = &pHow;

   if ((rstatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
         g_chainElementAddByPosTbl, flags, g_chainElementAddByPos)) 
       != FTCL_ARGV_SUCCESS)  {
       return(rstatus);
   }

   /*
    * Some error checking
    */
   if (strncmp(pWhere, "TAIL", 4) == 0)
       where = TAIL;
   else if (strncmp(pWhere, "HEAD", 4) == 0)
       where = HEAD;
   else  {
       where = (int) strtol(pWhere, &sp, 0);
       if (sp == pWhere || *sp != 0)  {
           Tcl_SetResult(a_interp, 
              "chainElementAddByPos: [n] MUST be one of HEAD, TAIL or an"
              " integer", TCL_VOLATILE);
           return TCL_ERROR;
       }
   }

   if (strncmp(pHow, "AFTER", 5) == 0) 
       how = AFTER;
   else if (strncmp(pHow, "BEFORE", 6) == 0)
       how = BEFORE;
   else  {
       Tcl_SetResult(a_interp, 
          "chainElementAddByPos: [how] MUST be one of BEFORE or AFTER only",
          TCL_VOLATILE);
       return TCL_ERROR;
   }

   /*
    * Get addresses bound to the handles
    */
   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (shTclHandleExprEval(a_interp, pElementName, &hElement, &pUnused) != 
       TCL_OK)
       return TCL_ERROR;

   type = hElement.type;

   pChain = (CHAIN *)hChain.ptr;
   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp,
          "chainElementAddByPos: Handle ", pChainName, " is not a CHAIN.",
          (char *)NULL);
       return TCL_ERROR;
   }

   retStatus = shChainElementAddByPos(pChain, hElement.ptr, 
                                      shNameGetFromType(type), where, how);

   Tcl_ResetResult(a_interp);

   if (retStatus != SH_SUCCESS) {
       if (retStatus == SH_TYPE_MISMATCH)  {
              Tcl_AppendResult(a_interp, 
                 "chainElementAddByPos: cannot add an element of type ",
                 shNameGetFromType(hElement.type),
                 "\nto a chain of type ", shNameGetFromType(pChain->type),
                 " . To use chains as heterogeneous containers,\ncreate one",
                 " with a type of GENERIC (eg. chainNew GENERIC).", 
                 (char *) NULL);
              return TCL_ERROR;
       } else if (retStatus == SH_CHAIN_INVALID_ELEM)  {
              Tcl_AppendResult(a_interp,
                 "chainElementAddByPos: n should be less than the size of ",
                 "chain ", pChainName, (char *) NULL);
              return TCL_ERROR;
       }
     }  
   return TCL_OK;
}

/*
 * ROUTINE: tclChainElementTypeGetByPos()
 *
 * DESCRIPTION:
 *   This function is called by TCL in response to the TCL extension
 *   "chainElementTypeGetByPos". It gets the TYPE of the nth element
 *   on the chain.
 *
 * CALL:
 *   (static int) tclChainElementAddByPos(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainElementTypeGetByPos = "chainElementTypeGetByPos";
static ftclArgvInfo g_chainElementTypeGetByPosTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Get the type of the desired element on a chain as a TCL list\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the chain" },
   {"<position>", FTCL_ARGV_STRING, NULL, NULL,
    "Position in chain of the desired element. You can specify"
    "either HEAD of TAIL or an integer" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};
static int
tclChainElementTypeGetByPos(ClientData a_clientData, Tcl_Interp *a_interp,
                            int a_argc, char **a_argv)
{
   char     *pPos, *pChainName, *sp;
   CHAIN    *pChain;
   HANDLE    hChain;
   TYPE      type;
   int       flags;
   int       rstat;
   unsigned  int pos;
   void     *pUnused;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   pPos = pChainName = NULL;

   g_chainElementTypeGetByPosTbl[1].dst = &pChainName;
   g_chainElementTypeGetByPosTbl[2].dst = &pPos;
   
   if ((rstat = shTclParseArgv(a_interp, &a_argc, a_argv, 
       g_chainElementTypeGetByPosTbl, flags, g_chainElementTypeGetByPos))
       != FTCL_ARGV_SUCCESS)  {
       return(rstat);
   }

   /*
    * Some error checking
    */
   if (strncmp(pPos, "TAIL", 4) == 0)
       pos = TAIL;
   else if (strncmp(pPos, "HEAD", 4) == 0)
       pos = HEAD;
   else  {
       pos = (int) strtol(pPos, &sp, 0);
       if (sp == pPos || *sp != 0)  {
           Tcl_SetResult(a_interp, 
              "chainElementTypeGetByPos: <position> MUST be one of HEAD, TAIL "
              "\nor an integer",  TCL_VOLATILE);
           return TCL_ERROR;
       }
   }

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   pChain = (CHAIN *)hChain.ptr;
   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp,
          "chainElementTypeGetByPos: Handle ", pChainName, " is not a CHAIN.",
          (char *)NULL);
       return TCL_ERROR;
   }

   Tcl_ResetResult(a_interp);

   type = shChainElementTypeGetByPos(pChain, pos);

   if (g_shChainErrno != SH_SUCCESS) {
       if (g_shChainErrno == SH_CHAIN_EMPTY)  {
           Tcl_AppendResult(a_interp, "chainElementTypeGetByPos: chain ",
                            pChainName, " is empty", (char *) NULL);
           return TCL_ERROR;
       } else if (g_shChainErrno == SH_CHAIN_INVALID_ELEM)  {
           Tcl_AppendResult(a_interp,
              "chainElementTypeGetByPos: <position> should be less than the ",
              "size of\nchain ", pChainName, (char *) NULL);
           return TCL_ERROR;
       }
   }
   Tcl_AppendResult(a_interp, "{type \"", shNameGetFromType(type), "\"}", 
                    (char *) NULL);

   return TCL_OK;
}
/*
 * ROUTINE: doTheJob()
 *
 * DESCRIPTION:
 *   doTheJob() is called by two TCL extensions: "chainElementGetByPos"
 *   and "chainElementRemByPos". Logically, both the extensions do the
 *   same work, except the first extension calls the dervish function
 *   shChainElementGetByPos() and the second extension calls 
 *   shChainElementRemByPos(). The dervish functions are passed as parameters
 *   to this routine. The signature of both the dervish functions are the
 *   same.
 *
 * CALL:
 *   (static void) doTheJob(const char *pTclExt, ftclArgvInfo *argTable, char *aCmdName,
 *                 void *(*chainFunc)(CHAIN *pChain, int pos),
 *                 ClientData clientData, Tcl_Interp *a_interp,
 *                 int a_argc, char **a_argv)
 *   pTclExt - name of the TCL extension
 *   argTable     - ftcl parse argument table
 *   aCmdName     - name of TCL command
 *   chainFunc - pointer to the required dervish function
 *   clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static int 
doTheJob(char *pTclExt, ftclArgvInfo *argTable, char *aCmdName,
         void *(*chainFunc)(CHAIN *pChain, unsigned int pos),
         ClientData a_clientData, Tcl_Interp *a_interp,
         int a_argc, char **a_argv)
{

   char     *pPos, *pChainName, *sp;
   CHAIN    *pChain;
   HANDLE    hChain;
   TYPE      elemType;
   int       flags;
   int       retStatus;
   unsigned  int pos;
   void     *pElemAddr, *pUnused;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   pPos = pChainName = NULL;

   argTable[1].dst = &pChainName;
   argTable[2].dst = &pPos;
   
   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              argTable, flags, aCmdName);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   /*
    * Some error checking
    */
   if (strncmp(pPos, "TAIL", 4) == 0)
       pos = TAIL;
   else if (strncmp(pPos, "HEAD", 4) == 0)
       pos = HEAD;
   else  {
       pos = (int) strtoul(pPos, &sp, 0);
       if (sp == pPos || *sp != 0)  {
           Tcl_ResetResult(a_interp);
           Tcl_AppendResult(a_interp, 
              pTclExt, ": <position> MUST be one of HEAD, TAIL or an integer",
              (char *) NULL);
           return TCL_ERROR;
       }
   }

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

    pChain = (CHAIN *)hChain.ptr;
    if (hChain.type != shTypeGetFromName("CHAIN"))  {
        Tcl_AppendResult(a_interp, pTclExt,
           ": Handle ", pChainName, " is not a CHAIN.", (char *)NULL);
        return TCL_ERROR;
    }

    Tcl_ResetResult(a_interp);

    if (pChain->nElements == 0)  {
        Tcl_AppendResult(a_interp, pTclExt,
           ": chain ", pChainName, " is empty", (char *) NULL);
        return TCL_ERROR;
    }   

    elemType = shChainElementTypeGetByPos(pChain, pos);

    pElemAddr = chainFunc(pChain, pos);
    if (pElemAddr == NULL) {
        if (g_shChainErrno == SH_CHAIN_EMPTY)  {
            Tcl_AppendResult(a_interp, pTclExt, ": chain ", pChainName, 
                             " is empty", (char *) NULL);
            return TCL_ERROR;
        } else if (g_shChainErrno == SH_CHAIN_INVALID_ELEM)  {
            Tcl_AppendResult(a_interp, pTclExt, 
               ": <position> should be less than the size of\nchain ",
               pChainName, (char *) NULL);
            return TCL_ERROR;
        }
     }

   /*
    * Okay. Now we have the address. Let's get a handle to it. First of all,
    * let's see if an handle is already bound to this address. If so, no
    * point in creating a new handle. If a handle is NOT bound, then we can
    * create a new handle
    */
   if (p_shTclHandleNameGet(a_interp, pElemAddr) == TCL_ERROR)  {
      char   handleBuf[HANDLE_NAMELEN];
      HANDLE hndl;

      if (p_shTclHandleNew(a_interp, handleBuf) == TCL_OK)  {
          hndl.ptr = pElemAddr;
          hndl.type = elemType;

          if (p_shTclHandleAddrBind(a_interp, hndl, handleBuf) == TCL_OK)
              Tcl_SetResult(a_interp, handleBuf, TCL_VOLATILE);
	  else  {
              Tcl_AppendResult(a_interp, pTclExt,
                 ": cannot bind handle to a chain element", (char *) NULL);
              return TCL_ERROR;
          }
      } else  {
           Tcl_AppendResult(a_interp, pTclExt,
              ": cannot get a new handle", (char *)NULL);
           return TCL_ERROR;
      }
   }

   return TCL_OK;
}

/*
 * ROUTINE: tclChainElementGetByPos()
 *
 * DESCRIPTION:
 *   This function is called by TCL in response to the TCL extension
 *   "chainElementGetByPos". It binds a handle to the nth element on
 *   the chain and returns the handle
 *
 * CALL:
 *   (static int) tclChainElementGetByPos(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainElementGetByPos = "chainElementGetByPos";
static ftclArgvInfo g_chainElementGetByPosTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Get a (possibly new) handle to the desired element on a chain\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the chain" },
   {"<position>", FTCL_ARGV_STRING, NULL, NULL,
    "Position in chain of the desired element. You can specify"
    "either HEAD or TAIL or an integer" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainElementGetByPos(ClientData a_clientData, Tcl_Interp *a_interp,
                        int a_argc, char **a_argv)
{
   return doTheJob("tclChainElementGetByPos", g_chainElementGetByPosTbl,
                   g_chainElementGetByPos,
		   (void *(*)(CHAIN *, unsigned int))shChainElementGetByPos,
                   a_clientData, a_interp, a_argc, a_argv);
}

/*
 * ROUTINE: tclChainSize()
 *
 * DESCRIPTION:
 *   This function is called by TCL in response to the TCL extension
 *   "chainSize". It returns the size of the given chain
 *
 * CALL:
 *   (static int) tclChainSize(ClientData a_clientData, Tcl_Interp *a_interp,
 *                             int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainSize = "chainSize";
static ftclArgvInfo g_chainSizeTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Returns the size of the given chain\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle bound to the chain" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};
static int
tclChainSize(ClientData a_clientData, Tcl_Interp *a_interp,
             int a_argc, char **a_argv)
{
   HANDLE hChain;
   void   *pUnused;
   char   *pHandleName,
          sizeBuf[10];
   int    flags,
          retStatus;

   pHandleName = NULL;
   flags = FTCL_ARGV_NO_LEFTOVERS;

   g_chainSizeTbl[1].dst  = &pHandleName;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, g_chainSizeTbl, 
               flags, g_chainSize);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pHandleName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;
   
   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainSize: handle ", pHandleName, " is a ",
          shNameGetFromType(hChain.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }

   sprintf(sizeBuf, "%d", shChainSize((CHAIN *)hChain.ptr));
   Tcl_SetResult(a_interp, sizeBuf, TCL_VOLATILE);

   return TCL_OK;
}

/*
 * ROUTINE: tclChainTypeGet()
 *
 * DESCRIPTION:
 *   This function is called by TCL in response to the TCL extension
 *   "chainTypeGet". It returns the type of the given chain as a TCL list
 *
 * CALL:
 *   (static int) tclChainTypeGet(ClientData a_clientData, 
 *                     Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainTypeGet = "chainTypeGet";
static ftclArgvInfo g_chainTypeGetTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Returns the type of the given chain as a TCL list\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle bound to the chain" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};
static int
tclChainTypeGet(ClientData a_clientData, Tcl_Interp *a_interp,
                int a_argc, char **a_argv)
{
   HANDLE hChain;
   CHAIN  *pChain;
   char   *pHandleName;
   void   *pUnused;
   int    flags,
          retStatus;

   pHandleName = NULL;
   flags = FTCL_ARGV_NO_LEFTOVERS;

   g_chainTypeGetTbl[1].dst  = &pHandleName;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, g_chainTypeGetTbl,
                              flags, g_chainTypeGet);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pHandleName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainTypeGet: handle ", pHandleName, " is a ",
          shNameGetFromType(hChain.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }

   pChain = (CHAIN *) hChain.ptr;
   Tcl_ResetResult(a_interp);

   Tcl_AppendResult(a_interp, "{type \"", 
                    shNameGetFromType(shChainTypeGet(pChain)),
                    "\"}", (char *) NULL);
   return TCL_OK;
}

/*
 * ROUTINE: tclChainTypeSet()
 *
 * DESCRIPTION:
 *   This function is called by TCL in response to the TCL extension
 *   "chainTypeSet". It forces the type of a chain to a new one.
 *
 * CALL:
 *   (static int) tclChainTypeSet(ClientData a_clientData, 
 *                     Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainTypeSet = "chainTypeSet";
static ftclArgvInfo g_chainTypeSetTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Reset the TYPE of a chain\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle bound to the chain" },
   {"<type>", FTCL_ARGV_STRING, NULL, NULL,
    "The new TYPE of the chain" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainTypeSet(ClientData a_clientData, Tcl_Interp *a_interp,
                int a_argc, char **a_argv)
{
   HANDLE hChain;
   char   *pHandleName,
          *pType;
   void   *pUnused;
   int    flags,
          retStatus;

   pHandleName = pType = NULL;
   flags = FTCL_ARGV_NO_LEFTOVERS;

   g_chainTypeSetTbl[1].dst  = &pHandleName;
   g_chainTypeSetTbl[2].dst  = &pType;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              g_chainTypeSetTbl, flags, g_chainTypeSet);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pHandleName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainTypeSet: handle ", pHandleName, " is a ",
          shNameGetFromType(hChain.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }

   (void) shChainTypeSet((CHAIN *)hChain.ptr, pType);

   Tcl_ResetResult(a_interp);
   
   return TCL_OK;
}

/*
 * ROUTINE: tclChainElementRemByPos()
 *
 * DESCRIPTION:
 *   This function is called by TCL in response to the TCL extension
 *   "chainElementRemByPos". It removes an element from the chain and 
 *   binds a handle (if not already bound) to this element.
 *
 * CALL:
 *   (static int) tclChainElementRemByPos(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainElementRemByPos = "chainElementRemByPos";
static ftclArgvInfo g_chainElementRemByPosTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Remove the desired element from a chain and bind a (possibly new)"
    "handle to it\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the chain" },
   {"<position>", FTCL_ARGV_STRING, NULL, NULL,
    "Position in chain of the desired element. You can specify"
    "either HEAD or TAIL or an integer" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainElementRemByPos(ClientData a_clientData, Tcl_Interp *a_interp,
                        int a_argc, char **a_argv)
{
   return doTheJob("tclChainElementRemByPos", g_chainElementRemByPosTbl,
                   g_chainElementRemByPos, shChainElementRemByPos,
                   a_clientData, a_interp, a_argc, a_argv);
}

/*
 * ROUTINE: tclChainJoin()
 *
 * DESCRIPTION:
 *   This function is called by TCL in response to the TCL extension
 *   "chainJoin". It appends a target chain to a source chain. The target
 *   chain is no longer availaible after a call to this extension.
 *
 * CALL:
 *   (static int) tclChainJoin(ClientData a_clientData, 
 *                             Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainJoin = "chainJoin";
static ftclArgvInfo g_chainJoinTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Join (append) <target-chain> to <source-chain>. <target-chain>\n"
    "will be deleted\n" },
   {"<source-chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle bound to source chain" },
   {"<target-chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle bound to target chain. This handle will be"
    "destroyed on success" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainJoin(ClientData a_clientData, Tcl_Interp *a_interp,
             int a_argc, char **a_argv)
{
   char *pSrcName, *pTargetName;
   void *pUnused;
   HANDLE hSrc, hTarget;
   int  flags,
        retStatus;
   RET_CODE rc;

   pSrcName = pTargetName = NULL;
   flags = FTCL_ARGV_NO_LEFTOVERS;

   g_chainJoinTbl[1].dst  = &pSrcName;
   g_chainJoinTbl[2].dst  = &pTargetName;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              g_chainJoinTbl, flags, g_chainJoin);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pSrcName, &hSrc, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (shTclHandleExprEval(a_interp, pTargetName, &hTarget, &pUnused) != 
             TCL_OK)
       return TCL_ERROR;

   if (hSrc.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainJoin: handle ", pSrcName, " is a ",
          shNameGetFromType(hSrc.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }

   if (hTarget.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainJoin: handle ", pTargetName, " is a ",
          shNameGetFromType(hTarget.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }

   Tcl_ResetResult(a_interp);

   rc = shChainJoin((CHAIN *)hSrc.ptr, (CHAIN *)hTarget.ptr);
   if (rc != SH_SUCCESS)
       if (rc == SH_TYPE_MISMATCH)  {
               Tcl_AppendResult(a_interp, 
                  "chainJoin: chain types do not match. The type of ",
                  pTargetName, " must be the same\nas ", pSrcName,
                  " OR ", pSrcName, " must be of type GENERIC", (char *)NULL);
               return TCL_ERROR;
       }


   /*
    * Delete handle to <TARGET-CHAIN> and declare success
    */
   if (p_shTclHandleDel(a_interp, pTargetName) == TCL_ERROR)  {
       Tcl_AppendResult(a_interp, 
          "chainJoin: cannot delete handle \"", pTargetName, "\"",
          (char *)NULL);
       return TCL_ERROR;
   } else  {
       Tcl_ResetResult(a_interp);
       Tcl_SetResult(a_interp, pSrcName, TCL_VOLATILE);
       return TCL_OK;
   }
}

/*
 * ROUTINE: tclChainCopy()
 *
 * DESCRIPTION:
 *   This function is called by TCL in response to the TCL extension
 *   "chainCopy". It copies an existing chain. A pointer to the newly
 *   created chain is returned
 *
 * CALL:
 *   (static int) tclChainCopy(ClientData a_clientData, 
 *                             Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainCopy = "chainCopy";
static ftclArgvInfo g_chainCopyTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Copy an existing chain. A handle to the newly created\n"
    "chain will be returned\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle bound to the existing chain" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainCopy(ClientData a_clientData, Tcl_Interp *a_interp,
             int a_argc, char **a_argv)
{
   char   *pChainName,
          handleBuf[HANDLE_NAMELEN];
   void   *pUnused;
   HANDLE hChain,
          newChainHndl;
   CHAIN  *pNewChain;
   int    flags, retValue;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   retValue = TCL_OK;
   
   g_chainCopyTbl[1].dst  = &pChainName;

   retValue = shTclParseArgv(a_interp, &a_argc, a_argv, 
                             g_chainCopyTbl, flags, g_chainCopy);
   if (retValue != FTCL_ARGV_SUCCESS)  {
       return(retValue);
   }

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK) 
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainCopy: handle ", pChainName, " is a ",
          shNameGetFromType(hChain.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }

   Tcl_ResetResult(a_interp);
   retValue = TCL_OK;
   pNewChain = shChainCopy((CHAIN *)hChain.ptr);

   if (pNewChain == NULL)  {
       if (g_shChainErrno == SH_CHAIN_EMPTY)  {
           Tcl_AppendResult(a_interp, "chainCopy: ",
              "cannot copy chain. It does not have any elements on it",
              (char *)NULL);
           retValue = TCL_ERROR;
       }
   } else  {
       newChainHndl.ptr  = pNewChain;
       newChainHndl.type = shTypeGetFromName("CHAIN");

       if (p_shTclHandleNew(a_interp, handleBuf) != TCL_OK)  {
           Tcl_SetResult(a_interp, 
              "chainCopy: cannot obtain a handle for chain", TCL_VOLATILE);
           retValue = TCL_ERROR;
       } else  {
           if (p_shTclHandleAddrBind(a_interp, newChainHndl, handleBuf) !=
               TCL_OK)  {
               Tcl_SetResult(a_interp, 
                  "chainCopy: cannot bind CHAIN address to the new chain"
                  "handle", TCL_VOLATILE);
               retValue = TCL_ERROR;
	   } else
               Tcl_SetResult(a_interp, handleBuf, TCL_VOLATILE);
	 }
   }

   return retValue;
}

/*
 * ROUTINE: tclChainTypeDefine()
 *
 * DESCRIPTION:
 *   tclChainTypeDefine() defines the type of the chain according to the
 *   following rules:
 *    1) if chain is empty, return the original type
 *    2) if chain type is GENERIC, but the chain contains homogeneous
 *       elements, return the type of the elements on the chain
 *    3) if chain type is GENERIC, and the chain contains heterogeneous
 *       elements, the chain type is left unchanged
 *   This routine is called in response to the TCL command "chainTypeDefine"
 *
 * CALL:
 *   (static int) tclChainTypeDefine(ClientData a_clientData, 
 *                      Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainTypeDefine = "chainTypeDefine";
static ftclArgvInfo g_chainTypeDefineTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Define the chain type according to it's contents\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle bound to the chain" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};
static int tclChainTypeDefine(ClientData a_clientData, Tcl_Interp *a_interp,
                              int a_argc, char **a_argv)
{
   char   *pHandleName;
   void   *pUnused;
   HANDLE hChain;
   int    flags, retStatus;
   
   flags = FTCL_ARGV_NO_LEFTOVERS;

   g_chainTypeDefineTbl[1].dst  = &pHandleName;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              g_chainTypeDefineTbl, flags, g_chainTypeDefine);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pHandleName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainTypeDefine: handle ", pHandleName, " is a ",
          shNameGetFromType(hChain.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }

   Tcl_SetResult(a_interp, 
      shNameGetFromType(shChainTypeDefine((CHAIN *) hChain.ptr)),
      TCL_VOLATILE);

   return TCL_OK;
}

/*
 * ROUTINE: tclChainCursorNew()
 *
 * DESCRIPTION:
 *   tclChainCursorNew() creates a new curosr and returns a handle bound to it.
 *   This routine is called in response to the TCL command "chainCursorNew"
 *
 * CALL:
 *   (static int) tclChainCursorNew(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainCursorNew = "chainCursorNew";
static ftclArgvInfo g_chainCursorNewTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Create a cursor for a chain and return a handle to it\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle of the chain to create the cursor on" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};
static int tclChainCursorNew(ClientData a_clientData, Tcl_Interp *a_interp,
                             int a_argc, char **a_argv)
{
   HANDLE   crsrHndl,
            hChain;
   CURSOR_T crsr, *cp;
   char     handleBuf[HANDLE_NAMELEN],
            *pChainName;
   void     *pUnused;
   int      retStatus,
            flags;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   pChainName = NULL;

   g_chainCursorNewTbl[1].dst  = &pChainName;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              g_chainCursorNewTbl, flags, g_chainCursorNew);
   if (retStatus != FTCL_ARGV_SUCCESS)  { 
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainCursorNew: handle ", pChainName, " is a ", 
          shNameGetFromType(hChain.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }
   
   if ( (crsr = shChainCursorNew((CHAIN *)hChain.ptr)) == INVALID_CURSOR) {
     shTclInterpAppendWithErrStack(a_interp); /* Append errors from stack - if any */
     return TCL_ERROR;
   }
   
   cp = (CURSOR_T *) shMalloc(sizeof(CURSOR_T));
   *cp = crsr;
   crsrHndl.ptr = (void *)cp;
   crsrHndl.type = shTypeGetFromName("CURSOR");
     
   if (p_shTclHandleNew(a_interp, handleBuf) != TCL_OK)  {
       Tcl_SetResult(a_interp, 
	  "chainCursorNew: cannot obtain a handle for chain", TCL_VOLATILE);
       return TCL_ERROR;
   }
       
   if (p_shTclHandleAddrBind(a_interp, crsrHndl, handleBuf) != TCL_OK)  {
       Tcl_SetResult(a_interp, 
	  "chainCursorNew: cannot bind CHAIN address to the new chain handle",
	  TCL_VOLATILE);
       return TCL_ERROR;
   }

   Tcl_SetResult(a_interp, handleBuf, TCL_VOLATILE);

   return TCL_OK;
}

/*
 * ROUTINE: tclChainCursorDel()
 *
 * DESCRIPTION:
 *   tclChainCursorDel() deletes the specified cursor.
 *   This routine is called in response to the TCL command "chainCursorDel"
 *
 * CALL:
 *   (static int) tclChainCursorDel(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainCursorDel = "chainCursorDel";
static ftclArgvInfo g_chainCursorDelTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Delete the specified cursor for the chain\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the chain" },
   {"<cursor>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the cursor" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int tclChainCursorDel(ClientData a_clientData, Tcl_Interp *a_interp,
                             int a_argc, char **a_argv)
{
   char     *pChainName, *pCursorName;
   HANDLE   hChain, hCursor;
   void     *pUnused;
   int      flags;
   int      retStatus;
   RET_CODE rc;
   CURSOR_T *cp;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   pChainName = pCursorName = NULL;

   g_chainCursorDelTbl[1].dst = &pChainName;
   g_chainCursorDelTbl[2].dst = &pCursorName;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              g_chainCursorDelTbl, flags, g_chainCursorDel);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (shTclHandleExprEval(a_interp, pCursorName, &hCursor,&pUnused) != TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, "chainCursorDel: Handle ", pChainName,
          " is a ", shNameGetFromType(hChain.type), ", not a CHAIN",
          (char *) NULL);
       return TCL_ERROR;
   }

   if (hCursor.type != shTypeGetFromName("CURSOR"))  {
       Tcl_AppendResult(a_interp, "chainCursorDel: Handle ", pCursorName,
          " is a ", shNameGetFromType(hCursor.type), ", not a CURSOR",
          (char *) NULL);
       return TCL_ERROR;
   }

   cp = (CURSOR_T *)hCursor.ptr;

   rc = shChainCursorDel((CHAIN *)hChain.ptr, *cp);
   if (rc != SH_SUCCESS) {
       Tcl_AppendResult(a_interp, 
          "chainCursorDel: invalid cursor encountered, cannot delete ",
          "cursor ", pCursorName, (char *) NULL);
       return TCL_ERROR;
   }  else  {
      /*
       * Delete handle to <CURSOR> and declare success
       */
      shFree(cp);
      if (p_shTclHandleDel(a_interp, pCursorName) == TCL_ERROR)  {
          Tcl_AppendResult(a_interp, 
             "chainCursorDel: cannot delete handle \"", pCursorName, "\"",
             (char *)NULL);
          return TCL_ERROR;
      } else  {
          Tcl_ResetResult(a_interp);
          return TCL_OK;
      }
   }
}

/*
 * ROUTINE: tclChainCursorCount()
 *
 * DESCRIPTION:
 *   tclChainCursorCount() returns a count of the cursors engaged by the chain
 *   This routine is called in response to the TCL command "chainCursorCount"
 *
 * CALL:
 *   (static int) tclChainCursorCount(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainCursorCount = "chainCursorCount";
static ftclArgvInfo g_chainCursorCountTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Get the number of cursors being used by the specified chain\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle bound to the chain" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int tclChainCursorCount(ClientData a_clientData, Tcl_Interp *a_interp,
                             int a_argc, char **a_argv)
{
   char     *pHandleName,
             buf[5];
   void     *pUnused;
   HANDLE    hChain;
   int       flags, retStatus;
   
   flags = FTCL_ARGV_NO_LEFTOVERS;

   g_chainCursorCountTbl[1].dst  = &pHandleName;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              g_chainCursorCountTbl, flags, g_chainCursorCount);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pHandleName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;
   
   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, 
          "chainCursorCount: handle ", pHandleName, " is a ",
          shNameGetFromType(hChain.type), ", not a CHAIN", (char *)NULL);
       return TCL_ERROR;
   }

   sprintf(buf, "%d", shChainCursorCount((CHAIN *)hChain.ptr));
   Tcl_SetResult(a_interp, buf, TCL_VOLATILE);

   return TCL_OK;
}

/*
 * ROUTINE: tclChainCursorSet()
 *
 * DESCRIPTION:
 *   tclChainCursorSet() sets a cursor to the specified element on the
 *   chain. This routine is called in response to the TCL extension
 *   "chainCursorSet"
 *
 * CALL:
 *   (static int) tclChainCursorSet(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainCursorSet = "chainCursorSet";
static ftclArgvInfo g_chainCursorSetTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Sets a cursor to a specific element on the chain\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle of the chain" },
   {"<cursor>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle of the cursor" },
   {"[n]", FTCL_ARGV_STRING, NULL, NULL,
    "Chain element to set the cursor to" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int tclChainCursorSet(ClientData a_clientData, Tcl_Interp *a_interp,
                             int a_argc, char **a_argv)
{
   char     *pChainName, *pCursorName,
            *pWhere, *sp;
   HANDLE    hChain, hCursor;
   int       flags;
   int       retStatus;
   unsigned  int where;
   void     *pUnused;
   CURSOR_T *cp;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   pChainName = pCursorName = NULL;
   pWhere = "HEAD";

   g_chainCursorSetTbl[1].dst  = &pChainName;
   g_chainCursorSetTbl[2].dst  = &pCursorName;
   g_chainCursorSetTbl[3].dst  = &pWhere;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              g_chainCursorSetTbl, flags, g_chainCursorSet);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (strncmp(pWhere, "TAIL", 4) == 0)
       where = TAIL;
   else if (strncmp(pWhere, "HEAD", 4) == 0)
       where = HEAD;
   else  {
       where = (int) strtol(pWhere, &sp, 0);
       if (sp == pWhere || *sp != 0)  {
           Tcl_SetResult(a_interp, 
              "chainCursorSet: [n] MUST be one of HEAD, TAIL or an integer",
              TCL_VOLATILE);
           return TCL_ERROR;
       }
   }

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (shTclHandleExprEval(a_interp, pCursorName, &hCursor, &pUnused) 
       != TCL_OK)
       return TCL_ERROR;

   Tcl_ResetResult(a_interp);

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, "chainCursorSet: Handle ", pChainName,
          " is a ", shNameGetFromType(hChain.type), " not a CHAIN",
          (char *) NULL);
       return TCL_ERROR;
   }

   if (hCursor.type != shTypeGetFromName("CURSOR"))  {
       Tcl_AppendResult(a_interp, "chainCursorSet: Handle ", pCursorName,
          " is a ", shNameGetFromType(hCursor.type), " not a CURSOR",
          (char *) NULL);
       return TCL_ERROR;
   }

   cp = (CURSOR_T *)hCursor.ptr;
   (void) shChainCursorSet((CHAIN *) hChain.ptr, *cp, where);

   Tcl_ResetResult(a_interp);

   if (g_shChainErrno != SH_SUCCESS)  {
       if (g_shChainErrno == SH_CHAIN_EMPTY)  {
           Tcl_AppendResult(a_interp, "chainCursorSet: chain ", pChainName,
              " is empty. Cannot set a cursor to an empty\nchain. Please add",
              " some elements first",
               (char *) NULL);
       } else if (g_shChainErrno == SH_CHAIN_INVALID_CURSOR)  {
           Tcl_AppendResult(a_interp, "chainCursorSet: ", pCursorName, 
              " is an invalid cursor. Please create\na valid one",
              " using the TCL extension chainCursorNew AND initialize\n",
              "it properly using chainCursorSet", (char *) NULL);
       } else if (g_shChainErrno == SH_CHAIN_INVALID_ELEM)  {
           Tcl_AppendResult(a_interp,
              "chainCursorSet: n should be less than the size of ",
              "chain ", pChainName, (char *) NULL);
       }
       return TCL_ERROR;
   } else
       return TCL_OK;
}

/*
 * ROUTINE: tclChainElementAddByCursor()
 *
 * DESCRIPTION:
 *   tclChainElementAddByCursor() inserts an element on a chain. It accepts
 *   5 command line arguments: <CHAIN> <CURSOR> <ELEMENT> <TYPE> [how]; i.e.
 *   <ELEMENT> is inserted on <CHAIN> relative to <CURSOR> and specified
 *   by [how]. The last argument is defaulted to AFTER, so that the new
 *   element is added after the cursor.
 *
 * CALL:
 *   (static int) tclChainElementAddByPos(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainElementAddByCursor = "chainElementAddByCursor";
static ftclArgvInfo g_chainElementAddByCursorTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Add an element to the chain using a cursor\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle of the chain" },
   {"<element>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle of the element to be added" },
   {"<cursor>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle of the cursor" },
   {"[how]", FTCL_ARGV_STRING, NULL, NULL,
    "How to add the new element in relation to the nth element" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainElementAddByCursor(ClientData a_clientData, Tcl_Interp *a_interp,
                           int a_argc, char **a_argv)
{
   char            *pChainName, *pElemName, *pCursorName, *pHow;
   HANDLE           hChain, hElem, hCursor;
   CHAIN           *pChain;
   RET_CODE         rc;
   CURSOR_T        *cp;
   CHAIN_ADD_FLAGS  how;
   void            *pUnused;
   int              flags;
   int              retStatus;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   pChainName = pElemName = pCursorName = NULL;
   pHow = "AFTER";

   g_chainElementAddByCursorTbl[1].dst  = &pChainName;
   g_chainElementAddByCursorTbl[2].dst  = &pElemName;
   g_chainElementAddByCursorTbl[3].dst  = &pCursorName;
   g_chainElementAddByCursorTbl[4].dst  = &pHow;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
               g_chainElementAddByCursorTbl, flags, g_chainElementAddByCursor);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (strncmp(pHow, "AFTER", 5) == 0) 
       how = AFTER;
   else if (strncmp(pHow, "BEFORE", 6) == 0)
       how = BEFORE;
   else  {
       Tcl_SetResult(a_interp, 
          "chainElementAddByCursor: [how] MUST be one of BEFORE or AFTER only",
          TCL_VOLATILE);
       return TCL_ERROR;
   }

   /*
    * Get addresses from the handles and make sure we have the correct 
    * handles
    */
   Tcl_ResetResult(a_interp);
   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;
   if (shTclHandleExprEval(a_interp, pElemName, &hElem, &pUnused) != TCL_OK)
       return TCL_ERROR;
   if (shTclHandleExprEval(a_interp, pCursorName, &hCursor, &pUnused)!= TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, "chainElemetAddByCursor: Handle ", 
          pChainName, " is a ", shNameGetFromType(hChain.type), " not a CHAIN",
          (char *) NULL);
       return TCL_ERROR;
   }

   if (hCursor.type != shTypeGetFromName("CURSOR"))  {
       Tcl_AppendResult(a_interp, "chainElementGetByCursor: Handle ", 
          pCursorName, " is a ", shNameGetFromType(hCursor.type), 
          " not a CURSOR", (char *) NULL);
       return TCL_ERROR;
   }

   cp = (CURSOR_T *) hCursor.ptr;
   pChain = (CHAIN *) hChain.ptr;

   rc = shChainElementAddByCursor(pChain, hElem.ptr, 
                                  shNameGetFromType(hElem.type), *cp, how);

   if (rc != SH_SUCCESS)  {
       Tcl_ResetResult(a_interp);

       if (rc == SH_TYPE_MISMATCH)  {
            Tcl_AppendResult(a_interp, "chainElementAddByCursor: ",
               "cannot add an element of type ", shNameGetFromType(hElem.type),
               "\nto a chain of type ", shNameGetFromType(pChain->type),
               ". To use chains as heterogeneous containers,\ncreate one",
               " with a type of GENERIC (eg. chainNew GENERIC)",
               (char *) NULL);
            return TCL_ERROR;
       } else if (rc == SH_CHAIN_INVALID_CURSOR)  {
            Tcl_AppendResult(a_interp, "chainElementAddByCursor: ", 
               pCursorName,
               " is an invalid cursor. Please create\na valid one",
               " using the TCL extension chainCursorNew AND initialize\n",
               "it properly using chainCursorSet", (char *) NULL);
            return TCL_ERROR;
       }
	  
   }
   
   return TCL_OK;
}

/*
 * ROUTINE: tclChainElementTypeGetByCursor()
 *
 * DESCRIPTION:
 *   tclChainElementTypeGetByCursor() gets the type of the element under
 *   the cursor in form of a TCL list.
 *
 * CALL:
 *   (static int) tclChainElementAddByPos(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainElementTypeGetByCursor = "chainElementTypeGetByCursor";
static ftclArgvInfo g_chainElementTypeGetByCursorTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Get the TYPE of element under the cursor\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the chain" },
   {"<cursor>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the cursor" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainElementTypeGetByCursor(ClientData a_clientData, Tcl_Interp *a_interp,
                               int a_argc, char **a_argv)
{
   HANDLE    hChain, hCursor;
   CURSOR_T *cp;
   CHAIN    *pChain;
   char     *pChainName, *pCursorName;
   void     *pUnused;
   int       flags;
   int       retStatus;
   TYPE      type;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   pChainName = pCursorName = NULL;

   g_chainElementTypeGetByCursorTbl[1].dst = &pChainName;
   g_chainElementTypeGetByCursorTbl[2].dst = &pCursorName;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
       g_chainElementTypeGetByCursorTbl, flags, g_chainElementTypeGetByCursor);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (shTclHandleExprEval(a_interp, pCursorName, &hCursor, &pUnused) 
       != TCL_OK)
       return TCL_ERROR;

   Tcl_ResetResult(a_interp);

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, "chainElementTypeGetByCursor: Handle ", 
          pChainName, " is a ", shNameGetFromType(hChain.type), " not a CHAIN",
          (char *) NULL);
       return TCL_ERROR;
   }

   if (hCursor.type != shTypeGetFromName("CURSOR"))  {
       Tcl_AppendResult(a_interp, "chainElementTypeGetByCursor: Handle ", 
          pCursorName, " is a ", shNameGetFromType(hCursor.type), 
          " not a CURSOR", (char *) NULL);
       return TCL_ERROR;
   }

   pChain = (CHAIN *) hChain.ptr;
   cp     = (CURSOR_T *) hCursor.ptr;

   type = shChainElementTypeGetByCursor(pChain, *cp);

   if (g_shChainErrno != SH_SUCCESS)  {
       if (g_shChainErrno == SH_CHAIN_INVALID_CURSOR)
           Tcl_AppendResult(a_interp, "chainElementTypeGetByCursor: ",
              pCursorName,
              " is an invalid cursor. Please create\na valid one",
              " using the TCL extension chainCursorNew AND initialize\n",
              "it properly using chainCursorSet", (char *) NULL);
       else if (g_shChainErrno == SH_CHAIN_INVALID_ELEM)
           Tcl_AppendResult(a_interp, "chainElementTypeGetByCursor: ",
              pCursorName,
              " cursor is referencing to an invalid chain\nelement; ",
              "probably an element that has been deleted. Please\nreset",
              " the cursor using chainCursorSet", (char *) NULL);
       return TCL_ERROR;
   } else  {
         Tcl_AppendResult(a_interp, "{type \"", shNameGetFromType(type), "\"}",
                         (char *) NULL);
         return TCL_OK;
   }
}

/*
 * ROUTINE: tclChainElementRemByCursor()
 *
 * DESCRIPTION:
 *   tclChainElementRemByCursor() removes the chain element under the
 *   the cursor and binds a (possibly new) handle to it. This handle
 *   name is returned in Tcl_Interp->result. This TCL extension is 
 *   called in response to "chainElementRemByCursor"
 *
 * CALL:
 *   (static int) tclChainElementRemByCursor(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainElementRemByCursor = "chainElementRemByCursor";
static ftclArgvInfo g_chainElementRemByCursorTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Remove the element under the cursor and bind a (possibly new)\n"
    "handle to it\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the chain" },
   {"<cursor>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the cursor" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainElementRemByCursor(ClientData a_clientData, Tcl_Interp *a_interp,
                           int a_argc, char **a_argv)
{
   char     *pChainName, *pCursorName;
   void     *pData, *pUnused;
   TYPE      type;
   HANDLE    hChain, hCursor;
   CURSOR_T *cp;
   int       retVal, flags;
   int       retStatus;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   retVal = TCL_OK;
   pChainName = pCursorName = NULL;

   g_chainElementRemByCursorTbl[1].dst = &pChainName;
   g_chainElementRemByCursorTbl[2].dst = &pCursorName;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                g_chainElementRemByCursorTbl, flags, g_chainElementRemByCursor);
   if (retStatus != FTCL_ARGV_SUCCESS) {
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (shTclHandleExprEval(a_interp, pCursorName, &hCursor, &pUnused) 
       != TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, "chainElementRemByCursor: Handle ", 
          pChainName, " is a ", shNameGetFromType(hChain.type), " not a CHAIN",
          (char *) NULL);
       return TCL_ERROR;
   }

   if (hCursor.type != shTypeGetFromName("CURSOR"))  {
       Tcl_AppendResult(a_interp, "chainElementRemByCursor: Handle ", 
          pCursorName, " is a ", shNameGetFromType(hCursor.type), 
          " not a CURSOR", (char *) NULL);
       return TCL_ERROR;
   }

   cp = (CURSOR_T *)hCursor.ptr;

   type  = shChainElementTypeGetByCursor((CHAIN *)hChain.ptr, *cp);
   pData = shChainElementRemByCursor((CHAIN *)hChain.ptr, *cp);

   Tcl_ResetResult(a_interp);

   if (pData == NULL)  {
       if (g_shChainErrno != SH_SUCCESS)  {
          if (g_shChainErrno == SH_CHAIN_INVALID_CURSOR)
              Tcl_AppendResult(a_interp, "chainElementRemByCursor: ",
                 pCursorName,
                 " is an invalid cursor. Please create\na valid one",
                 " using the TCL extension chainCursorNew AND initialize\n",
                 "it properly using chainCursorSet", (char *) NULL);
          else if (g_shChainErrno == SH_CHAIN_INVALID_ELEM)
              Tcl_AppendResult(a_interp, "chainElementRemByCursor: ",
                 pCursorName,
                 " cursor is referencing to an invalid chain\nelement; ",
                 "probably an element that has been deleted. Please\nreset",
                 " the cursor using chainCursorSet", (char *) NULL);
          retVal = TCL_ERROR;
       }
   } else  {
          /*
           * Okay. Now we have the address of the element. Let's get a handle
           * to it. First, see if an handle is already bound to the address.
           * If so, no point in creating a new handle. If a handle is NOT
           * bound, then we can create a new handle
           */
          if (p_shTclHandleNameGet(a_interp, pData) == TCL_ERROR)  {
              char   handleBuf[HANDLE_NAMELEN];
              HANDLE hndl;

              if (p_shTclHandleNew(a_interp, handleBuf) == TCL_OK)  {
                  hndl.ptr  = pData;
                  hndl.type = type;

                  if (p_shTclHandleAddrBind(a_interp, hndl, handleBuf) 
                           == TCL_OK)
                      Tcl_SetResult(a_interp, handleBuf, TCL_VOLATILE);
                  else  {
                      Tcl_AppendResult(a_interp, 
                         "chainElementRemByCursor: cannot bind handle to ",
                         "chain element", (char *) NULL);
                      retVal = TCL_ERROR;
                  }
              } else  {
                 Tcl_AppendResult(a_interp, 
                    "chainElementRemByCursor: cannot get a new handle", 
                    (char *) NULL);
                 retVal = TCL_ERROR;
	      }
          }
   }

   return retVal;
}

/*
 * ROUTINE: tclChainWalk()
 *
 * DESCRIPTION:
 *   tclChainWalk() traverses a chain in a direction specified by the user.
 *   It returns a handle to the element under the cursor. This routine is
 *   called in response to the TCL extension "chainWalk"
 *
 * CALL:
 *   (static int) tclChainWalk(ClientData a_clientData, Tcl_Interp *a_interp,
 *                int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainWalk = "chainWalk";
static ftclArgvInfo g_chainWalkTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Traverse a chain using a cursor. The chain can be modified\n"
    "during traversal\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the chain" },
   {"<cursor>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the cursor" },
   {"[whither]", FTCL_ARGV_STRING, NULL, NULL,
    "Traversal direction; one of PREVIOUS, THIS, or NEXT" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};
static int
tclChainWalk(ClientData a_clientData, Tcl_Interp *a_interp, int a_argc,
             char **a_argv)
{
   char   *pChainName, *pCursorName, *pWhither;
   void   *pData, *pUnused;
   HANDLE hChain, hCursor;
   CHAIN  *pChain;
   int    flags,
          retValue;
   int    retStatus;
   CURSOR_T *cp;
   CHAIN_WALK_FLAGS whither;
   static TYPE chain_type = UNKNOWN, cursor_type;
/*
 * only call shTypeGetFromName once for each type
 */
   if(chain_type == UNKNOWN) {
      chain_type = shTypeGetFromName("CHAIN");
      cursor_type = shTypeGetFromName("CURSOR");
   }

   retValue = TCL_OK;
   flags = FTCL_ARGV_NO_LEFTOVERS;
   pChainName = pCursorName = NULL;
   pWhither = "NEXT";

   g_chainWalkTbl[1].dst = &pChainName;
   g_chainWalkTbl[2].dst = &pCursorName;
   g_chainWalkTbl[3].dst = &pWhither;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              g_chainWalkTbl, flags, g_chainWalk);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   Tcl_ResetResult(a_interp);

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (shTclHandleExprEval(a_interp, pCursorName, &hCursor, &pUnused) 
       != TCL_OK)
       return TCL_ERROR;

   if (hChain.type != chain_type)  {
      Tcl_AppendResult(a_interp, "chainWalk: Handle ", pChainName, 
         "is a ", shNameGetFromType(hChain.type), " not a CHAIN", 
         (char *) NULL);
      return TCL_ERROR;
   }

   if (hCursor.type != cursor_type)  {
      Tcl_AppendResult(a_interp, "chainWalk: Handle ", pCursorName, 
         "is a ", shNameGetFromType(hCursor.type), " not a CURSOR", 
         (char *) NULL);
      return TCL_ERROR;
   }

   if (strncmp(pWhither, "NEXT", 4) == 0) 
       whither = NEXT;
   else if (strncmp(pWhither, "PREVIOUS", 8) == 0)
       whither = PREVIOUS;
   else if (strncmp(pWhither, "THIS", 4) == 0)
       whither = THIS;
   else  {
       Tcl_SetResult(a_interp, 
          "chainWalk: [whither] MUST be one of PREVIOUS, THIS, or NEXT only",
          TCL_VOLATILE);
       return TCL_ERROR;
   }

   pChain = (CHAIN *)hChain.ptr;
   cp     = (CURSOR_T *)hCursor.ptr;

   pData = shChainWalk(pChain, *cp, whither);
   if (pData == (void *) NULL && g_shChainErrno == SH_SUCCESS)  {
       Tcl_ResetResult(a_interp);
       Tcl_SetResult(a_interp, "", TCL_VOLATILE);
       retValue = TCL_OK;
   } else if (g_shChainErrno != SH_SUCCESS)  {
       if (g_shChainErrno == SH_CHAIN_EMPTY)  {
           /*
            * If a chain is empty, we do not necessarily want to return an
            * error. Instead, do what happens when a user walks off the end
            * of a chain - an empty string is retrurned.
            */
           Tcl_ResetResult(a_interp);
           Tcl_SetResult(a_interp, "", TCL_VOLATILE);
           retValue = TCL_OK;
       }  else if (g_shChainErrno == SH_CHAIN_INVALID_ELEM)  {
           Tcl_AppendResult(a_interp, "chainWalk: ", pCursorName,
              " cursor is referencing an invalid chain element; ",
              "probably\nan element that has been deleted. You can get the",
              " NEXT or PREVIOUS\nelements though", (char *) NULL);
           retValue = TCL_ERROR;
       }  else if (g_shChainErrno == SH_CHAIN_INVALID_CURSOR)  {
           Tcl_AppendResult(a_interp, "chainWalk: ", pCursorName, 
              " is an invalid cursor. Please create\na valid one using",
              " TCL extension chainCursorNew", (char *) NULL);
           retValue = TCL_ERROR;
       }
   }  else  {
       /*
        * We have the address of the element we want. See if a handle is
        * already bound to it. If so, return the handle name, else create
        * and bind a new handle to the address
        */
       if (p_shTclHandleNameGet(a_interp, pData) == TCL_ERROR)  {
           char   handleBuf[HANDLE_NAMELEN];
           HANDLE hndl;

           if (p_shTclHandleNew(a_interp, handleBuf) == TCL_OK)  {
               hndl.ptr  = pData;
               hndl.type = shChainElementTypeGetByCursor(pChain, *cp);

               if (p_shTclHandleAddrBind(a_interp, hndl, handleBuf) == TCL_OK)
                   Tcl_SetResult(a_interp, handleBuf, TCL_VOLATILE);
               else  {
                   Tcl_AppendResult(a_interp, "chainWalk: cannot bind",
                      " handle to a chain element", (char *)NULL);
                   retValue = TCL_ERROR;
               }
	   } else  {
               Tcl_AppendResult(a_interp, "chainWalk: cannot get a new",
                  " handle", (char *) NULL);
               retValue = TCL_ERROR;
           }
       }
   }

   return retValue;
}

/*
 * DESCRIPTION:
 *   Create an index for a chain
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *
 *   interp->result will contain the reason for the error.
 */
static char *g_chainIndexMake = "chainIndexMake";
static ftclArgvInfo g_chainIndexMakeTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Create an index for the given chain, " \
      "making access to elements _much_ faster"},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Handle to the chain" },
   {"-by_pos", FTCL_ARGV_CONSTANT, (void *)1, NULL,
				       "Index by position in chain (default)" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};
static int
tclChainIndexMake(ClientData clientData, Tcl_Interp *interp, int ac, char **av)
{
   int by_pos = 1;
   char   *chainName = NULL;
   CHAIN  *chain;
   int flags = FTCL_ARGV_NO_LEFTOVERS;
   HANDLE handle;
   void *pUnused = NULL;
   int type = SH_CHAIN_INDEX_BY_POS;	/* type of index */

   g_chainIndexMakeTbl[1].dst = &chainName;
   g_chainIndexMakeTbl[2].dst = &by_pos;

   if(shTclParseArgv(interp, &ac, av, g_chainIndexMakeTbl, flags,
				      g_chainIndexMake) != FTCL_ARGV_SUCCESS)  {
       return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if (shTclHandleExprEval(interp, chainName, &handle, &pUnused) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (handle.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp, "chainIndexMake: Handle ", chainName, 
		       "is a ", shNameGetFromType(handle.type), " not a CHAIN", 
		       (char *) NULL);
      return(TCL_ERROR);
   }
   chain = handle.ptr;

   if(by_pos) {
      type = SH_CHAIN_INDEX_BY_POS;
   }
/*
 * work
 */
   if(shChainIndexMake(chain, type) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*
 * DESCRIPTION:
 *   Delete a chain's index
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *
 *   interp->result will contain the reason for the error.
 */
static char *g_chainIndexDel = "chainIndexDel";
static ftclArgvInfo g_chainIndexDelTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Delete the given chain's index"},
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL, "Handle to the chain" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};
static int
tclChainIndexDel(ClientData clientData, Tcl_Interp *interp, int ac, char **av)
{
   char *chainName = NULL;
   CHAIN *chain;
   int flags = FTCL_ARGV_NO_LEFTOVERS;
   HANDLE handle;
   void *pUnused = NULL;

   g_chainIndexDelTbl[1].dst = &chainName;

   if(shTclParseArgv(interp, &ac, av, g_chainIndexDelTbl, flags,
				      g_chainIndexDel) != FTCL_ARGV_SUCCESS)  {
       return(TCL_ERROR);
   }
/*
 * process arguments
 */
   if (shTclHandleExprEval(interp, chainName, &handle, &pUnused) != TCL_OK) {
      return(TCL_ERROR);
   }
   if (handle.type != shTypeGetFromName("CHAIN")) {
      Tcl_AppendResult(interp, "chainIndexDel: Handle ", chainName, 
		       "is a ", shNameGetFromType(handle.type), " not a CHAIN", 
		       (char *) NULL);
      return(TCL_ERROR);
   }
   chain = handle.ptr;
/*
 * work
 */
   shChainIndexDel(chain);

   return(TCL_OK);
}

/*
 * ROUTINE: tclChainCursorCopy()
 *
 * DESCRIPTION:
 *   tclChainCursorCopy() copies a cursor. It returns, in Tcl_Interp, a 
 *   handle bound to the newly copied cursor.
 *
 * CALL:
 *   (static int) tclChainCursorCopy(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainCursorCopy = "chainCursorCopy";
static ftclArgvInfo g_chainCursorCopyTbl[] = {
  {NULL, FTCL_ARGV_HELP, NULL, NULL,
   "Copy an existing chain cursor\n" },
  {"<chain>",  FTCL_ARGV_STRING, NULL, NULL, "Handle to a valid chain"},
  {"<cursor>", FTCL_ARGV_STRING, NULL, NULL, "Handle to existing cursor"},
  {(char *) NULL, FTCL_ARGV_END, NULL, NULL, (char *) NULL}
};

static int
tclChainCursorCopy(ClientData a_clientData, Tcl_Interp *a_interp, int a_argc,
                   char **a_argv)
{
   char    *pChainName, *pOldCursorName, handleBuf[HANDLE_NAMELEN];
   void    *pUnused;
   HANDLE   hChain, hOldCursor, hNewCursor;
   CURSOR_T *pNewCursor, *pOldCursor;
   int      retStatus;
   RET_CODE switcheroo; /* this is to avoid big numbers in switch statements */

   pChainName = pOldCursorName = NULL;

   g_chainCursorCopyTbl[1].dst = &pChainName;
   g_chainCursorCopyTbl[2].dst = &pOldCursorName;   

   if ((retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
        g_chainCursorCopyTbl, 0, g_chainCursorCopy)) != FTCL_ARGV_SUCCESS) { 
       return(retStatus);
   }

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (shTclHandleExprEval(a_interp, pOldCursorName, &hOldCursor, &pUnused) 
       != TCL_OK) 
       return TCL_ERROR;

   Tcl_ResetResult(a_interp);

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
       Tcl_AppendResult(a_interp, "chainCursorCopy: Handle ", pChainName,
          " is a ", shNameGetFromType(hChain.type), " not a CHAIN",
          (char *) NULL);
       return TCL_ERROR;
   }

   if (hOldCursor.type != shTypeGetFromName("CURSOR"))  {
       Tcl_AppendResult(a_interp, "chainCursorCopy: Handle ", pOldCursorName,
          " is a ", shNameGetFromType(hChain.type), " not a CURSOR",
          (char *) NULL);
       return TCL_ERROR;
   }

   pNewCursor = (CURSOR_T *) shMalloc(sizeof(CURSOR_T));
   pOldCursor = (CURSOR_T *) hOldCursor.ptr;   


   /* just a little workaround to avoid the limits of the osf switch statement */
   switcheroo = shChainCursorCopy((CHAIN *)hChain.ptr, *pOldCursor, pNewCursor);
   if( switcheroo != SH_SUCCESS ) { 
     switch (shChainCursorCopy((CHAIN *)hChain.ptr, *pOldCursor, pNewCursor)) 
       {	 
       case SH_CHAIN_INVALID_CURSOR:
	 Tcl_AppendResult(a_interp, "chainCursorCopy: cursor ", pOldCursorName,
			  " is an invalid cursor", (char *) NULL);
	 return TCL_ERROR;
       default: Tcl_AppendResult(a_interp, "chainCursorCopy: Unknown Error", (char*) NULL );
	 return TCL_ERROR;
       }
   }
   
   hNewCursor.ptr = (void *) pNewCursor;
   hNewCursor.type = shTypeGetFromName("CURSOR");

   if (p_shTclHandleNew(a_interp, handleBuf) != TCL_OK)  {
       Tcl_SetResult(a_interp,
          "chainCursorCopy: cannot obtain a handle for cursor", TCL_VOLATILE);
          return TCL_ERROR;
   }

   if (p_shTclHandleAddrBind(a_interp, hNewCursor, handleBuf) != TCL_OK)  {
       Tcl_SetResult(a_interp,
          "chainCursorCopy: cannot bind address to the new cursor handle",
                     TCL_VOLATILE);
       return TCL_ERROR;
   }

   Tcl_SetResult(a_interp, handleBuf, TCL_VOLATILE);

   return TCL_OK;
}

/*
 * ROUTINE: tclChainSort()
 *
 * DESCRIPTION:
 *   tclChainSort() sorts a chain depending on the field specified. Also
 *   specified is the direction of the sort (increasing or decreasing)
 *   This function is called in response to the TCL extension "chainSort"
 *
 * CALL:
 *   (static int) tclChainSort(ClientData a_clientData, 
 *                Tcl_Interp *a_interp, int a_argc, char **a_argv)
 *   a_clientData - Unused
 *   a_interp     - TCL interpreter
 *   a_argc       - TCL command line argument count
 *   a_argv       - TCL command line argument array
 *
 * RETURNS:
 *   TCL_ERROR : on failure
 *   TCL_OK    : otherwise.
 *   a_interp->result will contain the reason for the error.
 */
static char *g_chainSort = "chainSort";
static ftclArgvInfo g_chainSortTbl[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Sort a chain depending on a specified field\n" },
   {"<chain>", FTCL_ARGV_STRING, NULL, NULL,
    "Handle to the chain" },
   {"<field>", FTCL_ARGV_STRING, NULL, NULL,
    "Field name to sort on" },
   {"-increasing", FTCL_ARGV_CONSTANT, (void *)1, NULL,
    "Direction of the sort" },
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclChainSort(ClientData a_clientData, Tcl_Interp *a_interp,
             int a_argc, char **a_argv)
{
   char     *pChainName, *pFieldName;
   void     *pUnused;
   HANDLE   hChain;
   int      flags,
            increasing;
   int      retStatus;
   RET_CODE rc;

   flags = FTCL_ARGV_NO_LEFTOVERS;
   increasing = 0;

   g_chainSortTbl[1].dst  = &pChainName;
   g_chainSortTbl[2].dst  = &pFieldName;
   g_chainSortTbl[3].dst  = &increasing;

   retStatus = shTclParseArgv(a_interp, &a_argc, a_argv, 
                              g_chainSortTbl, flags, g_chainSort);
   if (retStatus != FTCL_ARGV_SUCCESS)  {
       return(retStatus);
   }

   Tcl_ResetResult(a_interp);

   if (shTclHandleExprEval(a_interp, pChainName, &hChain, &pUnused) != TCL_OK)
       return TCL_ERROR;

   if (hChain.type != shTypeGetFromName("CHAIN"))  {
      Tcl_AppendResult(a_interp, "chainSort: Handle ", pChainName, 
         "is a ", shNameGetFromType(hChain.type), " not a CHAIN", 
         (char *) NULL);
      return TCL_ERROR;
   }

   rc = shChainSort((CHAIN *)hChain.ptr, pFieldName, increasing);
   if (rc != SH_SUCCESS)  { 
       if (rc == SH_TYPE_MISMATCH)
           Tcl_AppendResult(a_interp, "chainSort: chain ", pChainName,
                            " is of type GENERIC; cannot sort a ",
                            "heterogeneous chain", (char *) NULL);
       else if (rc == SH_FLD_SRCH_ERR)
           Tcl_AppendResult(a_interp, "chainSort: field ", pFieldName,
                            " is an invalid field; could not find it in the",
                            " schema table", (char *) NULL);
       else if (rc == SH_BAD_SCHEMA)
           Tcl_AppendResult(a_interp, "chainSort: bad schema encountered",
                            (char *) NULL);
       return TCL_ERROR;
   }
   
   return TCL_OK;
}

/**********************************************************************/
static char *g_chainCompare = "chainCompare";
static ftclArgvInfo g_chainCompareTbl[] = {
    {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Compare chain1 and chain2.  Based on the row and col names for the two \n"
    "chains, for each member of chain1, find the member of chain2 that is \n"
    "closest to it.  Put a link pointing to that member of chain2 on the \n"
    "closest chain. \n"  
    "If the distance between the two elements is less than minimumDelta,\n"
    "then also set the corresponding value in the vMask to 1.\n" },
    {"<chain1>",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"<chain2>",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"-row1Name",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"-col1Name",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"-row2Name",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"-col2Name",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"-minimumDelta",    FTCL_ARGV_DOUBLE, NULL, NULL, NULL},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };

static int 
tclChainCompare(ClientData clientData,Tcl_Interp *interp, int argc, char **argv)
{
  CHAIN *chain1=NULL;
  CHAIN *chain2=NULL;
  CHAIN *closest;
  VECTOR *vMask;
  int   retStatus;
  char *chain1NamePtr = NULL, *chain2NamePtr=NULL;
  char closestName[HANDLE_NAMELEN], vMaskName[HANDLE_NAMELEN];
  double minimumDelta = 2.0;
#define MAX_NAMELEN 30
  char* row1NamePtr="row";
  char* col1NamePtr = "col";
  char* row2NamePtr = "row";
  char* col2NamePtr = "col";

  char answer[100];

  g_chainCompareTbl[1].dst= &chain1NamePtr;
  g_chainCompareTbl[2].dst= &chain2NamePtr;
  g_chainCompareTbl[3].dst= &row1NamePtr;
  g_chainCompareTbl[4].dst= &col1NamePtr;
  g_chainCompareTbl[5].dst= &row2NamePtr;
  g_chainCompareTbl[6].dst= &col2NamePtr;
  g_chainCompareTbl[7].dst= &minimumDelta;
  
  retStatus = shTclParseArgv(interp, &argc, argv, g_chainCompareTbl, 
                             FTCL_ARGV_NO_LEFTOVERS, g_chainCompare); 
  if (retStatus == FTCL_ARGV_SUCCESS) {
    
    if (shTclAddrGetFromName
	(interp, chain1NamePtr, (void **) &chain1, "CHAIN") != TCL_OK) {
      
      Tcl_SetResult(interp, "Bad chain1 name", TCL_VOLATILE);
      return(TCL_ERROR);
    }
    
    if (shTclAddrGetFromName
	(interp, chain2NamePtr, (void **) &chain2, "CHAIN") != TCL_OK) {
      
      Tcl_SetResult(interp, "Bad chain2 name", TCL_VOLATILE);
      return(TCL_ERROR);
    }
    
  } else {
    return(retStatus);
  }

  if (shTclChainHandleNew
      (interp, closestName, "GENERIC", &closest) != TCL_OK) {
    Tcl_SetResult(interp, "Can't do shTclChainHandleNew ", TCL_STATIC);
    return TCL_ERROR;
  }
  vMask = shVectorNew(shChainSize(chain1));
  if (shTclHandleNew(interp, vMaskName, "VECTOR", vMask) != TCL_OK) {
    Tcl_SetResult(interp, "Can't bind to new VECTOR handle", TCL_STATIC);
    return(TCL_ERROR);
  }
  if (shChainCompare(chain1, row1NamePtr, col1NamePtr,
		  chain2, row2NamePtr, col2NamePtr,
		  minimumDelta, closest, vMask) != SH_SUCCESS) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, "", 0);
  sprintf(answer, "vMask %s", vMaskName);
  Tcl_AppendElement(interp, answer);
  sprintf(answer, "chain %s", closestName);
  Tcl_AppendElement(interp, answer);
  return(TCL_OK);
}
/**********************************************************************/
static char *g_chainCompare2 = "chainCompare2";
static ftclArgvInfo g_chainCompare2Tbl[] = {
    {NULL, FTCL_ARGV_HELP, NULL, NULL,
    "Compare chain1 and chain2.  Based on the row and col names for the two \n"
    "chains, for each member of chain1, find the member of chain2 that is \n"
    "closest to it.  Put a link pointing to that member of chain2 on the \n"
    "closest chain. \n"},
    {"<chain1>",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"<chain2>",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"-row1Name",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"-col1Name",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"-row2Name",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {"-col2Name",    FTCL_ARGV_STRING, NULL, NULL, NULL},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };

static int 
tclChainCompare2(ClientData clientData,Tcl_Interp *interp, int argc, char **argv)
{
  CHAIN *chain1=NULL;
  CHAIN *chain2=NULL;
  CHAIN *closest;
  int   retStatus;
  char *chain1NamePtr = NULL, *chain2NamePtr=NULL;
  char closestName[HANDLE_NAMELEN]; /* afMaskName[HANDLE_NAMELEN]; NOT USED HERE */
#define MAX_NAMELEN 30
  char* row1NamePtr="row";
  char* col1NamePtr = "col";
  char* row2NamePtr = "row";
  char* col2NamePtr = "col";

  g_chainCompare2Tbl[1].dst= &chain1NamePtr;
  g_chainCompare2Tbl[2].dst= &chain2NamePtr;
  g_chainCompare2Tbl[3].dst= &row1NamePtr;
  g_chainCompare2Tbl[4].dst= &col1NamePtr;
  g_chainCompare2Tbl[5].dst= &row2NamePtr;
  g_chainCompare2Tbl[6].dst= &col2NamePtr;
  
  retStatus = shTclParseArgv(interp, &argc, argv, g_chainCompare2Tbl, 
                             FTCL_ARGV_NO_LEFTOVERS, g_chainCompare2); 
  if (retStatus == FTCL_ARGV_SUCCESS) {
    
    if (shTclAddrGetFromName
	(interp, chain1NamePtr, (void **) &chain1, "CHAIN") != TCL_OK) {
      
      Tcl_SetResult(interp, "Bad chain1 name", TCL_VOLATILE);
      return(TCL_ERROR);
    }
    
    if (shTclAddrGetFromName
	(interp, chain2NamePtr, (void **) &chain2, "CHAIN") != TCL_OK) {
      
      Tcl_SetResult(interp, "Bad chain2 name", TCL_VOLATILE);
      return(TCL_ERROR);
    }
    
  } else {
    return(retStatus);
  }

  if (shTclChainHandleNew
      (interp, closestName, "GENERIC", &closest) != TCL_OK) {
    Tcl_SetResult(interp, "Can't do shTclChainHandleNew ", TCL_STATIC);
    return TCL_ERROR;
  }
  if (shChainCompare2(chain1, row1NamePtr, col1NamePtr,
		  chain2, row2NamePtr, col2NamePtr,
		  closest) != SH_SUCCESS) {
    shTclInterpAppendWithErrStack(interp);
    return(TCL_ERROR);
  }
  Tcl_SetResult(interp, closestName, 0);
  return(TCL_OK);
}

void
shTclChainDeclare(Tcl_Interp *a_interp)
{
   char *tclHelpFacil = "shChain";
   int flags = FTCL_ARGV_NO_LEFTOVERS;
   
   shTclDeclare(a_interp, g_chainNew, (Tcl_CmdProc *)tclChainNew,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainNewTbl, flags, g_chainNew), 
                shTclGetUsage(a_interp, g_chainNewTbl, flags, g_chainNew));

   shTclDeclare(a_interp, "chainDel", (Tcl_CmdProc *)tclChainDel,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainDelTbl, flags, g_chainDel), 
                shTclGetUsage(a_interp, g_chainDelTbl, flags, g_chainDel));

   /*
    * This TCL extension is as yet unavailaible. See the comments at the
    * definition of tclChainDestroy() for the reason.
    *
   shTclDeclare(a_interp, "chainDestroy", (Tcl_CmdProc *)tclChainDestroy,
          (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
          shTclGetArgInfo(a_interp, g_chainDestroyTbl, flags, g_chainDestroy), 
          shTclGetUsage(a_interp, g_chainDestroyTbl, flags, g_chainDestroy));
   */

   shTclDeclare(a_interp, "chainElementAddByPos", 
                (Tcl_CmdProc *)tclChainElementAddByPos,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainElementAddByPosTbl, flags, 
                                g_chainElementAddByPos), 
                shTclGetUsage(a_interp, g_chainElementAddByPosTbl, flags, 
                              g_chainElementAddByPos));

   shTclDeclare(a_interp, "chainElementTypeGetByPos", 
                (Tcl_CmdProc *)tclChainElementTypeGetByPos,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainElementTypeGetByPosTbl, 
                                flags, g_chainElementTypeGetByPos), 
                shTclGetUsage(a_interp, g_chainElementTypeGetByPosTbl, flags, 
                              g_chainElementTypeGetByPos));

   shTclDeclare(a_interp, "chainElementGetByPos", 
                (Tcl_CmdProc *)tclChainElementGetByPos,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainElementGetByPosTbl, flags, 
                                g_chainElementGetByPos), 
                shTclGetUsage(a_interp, g_chainElementGetByPosTbl, flags, 
                              g_chainElementGetByPos));

   shTclDeclare(a_interp, "chainElementRemByPos", 
                (Tcl_CmdProc *)tclChainElementRemByPos,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainElementRemByPosTbl, flags, 
                                g_chainElementRemByPos), 
                shTclGetUsage(a_interp, g_chainElementRemByPosTbl, flags, 
                              g_chainElementRemByPos));

   shTclDeclare(a_interp, "chainSize", 
                (Tcl_CmdProc *)tclChainSize,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainSizeTbl, flags, g_chainSize), 
                shTclGetUsage(a_interp, g_chainSizeTbl, flags, g_chainSize));

   shTclDeclare(a_interp, "chainTypeGet", 
                (Tcl_CmdProc *)tclChainTypeGet,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainTypeGetTbl, flags, 
                                g_chainTypeGet), 
                shTclGetUsage(a_interp, g_chainTypeGetTbl, flags, 
                              g_chainTypeGet));

   shTclDeclare(a_interp, "chainTypeSet", 
                (Tcl_CmdProc *)tclChainTypeSet,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainTypeSetTbl, flags, 
                                g_chainTypeSet), 
                shTclGetUsage(a_interp, g_chainTypeSetTbl, flags, 
                              g_chainTypeSet));

   shTclDeclare(a_interp, "chainJoin", 
                (Tcl_CmdProc *)tclChainJoin,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainJoinTbl, flags, g_chainJoin), 
                shTclGetUsage(a_interp, g_chainJoinTbl, flags, g_chainJoin));

   shTclDeclare(a_interp, "chainCopy", 
                (Tcl_CmdProc *)tclChainCopy,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainCopyTbl, flags, g_chainCopy), 
                shTclGetUsage(a_interp, g_chainCopyTbl, flags, g_chainCopy));

   shTclDeclare(a_interp, "chainTypeDefine", 
                (Tcl_CmdProc *)tclChainTypeDefine,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainTypeDefineTbl, flags, 
                                g_chainTypeDefine), 
                shTclGetUsage(a_interp, g_chainTypeDefineTbl, flags, 
                              g_chainTypeDefine));

   shTclDeclare(a_interp, "chainCursorNew", 
                (Tcl_CmdProc *)tclChainCursorNew,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainCursorNewTbl, flags, 
                                g_chainCursorNew), 
                shTclGetUsage(a_interp, g_chainCursorNewTbl, flags, 
                              g_chainCursorNew));

   shTclDeclare(a_interp, "chainCursorDel", 
                (Tcl_CmdProc *)tclChainCursorDel,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainCursorDelTbl, flags, 
                                g_chainCursorDel), 
                shTclGetUsage(a_interp, g_chainCursorDelTbl, flags, 
                              g_chainCursorDel));

   shTclDeclare(a_interp, "chainCursorCount", 
                (Tcl_CmdProc *)tclChainCursorCount,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainCursorCountTbl, flags, 
                                g_chainCursorCount), 
                shTclGetUsage(a_interp, g_chainCursorCountTbl, flags, 
                              g_chainCursorCount));

   shTclDeclare(a_interp, "chainCursorSet", 
                (Tcl_CmdProc *)tclChainCursorSet,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainCursorSetTbl, flags, 
                                g_chainCursorSet), 
                shTclGetUsage(a_interp, g_chainCursorSetTbl, flags, 
                              g_chainCursorSet));

   shTclDeclare(a_interp, "chainElementAddByCursor", 
                (Tcl_CmdProc *)tclChainElementAddByCursor,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainElementAddByCursorTbl, flags,
                                g_chainElementAddByCursor), 
                shTclGetUsage(a_interp, g_chainElementAddByCursorTbl, flags, 
                              g_chainElementAddByCursor));

   shTclDeclare(a_interp, "chainElementTypeGetByCursor", 
                (Tcl_CmdProc *)tclChainElementTypeGetByCursor,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainElementTypeGetByCursorTbl, 
                                flags, g_chainElementTypeGetByCursor), 
                shTclGetUsage(a_interp, g_chainElementTypeGetByCursorTbl, 
                              flags, g_chainElementTypeGetByCursor));

   shTclDeclare(a_interp, "chainElementRemByCursor", 
                (Tcl_CmdProc *)tclChainElementRemByCursor,
                (ClientData)0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainElementRemByCursorTbl, flags,
                                g_chainElementRemByCursor), 
                shTclGetUsage(a_interp, g_chainElementRemByCursorTbl, flags, 
                              g_chainElementRemByCursor));

   shTclDeclare(a_interp, "chainWalk",
                (Tcl_CmdProc *)tclChainWalk, (ClientData)0, 
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, 
                shTclGetArgInfo(a_interp, g_chainWalkTbl, flags, g_chainWalk), 
                shTclGetUsage(a_interp, g_chainWalkTbl, flags, g_chainWalk));

   shTclDeclare(a_interp, "chainWalk",
                (Tcl_CmdProc *)tclChainWalk, (ClientData)0, 
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, 
                shTclGetArgInfo(a_interp, g_chainWalkTbl, flags, g_chainWalk), 
                shTclGetUsage(a_interp, g_chainWalkTbl, flags, g_chainWalk));

   shTclDeclare(a_interp, "chainIndexMake",
                (Tcl_CmdProc *)tclChainIndexMake, (ClientData)0, 
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, 
                shTclGetArgInfo(a_interp, g_chainIndexMakeTbl, flags, g_chainIndexMake), 
                shTclGetUsage(a_interp, g_chainIndexMakeTbl, flags, g_chainIndexMake));

   shTclDeclare(a_interp, "chainIndexDel",
                (Tcl_CmdProc *)tclChainIndexDel, (ClientData)0, 
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, 
                shTclGetArgInfo(a_interp, g_chainIndexDelTbl, flags, g_chainIndexDel), 
                shTclGetUsage(a_interp, g_chainIndexDelTbl, flags, g_chainIndexDel));

   shTclDeclare(a_interp, "chainSort", (Tcl_CmdProc *) tclChainSort,
                (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainSortTbl, flags, g_chainSort), 
                shTclGetUsage(a_interp, g_chainSortTbl, flags, g_chainSort));

   shTclDeclare(a_interp,"chainCompare", 
		(Tcl_CmdProc *)tclChainCompare, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		tclHelpFacil, 
                shTclGetArgInfo(a_interp, g_chainCompareTbl, flags, 
                                g_chainCompare), 
                shTclGetUsage(a_interp, g_chainCompareTbl, flags, 
                              g_chainCompare));

   shTclDeclare(a_interp,"chainCompare2", 
		(Tcl_CmdProc *)tclChainCompare2, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		tclHelpFacil, 
                shTclGetArgInfo(a_interp, g_chainCompare2Tbl, flags, 
                                g_chainCompare2), 
                shTclGetUsage(a_interp, g_chainCompare2Tbl, flags, 
                              g_chainCompare2));

   shTclDeclare(a_interp, "chainCursorCopy", 
                (Tcl_CmdProc *) tclChainCursorCopy,
                (ClientData) 0, (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_chainCursorCopyTbl, flags, 
                                g_chainCursorCopy), 
                shTclGetUsage(a_interp, g_chainCursorCopyTbl, flags, 
                              g_chainCursorCopy));

   return;
}

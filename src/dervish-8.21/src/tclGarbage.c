/*
 * tclGarbage.c - TCL extensions for Dervish Garbage Collection/Memory Management
 *
 * Vijay K. Gurbani
 * (C) 1994 All Rights Reserved
 * Fermi National Accelerator Laboratory
 * Batavia, Illinois USA
 *
 * ----------------------------------------------------------------------------
 *
 * FILE:
 *   tclGarbage.c
 *
 * ABSTRACT:
 *   This file contains routines to provide TCL extensions for Dervish
 *   Garbage Collection/Memory Management routines. 
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>  /* For getpid() */

#include "ftcl.h"
#include "shCGarbage.h"
#include "shTclUtils.h"
#include "shTclHandle.h"
#include "shTclParseArgv.h"
#include "shTclErrStack.h"
#include "shC.h"

/*
 * Local TCL function prototypes. These functions are called in response
 * to the TCL extensions entered by the user
 */
#if 0
static int  tclMemSerialNumberPrint(ClientData, Tcl_Interp *, int, char **);
static int  tclMemFreeBlocks(ClientData, Tcl_Interp *, int, char **);
static int  tclMemBytesInUse(ClientData, Tcl_Interp *, int, char **);
static int  tclMemStatsPrint(ClientData, Tcl_Interp *, int, char **);
static int  tclMemBlocksPrint(ClientData, Tcl_Interp *, int, char **);
static int  tclMemAMPSize(ClientData, Tcl_Interp *, int, char **);
#endif
static void doTheJob(Tcl_Interp *, unsigned long (*)(void));
static void memPrint(SH_MEMORY *const);
static void memGet(SH_MEMORY *const);
static void memFree(void *);

#ifdef DEBUG_MEM

extern void pr_Amp(void);
extern void pr_Fmp(const int);

/*
 * These two functions are called only if the source has been compiled with
 * the token DEBUG_MEM defined. They display the AMP and the indexed FMP
 * respectively.
 */
static int  tclPr_Amp(ClientData, Tcl_Interp *, int, char **);
static int  tclPr_Fmp(ClientData, Tcl_Interp *, int, char **);

#endif

/*
 * Global variables used in this file only
 */
static char g_memFreeBlocks[] = "memFreeBlocks";
static Tcl_Interp   *g_interp = NULL;
static FILE         *g_Fp = NULL;


static ftclArgvInfo g_argTable[] = {
       { NULL,          FTCL_ARGV_HELP, NULL, NULL, 
	   "Free memory blocks bounded by two serial numbers\n"},
       {"<LOW_BOUND>",  FTCL_ARGV_INT,   NULL, NULL,
	  "Lower bound on serial number"},
       {"<HIGH_BOUND>", FTCL_ARGV_INT,   NULL, NULL,
	  "Higher bound on serial number"},
       {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };
/*
 * ROUTINE: tclMemFreeBlocks()
 *
 * DESCRIPTION:
 *   This TCL extension frees memory blocks bounded by two serial numbers
 *
 * CALL:
 *   (static int) tclMemFreeBlocks(ClientData a_clientData, 
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
static int tclMemFreeBlocks(ClientData a_clientData, Tcl_Interp *a_interp,
                       int a_argc, char **a_argv)
{
   int i, flags, rstatus;
   unsigned int lo, hi;

   flags = FTCL_ARGV_NO_LEFTOVERS;

   g_argTable[1].dst  = &lo;
   g_argTable[2].dst  = &hi;

   if ((rstatus = shTclParseArgv(a_interp, &a_argc, a_argv, g_argTable, flags,
				 g_memFreeBlocks))
       != FTCL_ARGV_SUCCESS)  {
       return rstatus;
   }

   Tcl_ResetResult(a_interp);

   g_interp = a_interp;

   /*
    * For return values of shMemFreeBlocks(), please see the documentation of
    * that function. The next interesting thing in the call to 
    * shMemFreeBlocks() is it's last parameter, memFree(). This function is
    * called for all blocks being freed. The interesting thing is that 
    * dervish increments a block's reference counter when a handle is bound
    * to that memory block (see p_shTclHandleAddrBind()). Consequently,
    * when deleteing (or freeing) a block, the block is NOT freed until the
    * handle bound to it is deleted by calling p_shTclHandleDel(). Thus,
    * memFree() does both things: 
    *   1) it shFree()'s the memory block, and
    *   2) it calls p_shTclHandleDel() for that memory block.
    * Note that if a handle is not bound to a block, then memFree() simply
    * calls shFree() on that block.
    */
   i = shMemFreeBlocks(lo, hi, memFree);
   if (i == 2)  {
       Tcl_SetResult(a_interp, 
          "memFreeBlocks: <LOW_BOUND> must be less then <HIGH_BOUND>",
          TCL_VOLATILE);
       return TCL_ERROR;
   }

   Tcl_ResetResult(a_interp);    

   return TCL_OK;
}

static void memFree(void *addr)
{
   char hBuf[HANDLE_NAMELEN];

   if (p_shTclHandleNameGet(g_interp, addr) == TCL_OK)  {
       strcpy(hBuf, g_interp->result);
       shFree(addr);
       p_shTclHandleDel(g_interp, hBuf);
   } else
       shFree(addr);
}

/*
 * Utility function called by various TCL extensions 
 */
static void doTheJob(Tcl_Interp *a_interp, unsigned long (*funcp)(void))
{
   char retValue[15];

   sprintf(retValue, "%ld", (*funcp)());

   Tcl_ResetResult(a_interp);
   Tcl_SetResult(a_interp, retValue, TCL_VOLATILE);
}
/*
 * ROUTINE: tclMemSerialNumberPrint()
 *
 * DESCRIPTION:
 *   This TCL extension gets the current memory allocation serial number
 *
 * CALL:
 *   (static int) tclMemSerialNumberPrint(ClientData a_clientData, 
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
static int tclMemSerialNumberPrint(ClientData a_clientData, 
				   Tcl_Interp *a_interp,
				   int a_argc, char **a_argv) /* NOTUSED */
{
   doTheJob(a_interp, shMemSerialNumberGet);

   return TCL_OK;
}

/*
 * Return the total number of bytes allocated so far from the OS.
 */
static int tclMemTotalBytes(ClientData a_clientData, Tcl_Interp *a_interp,
			    int a_argc, char **a_argv) /* NOTUSED */
{
   doTheJob(a_interp, shMemTotalBytesMalloced);

   return TCL_OK;   
}

/*
 *   This TCL extension prints the total bytes allocated so far from the
 *   OS, and utilised by shMalloc/Realloc. This includes memory allocated
 *   for internal bookkeeping headers
 */
static int tclMemActualBytes(ClientData a_clientData, Tcl_Interp *a_interp,
			    int a_argc, char **a_argv) /* NOTUSED */
{
   doTheJob(a_interp, shMemActualBytesMalloced);

   return TCL_OK;   
}

/*
 * ROUTINE: tclMemBytesInUse()
 *
 * DESCRIPTION:
 *   This TCL extension prints the total bytes in use; i.e. it prints the
 *   bytes currently in the AMP.
 *
 * CALL:
 *   (static int) tclMemBytesInUse(ClientData a_clientData, 
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
static int tclMemBytesInUse(ClientData a_clientData, Tcl_Interp *a_interp,
			    int a_argc, char **a_argv) /* NOTUSED */
{
   doTheJob(a_interp, shMemBytesInUse);

   return TCL_OK;   
}

/*
 * ROUTINE: tclMemBytesInPool()
 *
 * DESCRIPTION:
 *   This TCL extension prints the total bytes in the FMP.
 *
 * CALL:
 *   (static int) tclMemBytesInPool(ClientData a_clientData, 
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
static int tclMemBytesInPool(ClientData a_clientData, Tcl_Interp *a_interp,
			     int a_argc, char **a_argv) /* NOTUSED */
{
   doTheJob(a_interp, shMemBytesInPool);

   return TCL_OK;   
}

/*
 * ROUTINE: tclMemNumMallocs()
 *
 * DESCRIPTION:
 *   This TCL extension prints total number of times shMalloc() was called
 *
 * CALL:
 *   (static int) tclMemNumMallocs(ClientData a_clientData,
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
static int tclMemNumMallocs(ClientData a_clientData, Tcl_Interp *a_interp,
			    int a_argc, char **a_argv) /* NOTUSED */
{
   doTheJob(a_interp, shMemNumMallocs);

   return TCL_OK;   
}

/*
 * ROUTINE: tclMemNumFrees()
 *
 * DESCRIPTION:
 *   This TCL extension prints total number of times shFree() was called
 *
 * CALL:
 *   (static int) tclMemNumFrees(ClientData a_clientData,
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
static int tclMemNumFrees(ClientData a_clientData, Tcl_Interp *a_interp,
			  int a_argc, char **a_argv) /* NOTUSED */
{
   doTheJob(a_interp, shMemNumFrees);

   return TCL_OK;   
}

/*
 * ROUTINE: tclMemAMPSize()
 *
 * DESCRIPTION:
 *   This TCL extension prints total number of blocks currently in the AMP
 *
 * CALL:
 *   (static int) tclMemAMPSize(ClientData a_clientData,
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
static int tclMemAMPSize(ClientData a_clientData, Tcl_Interp *a_interp, 
                         int a_argc, char **a_argv) /* NOTUSED */
{
   doTheJob(a_interp, shMemAMPSize);

   return TCL_OK;
}

/*
 * ROUTINE: tclMemStatsPrint()
 *
 * DESCRIPTION:
 *   This TCL extension prints interesting statistics about DERVISH memory
 *   allocation
 *
 * CALL:
 *   (static int) tclMemStatsPrint(ClientData a_clientData,
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
static int tclMemStatsPrint(ClientData a_clientData,
			    Tcl_Interp *a_interp, /* NOTUSED */
			    int a_argc, char **a_argv) /* NOTUSED */
{
   shMemStatsPrint();

   return TCL_OK;
}

/*
 * ROUTINE: tclMemBlocksPrint()
 *
 * DESCRIPTION:
 *   This TCL extension prints the contents of the allocated memory pool (AMP)
 *
 * 
 * CALL:
 *   (static int) tclMemBlocksPrint(ClientData a_clientData,
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
static int tclMemBlocksPrint(ClientData a_clientData, Tcl_Interp *a_interp,
			     int a_argc, char **a_argv)	/* NOTUSED */
{
   if (shMemBytesInUse() == 0L)  {
       Tcl_ResetResult(a_interp);
       Tcl_SetResult(a_interp, "0", TCL_VOLATILE);
       return TCL_OK;
   }

   g_interp = a_interp;

#if defined (CHECK_LEAKS)
   printf("Serial No. Address\t     Size(Bytes)  File                 Line "
          "Type\n");
   printf("-------------------------------------------------------------"
          "------------------\n");
#else
   printf("Serial No.   Address\t\t   Size(Bytes)     Type\n");
   printf("--------------------------------------------------------\n");
#endif
   shMemTraverse(memPrint);

   return TCL_OK;
}

static void memPrint(SH_MEMORY *const mp)
{
   TYPE hType;
   char buf[HANDLE_NAMELEN];

   if (p_shTclHandleNameGet(g_interp, (void *) mp->pUser_memory) == TCL_OK)  {
       /*
        * Save handle name, interp result may get changed
        */
       strcpy(buf, g_interp->result);
       hType = p_shTclHandleTypeGet(g_interp, buf);
#if defined(CHECK_LEAKS)
       printf("%-10ld 0x%-17p %-10ld %-20s %4d  %s (%s)\n",
	      mp->serial_number, mp->pUser_memory, mp->user_bytes,
	      mp->file, mp->line, shNameGetFromType(hType), buf);
#else
       printf("%-10ld   0x%-18p  %-10ld      %s (%s)\n", mp->serial_number,
              mp->pUser_memory, mp->user_bytes,
	      shNameGetFromType(hType), buf);
#endif
   } else  {
       /*
        * Error getting the handle name; maybe there is no handle bound to
        * address. In this case, just print the address and the number of
        * bytes occupied by the address.
        */
#if defined(CHECK_LEAKS)
       printf("%-10ld 0x%-17p %-10ld %-20s %4d\n", mp->serial_number, 
              mp->pUser_memory, mp->user_bytes, mp->file, mp->line);
#else
       printf("%-10ld   0x%-18p  %-10ld\n", mp->serial_number, 
              mp->pUser_memory, mp->user_bytes);
#endif
   }

   Tcl_ResetResult(g_interp);
}

/*
 * ROUTINE: tclMemBlocksGet()
 *
 * DESCRIPTION:
 *   This TCL extension gets the contents of the allocated memory pool (AMP)
 *   in the form of a TCL list
 * 
 * CALL:
 *   (static int) tclMemBlocksGet(ClientData a_clientData,
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
static int tclMemBlocksGet(ClientData a_clientData, Tcl_Interp *a_interp,
			   int a_argc, char **a_argv) /* NOTUSED */
{
   char line[80];
   char *cp = NULL;
   char *cplast = NULL; 
   Tcl_ResetResult(a_interp);
   
   if (shMemBytesInUse() == 0L)
       return TCL_OK;

   g_interp = a_interp;

   /*
    * Since dervish and everything linked against it is already memory intensive,
    * let's not add to more memory fragmentation. In order to build a TCL list
    * of AMP, lets write the information to a temporary file and read in that
    * file into the TCL interp. If this is deemed slow, we can use Tcl_Merge()
    * to form lists on the fly and chew up some more memory.
    */
   g_Fp = tmpfile();
   if (g_Fp == NULL)  {
       Tcl_SetResult(a_interp, "memBlocksGet: cannot open temporary file",
          TCL_VOLATILE);
       return TCL_ERROR;
   }

   shMemTraverse(memGet);  /* Populate the file */

   Tcl_ResetResult(a_interp);

   rewind(g_Fp);

   while (fgets(line, sizeof(line) - 2, g_Fp) != NULL)  {
/*
    I think this shoul work better  N.K      
*/   
       cplast = line+sizeof(line) - 2;
       for (cp = line; cp<cplast; cp++) /* Get rid of new line at end */
            if (strcmp(cp, "\n") == 0)  {
                strcpy(cp,"\0");
                break;
            }
/*    */
       Tcl_AppendResult(g_interp, line, " ", (char *) NULL);
   }

   fclose(g_Fp);

   return TCL_OK;
}

static void memGet(SH_MEMORY *const mp)
{
   TYPE hType;
   char buf[HANDLE_NAMELEN];

   if (p_shTclHandleNameGet(g_interp, (void *)mp->pUser_memory) == TCL_OK)  {
         /*
          * Save handle name from interp result; interp result might get 
          * overwritten
          */
         sprintf(buf, g_interp->result);
         hType = p_shTclHandleTypeGet(g_interp, buf);
#if defined(CHECK_LEAKS)
         fprintf(g_Fp, "{"PTR_FMT" {%ld %ld %ld %s %d %s %s}}\n",
                 mp->pUser_memory, mp->serial_number,
		 mp->user_bytes, mp->actual_bytes,
                 mp->file, mp->line, buf, shNameGetFromType(hType));
#else
         fprintf(g_Fp, "{"PTR_FMT" {%ld %ld %ld %s %s}}\n",
                 mp->pUser_memory, mp->serial_number,
		 mp->user_bytes,  mp->actual_bytes,
		 buf, shNameGetFromType(hType));
#endif
   } else 
#if defined(CHECK_LEAKS)
         fprintf(g_Fp, "{"PTR_FMT" {%ld %ld %ld %s %d}}\n",mp->pUser_memory,
		 mp->serial_number, mp->user_bytes, mp->actual_bytes,
		 mp->file, mp->line);
#else
         fprintf(g_Fp, "{"PTR_FMT" {%ld %ld %ld}}\n",mp->pUser_memory,
		 mp->serial_number, mp->user_bytes,  mp->actual_bytes);
#endif

   return;
}

/*
 * Check all free and/or allocated memory
 */
static char *g_memCheck = "memCheck";

static ftclArgvInfo g_memCheck_Table[] = {
       { NULL,          FTCL_ARGV_HELP, NULL, NULL, 
	   "Check heap for corruption\n"},
       {"-abort",  FTCL_ARGV_CONSTANT, (void *)1, NULL,
	  "abort on encountering any corrupted block"},
       {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
    };

static int tclMemCheck(ClientData clientData, Tcl_Interp *interp,
			  int argc, char **argv)
{
   int flags, rstatus;
   int abort_on_error = 0;		/* abort on error? */
   int nbad;				/* number of bad blocks detected */

   flags = FTCL_ARGV_NO_LEFTOVERS;

   g_memCheck_Table[1].dst  = &abort_on_error;

   if((rstatus = shTclParseArgv(interp, &argc, argv, g_memCheck_Table,
			      flags, g_memCheck)) != FTCL_ARGV_SUCCESS)  {
       return(rstatus);
   }

   Tcl_ResetResult(interp);
/*
 * time to do the work
 */
   nbad = p_shMemCheck(1, 1, abort_on_error);

   if(nbad > 0)  {
      char buff[100];
      sprintf(buff,"memCheck: detected %d bad blocks", nbad);
      Tcl_SetResult(interp, buff, TCL_VOLATILE);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Defragment all free memory
 */
static char *g_memDefragment = "memDefragment";

static ftclArgvInfo g_memDefragment_Table[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL, "Defragment the freed memory pool\n" },
   {"-free",  FTCL_ARGV_CONSTANT, (void *)1, NULL,
      "return all freed memory to the O/S"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclMemDefragment(ClientData clientData,
		 Tcl_Interp *interp,
		 int ac, char **av)
{
   const int flags = FTCL_ARGV_NO_LEFTOVERS;
   int free_to_os = 0;			/* return freed memory to O/S? */
   int rstatus;

   g_memDefragment_Table[1].dst  = &free_to_os;
   if((rstatus = shTclParseArgv(interp, &ac, av, g_memDefragment_Table, flags,
				g_memDefragment)) != FTCL_ARGV_SUCCESS)  {
       return(rstatus);
   }

   Tcl_ResetResult(interp);
/*
 * time to do the work
 */
   if(shMemDefragment(free_to_os) < 0) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Set the minimum size for that shMalloc should ever request from the O/S
 */
static char *g_memBlocksizeSet = "memBlocksizeSet";

static ftclArgvInfo g_memBlocksizeSet_Table[] = {
   {NULL, FTCL_ARGV_HELP, NULL, NULL,
      "Set the blocksize for the freed memory pool, returning the old value\n"
	"If no size is specified, simply return the old value" },
   {"[size]", FTCL_ARGV_INT, NULL, NULL,
      "the minimum size for that shMalloc should ever request from the O/S"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
};

static int
tclMemBlocksizeSet(ClientData clientData,
		   Tcl_Interp *interp,
		   int ac, char **av)
{
   const int flags = FTCL_ARGV_NO_LEFTOVERS;
   size_t lsize;			/* the desired size, as a size_t */
   int size;				/* the desired size */
   int rstatus;

   size = -1;
   g_memBlocksizeSet_Table[1].dst  = &size;
   if((rstatus = shTclParseArgv(interp, &ac, av,
				g_memBlocksizeSet_Table, flags,
				g_memBlocksizeSet)) != FTCL_ARGV_SUCCESS)  {
       return(rstatus);
   }
   lsize = (size == -1) ? ~0UL : size;

   Tcl_ResetResult(interp);
/*
 * time to do the work
 */
   size = shMemBlocksizeSet(lsize);
/*
 * return value
 */
   {
      char buff[30];
      sprintf(buff, "%d", (int)size);
      Tcl_AppendResult(interp, buff, (char *)NULL);
   }

   return(TCL_OK);
}

void shTclMemDeclare(Tcl_Interp *a_interp)
{
   int flags = FTCL_ARGV_NO_LEFTOVERS;
   char *pHelp, *pUse;
   char *tclHelpFacil = "shMemory";

   pHelp = "Prints serial number for the most recently assigned memory block";
   pUse  = "memSerialNumber";
   shTclDeclare(a_interp, "memSerialNumber",
                (Tcl_CmdProc *)tclMemSerialNumberPrint, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   shTclDeclare(a_interp, g_memFreeBlocks, 
                (Tcl_CmdProc *)tclMemFreeBlocks, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_argTable, flags, g_memFreeBlocks),
                shTclGetUsage(a_interp, g_argTable, flags, g_memFreeBlocks));

   pHelp = "Print total bytes allocated so far from the OS";
   pUse  = "memTotalBytes";
   shTclDeclare(a_interp, "memTotalBytes", 
                (Tcl_CmdProc *)tclMemTotalBytes, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Print total bytes allocated so far from the OS, and utilised"
           "by shMalloc.\n"
	   "This includes bytes allocated for internal bookkeeping";
   pUse  = "memActualBytes";
   shTclDeclare(a_interp, "memActualBytes", 
                (Tcl_CmdProc *)tclMemActualBytes, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Print total bytes currently being used";
   pUse  = "memBytesInUse";
   shTclDeclare(a_interp, "memBytesInUse", 
                (Tcl_CmdProc *)tclMemBytesInUse, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Print total bytes currently in the \"Free Pool\"";
   pUse  = "memBytesInPool";
   shTclDeclare(a_interp, "memBytesInPool", 
                (Tcl_CmdProc *)tclMemBytesInPool, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Print total number of times shMalloc() was called";
   pUse  = "memNumMallocs";
   shTclDeclare(a_interp, "memNumMallocs", 
                (Tcl_CmdProc *)tclMemNumMallocs, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Print total number of times shFree() was called";
   pUse  = "memNumFrees";
   shTclDeclare(a_interp, "memNumFrees", 
                (Tcl_CmdProc *)tclMemNumFrees, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Print total number of blocks currently allocated";
   pUse  = "memNumBlocksInUse";
   shTclDeclare(a_interp, "memNumBlocksInUse", 
                (Tcl_CmdProc *)tclMemAMPSize, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Print interesting statistics about memory allocation and usage";
   pUse  = "memStatsPrint";
   shTclDeclare(a_interp, "memStatsPrint", 
                (Tcl_CmdProc *)tclMemStatsPrint, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Print number of current memory allocation requests";
   pUse  = "memAMPSize";
   shTclDeclare(a_interp, "memAMPSize", 
                (Tcl_CmdProc *)tclMemAMPSize, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Print contents of the allocated memory pool";
   pUse  = "memBlocksPrint";
   shTclDeclare(a_interp, "memBlocksPrint", 
                (Tcl_CmdProc *)tclMemBlocksPrint, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   pHelp = "Get contents of the allocated memory pool as a TCL list in the "
           "form {address {serial_number size [file lineno] [type handle]}}. "
	   "The last two item may not be present in all lists";
   pUse  = "memBlocksGet";
   shTclDeclare(a_interp, "memBlocksGet", 
                (Tcl_CmdProc *)tclMemBlocksGet, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);

   shTclDeclare(a_interp, g_memCheck, 
                (Tcl_CmdProc *)tclMemCheck, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_memCheck_Table, flags, g_memCheck),
                shTclGetUsage(a_interp, g_memCheck_Table, flags, g_memCheck));

   shTclDeclare(a_interp, g_memDefragment, 
                (Tcl_CmdProc *)tclMemDefragment, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_memDefragment_Table, flags,
				g_memDefragment),
                shTclGetUsage(a_interp, g_memDefragment_Table, flags,
			      g_memDefragment));

   shTclDeclare(a_interp, g_memBlocksizeSet, 
                (Tcl_CmdProc *)tclMemBlocksizeSet, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil,
                shTclGetArgInfo(a_interp, g_memBlocksizeSet_Table, flags,
				g_memBlocksizeSet),
                shTclGetUsage(a_interp, g_memBlocksizeSet_Table, flags,
			      g_memBlocksizeSet));

#ifdef DEBUG_MEM
   pHelp = "Print the Allocated Memory Pool";
   pUse  = "pr_Amp";
   shTclDeclare(a_interp, "pr_Amp",
                (Tcl_CmdProc *)tclPr_Amp, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);
   pHelp = "Print the Free Memory Pool indexed by indx";
   pUse  = "pr_Fmp indx";
   shTclDeclare(a_interp, "pr_Fmp", 
                (Tcl_CmdProc *)tclPr_Fmp, (ClientData)0,
                (Tcl_CmdDeleteProc *)0, tclHelpFacil, pHelp, pUse);
#endif

   return;
}

#ifdef DEBUG_MEM
static int  
tclPr_Amp(ClientData unused, Tcl_Interp *a_interp, int ac, char **av)
{
   pr_Amp();

   return TCL_OK;
}

static int  
tclPr_Fmp(ClientData unused, Tcl_Interp *a_interp, int ac, char **av)
{
   int indx;

   if (ac != 2)  {
       Tcl_SetResult(a_interp, 
          "pr_Fmp: specify an index; usage: pr_Fmp indx", TCL_VOLATILE);
       return TCL_ERROR;
   }

   indx = atoi(av[1]);

   if (indx >= NBUCKETS || indx < 0)  {
       char buf[5];

       sprintf(buf, "%d", NBUCKETS - 1);
       Tcl_ResetResult(a_interp);
       Tcl_AppendResult(a_interp, "pr_Fmp: index should be between 0 and ", 
                        buf, (char *) NULL);
       return TCL_ERROR;
   }

   pr_Fmp(indx);

   return TCL_OK;
}
#endif

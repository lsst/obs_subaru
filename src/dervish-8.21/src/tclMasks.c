/*
 * FILE: tclMasks.c
 *
 * This file contains Mask routine interfaces.
 *
 * ENTRY POINT          SCOPE   DESCRIPTION
 * -------------------------------------------------------------------------
 * shTclMaskNew          local   Create a new mask
 * shTclSubMaskNew       local   Create a new submask
 * shTclMaskDel          local   Delete a mask
 * shTclSubMaskGet       local   Get a list of sub masks attached to a mask
 * shTclMaskWrite        local   Write out a mask in FITS format to a file
 * shTclMaskRead         local   Read a mask from a FITS format file
 * shTclMaskSetWithNum   local   Set data pixels in mask to desired value
 * shTclMaskRowFlip      local   Flip mask pixels row-wise
 * shTclMaskColFlip      local   Flip mask pixels column-wise
 * shTclMaskSetWithMask  local   Set pixels in a mask according to another mask
 * shTclMaskInfoGet      local   Fill in and return the maskInfo structure
 * convert_num           local   Conversion function from integer->unsigned char
 * getMaskHandle         local   Iterator for getting sub mask handle names
 * generateLookupTable   local   Generate lookup table using TCL script or proc
 * shTclMaskDeclare      public  Declare TCL mask verbs to the world
 *
 * ENVIRONMENT:
 *    Ansi C (and, coming soon to a theatre, oops compiler near you, C++)
 *
 * AUTHOR
 *   Vijay K. Gurbani
 *   Creation date: October 27, 1993
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>          /* Prototype of unlink() */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "region.h"
#include "prvt/region_p.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "ftcl.h"
#include "shTclHandle.h"
#include "shCUtils.h"
#include "shTclTree.h"
#include "shCAssert.h"
#include "shTclUtils.h"
#include "libfits.h"
#include "shCRegUtils.h"
#include "shCSao.h"           /* Need this for CMD_HANDLE declaration used in */
#include "shTclVerbs.h"      /* shTclVerbs.h and the definition of LUTTOP    */
#include "shCFitsIo.h"       /* Declaration of MAXDDIRLEN and MAXDEXTLEN */
#include "shTclFits.h" 
#include "shCMask.h"
#include "tclInt.h" 
#include "tclExtdInt.h" 
#include "shCGarbage.h"
#include "shTclParseArgv.h"

/*
 * Static (private) function declarations (used only in this source file)
 */
static void getMaskHandle(const IPTR, const IPTR, void *);
static int  shTclMaskNew(ClientData, Tcl_Interp *, int, char *[]);
static int  shTclSubMaskNew(ClientData, Tcl_Interp *, int, char *[]);
static int  shTclMaskDel(ClientData, Tcl_Interp *, int, char *[]);
static int  shTclSubMaskGet(ClientData, Tcl_Interp *, int, char *[]);
static U8   convert_num(int);
static int  generateLookupTable(char *, Tcl_Interp *, int, char *, char *,
                                unsigned char [][LUTTOP]);

/*
 * Static global variables (used only in this source file)
 */
static char  **g_subMaskListp;
static short g_subMaskListIndex;

/*
 * ROUTINE:
 *   shTclMaskNew
 *
 * CALL:
 *   (static int)shTclMaskNew(ClientData a_clientData, Tcl_Interp *a_interp,
 *                            int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to create a new mask.
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_maskNew = 
   "USAGE: maskNew [-name name] [<nrows> <ncols>]";
static int
shTclMaskNew(ClientData a_clientData, Tcl_Interp *a_interp,
             int argc, char *argv[])
{
   int  i,
        errFlag,
        nrow,
        ncol,
        gotName;
   char *name,
        maskName[HANDLE_NAMELEN+1];
   MASK *maskPtr=NULL;
   HANDLE hndl;

   errFlag = 0;
   nrow = ncol = -1;
   gotName = 0;
   name = NULL;

   /*
    * Too bad there isn't a general purpose command line option collector...
    */

   for (i = 1; i < argc; i++)  {

        if (argv[i][0] == '-')  {    /* -name option present on command line */

            if (strcmp(argv[i], "-name") != 0)  {
                shErrStackPush("unknown option %s", argv[i]);
                errFlag = 1;
                break;
            }

            i++;
            if (i >= argc)  {
                shErrStackPush("-name should be specified as: -name name");
                errFlag = 1;
                break;
            }

            name = argv[i];   /* Point to the name */
            gotName = 1;
        }
        else  {
            if (nrow == -1)  {
                nrow = atoi(argv[i]);
                continue;
            }

            if (ncol == -1)
                ncol = atoi(argv[i]);
        }
   }

   if (argc >= 4 && gotName == 0)  {
       Tcl_AppendResult(a_interp, TclUsage_maskNew, (char *) NULL);
       return TCL_ERROR;
   }

   /*
    * If nrow was specified on the command line, ncol should be as well
    */
   if (errFlag == 0)
       if (nrow == -1 && ncol != -1)  {
           shErrStackPush("Either specify both nrow and ncol, or none at all");
           errFlag = 1;
       }

   if (errFlag == 0)
       if (ncol == -1 && nrow != -1)  {
           shErrStackPush("Either specify both nrow and ncol, or none at all");
           errFlag = 1;
       }

   if (errFlag)  {
       shTclInterpAppendWithErrStack(a_interp);
       Tcl_AppendResult(a_interp, TclUsage_maskNew, (char *) NULL);
       return TCL_ERROR;
   }

   if (nrow == -1)
       nrow = 0;
   if (ncol == -1)
       ncol = 0;

   /*
    * Okay, now to the real work...
    *
    * First, allocate TCL memory for a mask (i.e. create a handle)
    */
   if (p_shTclHandleNew(a_interp, maskName) != TCL_OK)  {
       shErrStackPush("Cannot create handle for a mask");
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * Now create a new mask. If the -name option is not specified on the
    * command line, provide a default name (hn)
    */
   if (name == NULL)
       name = maskName;

   maskPtr = shMaskNew(name, nrow, ncol);
   if (maskPtr == NULL)  {
       shErrStackPush("Cannot allocate memory for a mask");
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * Associate the mask with it's TCL handle
    */
   hndl.type = shTypeGetFromName("MASK");
   hndl.ptr  = maskPtr;
   if (p_shTclHandleAddrBind(a_interp, hndl, maskName) != TCL_OK)  {
       shErrStackPush("Cannot associate the mask with a TCL handle");
       shTclInterpAppendWithErrStack(a_interp);
       /*
        * Clear memory and TCL handle name
        */
       shMaskDel(maskPtr);
       (void) p_shTclHandleDel(a_interp, maskName);
       return TCL_ERROR;
   }

   /*
    * That should be all. Now return the mask handle name in interp
    */
   Tcl_SetResult(a_interp, maskName, TCL_VOLATILE);
   return TCL_OK;
}

/*
 * ROUTINE:
 *   shTclSubMaskNew
 *
 * CALL:
 *   (static int)shTclSubMaskNew(ClientData a_clientData, Tcl_Interp *a_interp,
 *                               int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to create a new sub mask.
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_subMaskNew = 
   "USAGE: subMaskNew <parent> nrow ncol srow scol";
static int
shTclSubMaskNew(ClientData a_clientData, Tcl_Interp *a_interp,
                int argc, char *argv[])
{
   int          nrow, ncol,
                srow, scol;
   char         *parentNamePtr,
                subMaskName[HANDLE_NAMELEN+1];
   MASK         *parentMaskPtr=NULL;
   MASK         *subMaskPtr=NULL;
   HANDLE       hndl;
   REGION_FLAGS flags;

   flags = NO_FLAGS;

   if (argc != 6)  {
       /*
        * All arguments required. (Look at TclUsage_subMaskNew above)
        */
       Tcl_AppendResult(a_interp, TclUsage_subMaskNew, (char *) NULL);
       return TCL_ERROR;
   }
   
   parentNamePtr = argv[1];
   nrow = atoi(argv[2]);
   ncol = atoi(argv[3]);
   srow = atoi(argv[4]);
   scol = atoi(argv[5]);

   /*
    * Make sure that parent exists for this mask.
    */
   if ((shTclAddrGetFromName(a_interp, parentNamePtr, (void **) &parentMaskPtr,
       "MASK") != TCL_OK) || (parentMaskPtr == NULL))
   {
       shErrStackPush("\nParent %s does not exist. Cannot create sub mask",
                      parentNamePtr);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * Allocate TCL memory for a sub mask (i.e. create a handle)
    */
   if (p_shTclHandleNew(a_interp, subMaskName) != TCL_OK)  {
       shErrStackPush("Cannot create handle for sub mask");
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * Now create a new sub mask. 
    */
   subMaskPtr = shSubMaskNew(subMaskName, parentMaskPtr, nrow, ncol,
                             srow, scol, flags);
   if (subMaskPtr == NULL)  {
       shErrStackPush("\nCannot allocate memory for sub mask");
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * Associate the sub mask with it's TCL handle
    */
   hndl.type = shTypeGetFromName("MASK");
   hndl.ptr  = subMaskPtr;
   if (p_shTclHandleAddrBind(a_interp, hndl, subMaskName) != TCL_OK)  {
       shErrStackPush("\nCannot associate the sub mask with a TCL handle");
       shTclInterpAppendWithErrStack(a_interp);
       /*
        * Clear memory and TCL handle name
        */
       (void) p_shTclHandleDel(a_interp, subMaskName);
       return TCL_ERROR;
   }

   /*
    * That should be all. Now return the sub mask handle name in interp
    */
   Tcl_SetResult(a_interp, subMaskName, TCL_VOLATILE);
   return TCL_OK;
}

/*
 * ROUTINE:
 *   shTclMaskDel
 *
 * CALL:
 *   (static int)shTclMaskDel(ClientData a_clientData, Tcl_Interp *a_interp,
 *                            int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to delete an existing mask.
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_maskDel = "USAGE: maskDel <mask>";
static int
shTclMaskDel(ClientData a_clientData, Tcl_Interp *a_interp,
             int argc, char *argv[])
{
   MASK *maskPtr=NULL;

   if (argc != 2)  {
       Tcl_AppendResult(a_interp, TclUsage_maskDel, (char *) NULL);
       return TCL_ERROR;
   }
 
   /*
    * Make sure that a mask of that name exists before deleting it.
    */
   if ((shTclAddrGetFromName(a_interp, argv[1], (void **) &maskPtr,
       "MASK") != TCL_OK) || (maskPtr == NULL))
   {
       shErrStackPush("\n%s: no such mask found", argv[1]);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * If this mask has submasks, then we cannot delete it.
    */
   if (maskPtr->prvt->nchild > 0L)  {
       shErrStackPush(
         "%s contains sub masks. Cannot delete a mask with active sub masks.",
         argv[1]);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * Okay to delete this mask.
    */
   (void) p_shTclHandleDel(a_interp, argv[1]);
   shMaskDel(maskPtr);

   return TCL_OK;
}

/*
 * ROUTINE:
 *   shTclSubMaskGet
 *
 * CALL:
 *   (static int)shTclSubMaskGet(ClientData a_clientData, Tcl_Interp *a_interp,
 *                               int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to create a list of all submasks attached
 *   to a parent mask specified in argv[1].
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_subMaskGet = "USAGE: subMaskGet <mask>";
static int
shTclSubMaskGet(ClientData a_clientData, Tcl_Interp *a_interp,
             int argc, char *argv[])
{
   MASK *maskPtr=NULL;
   char *resultp;
   int  i,
        memoryNeeded;

   if (argc != 2)  {
       Tcl_AppendResult(a_interp, TclUsage_subMaskGet, (char *) NULL);
       return TCL_ERROR;
   }

   /*
    * Make sure a valid parent mask handle is passed
    */
   if ((shTclAddrGetFromName(a_interp, argv[1], (void **) &maskPtr,
       "MASK") != TCL_OK) || (maskPtr == NULL))
   {
       shErrStackPush("\n%s: no such mask found", argv[1]);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * The fact that a mask does not have any submasks (signified by it's
    * child count being 0) is not really an error is it?
    */
   if (maskPtr->prvt->nchild == 0)
       return TCL_OK;


   g_subMaskListIndex = 0;
   g_subMaskListp = (char **) shMalloc(maskPtr->prvt->nchild * sizeof(char *));

   shTreeTraverse(maskPtr->prvt->children, getMaskHandle, a_interp);

   shAssert(g_subMaskListIndex == maskPtr->prvt->nchild);

   /*
    * Transfer the sub mask list from g_subMaskListp to resultp. While
    * allocating memory, add an extra byte for each element to hold the 
    * space in between two mask handles.
    */
   for (i = 0, memoryNeeded = 0; i < g_subMaskListIndex; i++)
        memoryNeeded += strlen(g_subMaskListp[i]) + 2;
 
   resultp = (char *) shMalloc(memoryNeeded + 1);

   for (i = 0; i < g_subMaskListIndex; i++)  {
        if (i == 0)
            strcpy(resultp, g_subMaskListp[i]);
        else
            strcat(resultp, g_subMaskListp[i]);
        strcat(resultp, " ");
   }

   /* 
    * Clean up...
    */
   for (i = 0; i < g_subMaskListIndex; i++)
        shFree(g_subMaskListp[i]);
   shFree(g_subMaskListp);
   
   Tcl_SetResult(a_interp, resultp, (Tcl_FreeProc *)shFree);

   return TCL_OK;
}

/*
 * ROUTINE:
 *   getMaskHandle
 *
 * CALL:
 *   (static void)getMaskHandle(const IPTR key, const IPTR val, void *a_interp)
 *   key      - Address of sub mask (supplied by shTreeTraverse())
 *   val      - not used
 *   a_interp - TCL Interpreter
 *
 * DESCRIPTION:
 *   An iterator that walks through a tree to obtain the address of the
 *   children of a mask (i.e. a mask's submasks)
 *
 * RETURNS:
 *   Nothing.
 */
static void
getMaskHandle(const IPTR key, const IPTR val, void *a_interp)
{
   Tcl_Interp *interp;
   char handleName[HANDLE_NAMELEN+1];

   interp = (Tcl_Interp *) a_interp;

   if (p_shTclHandleNameGet(interp, (MASK *) key) != TCL_OK)  {
       /*
        * Create the handle name
        */
       (void) shTclHandleNew(interp, handleName, "MASK", (MASK *) key);
       /*
        * Now get the handle name in interp->result
        */
       (void) p_shTclHandleNameGet(interp, (MASK *) key);
   }

   g_subMaskListp[g_subMaskListIndex] = 
          (char *) shMalloc(strlen(interp->result) + 1);
   strcpy(g_subMaskListp[g_subMaskListIndex], interp->result);
   g_subMaskListIndex++;
}

/*
 * ROUTINE:
 *   shTclMaskWrite
 *
 * CALL:
 *   (static int)shTclMaskWrite(ClientData a_clientData, Tcl_Interp *a_interp,
 *                              int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to write a mask in a FITS formatted
 *   file. Note, since masks do not have headers associated with them in
 *   this version of dervish, a default header will be generated.
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_maskWrite = 
            "USAGE: maskWriteAsFits [-nodefault] <mask> <file> [-pipe] [-tape]";
static char *g_maskWriteAsFits = "maskWriteAsFits";

static ftclArgvInfo g_maskWriteAsFitsArgTable[] = {
    {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Write a FITS file into a mask\n" },
    {"<mask>",  FTCL_ARGV_STRING,NULL, NULL, "Mask to write in to"},
    {"<FITSfile>",FTCL_ARGV_STRING,NULL, NULL,
       "Destination file or pipe descriptor if -pipe is present"},
    {"-dirset", FTCL_ARGV_STRING,NULL, NULL,
       "Destination Directory if not current one"},
    {"-nodefault", FTCL_ARGV_CONSTANT,(void*) 1, NULL, ""},
    {"-pipe", FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "If present, FITSfile is actually a pipe descriptor"},
    {"-tape", FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "Write to tape device specified in <FITSFILE>"},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };

static int shTclMaskWrite(
   ClientData a_clientData, 
   Tcl_Interp *a_interp,
   int argc, 
   char *argv[]
   )
{
MASK       *maskPtr=NULL;
char       *maskFileNamePtr=NULL;
char       *maskNamePtr=NULL;
DEFDIRENUM FITSdef;
int      Pipe = 0;
char*    dirsetValPtr = NULL;
int      flags = FTCL_ARGV_NO_LEFTOVERS;
FILE     *hfile = NULL;
int      nodefault = 0;
OpenFile *FilePtr = NULL;
RET_CODE rstatus;
int      inTape = 0;                    /* tape read option */

   FITSdef = DEF_MASK_FILE;

   /* Parse the command */
   g_maskWriteAsFitsArgTable[1].dst = &maskNamePtr;
   g_maskWriteAsFitsArgTable[2].dst = &maskFileNamePtr;
   g_maskWriteAsFitsArgTable[3].dst = &dirsetValPtr;
   g_maskWriteAsFitsArgTable[4].dst = &nodefault;
   g_maskWriteAsFitsArgTable[5].dst = &Pipe;
   g_maskWriteAsFitsArgTable[6].dst = &inTape;
  
   rstatus = shTclParseArgv(a_interp, &argc, argv, g_maskWriteAsFitsArgTable,
                            flags, g_maskWriteAsFits);
   if (rstatus == FTCL_ARGV_SUCCESS)
   {
      /* Can't have both -tape and -pipe */
      if (Pipe && inTape)
      {
           Tcl_AppendResult(a_interp, "\n  Illegal to specify both -tape and -pipe ",
			    (char *)NULL);
	   return TCL_ERROR;
      }
      /* We do not need to get the file specification if we are using a pipe */
      if (!Pipe && !inTape)
      {
           /* Get the dirset file specification defaulting info */
           if (nodefault) FITSdef = DEF_NONE;
           p_shTclDIRSETexpand(dirsetValPtr,a_interp, DEF_MASK_FILE, &FITSdef,
                               ((char *)0), ((char **)0), ((char **)0));
      }

     /*
      * Make sure that a mask of that name exists before writing it.
      */
     if ((shTclAddrGetFromName(a_interp, maskNamePtr, (void **) &maskPtr,
         "MASK") != TCL_OK) || (maskPtr == NULL))
     {
         shErrStackPush("\n%s: no such mask found", maskNamePtr);
         shTclInterpAppendWithErrStack(a_interp);
         return TCL_ERROR;
     }
 
           /* Validate the pipe we were given (if we were given one) */
      if (Pipe)
      {
         if ((FilePtr = Tcl_GetOpenFileStruct(a_interp, maskFileNamePtr)) == NULL)
         {
            shErrStackPush("%s: %s - invalid pipe", *argv, maskFileNamePtr);
            shTclInterpAppendWithErrStack(a_interp);
            return TCL_ERROR;
         }
         hfile = FilePtr->f;      /* Get ptr to pipes 'file' descriptor */
      }
  
     /*
      * Write the mask
      */
     if (shMaskWriteAsFits(maskPtr, maskFileNamePtr, FITSdef, hfile, inTape) != SH_SUCCESS) {
         shTclInterpAppendWithErrStack(a_interp);
         return TCL_ERROR;
     }
     else {
        Tcl_SetResult(a_interp, maskNamePtr, TCL_VOLATILE);
     }
   }
   else
      return TCL_ERROR;

   return TCL_OK;
}

/*
 * ROUTINE:
 *   shTclMaskSetWithNum
 *
 * CALL:
 *   (static int)shTclSetWithNum(ClientData a_clientData, Tcl_Interp *a_interp,
 *                               int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to populate a mask's pixel values as
 *   defined on the command line.
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_maskSetWithNum = 
            "USAGE: maskSetWithNum <mask> [-and | -or | -xor] <number>";
static int
shTclMaskSetWithNum(ClientData a_clientData, Tcl_Interp *a_interp,
                    int argc, char *argv[])
{
   MASK *maskPtr=NULL;
   int  i, j,
        number;
   char *num,
        *operation;   /* One of -and, -or, -xor, -none */
   U8   retValue;

   num = operation = (char *)0;

   if (argc < 3 || argc >= 5)  {
       Tcl_AppendResult(a_interp, TclUsage_maskSetWithNum, (char *) NULL);
       return TCL_ERROR;
   }

   /*
    * Now comes the tedious part to ensure that the command line arguments
    * make sense...
    */
   if (argc == 3 && argv[2][0] == '-')  {
       /*
        * If user specified: 'maskSetWithNum h0 -and', flag this as an error
        * because <number> was not specified.
        */
       Tcl_AppendResult(a_interp, TclUsage_maskSetWithNum, (char *) NULL);
       return TCL_ERROR;
   }

   if (argv[1][0] == '-')  {
       /*
        * User specified: 'maskSetWithNum -and 0'
        */
       Tcl_AppendResult(a_interp, TclUsage_maskSetWithNum, (char *) NULL);
       return TCL_ERROR;
   }

   if (argc == 4)  {
       operation = argv[2];
       num = argv[3];

   }
   else if (argc == 3)  {
       operation = "-none";
       num = argv[2];
   }

   if (strncmp(operation, "-and",  strlen(operation)) != 0 &&
       strncmp(operation, "-or",   strlen(operation)) != 0 &&
       strncmp(operation, "-xor",  strlen(operation)) != 0 &&
       strncmp(operation, "-none", strlen(operation)) != 0)
   {
       shErrStackPush("%s: Only one of -and, -or, -xor must be specified", 
                      *argv);
       shTclInterpAppendWithErrStack(a_interp);
       Tcl_AppendResult(a_interp, TclUsage_maskSetWithNum, (char *) NULL);
       return TCL_ERROR;
   }
      
   /*
    * Make sure that a mask of that name exists before operating on it
    */
   if ((shTclAddrGetFromName(a_interp, argv[1], (void **) &maskPtr,
       "MASK") != TCL_OK) || (maskPtr == NULL))
   {
       shErrStackPush("%s: no such mask found", argv[1]);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }
  
   if (Tcl_GetInt(a_interp, num, &number) != TCL_OK)  {
      shErrStackPush("%s: cannot get integer from %s", *argv, num);
      shTclInterpAppendWithErrStack(a_interp);
      return TCL_ERROR;
   }

   /*
    * Okay, now do the real work...
    */
   for (i = 0; i < maskPtr->nrow; i++)  {
        for (j = 0; j < maskPtr->ncol; j++)  {
             retValue = convert_num(number);
             if (strncmp(operation, "-and", 4) == 0)
                maskPtr->rows[i][j] = maskPtr->rows[i][j] & retValue;
             else if (strncmp(operation, "-or", 3) == 0)
                maskPtr->rows[i][j] = maskPtr->rows[i][j] | retValue;
             else if (strncmp(operation, "-xor", 4) == 0)
                maskPtr->rows[i][j] = maskPtr->rows[i][j] ^ retValue;
             else if (strncmp(operation, "-none", 5) == 0)
                maskPtr->rows[i][j] = retValue;
             else
                 shFatal("%s: unknown operation %s!", *argv, operation);
        }
   }

   return TCL_OK;
}

/*
 * A utility function to convert a number to unsigned char (0-255)
 */
static U8 convert_num(int number)
{
   if (number < (int)U8_MIN)
       return (U8_MIN);
   if (number > (int)U8_MAX)
       return U8_MAX;

   return (U8)number;
}

/*
 * ROUTINE:
 *   shTclMaskRead
 *
 * CALL:
 *   (static int)shTclMaskRead(ClientData a_clientData, 
 *                               Tcl_Interp *a_interp, int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to read a FITS data file into a mask
 *   structure. The mask's handle is passed in argv[1]. The corresponding
 *   mask is first deleted, and a new one created with the proper dimensions.
 *   The handle, however, is preserved between the deletion of the old
 *   mask, and the creation of the new one.
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_maskRead =
            "USAGE: maskReadAsFits [-nodefault] <mask> <file> [-pipe] [-tape]";
static char *g_maskReadAsFits = "maskReadAsFits";

static ftclArgvInfo g_maskReadAsFitsArgTable[] = {
    {NULL, FTCL_ARGV_HELP, NULL, NULL,
     "Read a FITS file into a mask\n" },
    {"<mask>",  FTCL_ARGV_STRING,NULL, NULL, "Mask to read in to"},
    {"<FITSfile>",FTCL_ARGV_STRING,NULL, NULL,
       "File containing the data or pipe descriptor if -pipe is present"},
    {"-dirset", FTCL_ARGV_STRING,NULL, NULL,
       "Directory where file resides if not current one"},
    {"-nodefault", FTCL_ARGV_CONSTANT,(void*) 1, NULL, ""},
    {"-pipe", FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "If present, FITSfile is actually a pipe descriptor"},
    {"-tape", FTCL_ARGV_CONSTANT, (void *)1, NULL,
       "Read from tape device specified in <FITSFILE>"},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
   };


static int shTclMaskRead(
   ClientData a_clientData, 
   Tcl_Interp *a_interp,
   int argc, 
   char *argv[]
   )
{
MASK     *maskPtr=NULL;
RET_CODE rstatus;
char     *maskNamePtr=NULL;              /* first parameter */
char	 *maskFileNamePtr;		/* second parameter */
DEFDIRENUM FITSdef;

int      Pipe = 0;
char*    dirsetValPtr = NULL;
int      flags = FTCL_ARGV_NO_LEFTOVERS;
FILE     *hfile = NULL;
OpenFile *FilePtr = NULL;
int      nodefault=0;
int      inTape = 0;                    /* tape read option */

   FITSdef = DEF_MASK_FILE;
   
   /* Parse the command */
   g_maskReadAsFitsArgTable[1].dst = &maskNamePtr;
   g_maskReadAsFitsArgTable[2].dst = &maskFileNamePtr;
   g_maskReadAsFitsArgTable[3].dst = &dirsetValPtr;
   g_maskReadAsFitsArgTable[4].dst = &nodefault;
   g_maskReadAsFitsArgTable[5].dst = &Pipe;
   g_maskReadAsFitsArgTable[6].dst = &inTape;
   
   rstatus = shTclParseArgv(a_interp, &argc, argv, g_maskReadAsFitsArgTable,
                            flags, g_maskReadAsFits);
   
   if (rstatus == FTCL_ARGV_SUCCESS)
   {
      /* Can't have both -tape and -pipe */
      if (Pipe && inTape)
      {
           Tcl_AppendResult(a_interp, "\n  Illegal to specify both -tape and -pipe ",
			    (char *)NULL);
	   return TCL_ERROR;
      }
      /* We do not need to get the file specification if we are using a pipe */
      if (!Pipe && !inTape)
      {
           /* Get the dirset file specification defaulting info */
           if (nodefault) FITSdef = DEF_NONE;
           p_shTclDIRSETexpand(dirsetValPtr,a_interp, DEF_MASK_FILE, &FITSdef,
                               ((char *)0), ((char **)0), ((char **)0));
      }
   
      /* Check the existance of the mask handle specified */
      if ((shTclAddrGetFromName(a_interp, maskNamePtr, (void **) &maskPtr, "MASK") 
          != TCL_OK) || (maskPtr == NULL))
      {
          shErrStackPush("%s: %s - no such mask exists", *argv, maskNamePtr);
          shTclInterpAppendWithErrStack(a_interp);
          return TCL_ERROR;
      }
      
           /* Validate the pipe we were given (if we were given one) */
      if (Pipe)
      {
         if ((FilePtr = Tcl_GetOpenFileStruct(a_interp, maskFileNamePtr)) == NULL)
         {
            shErrStackPush("%s: %s - invalid pipe", *argv, maskFileNamePtr);
            shTclInterpAppendWithErrStack(a_interp);
            return TCL_ERROR;
   
         }
         hfile = FilePtr->f;      /* Get ptr to pipes 'file' descriptor */
      }
   
      /* Read the mask */
      if (shMaskReadAsFits(maskPtr, maskFileNamePtr, FITSdef, hfile, inTape) != SH_SUCCESS) 
      {
          shTclInterpAppendWithErrStack(a_interp);
          return TCL_ERROR;
      }
      else 
         Tcl_SetResult(a_interp, maskPtr->name, TCL_VOLATILE);
   
   }
   else 
      return TCL_ERROR;
   return TCL_OK;
}

/*
 * ROUTINE:
 *   shTclMaskRowFlip
 *
 * CALL:
 *   (static int)shTclMaskRowFlip(ClientData a_clientData, Tcl_Interp *a_interp,
 *                                int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to flip a mask row-wise.
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_maskRowFlip = "USAGE: maskRowFlip <mask>";
static int
shTclMaskRowFlip(ClientData a_clientData, Tcl_Interp *a_interp,
                 int argc, char *argv[])
{
   MASK *maskPtr=NULL;

   if (argc != 2)  {
       Tcl_AppendResult(a_interp, TclUsage_maskRowFlip, (char *) NULL);
       return TCL_ERROR;
   }

   /* 
    * Get the mask pointer
    */
   if ((shTclAddrGetFromName(a_interp, argv[1], (void **) &maskPtr,
       "MASK") != TCL_OK) || (maskPtr == NULL))
   {
       shErrStackPush("\n%s: no such mask found", argv[1]);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   shMaskRowFlip(maskPtr);

   return TCL_OK;
}

/*
 * ROUTINE:
 *   shTclMaskColFlip
 *
 * CALL:
 *   (static int)shTclMaskColFlip(ClientData a_clientData, Tcl_Interp *a_interp,
 *                                int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to flip a mask column-wise.
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_maskColFlip = "USAGE: maskColFlip <mask>";
static int
shTclMaskColFlip(ClientData a_clientData, Tcl_Interp *a_interp,
                 int argc, char *argv[])
{
   MASK *maskPtr=NULL;
 
   if (argc != 2)  {
       Tcl_AppendResult(a_interp, TclUsage_maskColFlip, (char *) NULL);
       return TCL_ERROR;
   }
 
   /* 
    * Get the mask pointer
    */
   if ((shTclAddrGetFromName(a_interp, argv[1], (void **) &maskPtr,
       "MASK") != TCL_OK) || (maskPtr == NULL))
   {
       shErrStackPush("\n%s: no such mask found", argv[1]);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }
 
   shMaskColFlip(maskPtr);
 
   return TCL_OK;
}

/*
 * ROUTINE:
 *   shTclMaskSetWithMask
 *
 * CALL:
 *   (static int)shTclMaskSetWithMask(ClientData a_clientData, 
 *                      Tcl_Interp *a_interp, int argc, char *argv[])
 *   a_clientData - TCL passed variable address (can be NULL)
 *   a_interp     - TCL interpreter
 *   argc         - TCL command line argument count
 *   argv         - TCL command line argument vector
 *
 * DESCRIPTION:
 *   Callback function invoked by TCL to set a target mask using input masks
 *   and a TCL script or a procedure. If only two masks are specified
 *   on the command line, the result is stored in the second mask. The two
 *   input masks and the TCL script or procedure are used to generate a 
 *   LUTTOPxLUTTOP lookup table. The resultant mask is populated from this
 *   lookup table.
 *
 * RETURNS:
 *   TCL_OK : on success
 *   TCL_ERROR : otherwise
 */
static char *TclUsage_maskSetWithMask = 
"USAGE: maskSetWithMask <mask1> <mask2> [-o] <-t TCL script | -p TCL procedure> [result]";
static int
shTclMaskSetWithMask(ClientData a_clientData, Tcl_Interp *a_interp,
                     int argc, char *argv[])
{
   MASK   *maskPtr1=NULL,        /* mask1  */
          *maskPtr2=NULL,        /* mask2  */
          *maskPtr3=NULL;        /* result, mask2 if not specified. */
   char   *tclProcPtr,
          *tclScriptPtr,
          *maskHandle1,
          *maskHandle2,
          *maskHandle3;
   int    i, r, c,
          row, col,
          oflag;
   short  errFlag;
   unsigned char maskLookupTable[LUTTOP][LUTTOP];

   oflag = 0;

   if (argc < 4)  {
       Tcl_AppendResult(a_interp, TclUsage_maskSetWithMask, (char *) NULL);
       return TCL_ERROR;
   }
   
   errFlag         = 0;
   tclProcPtr      = (char *)0;
   tclScriptPtr    = (char *)0;
   maskHandle1     = (char *)0;
   maskHandle2     = (char *)0;
   maskHandle3     = (char *)0;
   
   /*
    * Now comes the grungy part of parsing command line arguments. There are
    * at the minimum 4 command line arguments. Only one of -t or -p should
    * be specified. If result is not specified, lookup table generated values
    * are stored in mask2.
    */
   for (i = 1; i < argc; i++)  {
        switch (argv[i][0])  {
           case '-' :
                if (argv[i][1] == 't')  {
                    if (tclProcPtr)  {
                        shErrStackPush("%s: specify either -t or -p only",
                                        *argv);
                        errFlag = 1;
                    }
                    else
                        tclScriptPtr = argv[++i];
                }
                else if (argv[i][1] == 'p')  {
                    if (tclScriptPtr)  {
                        shErrStackPush("%s: specify either -t or -p only",
                                        *argv);
                        errFlag = 1;
                    }
                    else
                        tclProcPtr = argv[++i];
                }
                else if (argv[i][1] == 'o')
                    oflag = 1;
                else  {
                    shErrStackPush("%s: illegal parameter %s", *argv, argv[i]);
                    errFlag = 1;
                }
                break;
           default :    /* Has to be one of the mask */
                if (!maskHandle1)
                    maskHandle1 = argv[i];
                else if (!maskHandle2)
                    maskHandle2 = argv[i];
                else 
                    maskHandle3 = argv[i];
                break; 
        }
        if (errFlag)
            break;
   }

   if (errFlag)  {
       shTclInterpAppendWithErrStack(a_interp);
       Tcl_AppendResult(a_interp, TclUsage_maskSetWithMask, (char *) NULL);
       return TCL_ERROR;
   }

   if (!tclScriptPtr && !tclProcPtr)  {
       /*
        * -p or -t not specified. One of them is needed
        */
       shErrStackPush("%s: one of -t or -p is required.", *argv);
       shTclInterpAppendWithErrStack(a_interp);
       Tcl_AppendResult(a_interp, TclUsage_maskSetWithMask, (char *) NULL);
       return TCL_ERROR;
   }
   
   if (maskHandle1 == NULL || maskHandle2 == NULL)  {
       shErrStackPush("%s: <mask1> and <mask2> expected.", *argv);
       shTclInterpAppendWithErrStack(a_interp);
       Tcl_AppendResult(a_interp, TclUsage_maskSetWithMask, (char *) NULL);
       return TCL_ERROR;
   }

   /*
    * Some housekeeping checks:
    * 1) maskHandle1 and maskHandle2 SHOULD exist
    * 2) maskHandle1 and maskHandle2 SHOULD be of the same dimensions
    */
   if ((shTclAddrGetFromName(a_interp, maskHandle1, (void **) &maskPtr1,
       "MASK") != TCL_OK) || (maskPtr1 == NULL))
   {
       shErrStackPush("\n%s: no such mask found", maskHandle1);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }
   if ((shTclAddrGetFromName(a_interp, maskHandle2, (void **) &maskPtr2,
       "MASK") != TCL_OK) || (maskPtr2 == NULL))
   {
       shErrStackPush("\n%s: no such mask found", maskHandle2);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   if (maskPtr1->nrow != maskPtr2->nrow && maskPtr1->ncol != maskPtr2->ncol)  {
       shErrStackPush("%s: %s and %s should have the same dimensions", *argv,
                      maskHandle1, maskHandle2);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * If 'result' parameter is not specified on the command line, store the 
    * the result in the mask identified by maskHandle2. If 'result' is
    * specified on the command line, we have to perform two checks:
    *   1) make sure a mask of that name exists,
    *   2) it's dimensions are the same as maskPtr1. If not make a new 
    *      mask keeping the same mask name, but changing the dimensions
    */
   if (!maskHandle3)  {
       maskHandle3 = maskHandle2;
       maskPtr3 = maskPtr2;
   }
   else  {
       if ((shTclAddrGetFromName(a_interp, maskHandle3, (void **) &maskPtr3,
           "MASK") != TCL_OK) || (maskPtr3 == NULL))
       {
           shErrStackPush("%s: %s - no such mask found", *argv, maskHandle3);
           shTclInterpAppendWithErrStack(a_interp);
           return TCL_ERROR;
       }

       if (maskPtr3->nrow != maskPtr1->nrow && maskPtr3->ncol != maskPtr1->ncol)
       {
           HANDLE hndl;

           if (shMaskDel(maskPtr3) != SH_SUCCESS)  {
               shErrStackPush("\n%s: unable to delete old mask", *argv);
               shTclInterpAppendWithErrStack(a_interp);
               return TCL_ERROR;
           }

           maskPtr3 = shMaskNew(maskHandle3, maskPtr1->nrow, maskPtr1->ncol);
           if (maskPtr3 == NULL)  {
               shErrStackPush("\n%s: error creating a new mask", *argv);
               shTclInterpAppendWithErrStack(a_interp);
               return TCL_ERROR;
           }
      
           /*
            * Re-bind the new address with the old handle name
            */
           hndl.type = shTypeGetFromName("MASK");
           hndl.ptr  = maskPtr3;
           if (p_shTclHandleAddrBind(a_interp, hndl, maskHandle3) != TCL_OK)  {
               shErrStackPush("%s: cannot rebind handle name %s to new mask",
                              *argv, maskHandle3);
               shTclInterpAppendWithErrStack(a_interp);
               return TCL_ERROR;
           }
       }
   }

   /*
    * By now we are assured that maskPtr1, maskPtr2, and maskPtr3 are all
    * valid and well. So let's generate the lookup table
    */
   if (!(generateLookupTable(*argv, a_interp, oflag, tclScriptPtr, 
         tclProcPtr, maskLookupTable)))
   {
       shErrStackPush("%s: cannot generate lookup table.", *argv);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }

   /*
    * Now set each pixel in resulting mask as follows:
    *
    *   Use maskPtr1 as row    vector
    *   Use maskPtr2 as column vector
    *   For each pixel in maskPtr1 and maskPtr2
    *       Set row    to maskPtr1->rows[][]
    *       Set column to maskPtr2->rows[][]
    *       Set maskPtr3->rows[][] to maskLookupTable[row][col]
    */
    for (r = 0; r < maskPtr1->nrow; r++)  {
         for (c = 0; c < maskPtr1->ncol; c++)  {
              row = maskPtr1->rows[r][c];
              col = maskPtr2->rows[r][c];
              maskPtr3->rows[r][c] = maskLookupTable[row][col];
         }
    }

   Tcl_SetResult(a_interp, maskHandle3, TCL_VOLATILE);
   return TCL_OK;
}

/*
 * ROUTINE:
 *   generateLookupTable
 *
 * CALL:
 *   (static int) generateLookupTable(char *a_argv, Tcl_Interp *a_interp, 
 *                    int oflag, char *a_tclScriptPtr, char *a_tclProcPtr,
 *                    unsigned char maskLookupTable[][])
 *   a_argv          - Name of the calling function
 *   a_interp        - TCL interpreter
 *   oflag           - Should we optimize?
 *   a_tclScriptPtr  - TCL script to execute (can be NULL)
 *   a_tclProcPtr    - TCL procedure to execute (can be NULL)
 *   maskLookupTable - Mask lookup table address
 *
 * DESCRIPTION:
 *   Generate a LUTTOPxLUTTOP look up table using the TCL script or
 *   procedure provided. One of a_tclScriptPtr or a_tclProcPtr is expected.
 *   -o flag only makes sense for TCL scripts, not procedures.
 *
 * RETURNS:
 *   1 : on success
 *   0 : otherwise
 */
static int
generateLookupTable(char *a_argv, Tcl_Interp *a_interp, int oflag,
                    char *a_tclScriptPtr, char *a_tclProcPtr, 
                    unsigned char maskLookupTable[LUTTOP][LUTTOP])
{
   short r, c,
         errFlag,
         has_mp1,         /* $mp1 present on command line */
         has_mp2;         /* $mp2 present on command line */
   char  *saved_mp1,      /* Save buffer for  $mp1 */
         *saved_mp2,      /*             for  $mp2 */
         *current_mp1,    /* Current value of $mp1 */
         *current_mp2;    /*               of $mp2 */
   int   result;
   long  l_result;
 

   errFlag = 0;
   saved_mp1 = saved_mp2 = (char *)0;

   /*
    * Clear the lookup table from previous calls
    */
   for (r = 0; r < LUTTOP; r++)
        for (c = 0; c < LUTTOP; c++)
             maskLookupTable[r][c] = (unsigned char)0;

   /*
    * The lookup table is generated as follows:
    *   for (r = 0; r < LUTTOP; r++)  {
    *        for (c = 0; c < LUTTOP; c++)  {
    *             evaluate TCL script or procedure with $mp1 = r and $mp2 = c
    *             save resulting value in lookup table[r][c]
    *        }
    *   }
    */

   if (a_tclScriptPtr)  {
       /*
        * Evaluate a tcl script (-t option given on command line)
        */
       char row[5],
            col[5];
            
       has_mp1 = has_mp2 = 0;

       if (strstr(a_tclScriptPtr, "mp1"))
           has_mp1 = 1;
       if (strstr(a_tclScriptPtr, "mp2"))
           has_mp2 = 1;

       if (has_mp1 == 0)  {
           /*
            * $mp1 IS required in -t option
            */
           shErrStackPush("%s: $mp1 IS required", a_argv);
           return 0;
       }

       /*
        * Save the current values of $mp1 and $mp2. These will be restored on
        * return from this function
        */
       if (has_mp1)  {
           current_mp1 = Tcl_GetVar(a_interp, "mp1", 0);
           if (current_mp1 != NULL)  {
               saved_mp1 = (char *)shMalloc(strlen(current_mp1)+1);
               strcpy(saved_mp1, current_mp1);
           }
       }
       if (has_mp2)  {
           current_mp2 = Tcl_GetVar(a_interp, "mp2", 0);
           if (current_mp2 != NULL)  {
               saved_mp2 = (char *)shMalloc(strlen(current_mp2)+1);
               strcpy(saved_mp2, current_mp2);
           }
       }

       for (r = 0; r < LUTTOP; r++)  {
            sprintf(row, "%d", r);
            if ((Tcl_SetVar(a_interp, "mp1", row, 0)) == NULL)  {
                shErrStackPush("%s: error setting $mp1", a_argv);
                errFlag = 1;
                break;
            }
            for (c = 0; c < LUTTOP; c++)  {
                 sprintf(col, "%d", c);
                 if ((Tcl_SetVar(a_interp, "mp2", col, 0)) == NULL)  {
                     shErrStackPush("%s: error setting $mp2", a_argv);
                     errFlag = 1;
                     break;
                 }

                 /* 
                  * If -o flag is set on command line, let's try to
                  * optimize...
                  */
                 if (oflag)  {
                     if (Tcl_ExprLong(a_interp, a_tclScriptPtr, &l_result) ==
                         TCL_OK)
                         maskLookupTable[r][c] = convert_num((int)l_result);
                     else  {
                         shErrStackPush(
                         "\n%s: cannot evaluate TCL script optimized.", a_argv);
                         shErrStackPush("Try removing the -o flag.");
                         errFlag = 1;
                         break;
                     }
                 }
                 else if (Tcl_Eval(a_interp, a_tclScriptPtr) == TCL_OK)
                 {
                     if (Tcl_GetInt(a_interp, a_interp->result, &result) ==
                         TCL_OK)
                        maskLookupTable[r][c] = convert_num(result);
                     else  {
                         shErrStackPush("\n%s: cannot evaluate TCL script",
                                        a_argv);
                         errFlag = 1;
                         break;
                     }
                 }
                 else  {
                     shErrStackPush("\n%s: cannot evaluate TCL script", a_argv);
                     errFlag = 1;
                     break;
                 }
            }
            if (errFlag)
                break;
       }
   }
   else if (a_tclProcPtr)  {
       /*
        * Evaluate a TCL procedure. (-p option given on command line)
        * The TCL procedure will always take 2 arguments (although it might
        * not use both of them). 
        */
       char *bufPtr;

       bufPtr = (char *)shMalloc(strlen(a_tclProcPtr) + 20);
       for (r = 0; r < LUTTOP; r++)  {
            for (c = 0; c < LUTTOP; c++)  {
                 sprintf(bufPtr, "%s %d %d", a_tclProcPtr, r, c);
                 if (Tcl_Eval(a_interp, bufPtr) == TCL_OK)  {
                     if ((Tcl_GetInt(a_interp, a_interp->result, &result)) == 
                         TCL_OK)
                         maskLookupTable[r][c] = convert_num(result);
                     else  {
                         shErrStackPush("\n%s: cannot evaluate TCL procedure",
                                        a_argv);
                         errFlag = 1;
                         break;
                     } 
                 }
                 else  {
                     shErrStackPush("\n%s: cannot evaluate TCL procedure",
                                    a_argv);
                     errFlag = 1;
                     break;
                 }
            }
            if (errFlag)  {
                shFree(bufPtr);
                break;
            }
       }
   }

   /*
    * Restore the old values of $mp1 and $mp2 and free memory associated with
    * their storages. I am not going to check the return status of Tcl_SetVar.
    * Even if it fails, I don't think that warrants returning an error code
    * from this function.
    */
   if (saved_mp1)  {
       (void) Tcl_SetVar(a_interp, "mp1", saved_mp1, 0);
       shFree(saved_mp1);
   }
   if (saved_mp2)  {
       (void) Tcl_SetVar(a_interp, "mp2", saved_mp2, 0);
       shFree(saved_mp2);
   }

   if (errFlag)
       return 0;

   return 1;
}

/*
 * This is not exactly a mandated function in the sense of it being defined
 * in tclMaskUtils.html. I put this function in for debugging purposes, and
 * will let it remain...it's pretty useful.
 */
static char *TclUsage_maskDisplayPixels =
            "USAGE: maskDisplayPixels <mask>";
static int
shTclMaskDisplayPixels(ClientData a_clientData, Tcl_Interp *a_interp,
                       int argc, char *argv[])
{
   MASK *maskPtr=NULL;
   int  i, j;
 
   if (argc != 2)  {
       Tcl_AppendResult(a_interp, TclUsage_maskDisplayPixels, (char *) NULL);
       return TCL_ERROR;
   }
 
   /*
    * Make sure that a mask of that name exists before deleting it.
    */
   if ((shTclAddrGetFromName(a_interp, argv[1], (void **) &maskPtr,
       "MASK") != TCL_OK) || (maskPtr == NULL))
   {
       shErrStackPush("\n%s: no such mask found", argv[1]);
       shTclInterpAppendWithErrStack(a_interp);
       return TCL_ERROR;
   }
 
   for (i = 0; i < maskPtr->nrow; i++)  {
        for (j = 0; j < maskPtr->ncol; j++)
            fprintf(stdout, "%u ", (unsigned char) maskPtr->rows[i][j]);
        fprintf(stdout, "\n");
   }
 
   return TCL_OK;
}

/*============================================================================
**============================================================================
**
**
** DESCRIPTION:
**	return Information about a mask in the form of a keyed list
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
#define LCL_BUFFER_SIZE 1000
static char *TclUsage_maskInfoGet        = "USAGE: maskInfoGet mask";
static int shTclMaskInfoGet(ClientData clientData, Tcl_Interp *interp, int argc, char **argv)
{
   MASK *mask=NULL;
   MASKINFO *maskInfo;
   RET_CODE rc;
   static char s[LCL_BUFFER_SIZE];

   if (argc != 2) {	
      Tcl_SetResult (interp, TclUsage_maskInfoGet, TCL_STATIC);
      return (TCL_ERROR);
   }
   if (shTclAddrGetFromName(interp, argv[1], (void **)&mask, "MASK") != TCL_OK) return (TCL_ERROR);

   rc = shMaskInfoGet(mask, &maskInfo);

   if (rc == SH_SUCCESS) {
     sprintf (s, "nrow %d ", mask->nrow); Tcl_AppendElement (interp, s);
     sprintf (s, "ncol %d ", mask->ncol); Tcl_AppendElement (interp, s);
     sprintf (s, "name %s ", mask->name); Tcl_AppendElement (interp, s);
     sprintf (s, "row0 %d ", mask->row0); Tcl_AppendElement (interp, s);
     sprintf (s, "col0 %d ", mask->col0); Tcl_AppendElement (interp, s);
     sprintf (s, "isSubMask %d ", maskInfo->isSubMask); Tcl_AppendElement (interp, s);
     sprintf (s, "pxAreContiguous %d ", maskInfo->pxAreContiguous); Tcl_AppendElement (interp, s);
     sprintf (s, "nSubMask %d ", maskInfo->nSubMask); Tcl_AppendElement (interp, s);
     return (TCL_OK);
   } else {
      Tcl_SetResult (interp, "Call to shMaskInfoGet failed", TCL_STATIC);
      return (TCL_ERROR);
   }
}
#undef LCL_BUFFER_SIZE

/*
 * ROUTINE:
 *   shTclMaskDeclare
 *
 * CALL:
 *   (static void) shTclMaskDeclare(Tcl_Interp *a_interp)
 *   a_interp     - TCL interpreter
 *
 * DESCRIPTION:
 *   The world's public interface to masks. Definition of all mask verbs.
 *
 * RETURNS:
 *   Nothing
 */
void shTclMaskDeclare(Tcl_Interp *a_interp)
{
   char *helpFacil = "shMask";

   shTclDeclare(a_interp, "maskNew", (Tcl_CmdProc *) shTclMaskNew,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Create a new mask", TclUsage_maskNew);
   shTclDeclare(a_interp, "subMaskNew", (Tcl_CmdProc *) shTclSubMaskNew,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Generate a sub-mask from the given mask.", 
                TclUsage_subMaskNew);
   shTclDeclare(a_interp, "maskDel", (Tcl_CmdProc *) shTclMaskDel,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Delete the specified mask", TclUsage_maskDel);
   shTclDeclare(a_interp, "subMaskGet", (Tcl_CmdProc *) shTclSubMaskGet,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Return a list of handles to a mask's sub-masks.", 
                TclUsage_subMaskGet);
   shTclDeclare(a_interp, "maskWriteAsFits", (Tcl_CmdProc *) shTclMaskWrite,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Write a mask to a UNIX FITS file.", TclUsage_maskWrite);
   shTclDeclare(a_interp, "maskSetWithNum", (Tcl_CmdProc *) shTclMaskSetWithNum,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Set pixels in MASK to NUMBER.", TclUsage_maskSetWithNum);
   shTclDeclare(a_interp, "maskReadAsFits", (Tcl_CmdProc *) shTclMaskRead,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Read a mask from a UNIX FITS file.",  TclUsage_maskRead);
   shTclDeclare(a_interp, "maskRowFlip", (Tcl_CmdProc *) shTclMaskRowFlip,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Flip pixels in a mask row-wise.", TclUsage_maskRowFlip);
   shTclDeclare(a_interp, "maskColFlip", (Tcl_CmdProc *) shTclMaskColFlip,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Flip the pixels in a mask column-wise.", TclUsage_maskColFlip);
   shTclDeclare(a_interp, "maskSetWithMasks", 
                (Tcl_CmdProc *) shTclMaskSetWithMask, (ClientData) 0, 
                (Tcl_CmdDeleteProc *) 0, helpFacil,
                 "Set each pixel in a target mask using source masks and a TCL expression or a TCL procedure.",
                TclUsage_maskSetWithMask);
   shTclDeclare(a_interp, "maskDisplayPixels", 
                (Tcl_CmdProc *) shTclMaskDisplayPixels,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Display pixels in mask.", TclUsage_maskDisplayPixels);
   shTclDeclare(a_interp, "maskInfoGet", 
                (Tcl_CmdProc *) shTclMaskInfoGet,
                (ClientData) 0, (Tcl_CmdDeleteProc *) 0, helpFacil,
                "Return information about a mask in the form of a keyed list",
		TclUsage_maskInfoGet);

   return;
}

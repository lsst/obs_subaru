/*************************************************************************
 *  Facility:           dervish                                            *
 *  Synopsis:           Tcl routines for SCHEMATRANS(translation tables) *
 *  Environment:        ANSI C                                           *
 *  Author:             Wei Peng                                         *
 *  Creation Date:      Feb 1, 1994                                      *
 *                                                                       *
 *  Last Modification:  April 20, 1994                                   *
 *************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftcl.h"
#include "dervish_msg_c.h"
#include "shChain.h"
#include "shCTbl.h"
#include "shCSchema.h"
#include "shCSchemaSupport.h"
#include "shCSchemaTrans.h"
#include "shCSchemaCnv.h"
#include "shTclVerbs.h"
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shTclHandle.h"
#include "tclExtend.h"   
#include "shTclUtils.h" 
#include "shTclParseArgv.h"

/************** schemaTransNew *****************************/
/* return a handle to table */

static char* shTclSCHTransNew_use = "Usage: schemaTransNew ";
static char* shTclSCHTransNew_help= "creates a new, empty table";
static char* shTclSCHTransNew_cmd = "schemaTransNew";

static int shTclSCHTransNew (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
 
{ 
    char name[HANDLE_NAMELEN];
    SCHEMATRANS *xptr = NULL;

    if(argc > 1) {
        Tcl_SetResult(a_interp, shTclSCHTransNew_use, TCL_STATIC);
        return (TCL_ERROR);
    }
    xptr=shSchemaTransNew();
    if(xptr==NULL) {
        Tcl_AppendResult(a_interp, "Error: no memory", TCL_VOLATILE);
        return TCL_ERROR;
    }
    if( shTclHandleNew(a_interp, name, "SCHEMATRANS", (void*) xptr) != TCL_OK )
     {
        if(xptr !=NULL) shSchemaTransDel(xptr);
        return(TCL_ERROR);
     }
    shSchemaTransInit(xptr,0);
    Tcl_SetResult(a_interp,name,TCL_VOLATILE);
    return (TCL_OK);
}



/************** schemaTransDel *****************************/


static char* shTclSCHTransDel_use = "Usage: schemaTransDel table_handle";
static char* shTclSCHTransDel_help= "deletes a table";
static char* shTclSCHTransDel_cmd = "schemaTransDel";

static int shTclSCHTransDel (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
 
{ 
    SCHEMATRANS *xptr =NULL;

    
    if(argc==2 ) 
     {  
        if(shTclAddrGetFromName(a_interp, argv[1], (void**)&xptr,"SCHEMATRANS")!=TCL_OK)
            return TCL_ERROR;
        shSchemaTransDel( xptr);
        p_shTclHandleDel(a_interp, argv[1]);
     }
    else
     {
        Tcl_SetResult(a_interp,shTclSCHTransDel_use,TCL_STATIC);
        return(TCL_ERROR);

     }
   
    return TCL_OK;
}

/**************  schemaTransEntryAdd *****************************/
/* return message "entry added */

#define shTclSCHTransEntryAdd_help " Adds a translation table entry for TBLCOL-to-schema conversion.\n"

static char* shTclSCHTransEntryAdd_cmd="schemaTransEntryAdd";

static ftclArgvInfo schemaTransEntryAddArgTable[] = {
    { NULL,             FTCL_ARGV_HELP,   NULL, NULL, shTclSCHTransEntryAdd_help },
    {"<schemaTrans>",   FTCL_ARGV_STRING, NULL, NULL, "translation table handle"},
    {"<convType>",      FTCL_ARGV_STRING, NULL, NULL, "entry type: name or cont"},
    {"<fitsSideName>",  FTCL_ARGV_STRING, NULL, NULL, "fits side field name"},
    {"<objSideName>",   FTCL_ARGV_STRING, NULL, NULL, "object side field name"},
    {"<objSideDataType>",FTCL_ARGV_STRING, NULL, NULL, "object side data type"},
    {"-ratio",           FTCL_ARGV_DOUBLE, NULL, NULL, "fits to obj units ratio"},
    {"-proc",           FTCL_ARGV_STRING, NULL, NULL, "Tcl constructor for the entry"},
    {"-dimen",          FTCL_ARGV_STRING, NULL, NULL, "dimension info of obj side (e.g, 50x30x20 to represent an array of [50][30][20])"},
    {"-position",       FTCL_ARGV_STRING, NULL, NULL, "Add this entry to the given pos (default: append (-1))"},
    {"-heaptype",       FTCL_ARGV_STRING, NULL, NULL, "heap base type (e.g., float, char, int or schema name)"},
    {"-heaplength",     FTCL_ARGV_STRING, NULL, NULL, "expression to get heaplength at runtime (an integer number of base blocks)"},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
  };

static int shTclSCHTransEntryAdd (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
 
{  

    
  SCHEMATRANS* xptr = NULL;
  double ratio=1.0;
  int    pos = -1;
  int    rstatus;

  char* schemaTransNamePtr = NULL;
  char* typePtr=NULL;
  char* srcPtr=NULL;
  char* dstPtr=NULL;
  char* dsttypePtr=NULL;
  char* procPtr=NULL;
  char* heaptypePtr=NULL;
  char* heaplengthPtr=NULL;
  char* sizePtr=NULL;
  
  
  schemaTransEntryAddArgTable[1].dst = &schemaTransNamePtr;
  schemaTransEntryAddArgTable[2].dst = &typePtr;
  schemaTransEntryAddArgTable[3].dst = &srcPtr;
  schemaTransEntryAddArgTable[4].dst = &dstPtr;
  schemaTransEntryAddArgTable[5].dst = &dsttypePtr;
  schemaTransEntryAddArgTable[6].dst = &ratio;
  schemaTransEntryAddArgTable[7].dst = &procPtr;
  schemaTransEntryAddArgTable[8].dst = &sizePtr;
  schemaTransEntryAddArgTable[9].dst = &pos;
  schemaTransEntryAddArgTable[10].dst = &heaptypePtr;
  schemaTransEntryAddArgTable[11].dst = &heaplengthPtr;
  
   
  if((rstatus = shTclParseArgv(a_interp, &argc, argv,
			       schemaTransEntryAddArgTable,
			       FTCL_ARGV_NO_LEFTOVERS,
			       shTclSCHTransEntryAdd_cmd))!=FTCL_ARGV_SUCCESS)
   {
        return (rstatus);
   }

   if(shTclAddrGetFromName(a_interp, schemaTransNamePtr,
                 (void **) &xptr,"SCHEMATRANS")!=TCL_OK)
   {
      Tcl_AppendResult(a_interp, "Bad SCHEMATRANS handle", (char *) NULL);
         return TCL_ERROR; 
   }
     
   if(shSchemaTransEntryAdd(xptr, convTypeFromName(typePtr), srcPtr, dstPtr , 
               dsttypePtr, heaptypePtr,
           heaplengthPtr, procPtr, sizePtr, ratio, pos)!=SH_SUCCESS)
    {
      shTclInterpAppendWithErrStack(a_interp);
      return(TCL_ERROR);
    }
  Tcl_SetResult(a_interp, "", TCL_VOLATILE);
  
  return(TCL_OK);
}

/**************  schemaTransEntryDel *****************************/
/* return message "entry deleted */

#define shTclSCHTransEntryDel_help  "deletes a translation table entry and its related continuation entries in a translation table.\n"

static char* shTclSCHTransEntryDel_cmd="schemaTransEntryDel";

static ftclArgvInfo schemaTransEntryDelArgTable[] = {
  { NULL,             FTCL_ARGV_HELP,   NULL, NULL, shTclSCHTransEntryDel_help },
  {"<schemaTrans>",   FTCL_ARGV_STRING, NULL, NULL, "SCHEMATRANS handle"},
  {"<entryNumber>",   FTCL_ARGV_INT,    NULL, NULL, "entry number"},
  {"-only",           FTCL_ARGV_CONSTANT,(void*) TRUE,NULL, "Set if only the given entry is to delete"},
  {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
 };

static int shTclSCHTransEntryDel (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
 
{      
  int rstatus;
  SCHEMATRANS* xptr = NULL;
  int   entry;
  char* schemaTransNamePtr = NULL;
  unsigned short delete_this_entry_only=FALSE;  /* default to be delete all related entries */
  
  schemaTransEntryDelArgTable[1].dst = &schemaTransNamePtr;
  schemaTransEntryDelArgTable[2].dst = &entry;
  schemaTransEntryDelArgTable[3].dst = &delete_this_entry_only;

  if ((rstatus = shTclParseArgv(a_interp, &argc, argv,
				schemaTransEntryDelArgTable,
				FTCL_ARGV_NO_LEFTOVERS,
				shTclSCHTransEntryDel_cmd))
      != FTCL_ARGV_SUCCESS)
   {
        return (rstatus);
   }
      
   if(shTclAddrGetFromName(a_interp, schemaTransNamePtr,
                 (void **) &xptr,"SCHEMATRANS")!=TCL_OK)
    {
         Tcl_AppendResult(a_interp, "Bad SCHEMATRANS handle", (char *) NULL);
         return TCL_ERROR;
      
   }
   
   if(shSchemaTransEntryDel(xptr, (unsigned int) entry,!delete_this_entry_only)!=SH_SUCCESS)
    {
      shTclInterpAppendWithErrStack(a_interp);
      return(TCL_ERROR);
    }

  Tcl_SetResult(a_interp, "", TCL_VOLATILE);
  return(TCL_OK);
}

/**************  schemaTransEntryImport *****************************/

#define shTclSCHTransEntryImport_help "Imports a number of entries from a source table to the destionation table.\n"

static char* shTclSCHTransEntryImport_cmd ="schemaTransEntryImport";

static ftclArgvInfo schemaTransEntryImporArgTable[] = {
   { NULL,      FTCL_ARGV_HELP,   NULL, NULL, shTclSCHTransEntryImport_help },
   {"<src>",    FTCL_ARGV_STRING, NULL, NULL, "Source translation table"},
   {"<dst>",    FTCL_ARGV_STRING, NULL, NULL, "Destination translation table"},
   {"-from",    FTCL_ARGV_INT,    NULL, NULL, "Starting entry"},
   {"-to",      FTCL_ARGV_INT,    NULL, NULL, "ending entry (inclusive)"},
   {"-at",      FTCL_ARGV_INT,    NULL, NULL, "Starting entry in dst table (default: end)"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
  };

static int shTclSCHTransEntryImport (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
 
{  

   int rstatus;
   SCHEMATRANS* src=NULL, *dst=NULL;
   char* srcNamePtr = NULL, *dstNamePtr = NULL;
   int src_start=0;
   int src_end=MAX_SCHEMATRANS_ENTRY_SIZE;
   int pos = -1;

   schemaTransEntryImporArgTable[1].dst = &srcNamePtr;
   schemaTransEntryImporArgTable[2].dst = &dstNamePtr;
   schemaTransEntryImporArgTable[3].dst = &src_start;
   schemaTransEntryImporArgTable[4].dst = &src_end;
   schemaTransEntryImporArgTable[5].dst = &pos;

   if((rstatus = shTclParseArgv(a_interp, &argc, argv,
				schemaTransEntryImporArgTable,
				FTCL_ARGV_NO_LEFTOVERS,
				shTclSCHTransEntryImport_cmd))
       !=FTCL_ARGV_SUCCESS) 
   {
        return (rstatus);
   }

   if(shTclAddrGetFromName(a_interp, srcNamePtr,(void **) &src,"SCHEMATRANS")!=TCL_OK)
                return TCL_ERROR;
   if(shTclAddrGetFromName(a_interp, dstNamePtr,(void **) &dst,"SCHEMATRANS")!=TCL_OK)
                return TCL_ERROR;
        
   if(shSchemaTransEntryImport(src, dst, src_start, src_end, pos)!=SH_SUCCESS)
    {
      shTclInterpAppendWithErrStack(a_interp);
      return(TCL_ERROR);
    }
 
  Tcl_SetResult(a_interp, "", TCL_VOLATILE);
  return(TCL_OK);
}

/**************  schemaTransEntryShow *****************************/

#define shTclSCHTransEntryShow_help "prints out selected entries in the table. Default to print all entries.\n"

static char* shTclSCHTransEntryShow_cmd="schemaTransEntryShow";

static ftclArgvInfo schemaTransEntryShowArgTable[] = {
    { NULL,            FTCL_ARGV_HELP,   NULL, NULL, shTclSCHTransEntryShow_help },
    {"<schemaTrans>",  FTCL_ARGV_STRING, NULL, NULL, "translation table handle"},
    {"-start",         FTCL_ARGV_INT,    NULL, NULL, "starting entry"},
    {"-stop",          FTCL_ARGV_INT,    NULL, NULL, "ending entry"},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
  };

static int shTclSCHTransEntryShow (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
 
{      
  int rstatus;
  int start = 0, stop = -1;
  char* schemaTransNamePtr = NULL;
  SCHEMATRANS* xptr = NULL;
  
  schemaTransEntryShowArgTable[1].dst = &schemaTransNamePtr;
  schemaTransEntryShowArgTable[2].dst = &start;
  schemaTransEntryShowArgTable[3].dst = &stop;

  if ((rstatus = shTclParseArgv(a_interp, &argc, argv,
				schemaTransEntryShowArgTable,
				FTCL_ARGV_NO_LEFTOVERS,
				shTclSCHTransEntryShow_cmd))
      !=FTCL_ARGV_SUCCESS ) 
  {    
        return (rstatus);
   }
      

  if(shTclAddrGetFromName(a_interp, schemaTransNamePtr,
                (void **) &xptr,"SCHEMATRANS")!=TCL_OK)
         return TCL_ERROR;
      
   shSchemaTransEntryShow(xptr, start, stop);
   return(TCL_OK);
}


/**************  schemaTransWriteToFile *****************************/

#define shTclSCHTransWriteToFile_help "writes out selected entries in the table. Default to write all entries.\n"

static char* shTclSCHTransWriteToFile_cmd="schemaTransWriteToFile";

static ftclArgvInfo schemaTransWriteToFileArgTable[] = {
    { NULL,            FTCL_ARGV_HELP,   NULL, NULL, shTclSCHTransWriteToFile_help },
    {"<schemaTrans>",  FTCL_ARGV_STRING, NULL, NULL, "translation table handle"},
    {"<fileName>",     FTCL_ARGV_STRING, NULL, NULL, "destination file name"},
    {"-start",         FTCL_ARGV_INT,    NULL, NULL, "starting entry"},
    {"-stop",          FTCL_ARGV_INT,    NULL, NULL, "ending entry"},
    {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
  };


static int shTclSCHTransWriteToFile (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
 
{      
  int rstatus;
  int start = 0, stop = -1;
  char* fileNamePtr = NULL;
  char* schemaTransNamePtr = NULL;

  SCHEMATRANS* xptr = NULL;

  schemaTransWriteToFileArgTable[1].dst = &schemaTransNamePtr;
  schemaTransWriteToFileArgTable[2].dst = &fileNamePtr;
  schemaTransWriteToFileArgTable[3].dst = &start;
  schemaTransWriteToFileArgTable[4].dst = &stop;
  
  if ((rstatus = shTclParseArgv(a_interp, &argc, argv,
				schemaTransWriteToFileArgTable,
				FTCL_ARGV_NO_LEFTOVERS,
				shTclSCHTransWriteToFile_cmd))
      !=FTCL_ARGV_SUCCESS) 
  {    
        return (rstatus);
  }
   
  if(shTclAddrGetFromName(a_interp,schemaTransNamePtr ,
                (void **) &xptr,"SCHEMATRANS")!=TCL_OK)
         return TCL_ERROR;

   
   if(shSchemaTransWriteToFile(xptr, fileNamePtr, start, stop)!=SH_SUCCESS)
   {     
        shTclInterpAppendWithErrStack(a_interp);
        Tcl_AppendResult(a_interp, "", (char *) NULL);
        return (TCL_ERROR);
   }
   
   return(TCL_OK);
}

/**************  schemaTransEntryAddFromFile *****************************/

static char *shTclSCHTransEntryAddFromFile_use = "Usage: schemaTransEntryAddFrom File "
"<tblHandle> <fileName>";

static char* shTclSCHTransEntryAddFromFile_help = "add entries listed in an "
"ascii file";

static char* shTclSCHTransEntryAddFromFile_cmd="schemaTransEntryAddFromFile";

static int shTclSCHTransEntryAddFromFile (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
 
{      
  SCHEMATRANS* xptr = NULL;
  
  if (argc==3) 
  {
      if(shTclAddrGetFromName(a_interp, argv[1],
                (void **) &xptr,"SCHEMATRANS")!=TCL_OK)
         return TCL_ERROR;
      
  }
  else
   {    
        Tcl_AppendResult(a_interp, shTclSCHTransEntryAddFromFile_use, (char *) NULL);
        return (TCL_ERROR);
   }
   if(shSchemaTransCreateFromFile(xptr, argv[2])!=SH_SUCCESS)
   {     
        shTclInterpAppendWithErrStack(a_interp);
        Tcl_AppendResult(a_interp, "", (char *) NULL);
        return (TCL_ERROR);
   }
   
   return(TCL_OK);
}


/**************  schemaTransEntryClearAll *****************************/
/* return message 'all entries deleted */


static char *shTclSCHTransEntryClearAll_use = "Usage: schemaTransEntryClearAll tableHandle";
static char* shTclSCHTransEntryClearAll_help = "clears out all the entries in the table.";
static char* shTclSCHTransEntryClearAll_cmd="schemaTransEntryClearAll";

static int shTclSCHTransEntryClearAll (
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
 
{      
  SCHEMATRANS* xptr = NULL;
  
  
  if (argc==2)
   {
        if(shTclAddrGetFromName(a_interp, argv[1],
                 (void **) &xptr,"SCHEMATRANS")!=TCL_OK)
         return TCL_ERROR;
   }
   else
   {
        Tcl_AppendResult(a_interp, shTclSCHTransEntryClearAll_use, (char *) NULL);
        return (TCL_ERROR);
   }
   
  shSchemaTransClearEntries(xptr);
  Tcl_SetResult(a_interp, "", TCL_VOLATILE);
  
  return(TCL_OK);
}


static char* facil = "shSchemaTrans";

void
shTclSchemaTransDeclare(Tcl_Interp *a_interp)
{

     int flags = FTCL_ARGV_NO_LEFTOVERS;

     shTclDeclare(a_interp, shTclSCHTransNew_cmd, shTclSCHTransNew,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclSCHTransNew_help,
               shTclSCHTransNew_use);

     shTclDeclare(a_interp, shTclSCHTransDel_cmd, shTclSCHTransDel,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclSCHTransDel_help,
               shTclSCHTransDel_use);
   
     shTclDeclare(a_interp, shTclSCHTransEntryAdd_cmd, shTclSCHTransEntryAdd,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclGetArgInfo(a_interp, schemaTransEntryAddArgTable, flags,
			       shTclSCHTransEntryAdd_cmd),
	       shTclGetUsage(a_interp, schemaTransEntryAddArgTable, flags,
			     shTclSCHTransEntryAdd_cmd));

     shTclDeclare(a_interp, shTclSCHTransEntryImport_cmd, shTclSCHTransEntryImport,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclGetArgInfo(a_interp, schemaTransEntryImporArgTable, flags,
			       shTclSCHTransEntryImport_cmd),
	       shTclGetUsage(a_interp, schemaTransEntryImporArgTable, flags,
			     shTclSCHTransEntryImport_cmd));

     shTclDeclare(a_interp, shTclSCHTransEntryDel_cmd, shTclSCHTransEntryDel,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclGetArgInfo(a_interp, schemaTransEntryDelArgTable, flags,
			       shTclSCHTransEntryDel_cmd),
	       shTclGetUsage(a_interp, schemaTransEntryDelArgTable, flags,
			     shTclSCHTransEntryDel_cmd));

     shTclDeclare(a_interp, shTclSCHTransEntryShow_cmd, shTclSCHTransEntryShow,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclGetArgInfo(a_interp, schemaTransEntryShowArgTable, flags,
			       shTclSCHTransEntryShow_cmd),
	       shTclGetUsage(a_interp, schemaTransEntryShowArgTable, flags,
			     shTclSCHTransEntryShow_cmd));

     shTclDeclare(a_interp, shTclSCHTransWriteToFile_cmd, shTclSCHTransWriteToFile,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclGetArgInfo(a_interp, schemaTransWriteToFileArgTable, flags,
			       shTclSCHTransWriteToFile_cmd),
	       shTclGetUsage(a_interp, schemaTransWriteToFileArgTable, flags,
			     shTclSCHTransWriteToFile_cmd));

     shTclDeclare(a_interp, shTclSCHTransEntryAddFromFile_cmd, shTclSCHTransEntryAddFromFile,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclSCHTransEntryAddFromFile_help,
               shTclSCHTransEntryAddFromFile_use);

     shTclDeclare(a_interp, shTclSCHTransEntryClearAll_cmd, shTclSCHTransEntryClearAll,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclSCHTransEntryClearAll_help,
               shTclSCHTransEntryClearAll_use);
     return;
}

/*************************************************************************
 *  Facility:           dervish                                            *
 *  Synopsis:           Tcl routines for conversions of TBLCOL to schema *
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
#include "shCGarbage.h"

/***************************tblToSchema *****************************/
/* return the schema or container handle that was passed in */

#define shTclTblToSchema_help "Converts Tblcol format to schema format.\n"

static char* shTclTblToSchema_cmd="tblToSchema";

static ftclArgvInfo tblToSchemaArgTable[] = {
   { NULL,         FTCL_ARGV_HELP,    NULL, NULL, shTclTblToSchema_help },
   {"<tblcol>",    FTCL_ARGV_STRING,  NULL, NULL, "source TBLCOL handle"},
   {"<object>",    FTCL_ARGV_STRING,  NULL, NULL, "container or object handle"},
   {"<schemaTrans>", FTCL_ARGV_STRING, NULL, NULL,"Translation table handle"},
   {"-proc", FTCL_ARGV_STRING, NULL, NULL, "Object Tcl constructor"},
   {"-row",  FTCL_ARGV_INT,    NULL, NULL, "Extract begins at this row"},
   {"-stopRow", FTCL_ARGV_INT, NULL, NULL, "Extract stops  at this row"},
   {"-schemaName",FTCL_ARGV_STRING, NULL, NULL,"Schema of the containees"},
   {"-objectReuse", FTCL_ARGV_CONSTANT, (void*)TRUE, NULL,"Set if use the objects"
"already in the container"},
   {"-handleRetain", FTCL_ARGV_CONSTANT, (void*)TRUE, NULL, "Set if handles are"
"retained"},
   {NULL, FTCL_ARGV_END, NULL, NULL, NULL}
  };

static int shTclTblToSchema(
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
{

    int     rstatus;
    HANDLE  schemaHdl;              		/* object */
    HANDLE  tblcolHdl;              		/* TBLCOL */
    HANDLE  xtblHdl;             		/* the translation table */
    HANDLE  genericHdl;

    SCHEMATRANS*      xtbl_ptr = NULL;		/* translation table */
    SCHEMATRANS_ENTRY* xptr     = NULL;


    const SCHEMA* schemaPtr=NULL;		/* the schema of the object(s) */
    char* 	instPtr=NULL; 			/* object address */
   
    ARRAY* 	containerArray=NULL;		/* container object */
    CHAIN*  	containerChain=NULL;
    CONTAINER_TYPE  container_type;
  
    int 	stopRow= -1, startRow = -1; 
    int         sugStopRow = -1, sugStartRow = -1;

    
    char 	result[MAX_NAME_LEN];		/* useful strings */
    char 	schemaHandleName[MAX_NAME_LEN];
    char*       schemaHandleNamePtr = NULL;
    char*       tblcolHandleNamePtr = NULL;
    char*       schemaTransNamePtr = NULL;

    char 	schemaName[MAX_NAME_LEN];         
    char 	*tclConstructPtr = NULL;
    char* 	schemaNamePtr=NULL;
   
    void*       walkPtr = NULL;                 /* use withe chain or list */

    char* 	instAddr[MAX_ADDRESS_SIZE];	/* address holder */
    char**	instAddrPtr=instAddr;   
    int     	inst_addr_size=MAX_ADDRESS_SIZE; 
    
    int  	k, row;				/* misc integers */
    int 	addrNum=0;
    int 	status;
    
    unsigned 	convert_all      = FALSE;	/* misc switches */	
    unsigned  	inst_addr_malloc = FALSE;  
    unsigned 	objectReuse      = FALSE;
    unsigned 	handleRetain     = FALSE;
    unsigned 	is_link          = FALSE;

   tblToSchemaArgTable[1].dst = &tblcolHandleNamePtr;
   tblToSchemaArgTable[2].dst = &schemaHandleNamePtr;
   tblToSchemaArgTable[3].dst = &schemaTransNamePtr;
   tblToSchemaArgTable[4].dst = &tclConstructPtr;
   tblToSchemaArgTable[5].dst = &sugStartRow;
   tblToSchemaArgTable[6].dst = &sugStopRow;
   tblToSchemaArgTable[7].dst = &schemaNamePtr;
   tblToSchemaArgTable[8].dst = &objectReuse;
   tblToSchemaArgTable[9].dst = &handleRetain;


   if((rstatus = shTclParseArgv(a_interp, &argc, argv, tblToSchemaArgTable,
				FTCL_ARGV_NO_LEFTOVERS,
				shTclTblToSchema_cmd)) == FTCL_ARGV_SUCCESS) 
   {

         status =shTclHandleGetFromName(a_interp, schemaHandleNamePtr
                            , &schemaHdl);
         if(status==TCL_OK) status=shTclHandleGetFromName(a_interp, 
                                tblcolHandleNamePtr, &tblcolHdl);
         if(status==TCL_OK) status=shTclHandleGetFromName(a_interp, 
                                schemaTransNamePtr, &xtblHdl);
         if(status!=TCL_OK) 
         {
            Tcl_AppendResult(a_interp, "Fail to get handle addresses", NULL);
            return TCL_ERROR;
         }

         if(schemaNamePtr == NULL)
         {    
             schemaNamePtr=strcpy(schemaName, shNameGetFromType(schemaHdl.type));
             instPtr = (char*) schemaHdl.ptr;
         }

        schemaPtr = shSchemaGet(schemaNamePtr);
        if(schemaPtr==NULL) 
        {
          Tcl_AppendResult(a_interp, "Unknown schema", (char *) NULL);
          return (TCL_ERROR);
        }
 

        xtbl_ptr=(SCHEMATRANS*) xtblHdl.ptr;
        if(xtbl_ptr == NULL ) 
        {
              Tcl_AppendResult(a_interp, "\nTranslation table NULL", NULL);
              return TCL_ERROR;
        }

       if(xtblHdl.type!=shTypeGetFromName("SCHEMATRANS"))
       {
           Tcl_AppendResult(a_interp, "No translation table.\n", NULL);
           return TCL_ERROR;
       }

       xptr = xtbl_ptr->entryPtr;

        if(shSpptGeneralSyntaxCheck(xtbl_ptr, (SCHEMA*) schemaPtr, FALSE)!=SH_SUCCESS)
        {
              shTclInterpAppendWithErrStack(a_interp);
              return TCL_ERROR;
        } 

     }

    else 
     {
          return (rstatus);
     }

    /* here we determine if what's passed in is handle to a container 
       or a handle to a schema object. If handle to a container, we'll
       extract all the objects from TBLCOL and put them in the container.
       If not, then just fill the object field with data from TBLCOL */

    if(strcmp(shNameGetFromType(schemaHdl.type), schemaNamePtr)==0)
     {
         /* case:  one single row at time */

         
         startRow = (sugStartRow >= 0) ? sugStartRow:0;
         stopRow = startRow + 1;
         instPtr = (char*) schemaHdl.ptr;
         container_type = NOT_A_CONTAINER;
     }
     else if(strcmp(shNameGetFromType(schemaHdl.type), "ARRAY")==0)
     {
         /* case: put the objects in the container array */
         
         startRow = (sugStartRow >= 0) ? sugStartRow: 0;
         stopRow = ((TBLCOL*)(tblcolHdl.ptr))->rowCnt; 
 
         /* if sugStopRow is valid, we'll use it */
  
         if(sugStopRow > 0 && sugStopRow < stopRow)  stopRow = sugStopRow; 

         containerArray = (ARRAY*) schemaHdl.ptr;
         containerArray->data.schemaType=shTypeGetFromName(schemaNamePtr);
         containerArray->info = (void*) shSchemaGet(schemaNamePtr);
         containerArray->infoType = containerArray->data.schemaType;
         if( containerArray->dimCnt == 0) containerArray->dimCnt=1;
         if( containerArray->dimCnt > 1) 
         { 
              Tcl_AppendResult(a_interp, "Multi-D array is not allowed.", NULL);
              return TCL_ERROR;
         }
         containerArray->nStar=1;   /* array of pointers */
         convert_all = TRUE;    
         container_type =   ARRAY_TYPE;    
         
            
     }
     else if(strcmp(shNameGetFromType(schemaHdl.type), "CHAIN")==0)
     {
         /* case: put the objects in the container */
         
         startRow = (sugStartRow >= 0) ? sugStartRow: 0;
         stopRow = ((TBLCOL*)(tblcolHdl.ptr))->rowCnt; 
 
         /* if sugStopRow is valid, we'll use it */
  
         if(sugStopRow > 0 && sugStopRow < stopRow)  stopRow = sugStopRow; 
         containerChain = (CHAIN*) schemaHdl.ptr;  
         
         convert_all = TRUE;
         container_type=CHAIN_TYPE;
     }

     else
     {
         Tcl_AppendResult(a_interp, "Not a legitimate container", NULL);
         return TCL_ERROR;
      }

      /* 
         if convert_all (container case) and proc on command line
         are given, we call Tcl_Eval to get the memories for the 
         new objects we're to create. If no proc, we'll call
         default constructor in schema or manually allocate memory
         in that order. Then put all the addresses in instAddrPtr,
         which points to an arrary of char*.

       */

      if(convert_all)
      {
         addrNum=0;
         if(stopRow - startRow>= inst_addr_size) 
           {
                 inst_addr_size = stopRow -startRow;
                 inst_addr_malloc=TRUE;
                 instAddrPtr = (char**) shMalloc((stopRow-startRow)*sizeof(char**));
         }
         if(!objectReuse && tclConstructPtr!=NULL) 
         {
           for(k=0; k < stopRow -startRow; k++)
            {
                 if((status=Tcl_Eval(a_interp, tclConstructPtr)) == TCL_OK)
                  {
                      strcpy(result, a_interp->result);
                      status=shTclHandleGetFromName(a_interp, result,
                                      &genericHdl);

                      /* delete the handle */
                      if(!handleRetain)
                     {
                         if(p_shTclHandleDel(a_interp, result) != TCL_OK) 
                        {
                               shTclInterpAppendWithErrStack(a_interp);
                               return(TCL_ERROR);
                         }
                      }
                      instAddrPtr[addrNum] = (char*) genericHdl.ptr;
                      addrNum++;
                              
                  }

                  if(status != TCL_OK || genericHdl.ptr== NULL)
                  {
                      Tcl_AppendResult(a_interp, "Cmd execution failure", (char *) NULL);
                      return (TCL_ERROR);
                   }
             } /* for loop */
          
         }  /* if(!objectReuse && ftclPresent("proc"))  */

        else if(objectReuse)
        {

             for(k=0; k < stopRow - startRow; k++) 
             {
		instAddrPtr[k]=shSpptObjGetFromContainer(
                             	container_type,
          			schemaHdl.ptr, 
          			&walkPtr,          
          			schemaNamePtr,
          			k);
                if(instAddrPtr[k]==NULL)
                {
		        Tcl_AppendResult(a_interp, "Not enough objects", NULL);
                        return TCL_ERROR;
                 }		
              }
              addrNum = stopRow - startRow;
      
         }
        
                   
     } /* if (covert_all) */
     
     
   for(row=0; row < stopRow - startRow; row++)
   {
      if(convert_all)   /* i.e, not single-row converting. Have to find address */
      {
          if(instAddrPtr!=NULL && addrNum > 0) instPtr = instAddrPtr[row];
          else if(schemaPtr->construct!=NULL) 
               instPtr = (char*) (*(schemaPtr->construct))();
          else
          {
                 instPtr = (char*) shMalloc(schemaPtr->size);
                 if(instPtr!=NULL) memset(instPtr, 0, schemaPtr->size);
          }
          if(instPtr==NULL)
          {
                  Tcl_AppendResult(a_interp, "All means to allocate memory for a schema inst failed\n", NULL);
                  return TCL_ERROR;
           }

       }  /* if(convert_all) */

       /* no proc given on command line, 
           so let's copy the address into instAddrPtr */

       if(addrNum <= 0)  instAddrPtr[row]=instPtr;

   } 

    if(shSpptTclObjectInit(a_interp, 
		xtbl_ptr, (void**) instAddrPtr,stopRow - startRow,
             (SCHEMA*) schemaPtr,handleRetain) !=SH_SUCCESS)
    {  
             Tcl_AppendResult(a_interp, "Object initializaton error", (char*) NULL);
             shTclInterpAppendWithErrStack(a_interp);
             return TCL_ERROR;
    }

 
   /* We've finally got these many addresses */

   if(convert_all) addrNum = stopRow - startRow;

   /* This is the beef!  */
   
   if(shCnvTblToSchema((TBLCOL*)tblcolHdl.ptr,(SCHEMATRANS*)xtblHdl.ptr, 
           schemaHdl.ptr, 
           schemaNamePtr,instAddrPtr, addrNum, startRow,stopRow, 
          container_type, objectReuse)!=SH_SUCCESS)
   {
          shTclInterpAppendWithErrStack(a_interp);
          return(TCL_ERROR);
   }

   if(inst_addr_malloc) shFree((char*) instAddrPtr);
   Tcl_SetResult(a_interp, schemaHandleName, TCL_VOLATILE);
   
   return (TCL_OK);
}


/**************  schemaToTbl *****************************/
/* return a handle to TBLOCL */

#define shTclSchemaToTbl_help "Converts instance(s) of a schema into tblcol format for use in writing to a fits file.\n"

static char* shTclSchemaToTbl_cmd = "schemaToTbl";

static ftclArgvInfo schemaToTblArgTable[] = {
  { NULL,            FTCL_ARGV_HELP,   NULL, NULL, shTclSchemaToTbl_help },
  {"<schemaHandle>", FTCL_ARGV_STRING, NULL, NULL, "source object/container handle"},
  {"<schemaTrans>",  FTCL_ARGV_STRING, NULL, NULL, "translation table handle"},
  {"-schemaName",    FTCL_ARGV_STRING, NULL, NULL, "schema of containees (required when schemaHandle is ARRAY"},
  {"-autoConvert",   FTCL_ARGV_CONSTANT, (void*) TRUE, NULL, "True if automatic conversion needed (Default: FALSE)"},
  {NULL, FTCL_ARGV_END, NULL, NULL,  NULL}
 };

static int shTclSchemaToTbl(
   ClientData   a_clientdata,    /* TCL parameter - not used */
   Tcl_Interp   *a_interp,      /*  TCL Interpreter structure */
   int          argc,           /*  TCL argument count */
   char         **argv          /*  TCL argument */
   )   
{
    int    rstatus;
    HANDLE schemaHdl;
    HANDLE xtblHdl ;
    SCHEMATRANS * xtbl_ptr=NULL;
    SCHEMATRANS_ENTRY *xptr = NULL;
    const SCHEMA* schemaPtr= NULL;

    HANDLE  tblcol;


    char  schemaName[MAX_NAME_LEN], *schemaNamePtr = NULL;
    char* schemaHandleNamePtr = NULL, *schemaTransNamePtr = NULL;
    char  name[HANDLE_NAMELEN];
    int status;
    unsigned want_auto_convert = FALSE;
    CONTAINER_TYPE containerType = NOT_A_CONTAINER;

    schemaToTblArgTable[1].dst = &schemaHandleNamePtr;
    schemaToTblArgTable[2].dst = &schemaTransNamePtr;
    schemaToTblArgTable[3].dst = &schemaNamePtr;
    schemaToTblArgTable[4].dst = &want_auto_convert;

    
    if((rstatus = shTclParseArgv(a_interp, &argc, argv, schemaToTblArgTable,
				 FTCL_ARGV_NO_LEFTOVERS,
				 shTclSchemaToTbl_cmd))==FTCL_ARGV_SUCCESS)
      {
         
         status =shTclHandleGetFromName(a_interp, schemaHandleNamePtr, &schemaHdl);
         if(status==TCL_OK) status=shTclHandleGetFromName(a_interp, 
                        schemaTransNamePtr, &xtblHdl);
         if(status!=TCL_OK) 
         {
            Tcl_AppendResult(a_interp, "Fail to get handle addresses", NULL);
            return TCL_ERROR;
         }

        if(schemaNamePtr==NULL) 
        {
            schemaNamePtr = strcpy(schemaName,shNameGetFromType(schemaHdl.type));
        }
        schemaPtr = shSchemaGet(schemaNamePtr);
        if(schemaPtr==NULL) 
        {
          Tcl_AppendResult(a_interp, "Unknown schema", (char *) NULL);
          return (TCL_ERROR);
        }
        
        xtbl_ptr =(SCHEMATRANS*) xtblHdl.ptr;
        if(xtbl_ptr == NULL ) 
        {
              Tcl_AppendResult(a_interp, "Translation table NULL", NULL);
              return TCL_ERROR;
        }
        if(xtblHdl.type!=shTypeGetFromName("SCHEMATRANS"))
        {
            Tcl_AppendResult(a_interp, "No translation table\n", NULL);
            return TCL_ERROR;
        }

        xptr = xtbl_ptr->entryPtr;
        
        if(shSpptGeneralSyntaxCheck(xtbl_ptr, (SCHEMA*) schemaPtr, TRUE)!=SH_SUCCESS)
        {
              shTclInterpAppendWithErrStack(a_interp);
              return TCL_ERROR;
        } 



     }
     else
     {
        return(rstatus);

     }
 
     tblcol.type = shTypeGetFromName("TBLCOL");
     
     if(schemaHdl.type==shTypeGetFromName(schemaName))
     {
          containerType = NOT_A_CONTAINER;
     }
     else if(schemaHdl.type==shTypeGetFromName("ARRAY"))
     {
         containerType = ARRAY_TYPE;
     }
     else if(schemaHdl.type == shTypeGetFromName("CHAIN"))
     {
         containerType = CHAIN_TYPE;
     }
     

     /* convert to TBLCOL */

     if(shCnvSchemaToTbl((TBLCOL**) &tblcol.ptr, schemaHdl.ptr, 
      (SCHEMATRANS*)xtblHdl.ptr, schemaNamePtr, containerType, want_auto_convert)!=SH_SUCCESS)
     {
          shTclInterpAppendWithErrStack(a_interp);
          Tcl_AppendResult(a_interp, "Conversion Failure\n", (char*) NULL);
          return TCL_ERROR;
     }

    /* make a new handle to TBLCOL and bind address */
     
    if( shTclHandleNew(a_interp, name, "TBLCOL", (void*) tblcol.ptr) != TCL_OK )
     {
        if(xtbl_ptr !=NULL) shSchemaTransDel(xtbl_ptr);
        return(TCL_ERROR);
     }

     Tcl_SetResult(a_interp,name,TCL_VOLATILE);
     return (TCL_OK);
}



static char* facil = "shTblSchemaConvert";

void
shTclTblColSchemaConversionDeclare(Tcl_Interp *a_interp)
{

    int flags = FTCL_ARGV_NO_LEFTOVERS;

    shTclDeclare(a_interp, shTclTblToSchema_cmd, shTclTblToSchema,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclGetArgInfo(a_interp, tblToSchemaArgTable, flags,
			       shTclTblToSchema_cmd),
               shTclGetUsage(a_interp, tblToSchemaArgTable, flags,
			     shTclTblToSchema_cmd));

    shTclDeclare(a_interp, shTclSchemaToTbl_cmd, shTclSchemaToTbl,
               (ClientData) 0, (Tcl_CmdDeleteProc *) NULL,
               facil,
               shTclGetArgInfo(a_interp, schemaToTblArgTable, flags,
			       shTclSchemaToTbl_cmd),
               shTclGetUsage(a_interp, schemaToTblArgTable, flags,
			     shTclSchemaToTbl_cmd));


   return;
}

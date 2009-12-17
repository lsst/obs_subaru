/*
 * TCL support for disk dump functions
 *
 * ENTRY POINT		SCOPE
 * tclDiskioDeclare	public	declare all the verbs defined in this module
 *
 * REQUIRED PRODUCTS:
 *	FTCL	 	TCL + Fermi extension
 *
 * Robert Lupton
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "shCAssert.h"
#include "shCUtils.h"
#include "region.h"
#include "shCSao.h"                      /* prototype of CMD_HANDLE */
#include "shCErrStack.h"
#include "shTclErrStack.h"
#include "shCDiskio.h"
#include "shCSchema.h"
#include "tcl.h"
#include "ftcl.h"
#include "shTclHandle.h"
#include "shCHash.h"
#include "shTclUtils.h"		/* needed for shTclDeclare prototype */
#include "shTclVerbs.h"
  
#define BSIZE 1000			/* size for temp buffers */

static char *module = "shDiskio";		/* name of this set of code */
static FILE *dfil = NULL;		/* file descriptor for dumps */

/*****************************************************************************/
/*
 * some static utility functions
 *
 * Return the name of a type, given it's name or (decimal) value
 */
static char *
get_type(char *tname)
{
   return(isdigit(*tname) ? shNameGetFromType((TYPE)atoi(tname)) : tname);
}

/*****************************************************************************/

static char *tclDumpOpen_use =
  "USAGE: dumpOpen file mode";
static char *tclDumpOpen_hlp =
  "Open a disk dump file file for read or write.\n"
  "Mode may be r, w, or a for read, write, or append. If you capitalise"
  "the mode, don't perform usual cleanup on close";

static int
tclDumpOpen(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   if (argc != 3) {	
      Tcl_SetResult(interp,tclDumpOpen_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(dfil != NULL) {
      Tcl_SetResult(interp,"A dump file is already open",TCL_STATIC);
      return(TCL_ERROR);
   }
   if((dfil = shDumpOpen(argv[1],argv[2])) == NULL) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclDumpReopen_use =
  "USAGE: dumpReopen file mode";
static char *tclDumpReopen_hlp =
  "Reopen a disk dump file file for read or write\n"
  "This is the same as dumpOpen, but doesn't initialise data structures";

static int
tclDumpReopen(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   if (argc != 3) {	
      Tcl_SetResult(interp,tclDumpReopen_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(dfil != NULL) {
      Tcl_SetResult(interp,"A dump file is already open",TCL_STATIC);
      return(TCL_ERROR);
   }
   if((dfil = shDumpReopen(argv[1],argv[2])) == NULL) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclDumpClose_use =
  "USAGE: dumpClose";
static char *tclDumpClose_hlp =
  "Close a disk dump file";

static int
tclDumpClose(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   int ret;
   
   if (argc != 1) {
      Tcl_SetResult(interp,tclDumpClose_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(dfil == NULL) {
      Tcl_SetResult(interp,"No dump file is currently open",TCL_STATIC);
      return(TCL_ERROR);
   }
   if(shDumpClose(dfil) == SH_SUCCESS) {
      ret = TCL_OK;
   } else {
      ret = TCL_ERROR;
   }
   dfil = NULL;

   return(ret);
}

/*****************************************************************************/

static char *tclDumpPtrsResolve_use =
  "USAGE: dumpPtrsResolve";
static char *tclDumpPtrsResolve_hlp =
  "Resolve pointer ids, converting them to pointers where possible";

static int
tclDumpPtrsResolve(
		   ClientData clientData,
		   Tcl_Interp *interp,
		   int argc,
		   char **argv
		   )
{
   if (argc != 1) {
      Tcl_SetResult(interp,tclDumpPtrsResolve_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shDumpPtrsResolve() != SH_SUCCESS) {
      return(TCL_ERROR);
   }

   return(TCL_OK);
}

/*****************************************************************************/

static char *tclDumpDateGet_use =
  "USAGE: dumpDateGet";
static char *tclDumpDateGet_hlp =
  "Return the date string from a dump file";

static int
tclDumpDateGet(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   char *date;
   
   if (argc != 1) {
      Tcl_SetResult(interp,tclDumpDateGet_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(dfil == NULL) {
      Tcl_SetResult(interp,"No dump file is currently open",TCL_STATIC);
      return(TCL_ERROR);
   }
   if((date = shDumpDateGet(dfil)) == NULL) {
      Tcl_SetResult(interp,"Can't get date",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,date,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclDumpDateDel_use =
  "USAGE: dumpDateDel file";
static char *tclDumpDateDel_hlp =
  "Overwrite the date string in a dump file with Xs."
  "You may not have a dump file open when you use this verb";

static int
tclDumpDateDel(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   if (argc != 2) {
      Tcl_SetResult(interp,tclDumpDateDel_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(dfil != NULL) {
      Tcl_SetResult(interp,"A dump file is currently open; "
		    "please close it and try again",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   shDumpDateDel(argv[1]);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclDumpTypeGet_use =
  "USAGE: dumpTypeGet";
static char *tclDumpTypeGet_hlp =
  "Return the type of the next thing in a dump file";

static int
tclDumpTypeGet(
	    ClientData clientData,
	    Tcl_Interp *interp,
	    int argc,
	    char **argv
	    )
{
   TYPE type;

   if (argc != 1) {
      Tcl_SetResult(interp,tclDumpTypeGet_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(dfil == NULL) {
      Tcl_SetResult(interp,"No dump file is currently open",TCL_STATIC);
      return(TCL_ERROR);
   }
   
   if(shDumpTypeGet(dfil,&type) != SH_SUCCESS) {
      Tcl_SetResult(interp,"Can't find next type",TCL_STATIC);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,shNameGetFromType(type),TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Write an object (described by a handle) to the dump file
 */
static char *tclDumpHandleWrite_use =
  "USAGE: dumpHandleWrite handle";
static char *tclDumpHandleWrite_hlp =
  "Write an object described by <handle> to the current dump";

static int
tclDumpHandleWrite(
		   ClientData clientData,
		   Tcl_Interp *interp,
		   int argc,
		   char **argv
		)
{
   HANDLE realHandle;
   HANDLE *hand = &realHandle;
   void   *userPtr;
   THING *thing;
   
   if(argc != 2) {
      Tcl_SetResult(interp,tclDumpHandleWrite_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(dfil == NULL) {
      Tcl_SetResult(interp,"No dump file is currently open",TCL_STATIC);
      return(TCL_ERROR);
   }

   if(shTclHandleExprEval(interp,argv[1],hand,&userPtr) != TCL_OK) {
      return(TCL_ERROR);
   }
/*
 * assemble a thing to write
 */
   thing = shThingNew(hand->ptr,hand->type);
   if(shDumpNextWrite(dfil,thing) != SH_SUCCESS) {
      shThingDel(thing);
      Tcl_AppendResult(interp,"Can't write ",argv[2],"\n",(char *)NULL);
      return(TCL_ERROR);
   }
   shThingDel(thing);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Write an object, given as a <value> and a <type> to the dump file. This
 * is only supposed to be used for primitive types, ints, unsigned ints, longs,
 * unsigned longs, floats, doubles, logicals, or strings
 */
static char *tclDumpValueWrite_use =
  "USAGE: dumpValueWrite <value> <type>";
static char *tclDumpValueWrite_hlp =
  "Write something, given as a <value> and a <type>, to the current dump";

static int
tclDumpValueWrite(
		  ClientData clientData,
		  Tcl_Interp *interp,
		  int argc,
		  char **argv
		  )
{
   void *thing_ptr;
   float fval;
   double dval;
   int ival;
   unsigned int uival;
   long lval;
   unsigned long ulval;
   LOGICAL bval;
   char *ptr;
   THING *thing;
   TYPE type;
   char *type_name;

   if(argc != 3) {
      Tcl_SetResult(interp,tclDumpValueWrite_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(dfil == NULL) {
      Tcl_SetResult(interp,"No dump file is currently open",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * get the value in a suitable form
 */
   type_name = get_type(argv[2]);
   type = shTypeGetFromName(type_name);
   thing_ptr = NULL;

   if(p_shTypeIsEnum(type) || strcmp(type_name,"INT") == 0) {
      ival = (int)strtol(argv[1],&ptr,0);
      if(ptr != argv[1]) {
	 thing_ptr = &ival;
      }
   } else if(strcmp(type_name,"UINT") == 0) {
      uival = (unsigned int)strtoul(argv[1],&ptr,0);
      if(ptr != argv[1]) {
         thing_ptr = &uival;
      }
   } else if(strcmp(type_name,"LONG") == 0) {
      lval = strtol(argv[1],&ptr,0);
      if(ptr != argv[1]) {
	 thing_ptr = &lval;
      }
   } else if(strcmp(type_name,"ULONG") == 0) {
      ulval = strtoul(argv[1],&ptr,0);
      if(ptr != argv[1]) {
	 thing_ptr = &ulval;
      }
   } else if(strcmp(type_name,"FLOAT") == 0) {
      fval = strtod(argv[1],&ptr);
      if(ptr != argv[1]) {
	 thing_ptr = &fval;
      }
   } else if(strcmp(type_name,"DOUBLE") == 0) {
      dval = strtod(argv[1],&ptr);
      if(ptr != argv[1]) {
	 thing_ptr = &dval;
      }
   } else if(strcmp(type_name,"STR") == 0) {
      thing_ptr = argv[1];
   } else if(strcmp(type_name,"LOGICAL") == 0) {
      if (shBooleanGet (argv[1], &bval) == SH_SUCCESS) {
         thing_ptr = &bval;
      }
   } else {
      shErrStackPush("tclDumpValueWrite: Can't write a %s",argv[2]);
      return(TCL_ERROR);
   }
   if(thing_ptr == NULL) {
      shErrStackPush("tclDumpValueWrite: Can't convert %s to type %s\n",
		     argv[1],argv[2]);
      return(TCL_ERROR);
   }
/*
 * assemble a thing to write
 */
   thing = shThingNew(thing_ptr,type);
   if(shDumpNextWrite(dfil,thing) != SH_SUCCESS) {
      shThingDel(thing);
      Tcl_AppendResult(interp,"Can't write ",argv[1],"\n",(char *)NULL);
      return(TCL_ERROR);
   }
   shThingDel(thing);

   return(TCL_OK);
}

/*****************************************************************************/
/*
 * Read the next data item from the dump file
 */
static char *tclDumpHandleRead_use =
  "USAGE: dumpHandleRead [shallow]";
static char *tclDumpHandleRead_hlp =
  "read the next item from the current dump, returning the handle";

static int
tclDumpHandleRead(
		ClientData clientData,
		Tcl_Interp *interp,
		int argc,
		char **argv
		)
{
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   THING *thing;
   int shallow = 0;			/* is this a shallow dump? */
   
   if(argc == 2) {
      shallow = atoi(argv[1]);
      argc--;
   }

   if(argc != 1) {
      Tcl_SetResult(interp,tclDumpHandleRead_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if(dfil == NULL) {
      Tcl_SetResult(interp,"No dump file is currently open",TCL_STATIC);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle and make it refer to whatever we just read
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
/*
 * we've made the handle, so it's safe to read the file
 */
   if((thing = shDumpNextRead(dfil,shallow)) == NULL) {
      p_shTclHandleDel(interp,name);
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }
   handle.ptr = thing->ptr;
   handle.type = thing->type;
   shAssert(p_shTclHandleAddrBind(interp,handle,name) == TCL_OK);
   /*
    * this can't fail, as we have just got the new name succesfully. The
    * reason for an assertion is that there will be a major memory leak
    * if the pointer that we just read can't be bound to a handle; we
    * just read from the file and we'll have no way to refer to the objects
    */

   /*
    * Free the thing structure.  Set the thing->ptr to NULL if it was that
    * way when we did a shThingNew.  This way the reference counter attached
    * to the data will be correct.
    */
   if (handle.type != shTypeGetFromName("STR"))
     {thing->ptr = NULL;}
   shThingDel(thing);

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/
/*
 * manipulate and use schema
 */
static void
schemaPrintToInterp(Tcl_Interp *interp, const SCHEMA *sch)
{
   char buff[BSIZE];
   Tcl_DString result;
   int i,j;

   Tcl_DStringInit( &result);
   for(i = 0;i < sch->nelem;i++) {
     Tcl_DStringStartSublist( &result);     
     if (strlen(sch->elems[i].name) !=0 )
       Tcl_DStringAppendElement ( &result, sch->elems[i].name);
     sprintf(buff,"%s",sch->elems[i].type);
     for(j = 0;j < sch->elems[i].nstar;j++) {
       strcat(buff,"*");
     }
     
     if(sch->elems[i].nelem != NULL) {	/* an array */
       char buff2[100];
       char *ptr1,*ptr2;
       strcat(buff,"[");
       
       for(ptr1 = sch->elems[i].nelem;*ptr1 != '\0';) {
	 ptr2 = buff2;
	 for(;;) {
	   if(*ptr1 == ' ' || *ptr1 == '\0') {
	     if(*ptr1 == ' ') {
	       ptr1++;
	       *ptr2++ = ']';
	       *ptr2++ = '[';
	     }
	     *ptr2 = '\0';
	     strcat(buff,buff2);
	     break;
	   }
	   *ptr2++ = *ptr1++;
	 }
       }
       strcat(buff,"]");
     }
     Tcl_DStringAppendElement ( &result, buff);
     Tcl_DStringEndSublist( &result);
   }
  Tcl_DStringResult(interp, &result);
}

/*****************************************************************************/

static char *tclHandleNewFromType_use =
  "USAGE: handleNewFromType type-name";
static char *tclHandleNewFromType_hlp =
  "make a new thing of the given <type>, using the constructor specified\n"
  "in its schema definition";

static int
tclHandleNewFromType(
		     ClientData clientData,
		     Tcl_Interp *interp,
		     int argc,
		     char **argv
		     )
{
   char buff[BSIZE];
   HANDLE handle;
   char name[HANDLE_NAMELEN];
   const SCHEMA *sch;
   
   if (argc != 2) {
      Tcl_SetResult(interp,tclHandleNewFromType_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if((sch = (SCHEMA *)shSchemaGet(get_type(argv[1]))) == NULL) {
      sprintf(buff,"Can't retrieve schema for %s",argv[1]);
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
      return(TCL_ERROR);
   }

   if(sch->construct == NULL) {
      sprintf(buff,"Schema for %s doesn't specify a constructor",argv[1]);
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
      return(TCL_ERROR);
   }
/*
 * ok, get a handle for our new object
 */
   if(p_shTclHandleNew(interp,name) != TCL_OK) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   handle.ptr = (*sch->construct)();
   handle.type = shTypeGetFromName(argv[1]);

   if(p_shTclHandleAddrBind(interp,handle,name) != TCL_OK) {
      sprintf(buff,"Can't bind to new %s handle",argv[1]);
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
      return(TCL_ERROR);
   }

   Tcl_SetResult(interp,name,TCL_VOLATILE);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclHandleDelFromType_use =
  "USAGE: handleDelFromType handle";
static char *tclHandleDelFromType_hlp =
  "delete an object given its handle, using the destructor specified\n"
  "in its schema definition";

static int
tclHandleDelFromType(
		     ClientData clientData,
		     Tcl_Interp *interp,
		     int argc,
		     char **argv
		     )
{
   char buff[BSIZE];
   HANDLE userHandle;
   void *userPtr;
   HANDLE *handle = &userHandle;
   char *handle_name;
   const SCHEMA *sch;
   
   
   if(argc==2) {
      handle_name = argv[1];
      if (shTclHandleExprEval(interp, handle_name, &userHandle, &userPtr) !=
	  TCL_OK) {
        
         return (TCL_ERROR);
      }
   } else {
      Tcl_SetResult(interp,tclHandleDelFromType_use,TCL_STATIC);
      
      return(TCL_ERROR);
   }

   

   if((sch = (SCHEMA *)shSchemaGet(shNameGetFromType(handle->type))) == NULL) {
      sprintf(buff,"Can't retrieve schema for %s",
	      shNameGetFromType(handle->type));
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
      return(TCL_ERROR);
   }

   if(sch->destruct == NULL) {
      sprintf(buff,"Schema for %s doesn't specify a destructor",
	      shNameGetFromType(handle->type));
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
      return(TCL_ERROR);
   }

   (*sch->destruct)(handle->ptr);
  
   /* Now delete the handle */
   (void) p_shTclHandleDel(interp, handle_name);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
}
/*****************************************************************************/
/** New routine. Wei Peng, 04/22/94 *******************************/
/** Modified by S. Kent to be consistent with the schemaGet and
*** schemaGetFromType methodology
**/

static char* tclSchemaKindGet_use = "USAGE: "
		"schemaKindGet handle-expression";
static char* tclSchemaKindGet_hlp = "return "
		"UNKNOWN, PRIM, ENUM or STRUCT for the schema "
		"of object pointed to by the handle expression";

static int
tclSchemaKindGet(
	ClientData clientData,
	Tcl_Interp *interp,
	int argc,
	char **argv
	)
{
     
     const SCHEMA* schema;
     HANDLE tmp_handle;
     void *userPtr;

     if(argc != 2) 
     {
          Tcl_SetResult(interp,tclSchemaKindGet_use,TCL_STATIC);
          return(TCL_ERROR);
     }  

     if(shTclHandleExprEval(interp, argv[1], &tmp_handle, &userPtr)!=TCL_OK) 
              return TCL_ERROR;
   schema = shSchemaGet(shNameGetFromType(tmp_handle.type));
   if(schema==NULL) 
   {
       Tcl_SetResult(interp, "UNKNOWN", TCL_STATIC);
       return TCL_OK;
   }
   if(schema->type == ENUM )
   {
      Tcl_SetResult(interp, "ENUM", TCL_STATIC);
      return TCL_OK;
   }
   if(schema->type == PRIM)
   {
      Tcl_SetResult(interp, "PRIM", TCL_STATIC);
      return TCL_OK;
   }
   if(schema->type == STRUCT)
   {
      Tcl_SetResult(interp, "STRUCT", TCL_STATIC);
      return TCL_OK;
   }
    
   return TCL_ERROR;
}



/*****************************************************************************/
/** New routine. Wei Peng, 04/22/94 *******************************/
/** Modified by S. Kent to be consistent with the schemaGet and
*** schemaGetFromType methodology
**/

static char* tclSchemaKindGetFromType_use = "USAGE: "
		"schemaKindGetFromType <schema-type>";
static char* tclSchemaKindGetFromType_hlp = "return "
		"UNKNOWN, PRIM, ENUM or STRUCT for the schema "
		"given by the type";

static int
tclSchemaKindGetFromType(
	ClientData clientData,
	Tcl_Interp *interp,
	int argc,
	char **argv
	)
{
     
     const SCHEMA* schema;

     if(argc != 2) 
     {
          Tcl_SetResult(interp,tclSchemaKindGetFromType_use,TCL_STATIC);
          return(TCL_ERROR);
     }  

   schema = shSchemaGet(argv[1]);
    
   if(schema==NULL) 
   {
       Tcl_SetResult(interp, "UNKNOWN", TCL_STATIC);
       return TCL_OK;
   }
   if(schema->type == ENUM )
   {
      Tcl_SetResult(interp, "ENUM", TCL_STATIC);
      return TCL_OK;
   }
   if(schema->type == PRIM)
   {
      Tcl_SetResult(interp, "PRIM", TCL_STATIC);
      return TCL_OK;
   }
   if(schema->type == STRUCT)
   {
      Tcl_SetResult(interp, "STRUCT", TCL_STATIC);
      return TCL_OK;
   }
    
   return TCL_ERROR;
}


/*****************************************************************************/

static char *tclSchemaGetFromType_use =
  "USAGE: schemaGetFromType type-name";
static char *tclSchemaGetFromType_hlp =
  "return a list containing the schema of a datatype given its name";

static int
tclSchemaGetFromType(
		     ClientData clientData,
		     Tcl_Interp *interp,
		     int argc,
		     char **argv
		     )
{
   char buff[BSIZE];
   const SCHEMA *sch;
   
   if (argc != 2) {
      Tcl_SetResult(interp,tclSchemaGetFromType_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   if((sch = (SCHEMA *)shSchemaGet(get_type(argv[1]))) == NULL) {
      sprintf(buff,"Can't retrieve schema for %s",argv[1]);
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
      return(TCL_ERROR);
   }
   schemaPrintToInterp(interp,sch);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclSchemaGet_use =
  "USAGE: schemaGet handle";
static char *tclSchemaGet_hlp =
  "return a list containing the schema of a datatype given a handle to it";

static int
tclSchemaGet(
	     ClientData clientData,
	     Tcl_Interp *interp,
	     int argc,
	     char **argv
	     )
{
   char buff[BSIZE];
   const SCHEMA *sch;
   char *type;
   HANDLE userHandle;
   void *userPtr;
   HANDLE *handle = &userHandle;
   
   if (argc != 2) {
      Tcl_SetResult(interp,tclSchemaGet_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/* Handle may be an expression.  Decode here */
   if (shTclHandleExprEval(interp, argv[1], &userHandle, &userPtr) !=
	TCL_OK) {
      Tcl_SetResult(interp,"Can't decode handle expression",TCL_VOLATILE);
      return(TCL_ERROR);
   }
   type = shNameGetFromType (handle->type);
   if((sch = (SCHEMA *)shSchemaGet(type)) == NULL) {
      sprintf(buff,"Can't retrieve schema for %s",type);
      Tcl_SetResult(interp,buff,TCL_VOLATILE);
      return(TCL_ERROR);
   }
   schemaPrintToInterp(interp,sch);
   return(TCL_OK);
}

/*****************************************************************************/

static char *tclSchemaHashPrintStat_use =
  "USAGE: schemaHashPrintStat";
#define tclSchemaHashPrintStat_hlp \
  ""

static int
tclSchemaHashPrintStat(
		       ClientData clientDatag,
		       Tcl_Interp *interp,
		       int ac,
		       char **av
		       )
{
   int flags = FTCL_ARGV_NO_LEFTOVERS;
   ftclArgvInfo opts[] = {
      {NULL, FTCL_ARGV_HELP, NULL, NULL, tclSchemaHashPrintStat_hlp},
      {NULL, FTCL_ARGV_END, NULL, NULL, NULL},
   };
   extern SHHASHTAB *schemaTable;	/* why isn't this in shCSchema.h? */

   shErrStackClear();

   if(ftcl_ParseArgv(interp,&ac,av,opts, flags) != FTCL_ARGV_SUCCESS) { 
      Tcl_AppendResult(interp,
		       ftcl_GetUsage(interp, opts,flags,
				     "schemaHashPrintStat",0,"\nUsage:","\n"),
		       ftcl_GetArgInfo(interp, opts,flags,
				       "schemaHashPrintStat",8), 
		       (char *)NULL);
      return(TCL_ERROR);
   }

   if(shHashPrintStat(schemaTable) != SH_SUCCESS) {
      shTclInterpAppendWithErrStack(interp);
      return(TCL_ERROR);
   }

   return(TCL_OK);
}


/*****************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
shTclDiskioDeclare(Tcl_Interp *interp) 
{
   shTclDeclare(interp,"dumpOpen", 
		(Tcl_CmdProc *)tclDumpOpen, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpOpen_hlp, tclDumpOpen_use);

   shTclDeclare(interp,"dumpReopen", 
		(Tcl_CmdProc *)tclDumpReopen, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpReopen_hlp, tclDumpReopen_use);

   shTclDeclare(interp,"dumpClose", 
		(Tcl_CmdProc *)tclDumpClose, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpClose_hlp, tclDumpClose_use);

   shTclDeclare(interp,"dumpPtrsResolve", 
		(Tcl_CmdProc *)tclDumpPtrsResolve, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpPtrsResolve_hlp, tclDumpPtrsResolve_use);

   shTclDeclare(interp,"dumpDateGet", 
		(Tcl_CmdProc *)tclDumpDateGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpDateGet_hlp, tclDumpDateGet_use);

   shTclDeclare(interp,"dumpDateDel", 
		(Tcl_CmdProc *)tclDumpDateDel, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpDateDel_hlp, tclDumpDateDel_use);

   shTclDeclare(interp,"dumpTypeGet", 
		(Tcl_CmdProc *)tclDumpTypeGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpTypeGet_hlp, tclDumpTypeGet_use);

   shTclDeclare(interp,"dumpHandleWrite", 
		(Tcl_CmdProc *)tclDumpHandleWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpHandleWrite_hlp, tclDumpHandleWrite_use);

   shTclDeclare(interp,"dumpValueWrite", 
		(Tcl_CmdProc *)tclDumpValueWrite, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpValueWrite_hlp, tclDumpValueWrite_use);

   shTclDeclare(interp,"dumpHandleRead", 
		(Tcl_CmdProc *)tclDumpHandleRead, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclDumpHandleRead_hlp, tclDumpHandleRead_use);

   shTclDeclare(interp,"handleNewFromType", 
		(Tcl_CmdProc *)tclHandleNewFromType, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclHandleNewFromType_hlp, tclHandleNewFromType_use);

   shTclDeclare(interp,"handleDelFromType", 
		(Tcl_CmdProc *)tclHandleDelFromType, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclHandleDelFromType_hlp, tclHandleDelFromType_use);

   shTclDeclare(interp,"schemaGetFromType", 
		(Tcl_CmdProc *)tclSchemaGetFromType, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSchemaGetFromType_hlp, tclSchemaGetFromType_use);

   /* WP, 04/22/94 */
   /* SMK  05/01/94 */
  shTclDeclare(interp,"schemaKindGet", 
		(Tcl_CmdProc *)tclSchemaKindGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSchemaKindGet_hlp, tclSchemaKindGet_use);

  shTclDeclare(interp,"schemaKindGetFromType",
		(Tcl_CmdProc *)tclSchemaKindGetFromType, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSchemaKindGetFromType_hlp,
			tclSchemaKindGetFromType_use);

  /* end of WP addition */

   shTclDeclare(interp,"schemaGet", 
		(Tcl_CmdProc *)tclSchemaGet, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSchemaGet_hlp,tclSchemaGet_use);

   shTclDeclare(interp,"schemaHashPrintStat",
		(Tcl_CmdProc *)tclSchemaHashPrintStat, 
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSchemaHashPrintStat_hlp,
		tclSchemaHashPrintStat_use);
} 

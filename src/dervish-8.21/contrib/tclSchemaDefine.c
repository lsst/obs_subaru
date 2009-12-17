/*
 * TCL support to load schema dynamically
 * to list types, and to return the full schema definition of a type
 *
 * Commands defined in this file:
 *
 *	schemaDefine
 *	typesList
 *	schemaGetFullFromType
 *
 */

#include <stdio.h>
#include <string.h>
#include "dervish.h"
#include "shCSchema.h"

/*
 * Schema Data
 */
extern SCHEMA **p_shSchema;		/* list of all known schema */
extern int p_shNSchema;			/* number of known schema */

static char *module = "contrib";
static char *tclSchemaDefine_use = "Usage: schemaDefine schema schema_elems(<nelems>)";
static char *tclSchemaDefine_hlp = "Define a set of schema definitions";
/*****************************************************************************/
static int
tclSchemaDefine(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )

{
   int nschema;
   int ischema;
   int i;
   int splitArgc;
   char **splitArgv;
   char *schName;
   SCHEMATYPE schType;
   int schSize;
   int schNelems;
   char *schElemId;
/*   char *schElemName; */
   char *schElemType;
   int schElemNstar;
   char *schElemDim;
   int schElemOffset;
   SCHEMA *schema;
   SCHEMA_ELEM *schemaElems;
   char buffer[133];
   char *arrayElem;
   RET_CODE result;

   if(argc != 3) {
      Tcl_SetResult(interp,tclSchemaDefine_use,TCL_STATIC);
      return(TCL_ERROR);
   }

/* Input is 2 arrays, 1st one with the names of schema types and the second
 * with their definitions.  Some modicum of error checking is done to make
 * sure that all is well, but I won't guarantee that erroneous input won't
 * core dump.
 * Indexing has the following pattern:
 *
 *   shema(1)   schemaElem(1)(1)
 *              schemaElem(1)(2)
 *   shema(2)   schemaElem(2)(1)
 *              schemaElem(2)(2)
 *              schemaElem(2)(3)
 * Contents are as follows:
 *	schema(i)	<typeName> [PRIM | STRUCT | ENUM] <size> <nelem>
 *	schemaElem(i)(j) <elemName> <elemTypeName> <nstar> ["<dim1 dim2 ...>" |
		         NULL] offset
*/

/* Get the number of elements in the 1st array. Use Tcl_Eval. */
   sprintf (buffer,"%s %s\n","array size",argv[1]);
/*   if (Tcl_VarEval (interp, "array size ", argv[1], NULL) != TCL_OK) */
   if (Tcl_Eval (interp, buffer) != TCL_OK)
	return TCL_ERROR;
   if (sscanf (interp->result, "%d",&nschema) == 0) {
	Tcl_SetResult (interp, "Bad return from sscanf",TCL_STATIC);
	return TCL_ERROR;
	}

/* Malloc storage */
   schema = (SCHEMA *)malloc((nschema+1)*sizeof(SCHEMA));

/* Main Loop */
for (ischema = 0; ischema < nschema; ischema++) {
   sprintf (buffer,"%s(%d)",argv[1],ischema+1);
/* Translate the 1st argument and split into a list. */
   if ((arrayElem=Tcl_GetVar(interp, buffer, TCL_LEAVE_ERR_MSG)) == NULL) {
	return (TCL_ERROR);
	}
   Tcl_SplitList (interp, arrayElem, &splitArgc, &splitArgv);
   if (splitArgc != 4) {
        Tcl_SetResult(interp, "Wrong number of elements in schema list",
                TCL_STATIC);
        return (TCL_ERROR);
        }
   schName = splitArgv[0];
/* No way to feed enum types through TCL, so decode here */
   if (strcmp(splitArgv[1],"STRUCT") == 0) {schType = STRUCT;}
   else if (strcmp(splitArgv[1],"PRIM") == 0) {schType = PRIM;}
   else if (strcmp(splitArgv[1],"ENUM") == 0) {schType = ENUM;}
   else {schType = UNKNOWN;}
   schSize = atoi(splitArgv[2]);
   schNelems = atoi(splitArgv[3]);
/* Fill in schema structure */
   strcpy(schema[ischema].name,schName);
   schema[ischema].type = schType;
   schema[ischema].size = schSize;
   schema[ischema].nelem = schNelems;
   schema[ischema].construct = NULL;
   schema[ischema].destruct = NULL;
   schema[ischema].read = NULL;
   schema[ischema].write = NULL;
   schema[ischema].lock = SCHEMA_NOLOCK;
/* Free the buffer */
   free(splitArgv);
   if (schNelems < 0) {
        Tcl_SetResult(interp, "Number of schema elements must be >= 0.",
                TCL_STATIC);
        return (TCL_ERROR);
        }
   if (schNelems >0) {
	schemaElems = (SCHEMA_ELEM *)malloc(schNelems*sizeof(SCHEMA_ELEM));
	} else {
	schemaElems = NULL;
	}
/* Fill in rest of schema structure */
   schema[ischema].elems = schemaElems;
/* Fill in the schema elements */
   for (i=0; i<schNelems; i++) {
        sprintf (buffer,"%s(%d)(%d)",argv[2],ischema+1,i+1);
	if ((arrayElem=Tcl_GetVar(interp, buffer, TCL_LEAVE_ERR_MSG)) == NULL) {
                free(schema);
                free(schemaElems);
                return (TCL_ERROR);
                }

/* Split the array element argument into a list. */
        Tcl_SplitList (interp, arrayElem, &splitArgc, &splitArgv);
        if (splitArgc != 5) {
           Tcl_SetResult(interp, "Wrong number of elements in schema list",
                TCL_STATIC);
	   if (splitArgv != (char **)0) free(splitArgv);
           return (TCL_ERROR);
           }
        schElemId = splitArgv[0];
        schElemType = splitArgv[1];
        schElemNstar = atoi(splitArgv[2]);
	if (strcmp(splitArgv[3],"NULL") != 0) {
/* Must malloc storage for the dimension character string */
	   schElemDim = (char *)malloc(strlen(splitArgv[3])+1);
           strcpy (schElemDim, splitArgv[3]);
	} else {
	   schElemDim = NULL;
	   }
        schElemOffset = atoi(splitArgv[4]);
        strcpy (schemaElems[i].name, schElemId);
        strcpy (schemaElems[i].type, schElemType);
#if SCHEMA_ELEMS_HAS_TTYPE
	schemaElems[i].ttype = UNKNOWN_SCHEMA;
#endif
/* For enums, the nstar parameter is useless.  We can usurp it to provide
 * the numerical value instead
*/
        if (schType == ENUM) {
	   schemaElems[i].value = schElemNstar;
	   schemaElems[i].nstar = 0;
	} else {
	   schemaElems[i].value = 0;
	   schemaElems[i].nstar = schElemNstar;
	   }
	schemaElems[i].size = 0;	/* Will be filled in later */
	schemaElems[i].nelem = schElemDim;
	schemaElems[i].i_nelem = 0;	/* Will be filled in later */
	schemaElems[i].offset = schElemOffset;
	free(splitArgv);
	}
   }
/* Set the final schema class to UNKNOWN */
   *schema[nschema].name = '\0';
   schema[nschema].type = UNKNOWN;
   schema[nschema].size = 0;
   schema[nschema].nelem = 0;
   schema[nschema].elems = 0;;
   schema[nschema].construct = NULL;
   schema[nschema].destruct = NULL;
   schema[nschema].read = NULL;
   schema[nschema].write = NULL;
   schema[nschema].lock = SCHEMA_NOLOCK;
   Tcl_ResetResult(interp);
   result = p_shSchemaLoad(schema);
   if (result != SH_SUCCESS) {
     if (result == SH_SCHEMA_LOCKED) {
       for (ischema = 0; ischema < nschema; ischema++) {
	 if ( (schema[ischema].lock & SCHEMA_LOCKED_OUT) != 0 ) {
	   shErrStackPush("%s is locked!  Please use another name.",
			  schema[ischema].name);
	 }
	 shTclInterpAppendWithErrStack(interp); 
	 return (TCL_ERROR);
       }
     };
     printf ("Error in loading schema\n");
     return (TCL_ERROR);
   }
   return (TCL_OK);
}

/*****************************************************************************/
static char *tclTypesList_use = "Usage: ypesList";
static char *tclTypesList_hlp = "Return a list of all types with schemas";

static int
tclTypesList(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{
/* SCHEMA **schema; */
/* int n; */
int i;


/*   if (p_shSchemaPointers(&schema, &n) != SH_SUCCESS) return TCL_ERROR; */

   for (i = 0; i < p_shNSchema; i++) {
	Tcl_AppendElement (interp, p_shSchema[i]->name);
	}
   return TCL_OK;
}

/****************************************************************************/
static char *tclSchemaGetFullFromType_use =
   "Usage: schemaGetFullFromType <type>";
static char *tclSchemaGetFullFromType_hlp =
 "Return the complete definition of a schema in the form used by schemaDefine.";

static int
tclSchemaGetFullFromType(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )
{

const SCHEMA *schema;
int i;
char buffer[80];	/* Much bigger than needed */

   if (argc != 2) {
	Tcl_SetResult (interp, tclSchemaGetFullFromType_use, TCL_STATIC);
	return TCL_ERROR;
	}

   if ((schema = shSchemaGet (argv[1])) == NULL) {
	Tcl_AppendResult (interp, "Type ",argv[1], " is unknown", (char *)NULL);
	return TCL_ERROR;
	}

/* Return the result as a list of lists.  The first list element is a line
 * that describes the bulk characteristics of each schema.  Each subsequent
 * element is a line describing each element in the schema
 */
/* First, schema(i) */
   Tcl_ResetResult (interp);
   Tcl_AppendResult (interp, "{", NULL);
   Tcl_AppendResult (interp, (char *)(schema->name), NULL);
   if (schema->type == PRIM) Tcl_AppendElement (interp, "PRIM");
   else if (schema->type == ENUM) Tcl_AppendElement (interp, "ENUM");
   else if (schema->type == STRUCT) Tcl_AppendElement (interp, "STRUCT");
   else {
	Tcl_SetResult (interp, "Unknown type of schema", TCL_STATIC);
	return TCL_ERROR;
	}
   sprintf (buffer, "%d", schema->size);
   Tcl_AppendElement (interp, buffer);
   sprintf (buffer, "%d", schema->nelem);
   Tcl_AppendElement (interp, buffer);
   Tcl_AppendResult (interp, "}", NULL);

/* Now loop through all the elements */
   for (i=0; i<schema->nelem; i++) {
	Tcl_AppendResult (interp, " {", NULL);
	Tcl_AppendResult (interp, schema->elems[i].name, NULL);
	Tcl_AppendElement (interp, schema->elems[i].type);
	if (schema->type == ENUM) {
	   sprintf (buffer, "%d", schema->elems[i].value);
	} else {
	   sprintf (buffer, "%d", schema->elems[i].nstar);
	   }
	Tcl_AppendElement (interp, buffer);
	if (schema->elems[i].nelem == NULL) {
	   Tcl_AppendElement (interp, "NULL");
	   }
	else {
	   Tcl_AppendElement (interp, schema->elems[i].nelem);
	   }
	sprintf (buffer, "%d", schema->elems[i].offset);
	Tcl_AppendElement (interp, buffer);
	Tcl_AppendResult (interp, "}", NULL);
	}
   return TCL_OK;
}

/*****************************************************************************/

/*
 * Declare my new tcl verbs to tcl
 */
void
shTclSchemaDefineDeclare (Tcl_Interp *interp) 
{
   shTclDeclare(interp,"schemaDefine", 
		(Tcl_CmdProc *)tclSchemaDefine,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSchemaDefine_hlp, tclSchemaDefine_use);

   shTclDeclare(interp,"typesList", 
		(Tcl_CmdProc *)tclTypesList,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclTypesList_hlp, tclTypesList_use);

   shTclDeclare(interp,"schemaGetFullFromType", 
		(Tcl_CmdProc *)tclSchemaGetFullFromType,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclSchemaGetFullFromType_hlp,
			tclSchemaGetFullFromType_use);
   return;
}


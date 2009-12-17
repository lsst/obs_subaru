/*****************************************************************************
******************************************************************************
** <AUTO>
   FILE:
  	tclProgram.c
   <HTML> 
   ABSTRACT:
	The follow routines are used to compile little programs to
	dereference pointers.  All the work of parsing schemas is done
	at the TCL level; programs are compiled into TCL arrays.
	These arrays are passed down here to be converted to C code.
</HTML>
</AUTO> */

/************************************************************************
**
**	This file contains many routines to create programs to evaluate
**	handle expressions at runtime.
**
** ENTRY POINT		SCOPE	DESCRIPTION
** -------------------------------------------------------------------------
** shProgramDel		Public	Delete PROGRAM data structure (lower level)
** tclProgramDel	Local	Delete PROGRAM data structure
** shProgramCreateBySchema	Public	C callable routine to create a program.
** shProgramCreate	Public	C callable routine to create a program.
** shProgramEval	Public	Run a compiled program
** -------------------------------------------------------------------------
**
**  
** ENVIRONMENT:
**	ANSI C.
**
** REQUIRED PRODUCTS:
**	Dervish
**
** AUTHORS:
**	Creation date:  Feb 1996
**	Steve Kent
**
******************************************************************************
******************************************************************************
*/

#include <math.h>
#include <string.h>

#include "dervish.h"
#include "shGeneric.h"
#include "shCProgram.h"

/*============================================================================  
**============================================================================
**
** LOCAL MACROS, DEFINITIONS, ETC.
**
**============================================================================
*/

static char *module="programCompile";

RET_CODE shSchemaLoadFromContrib(void);

/*****************************************************************************/
/***			shProgramDel					   ***/
/*****************************************************************************/
/* A minifunction (but public) to delete a program */

void shProgramDel (PROGRAM *program) {
   if (program != NULL) {
	if (program->prog != NULL) shFree (program->prog);
	if (program->dim != NULL) shFree (program->dim);
	if (program->sdim != NULL) shFree (program->sdim);
	shFree(program);
	}
   return;
   }

/*****************************************************************************/
/***			tclProgramDel					   ***/
/*****************************************************************************/
static char *tclProgramDel_use = "Usage: programDel progs offsets";
static char *tclProgramDel_hlp = "Delete program structure.";

static int
tclProgramDel(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )

{
   PROGRAM *program = NULL;

   if(argc != 2) {
      Tcl_SetResult(interp,tclProgramDel_use,TCL_STATIC);
      return(TCL_ERROR);
      }
/* get and check PROGRAM */
   if (shTclAddrGetFromName(interp, argv[1], (void **) &program, "PROGRAM")
      != TCL_OK) {
	Tcl_SetResult (interp, "programDel: bad name", TCL_VOLATILE);
	return TCL_ERROR;
	}

   shProgramDel (program);
   p_shTclHandleDel(interp, argv[1]);

   Tcl_SetResult(interp,"",TCL_STATIC);
   return(TCL_OK);
   }



/*****************************************************************************/
/***			shProgramCreateBySchema				   ***/
/*****************************************************************************/
/* This is a C callable public routine to create a program to dereference
 * handle expresssions.  Replace TCL proc with C code.  Input is a top level
 * SCHEMA and an attribute expression.  Supported syntax includes . -> and <>
 * (or []).  Thus, an attribute could be  a.b<17>->c   The attribute must
 * evaluate to a single item or an array of items (inline and/or out of line
 * OK) of PRIM or ENUM type or an error is returned. The returned program
 * is intended to be used to modify a pointer that initially points to
 * a top level struct to end up pointing to the desired data item.  The
 * operations supported are dereference and add an offset.
*/

PROGRAM *shProgramCreateBySchema(
          const SCHEMA  *inschema,	/* IN: Schema of data type */
          char *attr)			/* IN: Attribute */
{
   PROGRAM *program;
   SCHEMA *schema = (SCHEMA *)inschema;	/* Take care of casting woes */
   SCHEMA_ELEM *schemaElem = NULL;
   CHAIN *chain = NULL;
   PROGSTEP *step;

/* Decoded schema elem stuff */
   int nstar = 0;
   int ndim = 0;
   int dim[30];		/* Should be big enough for all arrays in SDSS*/
   int offset = 0;
   int elemSize = 0;
   int index;

   char buf[300];	/* Should be big enough to hold any elem name */
   char *ptr;
   char *last;
   char *test;
   int i;
   int incr;
   char *dptr;
   char *dlast;
   int nitem;

/********************** Begin Code *****************************************/

/* Break down attr into a sequence of tokens */
/* Store program steps on a chain temporarily, then copy to a more permanent
 * home at the end.
*/

   chain = shChainNew("PROGSTEP");
   ptr = attr-1;
   last = attr + strlen(attr)-1;

/* Loop through tokenization */
   while (1) {
	ptr++;
	if (ptr > last) break;

/* Is it a DOT ? */
	if (*ptr == '.') {
	   if (schema != NULL || schemaElem == NULL) goto error;
	   if (nstar != 0 || ndim != 0) goto error;
	   if ((schema = (SCHEMA *)shSchemaGet(schemaElem->type)) == NULL)
		goto error;
	   schemaElem = NULL;
	   continue;
	   }
/* Is it a -> ? */
	if (*ptr == '-') {
	   if (ptr == last) goto error;
	   ptr++;
	   if (*ptr != '>') goto error;
	   if (schema != NULL || schemaElem == NULL) goto error;
	   if (nstar != 1 || ndim != 0) goto error;
	   if ((schema = (SCHEMA *)shSchemaGet(schemaElem->type)) == NULL)
		goto error;
	   if (schema->type != STRUCT) goto error;
	   schemaElem = NULL;
	   step = shGenericNew("PROGSTEP");
	   step->prog = PROG_DEREF;
	   step->arg = 0;
	   shChainElementAddByPos (chain, step, "PROGSTEP", TAIL, AFTER);
	   continue;	   
	   }
/* Is it an array index ? */
	if (*ptr == '<' || *ptr == '[') {
	   if (schema != NULL || schemaElem == NULL) goto error;
	   if (schemaElem->nstar == 0 && schemaElem->nelem == NULL) goto error;
	   i=0; buf[0]='\0';
	   ptr++;
	   while (*ptr != '>' && *ptr != ']') {
		if (ptr > last) goto error;
		buf[i++] = *ptr;
		buf[i] = '\0';
		ptr++;
		}
/* Convert to integer; check for error */
	   if (i == 0) goto error;	/* No reference of the form <> or [] */
	   index = strtol(buf, &test, 10);
	   if (buf+strlen(buf) - test != 0) goto error;
	   if (ndim > 0) {
		if (index < 0 || index >= dim[0]) goto error;
		incr = elemSize;
		for (i=1; i<ndim; i++) {
		   incr *= dim[i];
		   }
		step = shGenericNew("PROGSTEP");
		step->prog = PROG_ADD;
		step->arg = incr * index;
		shChainElementAddByPos (chain, step, "PROGSTEP", TAIL, AFTER);
/* Adjust dimension info */
		ndim--;
		for (i=0; i<ndim; i++) {
		   dim[i] = dim[i+1];
		   }
		continue;
		}
	   if (nstar == 1) {
		step = shGenericNew("PROGSTEP");
		step->prog = PROG_DEREF;
		step->arg = elemSize * index;
		shChainElementAddByPos (chain, step, "PROGSTEP", TAIL, AFTER);
		nstar--;
		continue;
		}
	   step = (PROGSTEP *)shGenericNew("PROGSTEP");
	   step->prog = PROG_ADD;
	   step->arg = sizeof(void *) * index;
	   shChainElementAddByPos (chain, step, "PROGSTEP", TAIL, AFTER);
	   nstar--;
	   continue;
	   }
/* Assume that next token is name of an element in a struct */
	if (schema == NULL || schemaElem != NULL) goto error;
	i=0; buf[0]='\0';
	while (*ptr != '.' && *ptr != '<' && *ptr != '-' && *ptr != '[') {
	   if (ptr > last) break;
	   buf[i++] = *ptr;
	   buf[i]='\0';
	   ptr ++;
	   }
	ptr--;
/* I would use shSchemaElemGet here except that it assumes that schema has
 * been compiled in, and in shTransContrib, my top level schema is a
 * temporary artifact.  So search eplicitly by hand
*/

	schemaElem = NULL;
	for (i=0; i<schema->nelem; i++) {
	   if (strcmp(buf, schema->elems[i].name) == 0) {
		schemaElem = &schema->elems[i];
		break;
		}
	   }
	if (schemaElem 	== NULL) goto error;
/* Decode various fields */
	nstar = schemaElem->nstar;
/* Dimension info - a real bear */
	ndim = 0;
	if (schemaElem->nelem != NULL) {
	   dptr = schemaElem->nelem - 1;
	   dlast = schemaElem->nelem + strlen(schemaElem->nelem) - 1;
	   while (1) {
		dptr++;
		if (dptr > dlast) break;
		if (*dptr == ' ') continue;
		i=0; buf[0]='\0';
		while (*dptr != ' ') {
	   	   if (dptr > dlast) break;
		   buf[i++] = *dptr;
		   buf[i] = '\0';
		   dptr ++;
		   }
/* Convert to integer; check for error */
		if (i == 0) goto error;
		index = strtol(buf, &test, 10);
		if (buf+strlen(buf) - test != 0) goto error;
		dim[ndim] = index;
		ndim++;
		}
	   }		   
	offset = schemaElem->offset;
	step = shGenericNew("PROGSTEP");
	step->prog = PROG_ADD;
	step->arg = offset;
	shChainElementAddByPos (chain, step, "PROGSTEP", TAIL, AFTER);
/* Need to get element size */
	if ((schema = (SCHEMA *)shSchemaGet (schemaElem->type)) == NULL) \
		goto error;
	elemSize = schema->size;
	schema = NULL;
	}

/* Did we end up with an appropriate attribute?  Should have schema = 0,
 * schemaElem != 0 unless attribute was empty.
 * If attribute was empty, we have 0 length program.  If top level type is
 * a primitive, this is OK and makes sense in certain situations.
*/
   if (shChainSize(chain) > 0) {
	if (schema != NULL) goto error;
/* Refetch schema of ending datatype */
	if ((schema = (SCHEMA *)shSchemaGet (schemaElem->type)) == NULL) \
		goto error;
	}
   if (schema->type == PRIM) {
      ;					/* OK */
   } else if(schema->type == ENUM) {
      ;					/* also OK, as they look like ints */
   } else {
      goto error;
   }
/* Need to get element size */
   elemSize = schema->size;
	
/* Now create the output program */
   program = (PROGRAM *)shGenericNew("PROGRAM");

/* Next, the PROGSTEPs themselves. */
   program->nstep = shChainSize(chain);
   program->prog = (PROGSTEP *)shMalloc(program->nstep * sizeof(PROGSTEP));
   for (i=0; i<shChainSize(chain); i++) {
	step = shChainElementGetByPos (chain, i);
	program->prog[i] = *step;
	}

/* Useful output info */
   program->type = shTypeGetFromName(schema->name);
   program->nstar = nstar;
   program->ndim = ndim;
   nitem = 1;
   if (ndim > 0) {
	program->dim = (int *)shMalloc(ndim*sizeof(int));
	for (i=0; i<ndim; i++) {
	   program->dim[i] = dim[i];
	   nitem *= dim[i];
	   }
	}
   if (nstar > 0) {
	program->sdim = (int *)shMalloc(nstar*sizeof(int));
	for (i=0; i<nstar; i++) {
	   program->sdim[i] = 0;
	   }
	}
   program->nitem = nitem;
   if (nstar == 0) program->isize = elemSize;
	else program->isize = sizeof(void *);
   program->nout = 0;		/* Number of out-of-line items */
   program->size = elemSize;
   shGenericChainDel(chain);   
   return program;

error:
/*   printf ("Error compiling %s at %s\n", attr, ptr); */
   shErrStackPush ("Error compiling %s at %s\n", attr, ptr);
   shErrStackPush ("Error compiling %s at %s\n", attr, ptr);
   shGenericChainDel(chain);
   return NULL;
}

/**************************************************************************/
/*****			shProgramCreate					***/
/**************************************************************************/

PROGRAM *shProgramCreate(
          char *type, 		/* IN: Schema of data type */
          char *attr			/* IN: Attribute */
          )
{
   SCHEMA *schema = (SCHEMA *)shSchemaGet(type);
   return shProgramCreateBySchema (schema, attr);
}

/*****************************************************************************/
/***			tclProgramCreate				   ***/
/*****************************************************************************/
static char *tclProgramCreate_use = "Usage: programCreate type attribute";
static char *tclProgramCreate_hlp = "Create a schema evaluation prgram";

static int
tclProgramCreate(
          ClientData clientData,
          Tcl_Interp *interp,
          int argc,
          char **argv
          )

{
   char objName[HANDLE_NAMELEN];
   PROGRAM *program = NULL;

   if(argc != 3) {
      Tcl_SetResult(interp,tclProgramCreate_use,TCL_STATIC);
      return(TCL_ERROR);
   }

   program = shProgramCreate (argv[1], argv[2]);
   if (program == NULL) return TCL_ERROR;

/* Assign to a handle */
   if (shTclHandleNew (interp, objName, "PROGRAM", program) != TCL_OK)
                return(TCL_ERROR);
   Tcl_SetResult(interp,objName,TCL_VOLATILE);
   return(TCL_OK);

}

/*****************************************************************************/
/***			shProgramEval					   ***/
/*****************************************************************************/
/* Evaluate a program on an input pointer.  The evaluated pointer is returned
 * as the result.
*/
void *shProgramEval (
	void *top,		/*IN: Pointer to the top of a data structure */
	PROGRAM *program	/*IN: Pointer to a program */
	)
{
   int i;
   char *field = (char *)top;
   if (program == NULL) return NULL;
   if (field == NULL) return NULL;

   for (i=0; i<program->nstep; i++) {
	if (program->prog[i].prog == PROG_ADD) {
	   field += program->prog[i].arg;
	   continue;
	   }
	if (program->prog[i].prog == PROG_DEREF) {
	   field = *(char **)field;
	   if (field == NULL) return NULL;
	   field += program->prog[i].arg;
	   continue;
	   }
	}
   return (void *)field;
   }

/**************************************************************************/
/*
 * Declare my new tcl verbs to tcl
 */
void
shTclProgramCreateDeclare (Tcl_Interp *interp) 
{
   shTclDeclare(interp,"programCreate", 
		(Tcl_CmdProc *)tclProgramCreate,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProgramCreate_hlp, tclProgramCreate_use);

   shTclDeclare(interp,"programDel", 
		(Tcl_CmdProc *)tclProgramDel,
		(ClientData) 0,
		(Tcl_CmdDeleteProc *)NULL, 
		module, tclProgramDel_hlp, tclProgramDel_use);


/* Load schema definitions */
   shSchemaLoadFromContrib();
   return;
}


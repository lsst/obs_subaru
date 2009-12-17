/*
 * The following files are contributions made by Steve to Dervish Chains. They
 * have been moved from his ts package and put in $DERVISH_DIR/contrib. They
 * have also been dervish'ized in that the function names have been changed to
 * reflect the Dervish conventions. However, the one major drawback is that
 * not all TCL extensions have the appropriate C API. 
 *
 * Files are : $DERVISH_DIR/contrib/shChainContrib.c
 *             $DERVISH_DIR/contrib/tclChainContrib.c
 *             $DERVISH_DIR/contrib/shChainUtils.c
 *             $DERVISH_DIR/contrib/shChainContrib.h
 *
 * - Vijay K. Gurbani
 *   Feb. 08 1995
 */


/******* Support utilities shared by chainSearch, chainSort, chainSelect,
 ******* chainSet, and chainHandleSet.  Note: These utilities are declared
 ******* as static functions and thus are used only by inclusion in other
 ******* source files.  If someone wants to make them normal functions, be
 ******* sure to rename them to ensure uniqueness of names - I have not
 ******* been careful to prevent namespace pollution.  Many functions
 ******* have identically named but different implementation routines
 ******* in conversant !!!
*******/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "dervish.h"
#include "shChainContrib.h"
#include "shCProgram.h"

/* All flavors of comparison functions */

/* p_shBufCompare: the return code is -1, 0, 1 based on the comparison of
 * dbuff vs. qbuff.  These are compared with the desired code in opcode.
 * Finally if NOT is 1, we take the opposite result of the comparison.
*/
int p_shBufCompare (BUFDESC dbuff, BUFDESC qbuff, PREDTERM *predterm,
   int i) {
   int retcode;
   retcode = (predterm->compare)(&dbuff, &qbuff);
   if (retcode == predterm->opcode[i]) retcode = 1;
   else retcode = 0;
   if (predterm->not[i] == 1) retcode = 1 - retcode;
   return retcode;
   }   

int p_shPredCompare (BUFDESC dbuff, PREDTERM *predterm) {
   BUFDESC qbuff;
   int i;
   int retcode[2];

   for (i=0; i<=predterm->range; i++) {
	qbuff = predterm->key[i];
	retcode[i] = p_shBufCompare(dbuff, qbuff, predterm, i);
	}
   if (predterm->range == 0) return retcode[0];
/* For a range, we either return the union or the intersection */
   if (predterm->combine == 0) return (retcode[0] && retcode[1]);
   return (retcode[0] || retcode[1]);
   }

/* For enums, convert to integers.  The only comparison that makes sense
 * is equality.
*/

int p_shEnumCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(int *)(dbuff->data) < *(int *)(qbuff->data)) retcode =  -1;
   else if (*(int *)(dbuff->data) > *(int *)(qbuff->data))
	retcode =  1;
   else retcode =  0;
   return retcode;
   }

int p_shStrCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   char *null = "";
   char *addr1, *addr2;

   addr1 = *(char **)(dbuff->data);
   addr2 = *(char **)(qbuff->data);
/* Strings can have NULL pointers, so need to catch that fact */

   if (addr1 == NULL) addr1 = null;
   if (addr2 == NULL) addr2 = null;
   retcode = strcmp (addr1, addr2);
   return retcode;
   }

int p_shCharCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(char *)(dbuff->data) < *(char *)(qbuff->data))
	retcode =  -1;
   else if (*(char *)(dbuff->data) > *(char *)(qbuff->data))
	retcode =  1;
   else retcode =  0;
   return retcode;
   }

int p_shUcharCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(unsigned char *)(dbuff->data) < *(unsigned char *)(qbuff->data))
	retcode =  -1;
   else if (*(unsigned char *)(dbuff->data) > *(unsigned char *)(qbuff->data))
	retcode =  1;
   return retcode;
   }

int p_shShortCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(short *)(dbuff->data) < *(short *)(qbuff->data))
	retcode =  -1;
   else if (*(short *)(dbuff->data) > *(short *)(qbuff->data))
	retcode =  1;
   else retcode =  0;
   return retcode;
   }

int p_shUshortCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(unsigned short *)(dbuff->data) <
	*(unsigned short *)(qbuff->data)) retcode =  -1;
   else if (*(unsigned short *)(dbuff->data) >
	*(unsigned short *)(qbuff->data)) retcode =  1;
   else retcode =  0;
   return retcode;
   }

int p_shIntCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;

   if (*(int *)(dbuff->data) < *(int *)(qbuff->data)) retcode =  -1;
   else if (*(int *)(dbuff->data) > *(int *)(qbuff->data))
	retcode =  1;
   else retcode =  0;
   return retcode;
   }

int p_shIntBitCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode=0;

   if (*(int *)(dbuff->data) & *(int *)(qbuff->data)) retcode =  1;
   return retcode;
   }

int p_shUintCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(unsigned int *)(dbuff->data) <
	*(unsigned int *)(qbuff->data)) retcode =  -1;
   else if (*(unsigned int *)(dbuff->data) >
	*(unsigned int *)(qbuff->data)) retcode =  1;
   else retcode =  0;
   return retcode;
   }

int p_shLongCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(long *)(dbuff->data) < *(long *)(qbuff->data)) retcode =  -1;
   else if (*(long *)(dbuff->data) > *(long *)(qbuff->data))
	retcode =  1;
   else retcode =  0;
   return retcode;
   }

int p_shUlongCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(unsigned long *)(dbuff->data) <
	*(unsigned long *)(qbuff->data)) retcode =  -1;
   else if (*(unsigned long *)(dbuff->data) >
	*(unsigned long *)(qbuff->data)) retcode =  1;
   else retcode =  0;
   return retcode;
   }

int p_shFltCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(float *)(dbuff->data) < *(float *)(qbuff->data)) retcode =  -1;
   else if (*(float *)(dbuff->data) > *(float *)(qbuff->data))
		retcode =  1;
   else retcode =  0;
   return retcode;
   }

int p_shDblCompare (BUFDESC *dbuff, BUFDESC *qbuff) {
   int retcode;
   if (*(double *)(dbuff->data) < *(double *)(qbuff->data))
		retcode =  -1;
   else if (*(double *)(dbuff->data) > *(double *)(qbuff->data))
	retcode =  1;
   else retcode =  0;
   return retcode;
   }

/* Extract and set numerical types as doubles.  Chars are ignored at
 * present */

double p_shShortExtract (void *dbuff) {
   return (double)(*(short *)(dbuff));
   }

void p_shShortSet (void *dbuff, double val) {
   (*(short *)(dbuff)) = val;
   }

double p_shUshortExtract (void *dbuff) {
   return (double)(*(unsigned short *)(dbuff));
   }

void p_shUshortSet (void *dbuff, double val) {
   (*(unsigned short *)(dbuff)) = val;
   }

double p_shIntExtract (void *dbuff) {
   return (double)(*(int *)(dbuff));
   }

void p_shIntSet (void *dbuff, double val) {
   (*(int *)(dbuff)) = val;
   }

double p_shUintExtract (void *dbuff) {
   return (double)(*(unsigned int *)(dbuff));
   }

void p_shUintSet (void *dbuff, double val) {
   (*(unsigned int *)(dbuff)) = val;
   }

double p_shLongExtract (void *dbuff) {
   return (double)(*(long *)(dbuff));
   }

void p_shLongSet (void *dbuff, double val) {
   (*(long *)(dbuff)) = val;
   }

double p_shUlongExtract (void *dbuff) {
   return (double)(*(unsigned long *)(dbuff));
   }

void p_shUlongSet (void *dbuff, double val) {
   (*(unsigned long *)(dbuff)) = val;
   }

double p_shFltExtract (void *dbuff) {
   return (double)(*(float *)(dbuff));
   }

void p_shFltSet (void *dbuff, double val) {
   (*(float *)(dbuff)) = val;
   }

double p_shDblExtract (void *dbuff) {
   return (double)(*(double *)(dbuff));
   }

void p_shDblSet (void *dbuff, double val) {
   (*(double *)(dbuff)) = val;
   }

/* Given a parameter, return the type - actually, 2 versions of type. This
 * is redundant, but gets the job done in the most straightforward fashion.
*/

int p_shTclParamGet (Tcl_Interp *interp, const SCHEMA *schema,
	char *attribute, VTYPE *vtype, TYPE *type) {

const SCHEMA *localSchema;
PROGRAM *program;

/* Decode the type.  Do this by compiling a program - a little redundant,
   since we will repeat this later.
 */
   if ((program = shProgramCreateBySchema (schema, attribute)) == NULL) {
	Tcl_AppendResult (interp, "Error decoding attribute ",
		attribute, NULL);
	return TCL_ERROR;
	}
/* Make sure we have an elemental type */
   if (program->nstar != 0 || program->ndim != 0) {
	Tcl_AppendResult (interp, "Attribute is not an elemental type: ",
		attribute, NULL);
	return TCL_ERROR;
	}
   *type = program->type;
   shProgramDel(program);
/* Make sure that this is a primitive type */
   localSchema = shSchemaGet(shNameGetFromType(*type));
   if (localSchema == NULL) {
	Tcl_AppendResult (interp, "Unable to find schema for ",
		shNameGetFromType(*type), NULL);
	return TCL_ERROR;
	}
   if (localSchema->type != PRIM && localSchema->type != ENUM) {
	Tcl_AppendResult (interp, "Schema type is not primitive for ",
	   attribute, NULL);
	return TCL_ERROR;
	}
/* PTR types are rather useless here */
   if (*type == shTypeGetFromName("PTR")) {
	Tcl_AppendResult (interp, "Schema type is pointer (useless) ",
	   attribute, NULL);
	return TCL_ERROR;
	}
/* Convert dervish type to a local enum */
   if (localSchema->type == PRIM) {
	if (*type == shTypeGetFromName("STR")) *vtype = STR;
	else if (*type == shTypeGetFromName("CHAR")) *vtype = CHAR;
	else if (*type == shTypeGetFromName("UCHAR")) *vtype = UCHAR;
	else if (*type == shTypeGetFromName("SHORT")) *vtype = SHORT;
	else if (*type == shTypeGetFromName("USHORT")) *vtype = USHORT;
	else if (*type == shTypeGetFromName("INT")) *vtype = INT;
	else if (*type == shTypeGetFromName("UINT")) *vtype = UINT;
	else if (*type == shTypeGetFromName("LONG")) *vtype = LONG;
	else if (*type == shTypeGetFromName("ULONG")) *vtype = ULONG;
	else if (*type == shTypeGetFromName("FLOAT")) *vtype = FLOAT;
	else if (*type == shTypeGetFromName("DOUBLE")) *vtype = DOUBLE;
	else if (*type == shTypeGetFromName("TYPE")) *vtype = TTYPE;
	else {
	   Tcl_AppendResult (interp, attribute, "is not a searchable type",
		NULL);
	   return TCL_ERROR;
	   }
   } else {
	*vtype = EENUM;
	}
   return TCL_OK;
   }

/* Given characteristics of an attribute, decode an ascii field to match */

int p_shTclKeyData (VTYPE vtype, TYPE type, char *field, BUFDESC *buff) {

/* We could probably replace all of the below code with a single call to
 * shElemSet.  Well, too late now ...
*/

/* Scratch buffers */
   unsigned short scr_ushort;
   short scr_short;
   unsigned int scr_uint;
   int scr_int;
   long scr_long;
   unsigned long scr_ulong;
   float scr_float;
   double scr_double;
   const SCHEMA_ELEM *schemaElem;
/* Enums: Convert input character string to an int */
   if (vtype == EENUM) {
	if ((schemaElem = shSchemaElemGet (shNameGetFromType(type), field))
	   == NULL) return TCL_ERROR;
	buff->data = (void *)shMalloc(sizeof (int));
	*(int *)(buff->data) = schemaElem->value;
	return TCL_OK;
	}

/* TYPEs: Convert input character string to an int */
   if (vtype == TTYPE) {
	buff->data = (void *)shMalloc(sizeof (int));
	*(TYPE *)(buff->data) = shTypeGetFromName(field);
	return TCL_OK;
	}

/* 1 byte chars */
   if (vtype == CHAR || vtype == UCHAR) {
	   buff->data = (void *)shMalloc(sizeof(char));
	   memset ((void *)(buff->data),' ',1);
	   memcpy ((void *)(buff->data), field, 1);
	   return TCL_OK;
	   }
/* Deal with the case of strings. */
   if (vtype == STR) {
/* Tricky logic here: buff->data points to a temporary storage location that
 * points to the character string.  In order to make freeing easier later on,
 * let's allocate storage for the pointer and the string in one buffer.
 * Later, we will only make one call to free anyway.
*/
           buff->data = (void *)shMalloc(sizeof (char *) + strlen(field) + 1);
	   ((char **)(buff->data))[0] = (char *)&(((char **)(buff->data))[1]);
	   strcpy (((char **)(buff->data))[0], field);
           return TCL_OK;
           }
   if (vtype == USHORT) {
	if (sscanf (field, "%hu", &scr_ushort) == 1) {
	   buff->data = (void *)shMalloc(sizeof (unsigned short));
	   *(unsigned short *)(buff->data) = scr_ushort;
	   return TCL_OK;
	   }
	buff->data = NULL;
	return TCL_ERROR;
	}
   if (vtype == SHORT) {
	if (sscanf (field, "%hd", &scr_short) == 1) {
	   buff->data = (void *)shMalloc(sizeof (short));
	   *(short *)(buff->data) = scr_short;
	   return TCL_OK;
	   }
	buff->data = NULL;
	return TCL_ERROR;
	}
   if (vtype == UINT) {
	if (sscanf (field, "%u", &scr_uint) == 1) {
	   buff->data = (void *)shMalloc(sizeof (unsigned int));
	   *(unsigned int *)(buff->data) = scr_uint;
	   return TCL_OK;
	   }
	buff->data = NULL;
	return TCL_ERROR;
	}
   if (vtype == INT) {
	if (sscanf (field, "%d", &scr_int) == 1) {
	   buff->data = (void *)shMalloc(sizeof (int));
	   *(int *)(buff->data) = scr_int;
	   return TCL_OK;
	   }
	buff->data = NULL;
	return TCL_ERROR;
	}
   if (vtype == ULONG) {
	if (sscanf (field, "%u", &scr_ulong) == 1) {
	   buff->data = (void *)shMalloc(sizeof (unsigned long));
	   *(unsigned long *)(buff->data) = scr_ulong;
	   return TCL_OK;
	   }
	buff->data = NULL;
	return TCL_ERROR;
	}
   if (vtype == LONG) {
	if (sscanf (field, "%d", &scr_long) == 1) {
	   buff->data = (void *)shMalloc(sizeof (long));
	   *(long *)(buff->data) = scr_long;
	   return TCL_OK;
	   }
	buff->data = NULL;
	return TCL_ERROR;
	}
   if (vtype == FLOAT) {
	if (sscanf (field, "%f", &scr_float) == 1) {
	   buff->data = (void *)shMalloc(sizeof (float));
	   *(float *)(buff->data) = scr_float;
	   return TCL_OK;
	   }
	buff->data = NULL;
	return TCL_ERROR;
	}
   if (vtype == DOUBLE) {
	if (sscanf (field, "%lf", &scr_double) == 1) {
	   buff->data = (void *)shMalloc(sizeof (double));
	   *(double *)(buff->data) = scr_double;
	   return TCL_OK;
	   }
	buff->data = NULL;
	return TCL_ERROR;
	}
   buff->data = NULL;
   return TCL_ERROR;
   }

int p_shTclOp(char *field, OPERTYPE *opval) {
   if (strcmp(field,"=") == 0) *opval = O_EQ;
   else if (strcmp(field,"==") == 0) *opval = O_EQ;
   else if (strcmp(field,"!=") == 0) *opval = O_NE;
   else if (strcmp(field,"<") == 0) *opval = O_LT;
   else if (strcmp(field,"<=") == 0) *opval = O_LE;
   else if (strcmp(field,">") == 0) *opval = O_GT;
   else if (strcmp(field,">=") == 0) *opval = O_GE;
   else if (strcmp(field,"&") == 0) *opval = O_BA;
   else if (strcmp(field,"!&") == 0) *opval = O_BNA;
   else return TCL_ERROR;
   return TCL_OK;
   }


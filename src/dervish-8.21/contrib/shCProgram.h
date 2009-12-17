#ifndef SHPROGRAM_H
#define SHPROGRAM_H

#include "dervish.h"

/* typedef int PROGEXEC(char **field, int argpass); */

typedef enum {PROG_ADD, PROG_DEREF} PROGEXEC;

typedef struct {
	PROGEXEC prog;
	int arg;
	} PROGSTEP;	/* pragma SCHEMA */

typedef struct {
	int nstep;
/* The following info contains the equivalent of SCHEMA_ELEM information
 * except that no attribute name is provided, and the type is TYPE, not char*
 * and the dimensions are stored as ints, not char*
*/
	TYPE type;		/* Type of destination */
	int nstar;		/* Number of indirect dimensions */
	int *sdim;		/* Placeholder for dimension info */
	int ndim;		/* Number of inline dimensions */
	int *dim;		/* Dimension info */
/* Auxiliary quantities */
	int nitem;		/* Number of inline items */
	int isize;		/* Size of one inline item */
	int nout;		/* Number of out-of-line items */
	int size;		/* Size of destination item */
	PROGSTEP *prog;
	} PROGRAM;	/* pragma SCHEMA */

/* Function prototypes */

void shProgramDel (PROGRAM *prog);

PROGRAM *shProgramCreate (char *type, char *attr);
PROGRAM *shProgramCreateBySchema (const SCHEMA *schema, char *attr);

void *shProgramEval (void *field, PROGRAM *program);

#endif


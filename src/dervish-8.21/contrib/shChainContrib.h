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

#ifndef SH_CHAINUTILS_H
#define SH_CHAINUTILS_H

#include "tcl.h"
#include "shCProgram.h"

/* A subset of dervish primitive types that are supported */

typedef enum {STR, UCHAR, CHAR, USHORT, SHORT, UINT, INT, ULONG, LONG,
   FLOAT, DOUBLE, EENUM, TTYPE} VTYPE;		/* pragma NONE */

/* An enum to describe possible operations */
/* added Bitwise And by gsh@usno.navy.mil */
typedef enum opertype {
	O_EQ,
	O_NE,
	O_LT,
	O_LE,
	O_GT,
	O_GE,
	O_BA,
	O_BNA} OPERTYPE;		/* pragma NONE */

/* Define my own buffer descriptor (a clone of Versant's - since that is
   all that is needed
*/

typedef struct bufdesc {
/*	int length;  Don't reaaly need the length field, so don't use */
	void *data;
	void *source;	/* Used by tsChainSort only; pointer to source object */
	} BUFDESC;		/* pragma NONE */

/* Define my own predicate term structure */
typedef struct {
	VTYPE type;
	int (*compare)(BUFDESC *dbuff, BUFDESC *qbuff);
	char *attribute;
/* The following is a new faster dereferencing function */
	PROGRAM *program;
	int combine;
	int range;
	OPERTYPE oper[2];
	int opcode[2];
	int not[2];
	BUFDESC key[2];
	} PREDTERM;		/* pragma NONE */

int p_shBufCompare (BUFDESC, BUFDESC, PREDTERM *, int);
int p_shPredCompare (BUFDESC, PREDTERM *);
int p_shEnumCompare (BUFDESC *, BUFDESC *);
int p_shStrCompare (BUFDESC *, BUFDESC *);
int p_shCharCompare (BUFDESC *, BUFDESC *);
int p_shUcharCompare (BUFDESC *, BUFDESC *);
int p_shShortCompare (BUFDESC *, BUFDESC *);
int p_shUshortCompare (BUFDESC *, BUFDESC *);
int p_shIntCompare (BUFDESC *, BUFDESC *);
int p_shUintCompare (BUFDESC *, BUFDESC *);
int p_shLongCompare (BUFDESC *, BUFDESC *);
int p_shUlongCompare (BUFDESC *, BUFDESC *);
int p_shFltCompare (BUFDESC *, BUFDESC *);
int p_shDblCompare (BUFDESC *, BUFDESC *);
int p_shTclParamGet (Tcl_Interp *, const SCHEMA *, char *, VTYPE *, TYPE *);
int p_shTclKeyData (VTYPE, TYPE, char *, BUFDESC *);
int p_shTclOp(char *, OPERTYPE *);

int p_shIntBitCompare (BUFDESC *, BUFDESC *);


double p_shShortExtract (void *);
double p_shUshortExtract (void *);
double p_shIntExtract (void *);
double p_shUintExtract (void *);
double p_shLongExtract (void *);
double p_shUlongExtract (void *);
double p_shFltExtract (void *);
double p_shDblExtract (void *);

void p_shDblSet (void *, double);
void p_shUintSet (void *, double);
void p_shIntSet (void *, double);
void p_shUshortSet (void *, double);
void p_shShortSet (void *, double);
void p_shLongSet (void *, double);
void p_shUlongSet (void *, double);
void p_shFltSet (void *, double);

int shTclChainSearch (Tcl_Interp *, CHAIN *, char *, CHAIN *);
int shTclChainSelect (Tcl_Interp *, CHAIN *, char *, CHAIN *);
int shTclChainSort (Tcl_Interp *, CHAIN *, char *, int);
int shTclChainMatch (Tcl_Interp *, CHAIN **, char *, CHAIN *, char *, int);
int shTclChainSet (Tcl_Interp *, CHAIN *, char *, char *);
int shTclChainSetFromHandle (Tcl_Interp *, CHAIN *, char *, void *, TYPE);
int shTclChainTransXY (Tcl_Interp *, CHAIN *, char *, char *, double, double, 
                     double, double, double, double);
int shTclChainTransZXY (Tcl_Interp *, CHAIN *, char *, char *, char *,
		     double, double, double);
int shTclChainScale (Tcl_Interp *, CHAIN *, char *, double, double);

#endif



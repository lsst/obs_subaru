#ifndef _SHCSEVAL_H
#define _SHCSEVAL_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shCSEval.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the Expression Evaluator
**
** WARNING: 
**      We have explicitly excluded this file from Dervish.h.  This file contains
**      definitions of structures, function, macros, etc ..., that are needed
**      ONLY for the internal functions of the expression evaluator.  However
**      since some of the functionnalities of the expression evaluator have to
**      be added in the Sdb product.  
**      Only files adding functionnality to the expression evaluator need to
**      use this h file.
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Chih-hao Huang/Philippe G. Canal
**      Creation date: April. 16, 1992
**
******************************************************************************
******************************************************************************
*/

/* For now just include code.h! however this should be weed out and clean
   when things settled */

/* #include "prvt/seCode.h" */

#ifdef __cplusplus
extern "C"
{
#endif

/*----------------------------------------------------------------------------
**
** STRUCTURE DEFINITIONS
**
** All these structures are internal 
*/

/* define types of symbols */

/* Pay attention to the "handle implementation types" */

typedef enum
{
	T_DONTCARE,	/* doesn't matter	*/
	T_CHAR,		/* char			*/
	T_INT,		/* int			*/
	T_SHORT,	/* short		*/
	T_LONG,		/* long			*/
	T_U_CHAR,	/* unsigned char	*/
	T_U_INT,	/* unsigned int		*/
	T_U_SHORT,	/* unsigned short	*/
	T_U_LONG,	/* unsigned long	*/
	T_FLOAT,	/* float		*/
	T_DOUBLE,	/* double		*/
	T_CHAR_P,	/* char *		*/
	T_INT_P,	/* int *		*/
	T_SHORT_P,	/* short *		*/
	T_LONG_P,	/* long	*		*/
	T_U_CHAR_P,	/* unsigned char *	*/
	T_U_INT_P,	/* unsigned int *	*/
	T_U_SHORT_P,	/* unsigned short *	*/
	T_U_LONG_P,	/* unsigned long *	*/
	T_FLOAT_P,	/* float *		*/
	T_DOUBLE_P,	/* double *		*/
	T_STRING,	/* null ended character array	*/
	T_CHAIN,        /* handle to a CHAIN    */
	T_VECTOR,       /* handle to a VECTOR   */
	T_SCHEMA,       /* handle to a SCHEMA   */
	T_OBJECT,       /* handle to an OBJECT  */
	T_TBLCOL,       /* handle to a TBLCOL   */
	T_ARRAY,        /* handle to an ARRAY   */
	T_CHAR_A,	/* special case for character array */
	T_PTR,		/* generic pointer	*/
	T_CONT_PERIOD,	/* special type for substructure reference */
	T_CONT_ARROW,	/* special type for indirect reference */
	T_CONTINUE,	/* special type for continuation */
	T_ERROR         /* error                */
}	S_TYPE;

/* define classes of symbols */

typedef enum
{
	VAR,		/* variable, get its value from source	*/
	LITERAL,	/* constant, value is known locally	*/
	ARRAY2,		/* array of elementary type, get each	*/
			/* individual element from the source	*/
			/* "on demand"				*/
	FUNCTION,	/* function, call external		*/
	V_FUNCTION,	/* for volatile constructors		*/
	DATA_MEMBER,	/* data member of an object		*/
	METHOD,		/* method of an object			*/
	CURSOR2,	/* cursor to something			*/
	SYMBOL2,	/* just a name				*/
	HANDLE2,	/* dervish handle				*/
	DUMMY		/* no real meaning in its value		*/
}	S_CLASS;

/* For caching a constructor/function */

typedef void * FUNC_ID;

typedef struct function_info
{
	int argc;		/* number of arguments */
	FUNC_ID fid;
	HANDLE clusteringDirective;
	TYPE returnType;
}	FUNCTION_INFO;

typedef struct array_info
{
	TYPE	type;	/* schema type of the eleemnts in array */
	int	dim;
}	ARRAY_INFO;

typedef struct vec_info
{
	VECTOR *vec;	/* vector */
	int n;		/* current position */
}	VECTOR_INFO;

typedef struct tblcol_info
{
	ARRAY *array;	/* array of that column */
	S_TYPE type;	/* type of the element */
	int dim;	/* dimension of the column */
	int pos;	/* current position */
}	TBLCOL_COL_INFO;


/* only chain_element is concerned in execution */

typedef struct chain_info
{
	CHAIN_ELEM *cep;
	long offset;	/* offset in the schema if not simple */
	TYPE type;	/* dervish type */
	int indirection;	/* for pointer */
	int dim;	/* dimension */
	char *nelem;	/* ranges of dimensions */
}	CHAIN_INFO;

/* HANDLE_INFO should be consistent with HANDLE */

typedef struct handle_info
{
	void *ptr;	/* pointer to the real thing */
	TYPE type;	/* the dervish schema type */
	int indirection;	/* for pointer */
	int dim;	/* dimension, 0 for scalar type, >0 for
			 * array
			 */
	char *nelem;	/* ranges of dimensions */
}	HANDLE_INFO;

/* chandle, continue handle, is a partial handle. Its real value is
 * computed with the previous handle */

typedef struct chandle
{
	TYPE type;	/* dervish schema type */
	int indirection;	/* for pointer */
	long offset;	/* offset in shcema */
	int dim;	/* dimension */
	char *nelem;	/* ranges of dimension */
}	CHANDLE;

/* For method in an object */

typedef void * METHOD_ID;
typedef void * OBJECT_ID;

typedef struct mthd
{
	int argc;
	METHOD_ID mid;
}	MMETHOD;	

/* Not all type & class configurations are legal.
 *
 * Literals has only simple types, no pointers or arrays
 * Functions will never appear in evaluation stack, but will be in
 * code store.
 */

typedef struct data_element
{
	S_CLASS	d_class;
	S_TYPE	type;
	int	ind;	/* indirection of the type */
	int	size;

	/* Only elementary types may have values */

	union
	{
		int		i;
		char		c;
		short		s;
		long		l;
		float		f;
		double		d;
		unsigned char	uc;
		unsigned int	ui;
		unsigned short	us;
		unsigned long	ul;
		char		*cp;
		int		*ip;
		short		*sp;
		long		*lp;
		float		*fp;
		double		*dp;
		unsigned char	*ucp;
		unsigned int	*uip;
		unsigned short	*usp;
		unsigned long	*ulp;
		FUNCTION_INFO	fn;
		MMETHOD		md;
		char		*str;
/*
		char		symbol[64];
*/
		TBLCOL_COL_INFO	tci;
		OBJECT_ID	op;
		HANDLE		han;
		HANDLE_INFO	hi;
		CHANDLE		chan;
		CHAIN_INFO	ci;
		ARRAY_INFO	ar;
		VECTOR_INFO	vi;
		ARRAY*          arr;
		void		*gp;	/* generic pointer */
	}	value;
}	DATA_ELEMENT;


/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/

RET_CODE shSeConfig
   (
    void (*SEvalInit)(void),
    void (*SEvalEnd)(void),
    DATA_ELEMENT (*sdbFindConstructor) (char* className,
			       int argc,
			       DATA_ELEMENT * argv,
			       FUNC_ID* fid,
			       int persistent),
    DATA_ELEMENT (*sdbCallConstructor) (FUNC_ID fid,
			       int argc,
			       DATA_ELEMENT* argv,
			       TYPE returnType,
			       void* clusteringDirective,
			       int persistent),
    DATA_ELEMENT (*sdbFindMethod) (char* className,
			  char* methodName,
			  int argc,
			  DATA_ELEMENT * argv,
			  METHOD_ID* mid),
    DATA_ELEMENT (*sdbCallMethod) (HANDLE obj,
			  METHOD_ID mid,
			  int argc,
			  DATA_ELEMENT* argv),
    const char* (*sdbGetTclHandle) (Tcl_Interp* interp, HANDLE han),
    void* (*sdbFindClassFromType)(const char* TypeName),
	int (*realSeRegisterTransientObject)(HANDLE h),
	void (*realSeFlushTransientObjects)(void)
 );

/* It is not a pure C function! We need to fix that somehow */
int shSEval(Tcl_Interp* interp, char* expression, int start, int end, int debug, int mtime);

#ifdef __cplusplus
}
#endif

#endif




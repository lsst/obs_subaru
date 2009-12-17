#ifndef __CODE_H__
#define __CODE_H__

/* code.h -- data structure definitions for code.c and related files
 *
 * Description:
 * ===========
 * evaluation stack contains only operands, which are generic values
 * or "reference" to the variables, which might be in array form.
 * The generical values only come from (1) the literal explicitly
 * specified in the translation string, or the values returned from
 * function calls, including built-in functions and external
 * constructors.
 *
 * Definition of Evaluation Stack:
 * ==============================
 * Each entry is a three tuple of (type, d_class, value)
 *
 * where
 *	type is the "desitination" type expected. "Don't care" type
 *		implies that any type determined from source will be
 *		used as destination type.
 *
 *		A type with modifier is considered different type from
 *		its generic type.
 *
 *	d_class is the storage class of the symbol.
 *
 *	value may be the real value, reference to the method of getting
 *		value, ..., depending on the type and d_class.
 */

#include <dervish.h>      /* For dervish structures and schema */

#include <shCSEval.h>   /* For the part of the internals made semi-public for
			   Sdb */

#define MAX_STACK_DEPTH 1024
#define MAX_CODE_LENGTH 8192

/* Defined to pass the tcl interpreter to the
   lex tokeninzer 
   It is also use by the SEvalInterpeter to
   execute the "RETURN" internal command. */
extern Tcl_Interp* g_seInterp;

/* function prototype */

#ifdef __cplusplus
extern "C"
{
#endif
/* Definition for the code store */

typedef enum
{
	PUSH,	/* push on to evaluation stack	*/
	PUSHV,	/* special push for known value */
	POP,	/* this should never happen	*/
	EXEC,	/* execute a function		*/
	ADD,	/* addition on stack top	*/
	SUB,	/* sumtraction on stack top	*/
	MUL,	/* multiply on stack top	*/
	DIV,	/* division on stack top	*/
	NEG,	/* negation on stack top	*/
	MDT,	/* modify type on stack top	*/
	LIT,	/* literialize			*/
	ASSIGN,	/* assignment			*/
	FETCH,  /* fetch a value                */
	STD,	/* (object) scheduled to delete	*/
	RETURN, /* return the value in Tcl_Interp */
	SIN,	/* sin(x)			*/
	COS,	/* cos(x)			*/
	TAN,	/* tan(x)			*/
	ASIN,	/* asin(x)			*/
	ACOS,	/* acos(x)			*/
	ATAN,	/* atan(x)			*/
	EXP,	/* exp(x)			*/
	LOG,	/* log(x)			*/
	POW,	/* pow(x)			*/
	ABS,	/* abs(x)			*/
	ATAN2,	/* atan2(x, y)			*/
	LOG10,	/* log10(x)			*/
	SQRT,	/* sqrt(x)			*/
	MIN,	/* min(x, y)			*/
	MAX,	/* max(x, y)			*/
	LC,	/* lc(), the value of current loop counter */
	IGNORE,	/* pop off the top of the stack	*/
	STRCPY,	/* strcpy(s1,s2)		*/
	STRNCPY,/* strncpy(s1, s2, n)		*/
	STRCAT,	/* strcat(s1, s2)		*/
	STRNCAT,/* strncat(s1, s2, n)		*/
	STRCMP,	/* strcmp(s1, s2)		*/
	STRNCMP,/* strncmp(s1, s2, n)		*/
	STRCASECMP,  /* strcasecmp(s1, s2)	*/
	STRNCASECMP, /* strncasecmp(s1, s2, n)	*/
	STRLEN,	/* strlen(s)			*/
	STROFFSET,	/* offset in a string	*/
	LAND,	/* logical and			*/
	LOR,	/* logical or			*/
	LNOT,	/* logical not			*/
	EQ,	/* equal?			*/
	GT,	/* grater than?			*/
	GE,	/* grater than or equal to?	*/
	LT,	/* less than?			*/
	LE,	/* less than or equal to?	*/
	NE,	/* not equal?			*/
	SKIP0,	/* skip if zero			*/
	SKIP,	/* skip				*/
	CONTINUE,	/* continue		*/
	BREAK,	/* break			*/
	BAND,	/* bitwise and			*/
	BOR,	/* bitwise or			*/
	BXOR,	/* bitwise xor			*/
	SL,	/* shift left			*/
	SR,	/* shift right			*/
	STOP
}	OPCODE;

/* 
 * Internal Functions used inside the yacc/lex code
 *
 * Since these functions need to be seen by the linker 
 * they need to be of the form p_shSeXXXXX
 */

/* This functions are used in the grammar file to create the
   expression Evaluator 'Code' */
int p_shSeCodeGen(OPCODE opcode, DATA_ELEMENT operand);
int p_shStdSize(S_TYPE type);
void p_shSeAdjustIterLength(int);
S_TYPE p_shSeDervishType2ElemType(TYPE type);
int p_shSeIsNumericalType(S_TYPE);
S_TYPE p_shSeBinaryType(S_TYPE t1, S_TYPE t2);
int numeric_type(S_TYPE t);

/* This functions are used in the lex file to update the yacc (yylval)
   structures */

void p_shSeSymCopyHandle(S_TYPE type, HANDLE value);
void p_shSeSymCopyLong(long value);
void p_shSeSymCopyDouble(double value);
void p_shSeSymCopyString(const unsigned char* value);
void p_shSeSymCopyIdentifier(const unsigned char* value);
int p_shSe_code_idx(void);
void p_shSe_code_patch(int, int);

/* Unused/Undefined function functions */

DATA_ELEMENT get_var_value(char *name);
DATA_ELEMENT exec_function(char *name, int argc, DATA_ELEMENT *argv,
	int persistency, FUNC_ID *fid);
DATA_ELEMENT call_method(HANDLE* obj,
	int argc, DATA_ELEMENT *argv, METHOD_ID *mid);

/* List of initializable functions (they should be set by
   shSeConfig */

extern void (*seSEvalInit)(void);
extern void (*seSEvalEnd)(void);
extern DATA_ELEMENT (*seFindConstructor) (char* className,
			       int argc,
			       DATA_ELEMENT * argv,
			       FUNC_ID* fid,
			       int persistent);
extern DATA_ELEMENT (*seCallConstructor) (FUNC_ID fid,
			       int argc,
			       DATA_ELEMENT* argv,
			       TYPE returnType,
			       void* clusteringDirective,
			       int persistent);
extern DATA_ELEMENT (*seFindMethod) (char* className,
			  char* methodName,
			  int argc,
			  DATA_ELEMENT * argv,
			  METHOD_ID* mid);
extern DATA_ELEMENT (*seCallMethod) (HANDLE obj,
			  METHOD_ID mid,
			  int argc,
			  DATA_ELEMENT* argv);
extern const char* (*seGetTclHandle) (Tcl_Interp* interp, HANDLE han);
extern void* (*seFindClassFromName) (const char* TypeName);

/* static string handling routine */

extern char *register_string(char *s);
extern void clean_up_static_strings(void);

#ifdef __cplusplus
}
#endif

#endif

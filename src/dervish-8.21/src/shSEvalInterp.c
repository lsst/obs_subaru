#define SEDEBUG
/* shSEvalInterp.c -- evaluation stack manipulation routines
 *
 * warning! this comment is out of date. read at your own risk!
 *
 * Description:
 * ===========
 * evaluation stack contains only operands, which are generic values
 * or "reference" to the variables, which might be in array form.
 * The generical values only come from (1) the literal explicitly
 * specified in the translation string, or (2) the values returned from
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include "prvt/seCode.h"

/* Timing parameters -- measuring the speed for improvement */

static struct tms t0;		/* temporary at time zero */
static struct tms tcomp;	/* time spent in compilation */
static struct tms texec;	/* time spent in execution */

#ifdef __sgi
#include <limits.h>
#endif

#ifndef CLK_TCK
#define CLK_TCK 60
#endif

/* Definition of all global variable used */

#ifdef SEDEBUG
/* print debugging information */
static int g_debug = 0;
#endif

/* looping index */
static long lc;

/* lex input buffer */
char *lex_input = "";
char *lex_input_buffer;

/* Evaluation_stack */
static DATA_ELEMENT eval_stack[MAX_STACK_DEPTH];
static eval_idx = 0;


/* code store -- an array of code */
static struct code
{
	OPCODE		opcode;
	DATA_ELEMENT	operand;
}	code [MAX_CODE_LENGTH];

static int code_idx = 0;	/* index in the code store */
static int pc = 0;		/* program counter */
static int ev_length = 0;	/* interations */

/* Defined to pass the tcl interpreter to the
   lex tokeninzer 
   It is also use by the SEvalInterpeter to
   execute the "RETURN" internal command. */
Tcl_Interp* g_seInterp = NULL;

static DATA_ELEMENT ERROR_DATA = { DUMMY, T_DONTCARE, 0, 0, {0}};

/* local function prototype */

static int shSeGenerateCode(char *conversionString, Tcl_Interp* interp);
static void code_init(void);
static int exec_code(void);
static int single_step(OPCODE opcode, DATA_ELEMENT operand);

static int push(DATA_ELEMENT operand);
static DATA_ELEMENT pop(void);
static DATA_ELEMENT top(void);
static int built_in_arith(OPCODE opcode);
static int built_in_string(OPCODE opcode);
static int built_in_logic(OPCODE opcode);
static int built_in_bitwise(OPCODE opcode);
static int built_in_shift(OPCODE op);
static int do_function(FUNCTION_INFO fn, int persistent);
static int do_method(MMETHOD md);
static int return_top(void);
static int adjust_type(DATA_ELEMENT *e, S_TYPE type);
static int advance_cursor(int pc);
static int do_assign(DATA_ELEMENT a, DATA_ELEMENT d);
static int is_true(DATA_ELEMENT);

static DATA_ELEMENT get_value(DATA_ELEMENT);
static DATA_ELEMENT get_value_from_handle(DATA_ELEMENT);
static DATA_ELEMENT get_value_from_tblcol(ARRAY *, int pos, int dim);
static DATA_ELEMENT get_base_value(DATA_ELEMENT);

static DATA_ELEMENT get_address(DATA_ELEMENT);
static DATA_ELEMENT get_address_from_handle(DATA_ELEMENT);
static void* get_address_from_tblcol(ARRAY *array, int pos, int dim);
static DATA_ELEMENT get_base_address(DATA_ELEMENT v);

static int get_array_elem(ARRAY* array, S_TYPE type);
static int set_array_elem(ARRAY* array, S_TYPE type);
static void* get_array_elem_ptr(ARRAY* array);
static void **get_array_data_ptr(void **, int dim);
static long get_array_offset(long, int *, char **);
static long get_array_offset1(long, char **);
static DATA_ELEMENT array_decode(DATA_ELEMENT v);

static char *get_next_nelem_value(char *nelem, int *n);
static int data_compatible(DATA_ELEMENT a, DATA_ELEMENT d);

static S_TYPE ptr_type(S_TYPE);
static int is_ptr_type(DATA_ELEMENT);
static int is_ptr_stype(S_TYPE);
static S_TYPE base_type(S_TYPE ptr);

/* Local error handling functions */
static void error_type_conversion(S_TYPE t1, S_TYPE t2);
static void op_type_error(char *s, S_TYPE t1, S_TYPE t2);
static void fatal(char *s);
static void warn(char *s);

/* Arithmetic 'wrappers' */
static DATA_ELEMENT add(DATA_ELEMENT, DATA_ELEMENT);
static DATA_ELEMENT sub(DATA_ELEMENT, DATA_ELEMENT);
static DATA_ELEMENT mul(DATA_ELEMENT, DATA_ELEMENT);
static DATA_ELEMENT divide(DATA_ELEMENT, DATA_ELEMENT);
static DATA_ELEMENT neg(DATA_ELEMENT);
static DATA_ELEMENT f_sin(DATA_ELEMENT);
static DATA_ELEMENT f_cos(DATA_ELEMENT);
static DATA_ELEMENT f_tan(DATA_ELEMENT);
static DATA_ELEMENT f_asin(DATA_ELEMENT);
static DATA_ELEMENT f_acos(DATA_ELEMENT);
static DATA_ELEMENT f_atan(DATA_ELEMENT);
static DATA_ELEMENT f_exp(DATA_ELEMENT);
static DATA_ELEMENT f_log(DATA_ELEMENT);
static DATA_ELEMENT f_pow(DATA_ELEMENT, DATA_ELEMENT);
static DATA_ELEMENT f_abs(DATA_ELEMENT);
static DATA_ELEMENT f_atan2(DATA_ELEMENT, DATA_ELEMENT);
static DATA_ELEMENT f_log10(DATA_ELEMENT);
static DATA_ELEMENT f_sqrt(DATA_ELEMENT);
static DATA_ELEMENT f_min(DATA_ELEMENT, DATA_ELEMENT);
static DATA_ELEMENT f_max(DATA_ELEMENT, DATA_ELEMENT);
static DATA_ELEMENT f_lc(void);

/* Local debugging functions */

int dump_code_store(void);
static int dump_eval_stack(void);
char *show_entry(char *s, DATA_ELEMENT se);
char *show_type(char *s, S_TYPE type);
char *show_class(char *s, S_CLASS d_calss);
char *show_value(char *s, DATA_ELEMENT se);
char *show_code(char *s, OPCODE c);
static int code_top(OPCODE* opcode, DATA_ELEMENT* operand);
static void init_clock(void);
static void set_clock(struct tms *);
static void report_time(int);

/* local function that are used by shSEval.y or shSEval.l */
int shSe_yyparse(void);

/* This function is actually defined but explicitly not doing anything yet

static DATA_ELEMENT get_data_member_value(char *path, char *field_name);
*/

/* I know eventually I will be bitten by this for breaking POSIX.
 * strcasecmp() and strncasecmp() are not used internally for this
 * interpreter; they are interfaced to provide convenience for the
 * users, especially those who used to work with case insensitive
 * languages ...
 *
 *	-- C.-H. Huang
 */ 

/* Case-insensitive comparision routines from 4.3BSD.  */
extern int	strcasecmp(const char *, const char *);
extern int	strncasecmp(const char *, const char *, size_t);

/* Function use to initialiaze the 'sdb' part of the code
   when sdb is not present. */
static void empty_fn (void) {}
static void*return_null (void) { return NULL; }
static void fatal_fn (void)
{
  shFatal("Fatal call to an unitialized part of the \
expression evaluator\nSee SDB for correct initialization\n");
}
static DATA_ELEMENT return_dummy (void)
{
  DATA_ELEMENT d;
  d.d_class = DUMMY;
  shErrStackPush("Non Fatal call to a not installed part of the \
expression evaluator\nSee SDB for a more complete installation");
  return d;
}

/* List of initializable functions (they should be set by
   shSeConfig */

void (*seSEvalInit)(void) = &empty_fn;
void (*seSEvalEnd)(void) = &empty_fn;
DATA_ELEMENT (*seFindConstructor) (char* className,
			       int argc,
			       DATA_ELEMENT * argv,
			       FUNC_ID* fid,
			       int persistent) = (DATA_ELEMENT(*)(char*, int, DATA_ELEMENT*,
						  FUNC_ID*, int)) &return_dummy;
DATA_ELEMENT (*seCallConstructor) (FUNC_ID fid,
			       int argc,
			       DATA_ELEMENT* argv,
			       TYPE returnType,
			       void* clusteringDirective,
			       int persistent) = (DATA_ELEMENT (*) (FUNC_ID,int,DATA_ELEMENT*,
								   TYPE,void*,int))
						  &fatal_fn;
DATA_ELEMENT (*seFindMethod) (char* className,
			  char* methodName,
			  int argc,
			  DATA_ELEMENT * argv,
			  METHOD_ID* mid) = (DATA_ELEMENT (*) (char*,char*,int,DATA_ELEMENT*,
							     METHOD_ID*)) &fatal_fn;
DATA_ELEMENT (*seCallMethod) (HANDLE obj,
			  METHOD_ID mid,
			  int argc,
			  DATA_ELEMENT* argv) = (DATA_ELEMENT (*) (HANDLE,METHOD_ID,int,
								 DATA_ELEMENT*)) &fatal_fn;
const char* (*seGetTclHandle) (Tcl_Interp* interp, HANDLE han) =
                          (const char* (*)(Tcl_Interp*, HANDLE)) &fatal_fn;
void* (*seFindClassFromName) (const char* TypeName) = (void* (*) (const char*)) &return_null;

/* dealing with garbage collection mechanism for transient objects */

static int shSeRegisterTransientObject(HANDLE h)
{
	return(1);
}
int (*seRegisterTransientObject)(HANDLE h) = &shSeRegisterTransientObject;
void (*seFlushTransientObjects)(void)= &empty_fn;

/* 
 * Code Implementation.
 */


/* code_init() -- initilize the evaluation stack and code store */

static void code_init(void)
{
	eval_idx = 0;
	code_idx = 0;
	pc = 0;
	ev_length = 0;
}

/* p_shSeAdjustIterLength(n) -- adjust the length according to n */

void p_shSeAdjustIterLength(int n)
{
	if (n < 1) return;
	if ((ev_length == 0) || (n < ev_length))
	{
		ev_length = n;
	}
}

/* p_shSeCodeGen(opcode, operand) -- generate code */

int p_shSeCodeGen(OPCODE opcode, DATA_ELEMENT operand)
{
#ifdef PARSE_DEBUG
  char s1[64], s2[512];
  printf("code %5d %8s %s\n", code_idx,
	 show_code(s1, opcode),
	 show_entry(s2, operand));
#endif
	if (code_idx > MAX_CODE_LENGTH)
	{
		warn("code store overflow");
		return(-1);
	}
	else
	{
		code [code_idx].opcode = opcode;
		code [code_idx++].operand = operand;
	}
	return(code_idx - 1);
}

/* p_shSe_code_idx(void) -- next code gen location */

int p_shSe_code_idx(void)
{
	return(code_idx);
}

/* code_top() -- peek on the top of the code stack without removing it */

static int code_top(OPCODE* opcode, DATA_ELEMENT* operand)
{
	if (code_idx == 0)
	{
		warn("code store underflow");
		return(1);
	}
	else
	{
		*opcode = code [code_idx-1].opcode;
		*operand = code [code_idx-1].operand;
	}
	return(0);
}

/* p_shSe_code_patch(pos, num) -- patch the operand */

void p_shSe_code_patch(int pos, int num)
{
	if ((pos < 0) || (pos > code_idx))
	{
		shFatal("patched position out of range");
	}

	code[pos].operand.d_class = LITERAL;
	code[pos].operand.type = T_INT;
	code[pos].operand.value.i = num;
}

/* exec_code() -- execute the code in the code store */

static int exec_code(void)
{
#ifdef SEDEBUG
	char s1[64], s2[512];
#endif

	eval_idx = 0;	/* to be safe */

	for (pc = 0; pc < code_idx; pc++)
	{
#ifdef SEDEBUG
	        if (g_debug) printf("executing %8s %s\n",
			show_code(s1, code[pc].opcode),
			show_entry(s2, code[pc].operand));
#endif

		/* abort, whenever error occurs */

		switch (single_step(code[pc].opcode, code[pc].operand))
		{
		case 1:
			return(1);	/* error */
		case 2:
			return(2);	/* exhaust all interations */
		default:
			break;	/* just to make picky compiler happy */
		}

#ifdef SEDEBUG
		if (g_debug) dump_eval_stack();
#endif
	}

	return(0);
}

/* single_step() -- execute one instruction */

static int single_step(OPCODE opcode, DATA_ELEMENT operand)
{
	DATA_ELEMENT d, a;

	switch(opcode)
	{
	case PUSH:
	case PUSHV:
		/* check for cursors */

		if (operand.d_class == CURSOR2)
		{
			if (advance_cursor(pc))	/* reach the end */
			{
				return(2);	/* it's done, no more */
			}

			/* do something special for CHAIN */

			if (operand.type == T_CHAIN)
			{
				d.d_class = HANDLE2;
				d.type = T_SCHEMA;
				d.value.hi.ptr = (char *) (operand.value.ci.cep->pElement) + operand.value.ci.offset;
				d.value.hi.type = operand.value.ci.type;
				d.value.hi.indirection = operand.value.ci.indirection;
				d.value.hi.dim = operand.value.ci.dim;
				d.value.hi.nelem = operand.value.ci.nelem;
				d.size = operand.size;

				return(push(d));
			}
			else if (operand.type == T_VECTOR)
			{
				if (opcode == PUSH)
				{
					operand.d_class = HANDLE2;

					return(push(operand));
				}
				else	/* PUSHV */
				{
					d.d_class = LITERAL;
					d.type = T_DOUBLE;
					d.value.d =
						operand.value.vi.vec->
						vec[operand.value.vi.n];
					d.size = p_shStdSize(d.type);

					return(push(d));
				}
			}
			else if (operand.type == T_TBLCOL)
			{
				operand.d_class = HANDLE2;
				return(push(operand));
			}
		}
		return(push(operand));
	case POP:	/* impossible for now */
		warn("POP? how do you get here?");
		return(1);
	case IGNORE:	/* ignore the top */
		/* there are only two cases:
		 *    there is one value to ignore or none
		 */

		if (eval_idx)
		{
			d = get_value(pop());
			return(d.d_class == DUMMY);
		}
		return(0);
	case MDT:
		d = get_value(pop());
		if (d.d_class == DUMMY) return(1); /* error */
		if ((d.d_class == LITERAL) || (d.d_class == HANDLE2))
		{
			if (adjust_type(&d, operand.type))
				return(1);
		}
		else
		{
			d.type = operand.type;
			d.ind = operand.ind;
		}
		return(push(d));
	case LIT:
		d = get_value(pop());
		if (d.d_class == DUMMY) return(1); /* error */
		d.d_class = LITERAL;
		return(push(d));
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case NEG:
	case SIN:
	case COS:
	case TAN:
	case ASIN:
	case ACOS:
	case ATAN:
	case EXP:
	case LOG:
	case POW:
	case ABS:
	case ATAN2:
	case LOG10:
	case SQRT:
	case MIN:
	case MAX:
	case LC:
		return(built_in_arith(opcode));
	case STRCPY:
	case STRNCPY:
	case STRCAT:
	case STRNCAT:
	case STRCMP:
	case STRNCMP:
	case STRCASECMP:
	case STRNCASECMP:
	case STRLEN:
	case STROFFSET:
		return(built_in_string(opcode));
	case LAND:
	case LOR:
	case LNOT:
	case EQ:
	case LE:
	case LT:
	case GT:
	case GE:
	case NE:
		return(built_in_logic(opcode));
	case BAND:
	case BOR:
	case BXOR:
		return(built_in_bitwise(opcode));
	case SR:
	case SL:
		return(built_in_shift(opcode));
	case SKIP0:
		d = get_value(pop());
		if (is_true(d)) return(0);
	case SKIP:
		{
			/* remember to advance the cursors, even through
			 * they are skipped
			 */

			int i;

			for (i = 1; i <= operand.value.i; i++)
			{
				if (code[pc+i].operand.d_class == CURSOR2)
				{
					if (advance_cursor(pc + i))
					{
						return(2);
					}
				}
			}

			pc += operand.value.i;
			return(0);
		}
	case CONTINUE:
		{
			/* remember to advance the cursors, even through
			 * they are skipped
			 */

			int i;

			for (i = pc + 1; i < code_idx; i++)
			{
				if (code[i].operand.d_class == CURSOR2)
				{
					if (advance_cursor(i))
					{
						return(2);
					}
				}
			}

			pc = code_idx - 1;
			return(0);
		}
	case BREAK:
		return(2);
	case EXEC:
		if (operand.d_class == FUNCTION)
		{
			return(do_function(operand.value.fn, 1));
		}
		else if (operand.d_class == V_FUNCTION)
		{
			return(do_function(operand.value.fn, 0));
		}
		else
		{
			return(do_method(operand.value.md));
		}
	case FETCH:
		if (operand.d_class == ARRAY2)
		{
			return(get_array_elem(operand.value.arr,operand.type));
		}
		break;
	case RETURN:
		return(return_top());
	case STOP:
		warn("STOP? says who?");
		return(1);
	case ASSIGN:
		/* assginment */
		if (operand.d_class == ARRAY2)
		  {
		    return(set_array_elem(operand.value.arr, operand.type));
		  }
		else
		{
			d = get_value(pop());
			a = get_address(pop());

			if ((a.d_class == DUMMY)||(d.d_class == DUMMY))
			{
				return(1);
			}

			return(do_assign(a, d));
		}
	case STD:	/* scheduled to delete */
		d = top();
		if (d.type != T_OBJECT)	/* must be an object */
		{
			warn("Illegal object to be deleted");
			return(1);
		}
		return((*seRegisterTransientObject)(d.value.han));
	default:
		warn("Illegal instruction");
		return(1);
	}
	return(1);	/* Something must be wrong !!! */
}

/* debugging routines */

/* show_entry(s, se) -- put se in s using ascii format */

char *show_entry(char *s, DATA_ELEMENT se)
{
	char s1[64], s2[64], s3[512];

	sprintf(s, "[%8s] (%16s) %03d %s",
		show_class(s1, se.d_class),
		show_type(s2, se.type),
		se.size,
		show_value(s3, se));

	return(s);
}

/* show_type(s, type) -- put the ascii name of the type into s */

char *show_type(char *s, S_TYPE type)
{
	switch(type)
	{
	case T_DONTCARE:
		strcpy(s, "don't care");
		break;
	case T_CHAR:
		strcpy(s, "char");
		break;
	case T_INT:
		strcpy(s, "int");
		break;
	case T_SHORT:
		strcpy(s, "short");
		break;
	case T_LONG:
		strcpy(s, "long");
		break;
	case T_FLOAT:
		strcpy(s, "float");
		break;
	case T_DOUBLE:
		strcpy(s, "double");
		break;
	case T_U_CHAR:
		strcpy(s, "unsigned char");
		break;
	case T_U_INT:
		strcpy(s, "unsigned int");
		break;
	case T_U_SHORT:
		strcpy(s, "unsigned short");
		break;
	case T_U_LONG:
		strcpy(s, "unsigned long");
		break;
	case T_CHAR_P:
		strcpy(s, "char *");
		break;
	case T_INT_P:
		strcpy(s, "int *");
		break;
	case T_SHORT_P:
		strcpy(s, "short *");
		break;
	case T_LONG_P:
		strcpy(s, "long *");
		break;
	case T_FLOAT_P:
		strcpy(s, "float *");
		break;
	case T_DOUBLE_P:
		strcpy(s, "double *");
		break;
	case T_U_CHAR_P:
		strcpy(s, "unsigned char *");
		break;
	case T_U_INT_P:
		strcpy(s, "unsigned int *");
		break;
	case T_U_SHORT_P:
		strcpy(s, "unsigned short *");
		break;
	case T_U_LONG_P:
		strcpy(s, "unsigned long *");
		break;
	case T_STRING:
		strcpy(s, "string");
		break;
	case T_OBJECT:
		strcpy(s, "object");
		break;
	case T_SCHEMA:
		strcpy(s, "schema handle");
		break;
	case T_CHAIN:
		strcpy(s, "chain handle");
		break;
	case T_VECTOR:
		strcpy(s, "vector handle");
		break;
	case T_TBLCOL:
		strcpy(s, "tblcol handle");
		break;
	case T_ARRAY:
		strcpy(s, "array handle");
		break;
	case T_CONT_PERIOD:
		strcpy(s, "continue .");
		break;
	case T_CONT_ARROW:
		strcpy(s, "continue ->");
		break;
	case T_CONTINUE:
		strcpy(s, "continue");
		break;
	case T_ERROR:
		strcpy(s, "error");
		break;
	default:
		strcpy(s, "unknown");
		break;
	}

	return(s);
}

/* show_class(s, d_class) -- show ascii d_class name in s */

char *show_class(char *s, S_CLASS d_class)
{
	switch(d_class)
	{
	case VAR:
		strcpy(s, "VAR");
		break;
	case LITERAL:
		strcpy(s, "LITERAL");
		break;
	case ARRAY2:
		strcpy(s, "ARRAY2");
		break;
	case FUNCTION:
		strcpy(s, "FUNCTION");
		break;
	case V_FUNCTION:
		strcpy(s, "V_FUNCT");
		break;
	case DATA_MEMBER:
		strcpy(s, "DAT_MEM");
		break;
	case METHOD:
		strcpy(s, "METHOD");
		break;
	case CURSOR2:
		strcpy(s, "CURSOR");
		break;
	case SYMBOL2:
		strcpy(s, "SYMBOL");
		break;
	case HANDLE2:
		strcpy(s, "HANDLE");
		break;
	case DUMMY:
		strcpy(s, "DUMMY");
		break;
	default:
		strcpy(s, "UNKNOWN");
		break;
	}

	return(s);
}

/* show_value (s, se) -- show value of se.value in ascii format */

char *show_value(char *s, DATA_ELEMENT se)
{
	char s1[64];

	switch(se.d_class)
	{
	case DUMMY:
		s[0] = '\0';
		break;
/*
	case VAR:
		strcpy(s, se.value.symbol);
		break;
	case SYMBOL2:
		strcpy(s, se.value.symbol);
		break;
*/
	case LITERAL:
		switch(se.type)
		{
		case T_CHAR:
			sprintf(s, "'%c'", se.value.c);
			break;
		case T_INT:
			sprintf(s, "%d", se.value.i);
			break;
		case T_SHORT:
			sprintf(s, "%hd", se.value.s);
			break;
		case T_LONG:
			sprintf(s, "%ld", se.value.l);
			break;
		case T_FLOAT:
			sprintf(s, "%f", se.value.f);
			break;
		case T_DOUBLE:
			sprintf(s, "%f", se.value.d);
			break;
		case T_U_CHAR:
			sprintf(s, "'%c'", se.value.uc);
			break;
		case T_U_INT:
			sprintf(s, "%u", se.value.ui);
			break;
		case T_U_SHORT:
			sprintf(s, "%hu", se.value.us);
			break;
		case T_U_LONG:
			sprintf(s, "%lu", se.value.ul);
			break;
		case T_STRING:
		case T_CHAR_P:
			sprintf(s, "%s", se.value.str);
			break;
		case T_INT_P:
			sprintf(s, "%p", se.value.ip);
			break;
		case T_SHORT_P:
			sprintf(s, "%p", se.value.sp);
			break;
		case T_LONG_P:
			sprintf(s, "%p", se.value.lp);
			break;
		case T_FLOAT_P:
			sprintf(s, "%p", se.value.fp);
			break;
		case T_DOUBLE_P:
			sprintf(s, "%p", se.value.dp);
			break;
		case T_U_CHAR_P:
			sprintf(s, "%p", se.value.ucp);
			break;
		case T_U_INT_P:
			sprintf(s, "%p", se.value.uip);
			break;
		case T_U_SHORT_P:
			sprintf(s, "%p", se.value.usp);
			break;
		case T_U_LONG_P:
			sprintf(s, "%p", se.value.ulp);
			break;
/*
		case T_DONTCARE:
			strcpy(s, se.value.symbol);
			break;
*/
		case T_CHAIN:
		case T_VECTOR:
		case T_SCHEMA:
		case T_OBJECT:
		case T_TBLCOL:
		case T_ARRAY:
			sprintf(s, "%p", se.value.hi.ptr);
			break;
		default:
			strcpy(s, "unsupported literal type");
			break;
		}
		break;
	case ARRAY2:
		sprintf(s, "%p", se.value.arr);
		break;
	case FUNCTION:
	case V_FUNCTION:
		sprintf(s, "(%d) "PTR_FMT,
			se.value.fn.argc, se.value.fn.fid);
		break;
/*
	case DATA_MEMBER:
		sprintf(s, "%s.%s", se.value.dm.path,
			se.value.dm.field_name);
		break;
*/
	case METHOD:
		sprintf(s, "(%d) "PTR_FMT,
			se.value.md.argc, se.value.md.mid);
		break;
	case CURSOR2:
		switch(se.type)
		{
		case T_CHAIN:
			sprintf(s, PTR_FMT" %d %s %d %d %s",
				se.value.ci.cep, se.value.ci.offset,
				shNameGetFromType(se.value.ci.type),
				se.value.ci.indirection,
				se.value.ci.dim, se.value.ci.nelem);
			break;
		case T_VECTOR:
			sprintf(s, PTR_FMT" %d %d",
				se.value.vi.vec,
				se.value.vi.vec->dimen,
				se.value.vi.n);
			break;
		case T_TBLCOL:
			sprintf(s, PTR_FMT" %d (%s) %d",
				se.value.tci.array,
				se.value.tci.dim,
				show_type(s1, se.value.tci.type),
				se.value.tci.pos);
			break;
		default:
			sprintf(s, "UNKNOWN CURSOR TYPE");
			break;
		}
		break;
	case HANDLE2:
		switch (se.type)
		{
		case T_OBJECT:
			sprintf(s, PTR_FMT" %s",
				se.value.han.ptr,
				shNameGetFromType(se.value.han.type));
			break;
		case T_SCHEMA:
			sprintf(s, PTR_FMT" %s %d %d %s",
				se.value.hi.ptr,
				shNameGetFromType(se.value.hi.type),
				se.value.hi.indirection,
				se.value.hi.dim,
				(se.value.hi.dim)?se.value.hi.nelem:"NULL");
			break;
		case T_CONTINUE:
		case T_CONT_PERIOD:
		case T_CONT_ARROW:
			sprintf(s, "%s %d %ld %d %s",
				shNameGetFromType(se.value.chan.type),
				se.value.chan.indirection,
				se.value.chan.offset,
				se.value.chan.dim, se.value.chan.nelem);
			break;
		case T_VECTOR:
			sprintf(s, PTR_FMT" %d",
				se.value.vi.vec, se.value.vi.n);
			break;
		case T_TBLCOL:
			sprintf(s, PTR_FMT" %d (%s) %d",
				se.value.tci.array,
				se.value.tci.dim,
				show_type(s1, se.value.tci.type),
				se.value.tci.pos);
			break;
/*
		case T_OBJECT:
			sprintf(s, "C++ Object");
			break;
*/
		default:
			sprintf(s, "UNKNOWN HANDLE TYPE");
		}
		break;
	default:
		strcpy(s, "N/A");
		break;
	}

	return (s);
}

/* p_shStdSize(type) -- standard size of the type */

int p_shStdSize(S_TYPE type)
{
	switch(type)
	{
	case T_DONTCARE:
		return(0);
	case T_CHAR:
		return(sizeof(char));
	case T_INT:
		return(sizeof(int));
	case T_SHORT:
		return(sizeof(short));
	case T_LONG:
		return(sizeof(long));
	case T_FLOAT:
		return(sizeof(float));
	case T_DOUBLE:
		return(sizeof(double));
	case T_U_CHAR:
		return(sizeof(unsigned char));
	case T_U_INT:
		return(sizeof(unsigned int));
	case T_U_SHORT:
		return(sizeof(unsigned short));
	case T_U_LONG:
		return(sizeof(unsigned long));
	case T_CHAR_P:
		return(sizeof(char *));
	case T_INT_P:
		return(sizeof(int *));
	case T_SHORT_P:
		return(sizeof(short *));
	case T_LONG_P:
		return(sizeof(long *));
	case T_FLOAT_P:
		return(sizeof(float *));
	case T_DOUBLE_P:
		return(sizeof(double *));
	case T_U_CHAR_P:
		return(sizeof(unsigned char *));
	case T_U_INT_P:
		return(sizeof(unsigned int *));
	case T_U_SHORT_P:
		return(sizeof(unsigned short *));
	case T_U_LONG_P:
		return(sizeof(unsigned long *));
	case T_CHAIN:
		return(sizeof(CHAIN_INFO));
	case T_VECTOR:
		return(sizeof(VECTOR_INFO));
	case T_SCHEMA:
		return(sizeof(HANDLE_INFO));
	case T_CONTINUE:
	case T_CONT_PERIOD:
	case T_CONT_ARROW:
		return(sizeof(CHANDLE));
	default:
		return(0);
	}
}

/* dump_eval_stack() -- dump whole evaluation stack */

static int dump_eval_stack(void)
{
	int i;
	char s[1024];

	printf("\nEvaluation Stack:\n");
	printf(" Seq       Class           Type      Size Value\n");
	printf("============================================================================\n");
	for (i = 0; i < eval_idx; i++)
	{
		printf("[%03d] : %s\n", i, show_entry(s, eval_stack[i]));
	}
	printf("============================================================================\n\n");
	return(0);
}

/* dump_code_store() -- dump whole code store */

int dump_code_store(void)
{
	int i;
	char s[1024], s1[64];

	printf("\nCode Store:\n");
	printf(" Seq      Opcode    Class           Type      Size Value\n");
	printf("============================================================================\n");
	for (i = 0; i < code_idx; i++)
	{
		printf("[%03d] : %8s %s\n", i,
			show_code(s1, code[i].opcode),
			show_entry(s, code[i].operand));
	}
	printf("============================================================================\n\n");
	return(0);
}

char *show_code(char *s, OPCODE c)
{
	switch(c)
	{
	case PUSH:
		strcpy(s, "PUSH");
		break;
	case PUSHV:
		strcpy(s, "PUSHV");
		break;
	case POP:
		strcpy(s, "POP");
		break;
	case IGNORE:
		strcpy(s, "IGNORE");
		break;
	case EXEC:
		strcpy(s, "EXEC");
		break;
	case ADD:
		strcpy(s, "+");
		break;
	case SUB:
		strcpy(s, "-");
		break;
	case MUL:
		strcpy(s, "*");
		break;
	case DIV:
		strcpy(s, "/");
		break;
	case NEG:
		strcpy(s, "(neg -)");
		break;
	case STOP:
		strcpy(s, "STOP");
		break;
	case MDT:
		strcpy(s, "MDT");
		break;
	case LIT:
		strcpy(s, "LIT");
		break;
	case ASSIGN:
		strcpy(s, "ASSIGN");
		break;
	case FETCH:
		strcpy(s, "FETCH");
		break;
	case RETURN:
		strcpy(s, "RETURN");
		break;
	case STD:
		strcpy(s, "STD");
		break;
	case SIN:
		strcpy(s, "SIN");
		break;
	case COS:
		strcpy(s, "COS");
		break;
	case TAN:
		strcpy(s, "TAN");
		break;
	case ASIN:
		strcpy(s, "ASIN");
		break;
	case ACOS:
		strcpy(s, "ACOS");
		break;
	case ATAN:
		strcpy(s, "ATAN");
		break;
	case LOG:
		strcpy(s, "LOG");
		break;
	case EXP:
		strcpy(s, "EXP");
		break;
	case POW:
		strcpy(s, "POW");
		break;
	case ABS:
		strcpy(s, "ABS");
		break;
	case ATAN2:
		strcpy(s, "ATAN2");
		break;
	case LOG10:
		strcpy(s, "LOG10");
		break;
	case SQRT:
		strcpy(s, "SQRT");
		break;
	case MIN:
		strcpy(s, "MIN");
		break;
	case MAX:
		strcpy(s, "MAX");
		break;
	case LC:
		strcpy(s, "LC");
		break;
	case STRCPY:
		strcpy(s, "strcpy");
		break;
	case STRNCPY:
		strcpy(s, "strncpy");
		break;
	case STRCAT:
		strcpy(s, "strcat");
		break;
	case STRNCAT:
		strcpy(s, "strncat");
		break;
	case STRCMP:
		strcpy(s, "strcmp");
		break;
	case STRNCMP:
		strcpy(s, "strncmp");
		break;
	case STRCASECMP:
		strcpy(s, "strcasecmp");
		break;
	case STRNCASECMP:
		strcpy(s, "strncasecmp");
		break;
	case STRLEN:
		strcpy(s, "strlen");
		break;
	case STROFFSET:
		strcpy(s, "stroffset");
		break;
	case LAND:
		strcpy(s, "&&");
		break;
	case LOR:
		strcpy(s, "||");
		break;
	case LNOT:
		strcpy(s, "!");
		break;
	case EQ:
		strcpy(s, "==");
		break;
	case GT:
		strcpy(s, ">");
		break;
	case LT:
		strcpy(s, "<");
		break;
	case GE:
		strcpy(s, ">=");
		break;
	case LE:
		strcpy(s, "<=");
		break;
	case NE:
		strcpy(s, "!=");
		break;
	case BAND:
		strcpy(s, "&");
		break;
	case BOR:
		strcpy(s, "|");
		break;
	case BXOR:
		strcpy(s, "^");
		break;
	case SL:
		strcpy(s, "<<");
		break;
	case SR:
		strcpy(s, ">>");
		break;
	case SKIP0:
		strcpy(s, "SKIPon0");
		break;
	case SKIP:
		strcpy(s, "SKIP");
		break;
	case CONTINUE:
		strcpy(s, "continue");
		break;
	case BREAK:
		strcpy(s, "break");
		break;
	default:
		strcpy(s, "UNKNOWN");
		break;
	}

	return(s);
}

/* stack operations */

/* push(el) -- push el onto evaluation stack */

static int push(DATA_ELEMENT el)
{
#ifdef SEDEBUG
	char s [1024];
	if (g_debug) printf("push: %s\n", show_entry(s, el));
#endif
	if (eval_idx > MAX_STACK_DEPTH)
	{
		warn("evaluation stack overflow");
		return(1);
	}
	else
	{
		eval_stack[eval_idx++] = el;
	}
	return(0);
}

/* pop() -- pop the top off the stack */

static DATA_ELEMENT pop(void)
{
#ifdef SEDEBUG
	char s[1024];
#endif
	if (eval_idx <= 0)
	{
		warn("evaluation stack underflow");
		return(ERROR_DATA);	/* error */
	}
	else
	{
#ifdef SEDEBUG
	if (g_debug) printf(" pop: %s\n", show_entry(s, eval_stack[eval_idx - 1]));
#endif
		return(eval_stack[--eval_idx]);
	}
}

/* top() -- peek on the top of the stack without removing it */

static DATA_ELEMENT top(void)
{
	if (eval_idx <= 0)
	{
		warn("peek on empty stack");
		return(ERROR_DATA);	/* error */
	}
	else
	{
		return(eval_stack[eval_idx-1]);
	}
}

/* built-in evaluation routines */

/* The complication comes from the type promotion in mixed type
 * operation. The implementation here leaves all type promotions
 * to the (C) compiler. They should follow the rules described in
 * the language's manual.
 */

static int built_in_arith(OPCODE op)
{
	DATA_ELEMENT op1, op2, result;

	if (op == LC)	/* no argument for LC */
	{
		return(push(f_lc()));
	}

	/* remember it is in the stack !!		*/
	/* therefore, the second operand is poped first */

	op2 = get_value(pop());	/* translate the value */

	if (op2.d_class == DUMMY) return(1); /* error */

#ifdef SEDEBUG
	{	char s[1024];

		if (g_debug)
		{
			printf("op2 = %s\n", show_entry(s, op2));
		}
	}
#endif

	result.size = op2.size;

	switch (op)
	{
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case POW:
	case ATAN2:
	case MIN:
	case MAX:
		op1 = get_value(pop()); /* translate the value */
		if (op1.d_class == DUMMY) return(1); /* error */
		result.size = (op2.size < op1.size)?op1.size:op2.size;

#ifdef SEDEBUG
	{	char s[1024];

		if (g_debug)
		{
			printf("op1 = %s\n", show_entry(s, op1));
		}
	}
#endif

	}

	switch(op)
	{
	case ADD:
		result = add(op1, op2);
		break;
	case SUB:
		result = sub(op1, op2);
		break;
	case MUL:
		result = mul(op1, op2);
		break;
	case DIV:
		result = divide(op1, op2);
		break;
	case NEG:
		result = neg(op2);
		break;
	case SIN:
		result = f_sin(op2);
		break;
	case COS:
		result = f_cos(op2);
		break;
	case TAN:
		result = f_tan(op2);
		break;
	case ASIN:
		result = f_asin(op2);
		break;
	case ACOS:
		result = f_acos(op2);
		break;
	case ATAN:
		result = f_atan(op2);
		break;
	case EXP:
		result = f_exp(op2);
		break;
	case LOG:
		result = f_log(op2);
		break;
	case POW:
		result = f_pow(op1, op2);
		break;
	case ABS:
		result = f_abs(op2);
		break;
	case ATAN2:
		result = f_atan2(op1, op2);
		break;
	case LOG10:
		result = f_log10(op2);
		break;
	case SQRT:
		result = f_sqrt(op2);
		break;
	case MIN:
		result = f_min(op1, op2);
		break;
	case MAX:
		result = f_max(op1, op2);
		break;
	default:
		warn("Illegal operator");
		return(1);
	}

	result.size = p_shStdSize(result.type);
#ifdef SEDEBUG
	{	char s[1024];

		if (g_debug)
		{
			printf("result = %s\n", show_entry(s, result));
		}
	}
#endif

	if (result.d_class == DUMMY) return(1); /* error */

	return(push(result));
}

/* built_in_string(op) -- built-in string functions
 *	These functions minic their counter part in C standard library
 */

static int built_in_string(OPCODE op)
{
	DATA_ELEMENT op1, op2, op3;
	DATA_ELEMENT result;

	switch (op)
	{
	case STRCPY:
		op2 = get_value(pop());
		op1 = get_value(pop());
		strcpy(op1.value.str, op2.value.str);
		return(push(op1));
	case STRNCPY:
		op3 = get_value(pop());
		op2 = get_value(pop());
		op1 = get_value(pop());
		if (adjust_type(&op3, T_INT))
		{
			warn("wrong argument type for strncpy");
			return(1);
		}
		strncpy(op1.value.str, op2.value.str, (size_t) op3.value.i);
		return(push(op1));
	case STRCAT:
		op2 = get_value(pop());
		op1 = get_value(pop());
		strcat(op1.value.str, op2.value.str);
		return(push(op1));
	case STRNCAT:
		op3 = get_value(pop());
		op2 = get_value(pop());
		op1 = get_value(pop());
		if(adjust_type(&op3, T_INT))
		{
			warn("wrong argument type for strncat");
			return(1);
		}
		strncat(op1.value.str, op2.value.str, (size_t) op3.value.i);
		return(push(op1));
	case STRCMP:
		op2 = get_value(pop());
		op1 = get_value(pop());
		result.d_class = LITERAL;
		result.type = T_INT;
		result.size = p_shStdSize(T_INT);
		result.ind = 0;
		result.value.i = strcmp(op1.value.str, op2.value.str);
		return(push(result));
	case STRNCMP:
		op3 = get_value(pop());
		op2 = get_value(pop());
		op1 = get_value(pop());
		if(adjust_type(&op3, T_INT))
		{
			warn("wrong argument type for strncmp");
			return(1);
		}
		result.d_class = LITERAL;
		result.type = T_INT;
		result.size = p_shStdSize(T_INT);
		result.ind = 0;
		result.value.i = strncmp(op1.value.str, op2.value.str,
			(size_t) op3.value.i);
		return(push(result));
	case STRCASECMP:
		op2 = get_value(pop());
		op1 = get_value(pop());
		result.d_class = LITERAL;
		result.type = T_INT;
		result.size = p_shStdSize(T_INT);
		result.ind = 0;
		result.value.i = strcasecmp(op1.value.str, op2.value.str);
		return(push(result));
	case STRNCASECMP:
		op3 = get_value(pop());
		op2 = get_value(pop());
		op1 = get_value(pop());
		if(adjust_type(&op3, T_INT))
		{
			warn("wrong argument type for strncasecmp");
			return(1);
		}
		result.d_class = LITERAL;
		result.type = T_INT;
		result.size = p_shStdSize(T_INT);
		result.ind = 0;
		result.value.i = strncasecmp(op1.value.str, op2.value.str,
			(size_t) op3.value.i);
		return(push(result));
	case STRLEN:
		op1 = get_value(pop());
		result.d_class = LITERAL;
		result.type = T_INT;
		result.size = p_shStdSize(T_INT);
		result.ind = 0;
		result.value.i = strlen(op1.value.str);
		return(push(result));
	case STROFFSET:
		op2 = get_value(pop());
		op1 = get_value(pop());
		if (adjust_type(&op2, T_INT))
		{
			warn("wrong argument type for stroffset");
			return(1);
		}
		/* check overflow here */
		if (op1.size < op2.value.i)
		{
			op1.value.cp += op1.size;
			op1.size = 0;
		}
		else
		{
			op1.value.cp += op2.value.i;
			op1.size -= op2.value.i;
		}
		return(push(op1));
	default:
		warn("unknown string function");
		return(1);
	}
}

/* built_in_logic(op) -- logical operations */

static int built_in_logic(OPCODE op)
{
	DATA_ELEMENT DTRUE = {LITERAL, T_INT, 0, sizeof(int), {1}};
	DATA_ELEMENT DFALSE = {LITERAL, T_INT, 0, sizeof(int), {0}};
	DATA_ELEMENT op1, op2;

	op2 = get_value(pop());

	if (op == LNOT)
	{
		if (is_true(op2))
		{
			return(push(DFALSE));
		}
		else
		{
			return(push(DTRUE));
		}
	}

	op1 = get_value(pop());

	if (adjust_type(&op2, T_DOUBLE) || adjust_type(&op1, T_DOUBLE))
	{
		warn("wrong logical types");
		return(1);
	}

	if (op == LAND)
	{
		if (is_true(op1) && is_true(op2))
		{
			return(push(DTRUE));
		}
		else
		{
			return(push(DFALSE));
		}
	}
	else if (op == LOR)
	{
		if (is_true(op1) || is_true(op2))
		{
			return(push(DTRUE));
		}
		else
		{
			return(push(DFALSE));
		}
	}

	switch (op)
	{
	case EQ:
		if (op1.value.d == op2.value.d)
		{
			return(push(DTRUE));
		}
		else
		{
			return(push(DFALSE));
		}
	case GT:
		if (op1.value.d > op2.value.d)
		{
			return(push(DTRUE));
		}
		else
		{
			return(push(DFALSE));
		}
	case GE:
		if (op1.value.d >= op2.value.d)
		{
			return(push(DTRUE));
		}
		else
		{
			return(push(DFALSE));
		}
	case LT:
		if (op1.value.d < op2.value.d)
		{
			return(push(DTRUE));
		}
		else
		{
			return(push(DFALSE));
		}
	case LE:
		if (op1.value.d <= op2.value.d)
		{
			return(push(DTRUE));
		}
		else
		{
			return(push(DFALSE));
		}
	case NE:
		if (op1.value.d != op2.value.d)
		{
			return(push(DTRUE));
		}
		else
		{
			return(push(DFALSE));
		}
	default:
		warn("unknown logical operation");
		return(1);
	}
}

/* is_true(DATA_ELEMENT) -- none zero value, for all types */

static int is_true(DATA_ELEMENT d)
{
	switch (d.type)
	{
	case T_FLOAT:
		return(d.value.f != 0.0);
	case T_DOUBLE:
		return(d.value.d != 0.0);
	case T_INT:
		return(d.value.i != 0);
	case T_CHAR:
		return(d.value.c != 0);
	case T_LONG:
		return(d.value.l != 0);
	case T_SHORT:
		return(d.value.s != 0);
	case T_U_SHORT:
		return(d.value.us != 0);
	case T_U_LONG:
		return(d.value.ul != 0);
	case T_U_CHAR:
		return(d.value.uc != 0);
	case T_CHAR_P:
	case T_INT_P:
	case T_LONG_P:
	case T_SHORT_P:
	case T_FLOAT_P:
	case T_DOUBLE_P:
	case T_U_CHAR_P:
	case T_U_INT_P:
	case T_U_LONG_P:
	case T_U_SHORT_P:
		return(d.value.gp != NULL);
	default:
		warn("Illegal logic type");
		return(0);	/* assumed false */
	}
}

/* built_in_bitwise(op) -- only works for integral numbers */

static int built_in_bitwise(OPCODE op)
{
	DATA_ELEMENT op1, op2, result;
	long wp1, wp2;

	op2 = get_value(pop());
	op1 = get_value(pop());

	wp1 = wp2 = 0;	/* clearing working operands */

	memcpy(&wp1, &(op1.value.l), op1.size);
	memcpy(&wp2, &(op2.value.l), op2.size);

	switch (op)
	{
	case BOR:
		result.value.l = wp1 | wp2;
		break;
	case BAND:
		result.value.l = wp1 & wp2;
		break;
	case BXOR:
		result.value.l = wp1 ^ wp2;
		break;
	default:
		warn("unknown bitwise operator");
		return(1);
	}

	if (op2.size > op1.size)
	{
		result.size = op2.size;
		result.type = op2.type;
	}
	else
	{
		result.size = op1.size;
		result.type = op1.type;
	}

	result.d_class = LITERAL;
	result.ind = 0;

	push(result);
	return(0);
}

/* built_in_shift(op) -- only works for integral numbers */

static int built_in_shift(OPCODE op)
{
	DATA_ELEMENT op1, op2;
	long wp;

	op2 = get_value(pop());
	op1 = get_value(pop());

	if (adjust_type(&op2, T_INT))
	{
		warn("bit-offset in shift operation must be a number");
		return(1);
	}

	wp = 0;	/* clearing working operand */

	memcpy(&wp, &(op1.value.l), op1.size);

	switch (op)
	{
	case SR:
		op1.value.l = wp >> op2.value.i;
		break;
	case SL:
		op1.value.l = wp << op2.value.i;
		break;
	default:
		warn("unknown shift operator");
		return(1);
	}

	push(op1);
	return(0);
}

/* get_value(v) -- get the value of v by all means */

static DATA_ELEMENT get_value(DATA_ELEMENT v)
{
	DATA_ELEMENT s;
	char s1[512], s2[512];

	switch (v.d_class)
	{
	case LITERAL:
	case DUMMY:
		return(v);
/*
	case DATA_MEMBER:
		s = get_data_member_value(v.value.dm.path,
			v.value.dm.field_name);
		if (s.d_class == DUMMY)
		{
			sprintf(s1, "No such data member \"%s\"",
				show_value(s2, v));
			warn(s1);
			return(ERROR_DATA);
		}
		break;
*/
	case CURSOR2:
		warn("CURSOR can't appear in evaluation stack!");
		return(ERROR_DATA);
	case HANDLE2:
		if (v.type == T_VECTOR)
		{
			/* only double */

			s.d_class = LITERAL;
			s.type = T_DOUBLE;
			s.value.d = v.value.vi.vec->vec[v.value.vi.n];
			s.size = p_shStdSize(T_DOUBLE);
			return(s);
		}
		else if (v.type == T_TBLCOL)
		{
			/* tblcol handle */

			return(get_value_from_tblcol(v.value.tci.array,
				v.value.tci.pos, v.value.tci.dim));
		}
		else if (v.type == T_OBJECT)
		{
		        return(v);
		}
		else
		{
			return(get_value_from_handle(v));
		}
	default:
		sprintf(s1, "Don't know how to get_value(%s)", show_entry(s2, v));
		warn(s1);
		return(ERROR_DATA);
	}
/*
	if (adjust_type(&s, v.type)) return(ERROR_DATA);
	return (s);
*/
}

/* get_address(v) -- get the address of v by all means, the result
 *			is stored in a DATA_ELEMENT which is a literal
 *			with a proper pointer type
 */

static DATA_ELEMENT get_address(DATA_ELEMENT v)
{
	DATA_ELEMENT s;
	char s1[512], s2[512];

	switch (v.d_class)
	{
	case LITERAL:
		if (v.type == T_STRING)
		{
			return(v);
		}
		else
		{
			warn("value can not be assigned to a literal");
			return(ERROR_DATA);
		}
	case CURSOR2:
		warn("CURSOR can't appear in evaluation stack!");
		return(ERROR_DATA);
	case HANDLE2:
		if (v.type == T_VECTOR)
		{
			s.d_class = LITERAL;
			s.type = T_DOUBLE;
			s.ind = 1;
			s.size = sizeof(double *);
			s.value.cp = (char *) &(v.value.vi.vec->vec[v.value.vi.n]);
			return(s);
		}
		else if (v.type == T_TBLCOL)
		{
			s.d_class = LITERAL;
			s.type = v.value.tci.type;
			s.ind = 1;
			s.size = sizeof(void *);
			s.value.cp = (char *) get_address_from_tblcol(
						v.value.tci.array,
						v.value.tci.pos,
						v.value.tci.dim);
			return(s);
		}
		else
		{
			s = get_address_from_handle(v);
		}
		break;
	default:
		sprintf(s1, "Don't know how to get_address(%s)", show_entry(s2, v));
		warn(s1);
		return(ERROR_DATA);
	}
/*
	if (adjust_type(&s, v.type)) return(ERROR_DATA);
*/
	return (s);
}


/* argument store for external function call */

static DATA_ELEMENT g_argument[100];

/* invoke function */

/* do_function(fn, type, persistency) -- execute function fn, the
 * expected return type is in "type"
 */

static int do_function(FUNCTION_INFO fn, int persistent)
{
	int i, n;
	DATA_ELEMENT result;

	n = fn.argc;	/* get the number of arguments */

	for (i = 0; i < n; i++)	/* prepare argument store */
	{
	  /* Replaced pop by get_value(pop()) because of change
	     of 'semantic' of the data element -- This does not
	     guarantee that this function is totally up to date !!
	     it still seem to fail with chains arguments!!*/
		g_argument[n-i-1] = get_value(pop());
		if (g_argument[n-i-1].d_class == DUMMY)
			return(1);
	}

	/* execute the constructor, and push the new object on the stack */
	result = (*seCallConstructor)(fn.fid, fn.argc, g_argument, fn.returnType,
			fn.clusteringDirective.ptr, persistent);
	if (result.d_class == DUMMY) return 1;
	return(push(result));
}

/* do_method(md) -- execute method md */

static int do_method(MMETHOD md)
{
	int i, n;
	DATA_ELEMENT result;

	n = md.argc;	/* get the number of arguments */

	for (i = 0; i < n; i++)	/* prepare argument store */
	{
	  /* Replaced pop by get_value(pop()) because of change
	     of 'semantic' of the data element -- This does not
	     guarantee that this function is totally up to date !!*/
		g_argument[n-i-1] = get_value(pop());
		if (g_argument[n-i-1].d_class == DUMMY)
			return(1);
	}

	/* get object to execute method on */
	result = get_value(pop());
	if (result.d_class != HANDLE2 || result.type != T_OBJECT)
	{
		return(1);
	}

	/* execute the method */
	result = (*seCallMethod)(result.value.han, md.mid, md.argc, g_argument);
	if (result.d_class == DUMMY) return 1;

	/* if there is a return value, push it on the operand stack */
	if (result.type != T_DONTCARE)
	  return(push(result));
	else
	  return(0);
}

/* return_top() -- return the top value in the interp string */
static int return_top(void)
{
  DATA_ELEMENT d;
  char s[100];

  d = get_value(pop());
  if (d.type == T_OBJECT)
    {
      Tcl_AppendElement(g_seInterp,
			(char*) (*seGetTclHandle)(g_seInterp, d.value.han));
    }
  else if (d.type == T_SCHEMA)
    {
      /* KLUDGE: This should not rely on SDB for a SCHEMA, but for now
	 lets go with it */
      Tcl_AppendElement(g_seInterp,
			(char*) (*seGetTclHandle)(g_seInterp, d.value.han));
    }
  else
    {
      Tcl_AppendElement(g_seInterp, show_value(s, d));
    }
  return 0;
}

/* get_array_elem(array, type) -- get the value of an ARRAY element */

static int get_array_elem(ARRAY* array, S_TYPE type)
{
  DATA_ELEMENT result;
  void* dataPtr;

  /* fetch pointer to data */
  if ((dataPtr = get_array_elem_ptr(array)) == NULL) return 1;

  /* set result */
  result.d_class = LITERAL;
  result.type = type;
  switch (type)
    {
    case T_SHORT:
      result.value.s = * (short*)dataPtr;
      break;
    case T_U_SHORT:
      result.value.us = * (unsigned short*)dataPtr;
      break;
    case T_INT:
      result.value.i = * (int*)dataPtr;
      break;
    case T_U_INT:
      result.value.ui = * (unsigned int*)dataPtr;
      break;
    case T_LONG:
      result.value.l = * (long*)dataPtr;
      break;
    case T_U_LONG:
      result.value.ul = * (unsigned long*)dataPtr;
      break;
    case T_FLOAT:
      result.value.f = * (float*)dataPtr;
      break;
    case T_DOUBLE:
      result.value.d = * (double*)dataPtr;
      break;
    default:
      return 1;
    }

  /* push result on the stack */
  return(push(result));
}

/* set_array_elem(array) -- set the value of an ARRAY element */

static int set_array_elem(ARRAY* array, S_TYPE type)
{
  DATA_ELEMENT result;
  void* dataPtr;

  /* get value to stuff */
  result = get_value(pop());

  /* fetch pointer to data */
  if ((dataPtr = get_array_elem_ptr(array)) == NULL) return 1;

  /* adjust its type */
  if (adjust_type(&result, type))
    return 1;

  /* set result */
  switch (type)
    {
    case T_SHORT:
      *(short*)dataPtr = result.value.s;
      break;
    case T_U_SHORT:
      *(unsigned short*)dataPtr = result.value.us;
      break;
    case T_INT:
      *(int*)dataPtr = result.value.i;
      break;
    case T_U_INT:
      *(unsigned int*)dataPtr = result.value.ui;
      break;
    case T_LONG:
      *(long*)dataPtr = result.value.l;
      break;
    case T_U_LONG:
      *(unsigned long*)dataPtr = result.value.ul;
      break;
    case T_FLOAT:
      *(float*)dataPtr = result.value.f;
      break;
    case T_DOUBLE:
      *(double*)dataPtr = result.value.d;
      break;
    default:
      return 1;
    }

  /* return OK */
  return 0;
}

/* fetch the data pointer to an element in an array, fetching the indexes
   to the element off of the operand stack */

static void* get_array_elem_ptr(ARRAY* array)
{
  int i, n, offset;
  int idx[shArrayDimLim];
	DATA_ELEMENT d;

  /* get the number of indices */
  n = array->dimCnt;

  /* fetch indices */
  for (i = 0; i < n; i++)
    {
      d = get_value(pop());
      if (d.d_class != LITERAL)
	{
	  shErrStackPush("get_array_elem_ptr: non-literal as index");
	  return NULL;
	}
      switch (d.type)
	{
	case T_SHORT:
	  idx[n-i-1] = d.value.s;
	  break;
	case T_INT:
	  idx[n-i-1] = d.value.i;
	  break;
	case T_LONG:
	  idx[n-i-1] = (int) d.value.l;
	  break;
	case T_U_SHORT:
	  idx[n-i-1] = d.value.us;
	  break;
	case T_U_INT:
	  idx[n-i-1] = d.value.ui;
	  break;
	case T_U_LONG:
	  idx[n-i-1] = (int) d.value.ul;
	  break;
	default:
	  return NULL;
	}
      /* check that index is in legal range */
      if (idx[n-i-1] < 0)
	{
	  shErrStackPush("negative index");
	  return NULL;
	}
      if (idx[n-i-1] >= array->dim[n-i-1])
	{
	  shErrStackPush("index (%d) outside allowed range (0-%d)",
			 idx[n-i-1], array->dim[n-i-1]-1);
	  return NULL;
	}
    }

  /* calculate offset in elements */
  offset = idx[n-1];
  for (i = n - 2; i >= 0; i--)
    offset += (int)array->subArrCnt[i] * idx[i];

  /* calculate pointer to data */
  return array->data.dataPtr + offset * array->data.incr;
}

/* adjust_type -- type conversion between source and expected types
 * This only works for simple types. Type conversion is done though
 * the casting as defined in the language
 */

static int adjust_type(DATA_ELEMENT *sp, S_TYPE type)
{
	/* special treatment for HANDLES, which are "pointers".
	 * they could be casted into any pointer type ...
	 */

	if ((sp->d_class == HANDLE2) && (sp->type == T_SCHEMA))
	{
		/* yes. I know how to do it. */

		if ((sp->value.hi.nelem == NULL) && (sp->value.hi.indirection == 0))
		{
			warn("adjust_type(): the handle is not a pointer");
			return(1);
		}

		if (!is_ptr_stype(type))
		{
			warn("adjust_type(): Converting handle to none pointer type is not allowed. You have to pay $100 application fee first");
			return(1);
		}

		/* no further question */

		sp->d_class = LITERAL;
		sp->value.cp = (char *) sp->value.hi.ptr;
		sp->type = type;
		sp->size = p_shStdSize(sp->type);
		sp->ind = 0;
		return(0);
	}

	if (sp->d_class != LITERAL)
	{
		warn("none literal for type adjustment");
		return(1);
	}

	if ((type == T_DONTCARE) || (type == sp->type)) /* happy ending */
		return(0);

	switch (sp->type)
	{
	case T_DONTCARE:
		break;
	case T_CHAR:
		switch(type)
		{
		case T_INT:
			sp->value.i = (int) sp->value.c;
			break;
		case T_SHORT:
			sp->value.s = (short) sp->value.c;
			break;
		case T_LONG:
			sp->value.l = (long) sp->value.c;
			break;
		case T_FLOAT:
			sp->value.f = (float) sp->value.c;
			break;
		case T_DOUBLE:
			sp->value.d = (double) sp->value.c;
			break;
		case T_U_CHAR:
			sp->value.uc = (unsigned char) sp->value.c;
			break;
		case T_U_INT:
			sp->value.ui = (unsigned int) sp->value.c;
			break;
		case T_U_SHORT:
			sp->value.us = (unsigned short) sp->value.c;
			break;
		case T_U_LONG:
			sp->value.ul = (unsigned long) sp->value.c;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	case T_INT:
		switch(type)
		{
		case T_CHAR:
			sp->value.c = (char) sp->value.i;
			break;
		case T_SHORT:
			sp->value.s = (short) sp->value.i;
			break;
		case T_LONG:
			sp->value.l = (long) sp->value.i;
			break;
		case T_FLOAT:
			sp->value.f = (float) sp->value.i;
			break;
		case T_DOUBLE:
			sp->value.d = (double) sp->value.i;
			break;
		case T_U_CHAR:
			sp->value.uc = (unsigned char) sp->value.i;
			break;
		case T_U_INT:
			sp->value.ui = (unsigned int) sp->value.i;
			break;
		case T_U_SHORT:
			sp->value.us = (unsigned short) sp->value.i;
			break;
		case T_U_LONG:
			sp->value.ul = (unsigned long) sp->value.i;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	case T_SHORT:
		switch(type)
		{
		case T_CHAR:
			sp->value.c = (char) sp->value.s;
			break;
		case T_INT:
			sp->value.i = (int) sp->value.s;
			break;
		case T_LONG:
			sp->value.l = (long) sp->value.s;
			break;
		case T_FLOAT:
			sp->value.f = (float) sp->value.s;
			break;
		case T_DOUBLE:
			sp->value.d = (double) sp->value.s;
			break;
		case T_U_CHAR:
			sp->value.uc = (unsigned char) sp->value.s;
			break;
		case T_U_INT:
			sp->value.ui = (unsigned int) sp->value.s;
			break;
		case T_U_SHORT:
			sp->value.us = (unsigned short) sp->value.s;
			break;
		case T_U_LONG:
			sp->value.ul = (unsigned long) sp->value.s;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	case T_LONG:
		switch(type)
		{
		case T_CHAR:
			sp->value.c = (char) sp->value.l;
			break;
		case T_INT:
			sp->value.i = (int) sp->value.l;
			break;
		case T_SHORT:
			sp->value.s = (short) sp->value.l;
			break;
		case T_FLOAT:
			sp->value.f = (float) sp->value.l;
			break;
		case T_DOUBLE:
			sp->value.d = (double) sp->value.l;
			break;
		case T_U_CHAR:
			sp->value.uc = (unsigned char) sp->value.l;
			break;
		case T_U_INT:
			sp->value.ui = (unsigned int) sp->value.l;
			break;
		case T_U_SHORT:
			sp->value.us = (unsigned short) sp->value.l;
			break;
		case T_U_LONG:
			sp->value.ul = (unsigned long) sp->value.l;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	case T_FLOAT:
		switch(type)
		{
		case T_CHAR:
			sp->value.c = (char) sp->value.f;
			break;
		case T_INT:
			sp->value.i = (int) sp->value.f;
			break;
		case T_SHORT:
			sp->value.s = (short) sp->value.f;
			break;
		case T_LONG:
			sp->value.l = (long) sp->value.f;
			break;
		case T_DOUBLE:
			sp->value.d = (double) sp->value.f;
			break;
		case T_U_CHAR:
			sp->value.uc = (unsigned char) sp->value.f;
			break;
		case T_U_INT:
			sp->value.ui = (unsigned int) sp->value.f;
			break;
		case T_U_SHORT:
			sp->value.us = (unsigned short) sp->value.f;
			break;
		case T_U_LONG:
			sp->value.ul = (unsigned long) sp->value.f;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	case T_DOUBLE:
		switch(type)
		{
		case T_CHAR:
			sp->value.c = (char) sp->value.d;
			break;
		case T_INT:
			sp->value.i = (int) sp->value.d;
			break;
		case T_SHORT:
			sp->value.s = (short) sp->value.d;
			break;
		case T_LONG:
			sp->value.l = (long) sp->value.d;
			break;
		case T_FLOAT:
			sp->value.f = (float) sp->value.d;
			break;
		case T_U_CHAR:
			sp->value.uc = (unsigned char) sp->value.d;
			break;
		case T_U_INT:
			sp->value.ui = (unsigned int) sp->value.d;
			break;
		case T_U_SHORT:
			sp->value.us = (unsigned short) sp->value.d;
			break;
		case T_U_LONG:
			sp->value.ul = (unsigned long) sp->value.d;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	case T_U_CHAR:
		switch(type)
		{
		case T_CHAR:
			sp->value.c = (char) sp->value.uc;
			break;
		case T_INT:
			sp->value.i = (int) sp->value.uc;
			break;
		case T_SHORT:
			sp->value.s = (short) sp->value.uc;
			break;
		case T_LONG:
			sp->value.l = (long) sp->value.uc;
			break;
		case T_FLOAT:
			sp->value.f = (float) sp->value.uc;
			break;
		case T_DOUBLE:
			sp->value.d = (double) sp->value.uc;
			break;
		case T_U_INT:
			sp->value.ui = (unsigned int) sp->value.uc;
			break;
		case T_U_SHORT:
			sp->value.us = (unsigned short) sp->value.uc;
			break;
		case T_U_LONG:
			sp->value.ul = (unsigned long) sp->value.uc;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	case T_U_INT:
		switch(type)
		{
		case T_CHAR:
			sp->value.c = (char) sp->value.ui;
			break;
		case T_INT:
			sp->value.i = (int) sp->value.ui;
			break;
		case T_SHORT:
			sp->value.s = (short) sp->value.ui;
			break;
		case T_LONG:
			sp->value.l = (long) sp->value.ui;
			break;
		case T_FLOAT:
			sp->value.f = (float) sp->value.ui;
			break;
		case T_DOUBLE:
			sp->value.d = (double) sp->value.ui;
			break;
		case T_U_CHAR:
			sp->value.uc = (unsigned char) sp->value.ui;
			break;
		case T_U_SHORT:
			sp->value.us = (unsigned short) sp->value.ui;
			break;
		case T_U_LONG:
			sp->value.ul = (unsigned long) sp->value.ui;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	case T_U_SHORT:
		switch(type)
		{
		case T_CHAR:
			sp->value.c = (char) sp->value.us;
			break;
		case T_INT:
			sp->value.i = (int) sp->value.us;
			break;
		case T_SHORT:
			sp->value.s = (short) sp->value.us;
			break;
		case T_LONG:
			sp->value.l = (long) sp->value.us;
			break;
		case T_FLOAT:
			sp->value.f = (float) sp->value.us;
			break;
		case T_DOUBLE:
			sp->value.d = (double) sp->value.us;
			break;
		case T_U_CHAR:
			sp->value.uc = (unsigned char) sp->value.us;
			break;
		case T_U_INT:
			sp->value.ui = (unsigned int) sp->value.us;
			break;
		case T_U_LONG:
			sp->value.ul = (unsigned long) sp->value.us;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	case T_U_LONG:
		switch(type)
		{
		case T_CHAR:
			sp->value.c = (char) sp->value.ul;
			break;
		case T_INT:
			sp->value.i = (int) sp->value.ul;
			break;
		case T_SHORT:
			sp->value.s = (short) sp->value.ul;
			break;
		case T_LONG:
			sp->value.l = (long) sp->value.ul;
			break;
		case T_FLOAT:
			sp->value.f = (float) sp->value.ul;
			break;
		case T_DOUBLE:
			sp->value.d = (double) sp->value.ul;
			break;
		case T_U_CHAR:
			sp->value.uc = (unsigned char) sp->value.ul;
			break;
		case T_U_INT:
			sp->value.ui = (unsigned int) sp->value.ul;
			break;
		case T_U_SHORT:
			sp->value.us = (unsigned short) sp->value.ul;
			break;
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;

	/* Lazy implementation of pointer types' conversion,
	 * All pointers are created equal!
	 */

	case T_CHAR_P:
	case T_INT_P:
	case T_SHORT_P:
	case T_LONG_P:
	case T_FLOAT_P:
	case T_DOUBLE_P:
	case T_U_CHAR_P:
	case T_U_INT_P:
	case T_U_SHORT_P:
	case T_U_LONG_P:
	case T_PTR:
		switch(type)
		{
		case T_CHAR_P:
		case T_INT_P:
		case T_SHORT_P:
		case T_LONG_P:
		case T_FLOAT_P:
		case T_DOUBLE_P:
		case T_U_CHAR_P:
		case T_U_INT_P:
		case T_U_SHORT_P:
		case T_U_LONG_P:
		case T_PTR:
			break;		/* do nothing */
		default:
			error_type_conversion(sp->type, type);
			return(1);
		}
		break;
	default:
		error_type_conversion(sp->type, type);
		return(1);
	}

	sp->type = type;
	return(0);
}

/* error_type_coversion(t1, t2) -- print error message */

static void error_type_conversion(S_TYPE t1, S_TYPE t2)
{
	char s1[64], s2[64];

	shErrStackPush("error> ugly type conversion (%12s) --> (%12s)\n",
		show_type(s1, t1), show_type(s2, t2));
}

/* add(op1, op2) -- op1 + op2, with type conversion */

static DATA_ELEMENT add(DATA_ELEMENT op1, DATA_ELEMENT op2)
{
	DATA_ELEMENT result;

	result.d_class = LITERAL;		/* it must be literal */

	switch(op1.type)
	{
	case T_CHAR:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.c + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.c + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.c + op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.c + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.c + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.c + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.c + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.c + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.c + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.c + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_INT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.i + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.i + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.i + op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.i + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.i + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.i + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.i + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.i + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.i + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.i + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_SHORT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.s + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.s + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.s + op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.s + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.s + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.s + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.s + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.s + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.s + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.s + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_LONG:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_LONG;
			result.value.l = op1.value.l + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.l + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.l + op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.l + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.l + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.l + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.l + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_LONG;
			result.value.l = op1.value.l + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.l + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.l + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_FLOAT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_FLOAT;
			result.value.f = op1.value.f + op2.value.c;
			break;
		case T_LONG:
			result.type = T_FLOAT;
			result.value.f = op1.value.f + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.f + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_FLOAT;
			result.value.f = op1.value.f + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_FLOAT;
			result.value.f = op1.value.f + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_DOUBLE:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.c;
			break;
		case T_LONG:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_CHAR:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.uc + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.uc + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.uc + op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.uc + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.uc + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.uc + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_CHAR;
			result.value.uc = op1.value.uc + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.uc + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_SHORT;
			result.value.us = op1.value.uc + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.uc + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_INT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.ui + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.ui + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.ui + op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.ui + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.ui + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.ui + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ui + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_SHORT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.us + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.us + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.us + op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.us + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.us + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.us + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_SHORT;
			result.value.us = op1.value.us + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.us + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_SHORT;
			result.value.us = op1.value.us + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.us + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_LONG:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_LONG;
			result.value.l = op1.value.ul + op2.value.i;
			break;
		case T_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.ul + op2.value.s;
			break;
		case T_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.ul + op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.ul + op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.ul + op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.ul + op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul + op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul + op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul + op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul + op2.value.ul;
			break;
		default:
			op_type_error("+", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	default:
		op_type_error("+", op1.type, op2.type);
		return(ERROR_DATA);
	}

	return(result);
}

/* sub(op1, op2) -- op1 - op2, with type conversion */

static DATA_ELEMENT sub(DATA_ELEMENT op1, DATA_ELEMENT op2)
{
	DATA_ELEMENT result;

	result.d_class = LITERAL;		/* it must be literal */

	switch(op1.type)
	{
	case T_CHAR:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.c - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.c - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.c - op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.c - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.c - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.c - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.c - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.c - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.c - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.c - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_INT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.i - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.i - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.i - op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.i - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.i - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.i - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.i - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.i - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.i - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.i - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_SHORT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.s - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.s - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.s - op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.s - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.s - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.s - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.s - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.s - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.s - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.s - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_LONG:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_LONG;
			result.value.l = op1.value.l - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.l - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.l - op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.l - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.l - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.l - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.l - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_LONG;
			result.value.l = op1.value.l - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.l - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.l - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_FLOAT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_FLOAT;
			result.value.f = op1.value.f - op2.value.c;
			break;
		case T_LONG:
			result.type = T_FLOAT;
			result.value.f = op1.value.f - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.f - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_FLOAT;
			result.value.f = op1.value.f - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_FLOAT;
			result.value.f = op1.value.f - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_DOUBLE:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.c;
			break;
		case T_LONG:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_CHAR:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.uc - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.uc - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.uc - op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.uc - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.uc - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.uc - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_CHAR;
			result.value.uc = op1.value.uc - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.uc - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_SHORT;
			result.value.us = op1.value.uc - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.uc - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_INT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.ui - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.ui - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.ui - op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.ui - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.ui - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.ui - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ui - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_SHORT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.us - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.us - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.us - op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.us - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.us - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.us - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_SHORT;
			result.value.us = op1.value.us - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.us - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_SHORT;
			result.value.us = op1.value.us - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.us - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_LONG:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_LONG;
			result.value.l = op1.value.ul - op2.value.i;
			break;
		case T_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.ul - op2.value.s;
			break;
		case T_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.ul - op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.ul - op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.ul - op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.ul - op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul - op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul - op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul - op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul - op2.value.ul;
			break;
		default:
			op_type_error("-", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	default:
		op_type_error("-", op1.type, op2.type);
		return(ERROR_DATA);
	}

	return(result);
}

/* mul(op1, op2) -- op1 * op2, with type conversion */

static DATA_ELEMENT mul(DATA_ELEMENT op1, DATA_ELEMENT op2)
{
	DATA_ELEMENT result;

	result.d_class = LITERAL;		/* it must be literal */

	switch(op1.type)
	{
	case T_CHAR:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.c * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.c * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.c * op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.c * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.c * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.c * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.c * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.c * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.c * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.c * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_INT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.i * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.i * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.i * op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.i * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.i * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.i * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.i * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.i * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.i * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.i * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_SHORT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.s * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.s * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.s * op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.s * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.s * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.s * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.s * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.s * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.s * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.s * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_LONG:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_LONG;
			result.value.l = op1.value.l * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.l * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.l * op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.l * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.l * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.l * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.l * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_LONG;
			result.value.l = op1.value.l * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.l * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.l * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_FLOAT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_FLOAT;
			result.value.f = op1.value.f * op2.value.c;
			break;
		case T_LONG:
			result.type = T_FLOAT;
			result.value.f = op1.value.f * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.f * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_FLOAT;
			result.value.f = op1.value.f * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_FLOAT;
			result.value.f = op1.value.f * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_DOUBLE:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.c;
			break;
		case T_LONG:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_CHAR:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.uc * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.uc * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.uc * op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.uc * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.uc * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.uc * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_CHAR;
			result.value.uc = op1.value.uc * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.uc * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_SHORT;
			result.value.us = op1.value.uc * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.uc * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_INT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.ui * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.ui * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.ui * op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.ui * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.ui * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.ui * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ui * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_SHORT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.us * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.us * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.us * op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.us * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.us * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.us * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_SHORT;
			result.value.us = op1.value.us * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.us * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_SHORT;
			result.value.us = op1.value.us * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.us * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_LONG:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_LONG;
			result.value.l = op1.value.ul * op2.value.i;
			break;
		case T_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.ul * op2.value.s;
			break;
		case T_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.ul * op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.ul * op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.ul * op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.ul * op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul * op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul * op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul * op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul * op2.value.ul;
			break;
		default:
			op_type_error("*", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	default:
		op_type_error("*", op1.type, op2.type);
		return(ERROR_DATA);
	}

	return(result);
}

/* divide(op1, op2) -- op1 / op2, with type conversion */

static DATA_ELEMENT divide(DATA_ELEMENT op1, DATA_ELEMENT op2)
{
	DATA_ELEMENT result;

	result.d_class = LITERAL;		/* it must be literal */

	switch(op1.type)
	{
	case T_CHAR:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.c / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.c / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.c / op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.c / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.c / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.c / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.c / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.c / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.c / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.c / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_INT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.i / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.i / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.i / op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.i / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.i / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.i / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.i / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.i / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.i / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.i / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_SHORT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.s / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.s / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.s / op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.s / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.s / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.s / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.s / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_INT;
			result.value.i = op1.value.s / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.s / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.s / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_LONG:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_LONG;
			result.value.l = op1.value.l / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.l / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.l / op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.l / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.l / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.l / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.l / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_LONG;
			result.value.l = op1.value.l / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.l / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.l / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_FLOAT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_FLOAT;
			result.value.f = op1.value.f / op2.value.c;
			break;
		case T_LONG:
			result.type = T_FLOAT;
			result.value.f = op1.value.f / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.f / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_FLOAT;
			result.value.f = op1.value.f / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_FLOAT;
			result.value.f = op1.value.f / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_FLOAT;
			result.value.f = op1.value.f / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_DOUBLE:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.c;
			break;
		case T_LONG:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_DOUBLE;
			result.value.d = op1.value.d / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_CHAR:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.uc / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.uc / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_CHAR;
			result.value.c = op1.value.uc / op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.uc / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.uc / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.uc / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_CHAR;
			result.value.uc = op1.value.uc / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.uc / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_SHORT;
			result.value.us = op1.value.uc / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.uc / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_INT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.ui / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_INT;
			result.value.i = op1.value.ui / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_INT;
			result.value.i = op1.value.ui / op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.ui / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.ui / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.ui / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_INT;
			result.value.ui = op1.value.ui / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ui / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_SHORT:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_INT;
			result.value.i = op1.value.us / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_SHORT;
			result.value.s = op1.value.us / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_SHORT;
			result.value.s = op1.value.us / op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.us / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.us / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.us / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_SHORT;
			result.value.us = op1.value.us / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_INT;
			result.value.ui = op1.value.us / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_SHORT;
			result.value.us = op1.value.us / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.us / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	case T_U_LONG:
		switch(op2.type)
		{
		case T_INT:
			result.type = T_LONG;
			result.value.l = op1.value.ul / op2.value.i;
			break;
		case T_SHORT:
			result.type = T_LONG;
			result.value.l = op1.value.ul / op2.value.s;
			break;
		case T_CHAR:
			result.type = T_LONG;
			result.value.l = op1.value.ul / op2.value.c;
			break;
		case T_LONG:
			result.type = T_LONG;
			result.value.l = op1.value.ul / op2.value.l;
			break;
		case T_FLOAT:
			result.type = T_FLOAT;
			result.value.f = op1.value.ul / op2.value.f;
			break;
		case T_DOUBLE:
			result.type = T_DOUBLE;
			result.value.d = op1.value.ul / op2.value.d;
			break;
		case T_U_CHAR:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul / op2.value.uc;
			break;
		case T_U_INT:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul / op2.value.ui;
			break;
		case T_U_SHORT:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul / op2.value.us;
			break;
		case T_U_LONG:
			result.type = T_U_LONG;
			result.value.ul = op1.value.ul / op2.value.ul;
			break;
		default:
			op_type_error("/", op1.type, op2.type);
			return(ERROR_DATA);
		}
		break;
	default:
		op_type_error("/", op1.type, op2.type);
		return(ERROR_DATA);
	}

	return(result);
}

/* neg(op1) -- - op1 */

static DATA_ELEMENT neg(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_CHAR:
		op1.value.c = - op1.value.c;
		break;
	case T_INT:
		op1.value.i = - op1.value.i;
		break;
	case T_SHORT:
		op1.value.s = - op1.value.s;
		break;
	case T_LONG:
		op1.value.l = - op1.value.l;
		break;
	case T_FLOAT:
		op1.value.f = - op1.value.f;
		break;
	case T_DOUBLE:
		op1.value.d = - op1.value.d;
		break;
	case T_U_CHAR:
		op1.type = T_CHAR;
		op1.value.c = - op1.value.uc;
		break;
	case T_U_INT:
		op1.type = T_INT;
		op1.value.i = - op1.value.ui;
		break;
	case T_U_SHORT:
		op1.type = T_SHORT;
		op1.value.s = - op1.value.us;
		break;
	case T_U_LONG:
		op1.type = T_LONG;
		op1.value.l = - op1.value.ul;
		break;
	default:
		warn("neg: wrong type");
		return(ERROR_DATA);
	}

	return (op1);
}

/* f_sin(op1) -- sin(op1) */

static DATA_ELEMENT f_sin(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_CHAR:
		op1.value.d = sin((double) op1.value.c);
		break;
	case T_INT:
		op1.value.d = sin((double) op1.value.i);
		break;
	case T_SHORT:
		op1.value.d = sin((double) op1.value.s);
		break;
	case T_LONG:
		op1.value.d = sin((double) op1.value.l);
		break;
	case T_FLOAT:
		op1.value.d = sin((double) op1.value.f);
		break;
	case T_DOUBLE:
		op1.value.d = sin((double) op1.value.d);
		break;
	case T_U_CHAR:
		op1.value.d = sin((double) op1.value.uc);
		break;
	case T_U_INT:
		op1.value.d = sin((double) op1.value.ui);
		break;
	case T_U_SHORT:
		op1.value.d = sin((double) op1.value.us);
		break;
	case T_U_LONG:
		op1.value.d = sin((double) op1.value.ul);
		break;
	default:
		warn("sin: wrong type");
		return(ERROR_DATA);
	}

	op1.type = T_DOUBLE;
	return (op1);
}

/* f_cos(op1) -- cos(op1) */

static DATA_ELEMENT f_cos(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_CHAR:
		op1.value.d = cos((double) op1.value.c);
		break;
	case T_INT:
		op1.value.d = cos((double) op1.value.i);
		break;
	case T_SHORT:
		op1.value.d = cos((double) op1.value.s);
		break;
	case T_LONG:
		op1.value.d = cos((double) op1.value.l);
		break;
	case T_FLOAT:
		op1.value.d = cos((double) op1.value.f);
		break;
	case T_DOUBLE:
		op1.value.d = cos((double) op1.value.d);
		break;
	case T_U_CHAR:
		op1.value.d = cos((double) op1.value.uc);
		break;
	case T_U_INT:
		op1.value.d = cos((double) op1.value.ui);
		break;
	case T_U_SHORT:
		op1.value.d = cos((double) op1.value.us);
		break;
	case T_U_LONG:
		op1.value.d = cos((double) op1.value.ul);
		break;
	default:
		warn("cos: wrong type");
		return(ERROR_DATA);
	}

	op1.type = T_DOUBLE;
	return (op1);
}

/* f_tan(op1) -- tan(op1) */

static DATA_ELEMENT f_tan(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_CHAR:
		op1.value.d = tan((double) op1.value.c);
		break;
	case T_INT:
		op1.value.d = tan((double) op1.value.i);
		break;
	case T_SHORT:
		op1.value.d = tan((double) op1.value.s);
		break;
	case T_LONG:
		op1.value.d = tan((double) op1.value.l);
		break;
	case T_FLOAT:
		op1.value.d = tan((double) op1.value.f);
		break;
	case T_DOUBLE:
		op1.value.d = tan((double) op1.value.d);
		break;
	case T_U_CHAR:
		op1.value.d = tan((double) op1.value.uc);
		break;
	case T_U_INT:
		op1.value.d = tan((double) op1.value.ui);
		break;
	case T_U_SHORT:
		op1.value.d = tan((double) op1.value.us);
		break;
	case T_U_LONG:
		op1.value.d = tan((double) op1.value.ul);
		break;
	default:
		warn("tan: wrong type");
		return(ERROR_DATA);
	}

	op1.type = T_DOUBLE;
	return (op1);
}

/* f_asin(op1) -- asin(op1) */

static DATA_ELEMENT f_asin(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_CHAR:
		op1.value.d = asin((double) op1.value.c);
		break;
	case T_INT:
		op1.value.d = asin((double) op1.value.i);
		break;
	case T_SHORT:
		op1.value.d = asin((double) op1.value.s);
		break;
	case T_LONG:
		op1.value.d = asin((double) op1.value.l);
		break;
	case T_FLOAT:
		op1.value.d = asin((double) op1.value.f);
		break;
	case T_DOUBLE:
		op1.value.d = asin((double) op1.value.d);
		break;
	case T_U_CHAR:
		op1.value.d = asin((double) op1.value.uc);
		break;
	case T_U_INT:
		op1.value.d = asin((double) op1.value.ui);
		break;
	case T_U_SHORT:
		op1.value.d = asin((double) op1.value.us);
		break;
	case T_U_LONG:
		op1.value.d = asin((double) op1.value.ul);
		break;
	default:
		warn("asin: wrong type");
		return(ERROR_DATA);
	}

	op1.type = T_DOUBLE;
	return (op1);
}

/* f_acos(op1) -- acos(op1) */

static DATA_ELEMENT f_acos(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_CHAR:
		op1.value.d = acos((double) op1.value.c);
		break;
	case T_INT:
		op1.value.d = acos((double) op1.value.i);
		break;
	case T_SHORT:
		op1.value.d = acos((double) op1.value.s);
		break;
	case T_LONG:
		op1.value.d = acos((double) op1.value.l);
		break;
	case T_FLOAT:
		op1.value.d = acos((double) op1.value.f);
		break;
	case T_DOUBLE:
		op1.value.d = acos((double) op1.value.d);
		break;
	case T_U_CHAR:
		op1.value.d = acos((double) op1.value.uc);
		break;
	case T_U_INT:
		op1.value.d = acos((double) op1.value.ui);
		break;
	case T_U_SHORT:
		op1.value.d = acos((double) op1.value.us);
		break;
	case T_U_LONG:
		op1.value.d = acos((double) op1.value.ul);
		break;
	default:
		warn("acos: wrong type");
		return(ERROR_DATA);
	}

	op1.type = T_DOUBLE;
	return (op1);
}

/* f_atan(op1) -- atan(op1) */

static DATA_ELEMENT f_atan(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_CHAR:
		op1.value.d = atan((double) op1.value.c);
		break;
	case T_INT:
		op1.value.d = atan((double) op1.value.i);
		break;
	case T_SHORT:
		op1.value.d = atan((double) op1.value.s);
		break;
	case T_LONG:
		op1.value.d = atan((double) op1.value.l);
		break;
	case T_FLOAT:
		op1.value.d = atan((double) op1.value.f);
		break;
	case T_DOUBLE:
		op1.value.d = atan((double) op1.value.d);
		break;
	case T_U_CHAR:
		op1.value.d = atan((double) op1.value.uc);
		break;
	case T_U_INT:
		op1.value.d = atan((double) op1.value.ui);
		break;
	case T_U_SHORT:
		op1.value.d = atan((double) op1.value.us);
		break;
	case T_U_LONG:
		op1.value.d = atan((double) op1.value.ul);
		break;
	default:
		warn("atan: wrong type");
		return(ERROR_DATA);
	}

	op1.type = T_DOUBLE;
	return (op1);
}

/* f_exp(op1) -- exp(op1) */

static DATA_ELEMENT f_exp(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_CHAR:
		op1.value.d = exp((double) op1.value.c);
		break;
	case T_INT:
		op1.value.d = exp((double) op1.value.i);
		break;
	case T_SHORT:
		op1.value.d = exp((double) op1.value.s);
		break;
	case T_LONG:
		op1.value.d = exp((double) op1.value.l);
		break;
	case T_FLOAT:
		op1.value.d = exp((double) op1.value.f);
		break;
	case T_DOUBLE:
		op1.value.d = exp((double) op1.value.d);
		break;
	case T_U_CHAR:
		op1.value.d = exp((double) op1.value.uc);
		break;
	case T_U_INT:
		op1.value.d = exp((double) op1.value.ui);
		break;
	case T_U_SHORT:
		op1.value.d = exp((double) op1.value.us);
		break;
	case T_U_LONG:
		op1.value.d = exp((double) op1.value.ul);
		break;
	default:
		warn("exp: wrong type");
		return(ERROR_DATA);
	}

	op1.type = T_DOUBLE;
	return (op1);
}

/* f_log(op1) -- log(op1) */

static DATA_ELEMENT f_log(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_CHAR:
		op1.value.d = log((double) op1.value.c);
		break;
	case T_INT:
		op1.value.d = log((double) op1.value.i);
		break;
	case T_SHORT:
		op1.value.d = log((double) op1.value.s);
		break;
	case T_LONG:
		op1.value.d = log((double) op1.value.l);
		break;
	case T_FLOAT:
		op1.value.d = log((double) op1.value.f);
		break;
	case T_DOUBLE:
		op1.value.d = log((double) op1.value.d);
		break;
	case T_U_CHAR:
		op1.value.d = log((double) op1.value.uc);
		break;
	case T_U_INT:
		op1.value.d = log((double) op1.value.ui);
		break;
	case T_U_SHORT:
		op1.value.d = log((double) op1.value.us);
		break;
	case T_U_LONG:
		op1.value.d = log((double) op1.value.ul);
		break;
	default:
		warn("log: wrong type");
		return(ERROR_DATA);
	}

	op1.type = T_DOUBLE;
	return (op1);
}

/* f_pow(op1, op2) -- pow(op1, op2) */

static DATA_ELEMENT f_pow(DATA_ELEMENT op1, DATA_ELEMENT op2)
{
	DATA_ELEMENT r;

	if (adjust_type(&op1, T_DOUBLE)) return(ERROR_DATA);
	if (adjust_type(&op2, T_DOUBLE)) return(ERROR_DATA);

	r.value.d = pow(op1.value.d, op2.value.d);
	r.d_class = LITERAL;
	r.type = T_DOUBLE;

	return(r);
}

/* f_atan2(op1, op2) -- atan2(op1, op2) */

static DATA_ELEMENT f_atan2(DATA_ELEMENT op1, DATA_ELEMENT op2)
{
	DATA_ELEMENT r;

	if (adjust_type(&op1, T_DOUBLE)) return(ERROR_DATA);
	if (adjust_type(&op2, T_DOUBLE)) return(ERROR_DATA);

	r.value.d = atan2(op1.value.d, op2.value.d);
	r.d_class = LITERAL;
	r.type = T_DOUBLE;

	return(r);
}

/* f_abs(op1) -- abs(op1) */

static DATA_ELEMENT f_abs(DATA_ELEMENT op1)
{
	switch (op1.type)
	{
	case T_U_INT:
	case T_U_LONG:
	case T_U_CHAR:
	case T_U_SHORT:
		break;
	case T_CHAR:
		/* to silence the picky compiler:
		 * char may or may not be unsigned
		 */
		if (op1.value.c <= 0) op1.value.c = - op1.value.c;
		break;
	case T_INT:
		if (op1.value.i < 0) op1.value.i = - op1.value.i;
		break;
	case T_SHORT:
		if (op1.value.s < 0) op1.value.s = - op1.value.s;
		break;
	case T_LONG:
		if (op1.value.l < 0) op1.value.l = - op1.value.l;
		break;
	case T_FLOAT:
		if (op1.value.f < 0) op1.value.f = - op1.value.f;
		break;
	case T_DOUBLE:
		if (op1.value.d < 0) op1.value.d = - op1.value.d;
		break;
	}
	return(op1);
}

/* f_log10(op1) -- log10(op1) */

static DATA_ELEMENT f_log10(DATA_ELEMENT op1)
{
	if (adjust_type(&op1, T_DOUBLE)) return(ERROR_DATA);

	op1.value.d = log10(op1.value.d);
	return(op1);
}

/* f_sqrt(op1) -- sqrt(op1) */

static DATA_ELEMENT f_sqrt(DATA_ELEMENT op1)
{
	if (adjust_type(&op1, T_DOUBLE)) return(ERROR_DATA);

	op1.value.d = sqrt(op1.value.d);
	return(op1);
}

/* f_min(op1, op2) -- min(op1, op2) */

static DATA_ELEMENT f_min(DATA_ELEMENT op1, DATA_ELEMENT op2)
{
	DATA_ELEMENT op11, op22;

	op11 = op1;
	op22 = op2;
	if (adjust_type(&op11, T_DOUBLE) || adjust_type(&op22, T_DOUBLE))
	{
		warn("wrong argument type for min()");
		return(ERROR_DATA);
	}

	if (op11.value.d > op22.value.d)
	{
		return(op2);
	}
	else
	{
		return(op1);
	}
}

/* f_max(op1, op2) -- max(op1, op2) */

static DATA_ELEMENT f_max(DATA_ELEMENT op1, DATA_ELEMENT op2)
{
	DATA_ELEMENT op11, op22;

	op11 = op1;
	op22 = op2;
	if (adjust_type(&op11, T_DOUBLE) || adjust_type(&op22, T_DOUBLE))
	{
		warn("wrong argument type for max()");
		return(ERROR_DATA);
	}

	if (op11.value.d < op22.value.d)
	{
		return(op2);
	}
	else
	{
		return(op1);
	}
}

/* f_lc() -- lc() -- return current loop index */

static DATA_ELEMENT f_lc(void)
{
	DATA_ELEMENT d;

	d.d_class = LITERAL;
	d.type = T_LONG;
	d.size = p_shStdSize(T_LONG);
	d.value.l = lc;
	return(d);
}

S_TYPE p_shSeBinaryType(S_TYPE t1, S_TYPE t2)
{
  switch(t1)
    {
    case T_CHAR:
      switch(t2)
	{
	case T_CHAR:      return T_CHAR;
	case T_SHORT:     return T_SHORT;
	case T_INT:       return T_INT;
	case T_LONG:      return T_LONG;
	case T_U_CHAR:    return T_CHAR;
	case T_U_SHORT:   return T_SHORT;
	case T_U_INT:     return T_INT;
	case T_U_LONG:    return T_LONG;
	case T_FLOAT:     return T_FLOAT;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    case T_SHORT:
      switch(t2)
	{
	case T_CHAR:      return T_SHORT;
	case T_SHORT:     return T_SHORT;
	case T_INT:       return T_INT;
	case T_LONG:      return T_LONG;
	case T_U_CHAR:    return T_SHORT;
	case T_U_SHORT:   return T_SHORT;
	case T_U_INT:     return T_INT;
	case T_U_LONG:    return T_LONG;
	case T_FLOAT:     return T_FLOAT;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    case T_INT:
      switch(t2)
	{
	case T_CHAR:      return T_INT;
	case T_SHORT:     return T_INT;
	case T_INT:       return T_INT;
	case T_LONG:      return T_LONG;
	case T_U_CHAR:    return T_INT;
	case T_U_SHORT:   return T_INT;
	case T_U_INT:     return T_INT;
	case T_U_LONG:    return T_LONG;
	case T_FLOAT:     return T_FLOAT;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    case T_LONG:
      switch(t2)
	{
	case T_CHAR:      return T_LONG;
	case T_SHORT:     return T_LONG;
	case T_INT:       return T_LONG;
	case T_LONG:      return T_LONG;
	case T_U_CHAR:    return T_LONG;
	case T_U_SHORT:   return T_LONG;
	case T_U_INT:     return T_LONG;
	case T_U_LONG:    return T_LONG;
	case T_FLOAT:     return T_FLOAT;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    case T_U_CHAR:
      switch(t2)
	{
	case T_CHAR:      return T_CHAR;
	case T_SHORT:     return T_SHORT;
	case T_INT:       return T_INT;
	case T_LONG:      return T_LONG;
	case T_U_CHAR:    return T_U_CHAR;
	case T_U_SHORT:   return T_U_SHORT;
	case T_U_INT:     return T_U_INT;
	case T_U_LONG:    return T_U_LONG;
	case T_FLOAT:     return T_FLOAT;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    case T_U_SHORT:
      switch(t2)
	{
	case T_CHAR:      return T_SHORT;
	case T_SHORT:     return T_SHORT;
	case T_INT:       return T_INT;
	case T_LONG:      return T_LONG;
	case T_U_CHAR:    return T_U_SHORT;
	case T_U_SHORT:   return T_U_SHORT;
	case T_U_INT:     return T_U_INT;
	case T_U_LONG:    return T_U_LONG;
	case T_FLOAT:     return T_FLOAT;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    case T_U_INT:
      switch(t2)
	{
	case T_CHAR:      return T_INT;
	case T_SHORT:     return T_INT;
	case T_INT:       return T_INT;
	case T_LONG:      return T_LONG;
	case T_U_CHAR:    return T_U_INT;
	case T_U_SHORT:   return T_U_INT;
	case T_U_INT:     return T_U_INT;
	case T_U_LONG:    return T_U_LONG;
	case T_FLOAT:     return T_FLOAT;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    case T_U_LONG:
      switch(t2)
	{
	case T_CHAR:      return T_LONG;
	case T_SHORT:     return T_LONG;
	case T_INT:       return T_LONG;
	case T_LONG:      return T_LONG;
	case T_U_CHAR:    return T_U_LONG;
	case T_U_SHORT:   return T_U_LONG;
	case T_U_INT:     return T_U_LONG;
	case T_U_LONG:    return T_U_LONG;
	case T_FLOAT:     return T_FLOAT;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    case T_FLOAT:
      switch(t2)
	{
	case T_CHAR:      return T_FLOAT;
	case T_SHORT:     return T_FLOAT;
	case T_INT:       return T_FLOAT;
	case T_LONG:      return T_FLOAT;
	case T_U_CHAR:    return T_FLOAT;
	case T_U_SHORT:   return T_FLOAT;
	case T_U_INT:     return T_FLOAT;
	case T_U_LONG:    return T_FLOAT;
	case T_FLOAT:     return T_FLOAT;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    case T_DOUBLE:
      switch(t2)
	{
	case T_CHAR:      return T_DOUBLE;
	case T_SHORT:     return T_DOUBLE;
	case T_INT:       return T_DOUBLE;
	case T_LONG:      return T_DOUBLE;
	case T_U_CHAR:    return T_DOUBLE;
	case T_U_SHORT:   return T_DOUBLE;
	case T_U_INT:     return T_DOUBLE;
	case T_U_LONG:    return T_DOUBLE;
	case T_FLOAT:     return T_DOUBLE;
	case T_DOUBLE:    return T_DOUBLE;
	default:          return T_ERROR;
	}
    default:          return T_ERROR;
    }
}

static void op_type_error(char *s, S_TYPE t1, S_TYPE t2)
{
	char s1[128], s2[128], ss[512];

	sprintf(ss, "wrong type (%s) %s (%s)", show_type(s1, t1), s,
		show_type(s2, t2));

	warn(ss);
}


/* fatal(s) -- print s and quit */

static void fatal(char *s)
{
	printf("fatal> %s\n", s);
	exit(0);
}

/* warn(s) -- print s as error message */

static void warn(char *s)
{
	printf("error> %s\n", s);
}

/* shSeGenerateCode(s, interp) -- generate mapping code from s */

int shSeGenerateCode(char *s, Tcl_Interp* interp)
{
	int shSe_yyparse(void);

        g_seInterp = interp;
	code_init();
	lex_input_buffer = lex_input = s;
	return(shSe_yyparse());
}

#ifdef SYS_TEST

main(argc, argv)
int argc;
char **argv;
{
	printf("sizeof(DATA_ELEMENT)      = %d\n", sizeof(DATA_ELEMENT));
	printf("sizeof(eval_stack)       = %d\n", sizeof(eval_stack));
	printf("sizeof(code)             = %d\n", sizeof(code));
}

#endif

#if 0
S_TYPE p_shSeDervishType2ElemType(TYPE type)
{
  if (type == shTypeGetFromName("SHORT"))
    return T_SHORT;
  else if (type == shTypeGetFromName("USHORT"))
    return T_U_SHORT;
  else if (type == shTypeGetFromName("INT"))
    return T_INT;
  else if (type == shTypeGetFromName("UINT"))
    return T_U_INT;
  else if (type == shTypeGetFromName("LONG"))
    return T_LONG;
  else if (type == shTypeGetFromName("ULONG"))
    return T_U_LONG;
  else if (type == shTypeGetFromName("FLOAT"))
    return T_FLOAT;
  else if (type == shTypeGetFromName("DOUBLE"))
    return T_DOUBLE;
  else
    return T_ERROR;
}
#endif

/* p_shSeDervishType2ElemType(type) -- converting dervish type to SE type */

S_TYPE p_shSeDervishType2ElemType(TYPE type)
{
	static TYPE SHORT;
	static TYPE USHORT;
	static TYPE INT;
	static TYPE UINT;
	static TYPE LONG;
	static TYPE ULONG;
	static TYPE FLOAT;
	static TYPE DOUBLE;
	static TYPE CHAR;
	static TYPE STR;
	static TYPE PTR;
	static int init = 0;

	if (!init) {
		SHORT = shTypeGetFromName("SHORT");
		USHORT = shTypeGetFromName("USHORT");
		INT = shTypeGetFromName("INT");
		UINT = shTypeGetFromName("UINT");
		LONG = shTypeGetFromName("LONG");
		ULONG = shTypeGetFromName("ULONG");
		FLOAT = shTypeGetFromName("FLOAT");
		DOUBLE = shTypeGetFromName("DOUBLE");
		CHAR = shTypeGetFromName("CHAR");
		STR = shTypeGetFromName("STR");
		PTR = shTypeGetFromName("PTR");
		init = 1;
	}

	if (type == INT)	return T_INT;
	if (type == LONG)	return T_LONG;
	if (type == FLOAT)	return T_FLOAT;
	if (type == DOUBLE)	return T_DOUBLE;
	if (type == SHORT)	return T_SHORT;
	if (type == USHORT)	return T_U_SHORT;
	if (type == UINT)	return T_U_INT;
	if (type == ULONG)	return T_U_LONG;
	if (type == CHAR)	return T_CHAR;
	if (type == STR)	return T_STRING;
	if (type == PTR)	return T_PTR;

	return T_ERROR;
}

/*
static DATA_ELEMENT get_data_member_value(char *class_name, char *field_name)
{
      DATA_ELEMENT rvalue;

      shErrStackPush("Member access not supported: %s.%s",class_name,
                  field_name);
      rvalue.d_class = DUMMY;
      return rvalue;
}
*/

/* get_value_from_tblcol(array, pos, dim) -- get the value from a tblcol
 * 						column, which may be an
 *						multi-dimensional array
 *						of a primary type
 */

static DATA_ELEMENT get_value_from_tblcol(ARRAY *array, int pos, int dim)
{
	DATA_ELEMENT d;
	S_TYPE type;
	int size;
	void *adptr;

#ifdef SEDEBUG
	if (g_debug) 
	  {
	    printf("get_value_from_tblcol(0x%08x %d %d) = ", array, pos, dim);
	    fflush(stdout);
	  }
#endif
	type = p_shSeDervishType2ElemType(array->data.schemaType);
	if (type == T_ERROR)
	{
		return(ERROR_DATA);
	}

	size = array->data.incr;

	if (dim)
	{
		d = get_value(pop());
		if (adjust_type(&d, T_INT))
		{
			shErrStackPush("None value for index");
			return(ERROR_DATA);
		}
		adptr = get_array_data_ptr(
			(void **) *(((void **) array->arrayPtr) + pos),
			dim - 1);
		adptr = (char *) adptr + d.value.i * size;
	}
	else
	{
		adptr = array->data.dataPtr + pos * size;
	}

	d.d_class = LITERAL;
	d.type = type;
	d.size = p_shStdSize(type);
#ifdef SEDEBUG
	if (g_debug)
	  {
	    printf("(size = %d) ", d.size);
	    printf("(adptr = 0x%08x) ", adptr);
	    fflush(stdout);
	  }
#endif
	memcpy(&(d.value.c), (char *) adptr, d.size);
#ifdef SEDEBUG
{
	char s[1024];

	if (g_debug) 
	  {
	    printf("%s\n", show_value(s, d));
	    fflush(stdout);
	  }
}
#endif
	return(d);
}

static void **get_array_data_ptr(void **ptr, int dim)
{
	DATA_ELEMENT d;
	void **a;

#ifdef SEDEBUG
	if (g_debug) printf("get_array_data_ptr(0x%08x, %d)\n", ptr, dim);
#endif
	if (dim <= 0) return (ptr);

	d = get_value(pop());
	if (adjust_type(&d, T_INT))
	{
		shErrStackPush("None value for index");
		return(NULL);
	}

	if ((a = get_array_data_ptr(ptr, dim - 1)) == NULL)
	{
		shErrStackPush("Error in getting array element in TBLCOL");
		return(NULL);
	}

#ifdef SEDEBUG
	if (g_debug) printf("return(0x%08x)\n", (void **) *(a + d.value.i));
#endif
	return ((void **) *(a + d.value.i));
}

/* get_address_from_tblcol(array, pos, dim) -- get the address from a
 *						tblcol column, which
 *						may be an multi-
 *						dimensional array
 *						of a primary type
 */

static void *get_address_from_tblcol(ARRAY *array, int pos, int dim)
{
	DATA_ELEMENT d;
	int size;
	void *adptr;

	size = array->data.incr;
	if (dim)
	{
		d = get_value(pop());
		if (adjust_type(&d, T_INT))
		{
			shErrStackPush("None value for index");
			return(NULL);
		}
		adptr = get_array_data_ptr(
			(void **) *(((void **) array->arrayPtr) + pos),
			dim - 1);
		adptr = (char *) adptr + d.value.i * size;
	}
	else
	{
		adptr = array->data.dataPtr + pos * size;
	}
	return(adptr);
}

static DATA_ELEMENT get_value_from_handle(DATA_ELEMENT v)
{
	DATA_ELEMENT a;

	/* v must be HANDLE2 class */

	if (v.d_class != HANDLE2)
	{
		warn("wrong handle");
		return(ERROR_DATA);
	}

	switch(v.type)
	{
	case T_SCHEMA:
		return(get_base_value(v));
	case T_CONT_PERIOD:
	case T_CONT_ARROW:
		v = array_decode(v);
		a = get_value(pop());	/* get previous address */
#ifdef SEDEBUG
{
	char s[1024];
	if (g_debug) 
	  {
	    printf("get_value_from_handle: address entry: %s\n", show_entry(s, a));
	    fflush(stdin);
	  }
}
#endif
		if ((a.d_class != HANDLE2) || (a.type != T_SCHEMA))
		{
			warn("wrong address reference");
			return(ERROR_DATA);
		}

		if (v.type == T_CONT_ARROW)
		{
#ifdef SEDEBUG
	if (g_debug) printf("a.value.hi.ptr = 0x%08x\n", a.value.hi.ptr);
#endif
			a.value.hi.ptr = *((void **) a.value.hi.ptr);
#ifdef SEDEBUG
	if (g_debug) printf("a.value.hi.ptr = 0x%08x\n", a.value.hi.ptr);
#endif
		}
		a.value.hi.ptr = (char *) a.value.hi.ptr + v.value.chan.offset;
#ifdef SEDEBUG
	if (g_debug) printf("a.value.hi.ptr = 0x%08x\n", a.value.hi.ptr);
#endif
		a.value.hi.indirection = v.value.chan.indirection;
		a.value.hi.dim = v.value.chan.dim;
		a.value.hi.nelem = v.value.chan.nelem;
		a.value.hi.type = v.value.chan.type;
		a.size = v.size;
		return(get_base_value(a));
	default:
		warn("unknown handle");
		return(ERROR_DATA);
	}
}

static DATA_ELEMENT get_address_from_handle(DATA_ELEMENT v)
{
	DATA_ELEMENT a;

	/* v must be HANDLE2 class */

	if (v.d_class != HANDLE2)
	{
		warn("wrong handle");
		return(ERROR_DATA);
	}

	switch(v.type)
	{
	case T_SCHEMA:
		return(get_base_address(v));
	case T_CONT_PERIOD:
	case T_CONT_ARROW:
		v = array_decode(v);
		a = get_value(pop());	/* get previous address */
#ifdef SEDEBUG
{
	char s[1024];
	if (g_debug) 
	  {
	    printf("get_address_from_handle: address entry: %s\n", show_entry(s, a));
	    fflush(stdin);
	  }
}
#endif
		if ((a.d_class != HANDLE2) || (a.type != T_SCHEMA))
		{
			warn("wrong address reference");
			return(ERROR_DATA);
		}

		if (v.type == T_CONT_ARROW)
		{
			a.value.hi.ptr = *((char **) a.value.hi.ptr);
		}
		a.value.hi.ptr = (char *) a.value.hi.ptr + v.value.chan.offset;
		a.value.hi.indirection = v.value.chan.indirection;
		a.value.hi.dim = v.value.chan.dim;
		a.value.hi.nelem = v.value.chan.nelem;
		a.value.hi.type = v.value.chan.type;
		a.size = v.size;
		return(get_base_address(a));
	default:
		warn("unknown handle");
		return(ERROR_DATA);
	}
}

/* get_base_value(v) -- take care of the indexing in array and the pointers */

static DATA_ELEMENT get_base_value(DATA_ELEMENT v)
{
	DATA_ELEMENT d;

#ifdef SEDEBUG
{
	char s[1024];
	if (g_debug) 
	  {
	    printf("get_base_value(%s)\n", show_entry(s, v));
	    fflush(stdin);
	  }
}
#endif
	v = array_decode(v);

	d.type = p_shSeDervishType2ElemType(v.value.hi.type);
	d.ind = v.value.hi.indirection;
#ifdef SEDEBUG
{
	char s[1024];

	if (g_debug) 
	  {
	    printf("d.type = %s\n", show_type(s, d.type));
	    fflush(stdin);
	  }
}
#endif
	if (d.type == T_STRING)		/* string is a special case */
	{
		d.d_class = LITERAL;
		d.size = v.size;
		d.value.str = (char *) v.value.hi.ptr;

		return(d);
	}
	else if (v.value.hi.nelem != NULL)	/* block of elements */
	{
		return(v);
	}
	else if (d.type != T_ERROR)	/* for elementary types */
	{
		d.d_class = LITERAL;
		d.size = (d.ind)?sizeof(void *):p_shStdSize(d.type);
		memcpy(&d.value.c, (char *) v.value.hi.ptr, d.size);
#ifdef SEDEBUG
{
	char s[1024];

	if (g_debug) 
	  {
	    printf("get_base_value() = %s\n", show_entry(s, d));
	    fflush(stdin);
	  }
}
#endif
		return(d);
	}
	else
	{
		return(v);
	}
}

/* array_decode(v) -- have the pointer point to the right position */

static DATA_ELEMENT array_decode(DATA_ELEMENT v)
{
	long offset;

#ifdef SEDEBUG
{
	char s[1024];

	if (g_debug) printf("array_decode(%s) ...\n", show_entry(s, v));
}
#endif
	switch (v.type)
	{
	case T_CONT_PERIOD:
	case T_CONT_ARROW:
		if (v.value.chan.nelem != NULL)
		{
			offset = get_array_offset(v.value.chan.dim,
				&(v.size), &(v.value.chan.nelem));

			if (offset < 0)
			{
				warn("array index out of range");
				return(ERROR_DATA);
			}

			v.value.chan.offset += offset;
			v.value.chan.dim = 0;
		}
#ifdef SEDEBUG
{
	char s[1024];

	if (g_debug) printf("return from previous array_decode: %s\n", show_entry(s, v));
}
#endif
		return(v);
	case T_SCHEMA:
		if (v.value.hi.nelem != NULL)	/* an array */
		{
			offset = get_array_offset(v.value.hi.dim,
				&(v.size), &(v.value.hi.nelem));

			if (offset < 0)
			{
				warn("array index out of range");
				return(ERROR_DATA);
			}

			v.value.hi.ptr = (char *) v.value.hi.ptr + offset;
			v.value.hi.dim = 0;
		}
#ifdef SEDEBUG
{
	char s[1024];

	if (g_debug) printf("return from previous array_decode: %s\n", show_entry(s, v));
}
#endif
		return(v);
	default:
		warn("unknown handle type in array_decode()");
		break;
	}
	return(ERROR_DATA);
}

static DATA_ELEMENT get_base_address(DATA_ELEMENT v)
{
	DATA_ELEMENT d;

	v = array_decode(v);

	if ((v.value.hi.nelem != NULL) ||
		(p_shSeDervishType2ElemType(v.value.hi.type) == T_ERROR))
	{
		return(v);
	}
	else
	{
		d.d_class = LITERAL;
		d.type = p_shSeDervishType2ElemType(v.value.hi.type);
		d.ind = v.value.hi.indirection + 1;
		d.value.cp = (char *) v.value.hi.ptr;

		return(d);
	}
}

/* get_array_offset(dim, size, *nelem) -- calculate the offset based on
 *	dim: dimension, size: the size of each element, and
 *	*nelem: the ranges
 *
 *	the number of dimensions in the expressing might be less than
 *	the actual declared one.
 *
 *	for example:
 *
 *	double d[10][9][8][7];
 *
 *		...d[2][5]...
 *
 *	in this case, d is consider a 10 X 9 array of 8 X 7 doubles.
 *	the unit size will be sizeof(double)*8*7
 */

static long get_array_offset(long dim, int *size, char **nelem)
{
	long offset;
	char *np;
	int range;

#ifdef SEDEBUG
	if (g_debug)
	{
		printf("get_array_offset(dim=%d, *size=%d, *nelem=\"%s\")\n", dim, *size, *nelem);
	}
#endif
	offset = get_array_offset1(dim, nelem);

	/* handle the remaining indexes, if any, and adjust size */

	if (**nelem == '\0')
	{
		*nelem = NULL;
	}
	else
	{
		np = *nelem;
		while ((np = get_next_nelem_value(np, &range)) != NULL)
		{
			*size *= range;
		}
	}

	offset *= *size;
#ifdef SEDEBUG
	if (g_debug)
	{
		printf("adjusted_offset=%d, *size=%d\n", offset, *size);
	}
#endif
	return(offset);
}

/* get_array_offset1() is the recursion body of get_array_offset
 *
 *	the reason of using recursion here is not to impose any
 *	constraint on the number of dimensions of an array.
 *	(the indexes appear in the stack in reverse order!)
 *	Of course the size calculation may be merged into
 *	get_array_offset() itself and hence eliminate the need of a
 *	separate get_array_offset1(). However, in that case, it requires
 *	one more multiplication in each recursion, which, can be easily
 *	performed as just one in the end, as in get_arrY_offest().
 */

static long get_array_offset1(long dim, char **nelem)
{
	long offset;
	int range;
	DATA_ELEMENT d;

	if (dim <= 0)	/* boundary condition */
	{
		return (0);
	}

	d = get_value(pop());
	adjust_type(&d, T_INT);
	offset = get_array_offset1(dim - 1, nelem);
	*nelem = get_next_nelem_value(*nelem, &range);
	offset = offset * range + d.value.i;
	return(offset);
}
			
/* char *get_next_nelem_value(char *nelem, int *value) --
 *	get the first value from nelem string and return the current
 *	position in it
 */

static char *get_next_nelem_value(char *nelem, int *value)
{
	while ((*nelem != '\0') && !isdigit(*nelem)) nelem++;
	if (*nelem == '\0')	/* no more */
	{
		*value = 0;
		return (NULL);
	}
	sscanf(nelem, "%d", value);
	while ((*nelem != '\0') && isdigit(*nelem)) nelem++;
	while ((*nelem != '\0') && !isdigit(*nelem)) nelem++;
	return (nelem);
}

/* ptr_type(base_type) -- return the pointer type of the base type */

static S_TYPE ptr_type(S_TYPE base)
{
	switch (base)
	{
	case T_CHAR:	return(T_CHAR_P);
	case T_INT:	return(T_INT_P);
	case T_SHORT:	return(T_SHORT_P);
	case T_LONG:	return(T_LONG_P);
	case T_U_CHAR:	return(T_U_CHAR_P);
	case T_U_INT:	return(T_U_INT_P);
	case T_U_SHORT:	return(T_U_SHORT_P);
	case T_U_LONG:	return(T_U_LONG_P);
	case T_FLOAT:	return(T_FLOAT_P);
	case T_DOUBLE:	return(T_DOUBLE_P);
	case T_STRING:	return(T_STRING);
	default:	return(T_ERROR);
	}
}

/* base_type(ptr_type) -- return the base type of a pointer type */

static S_TYPE base_type(S_TYPE ptr)
{
	switch (ptr)
	{
	case T_CHAR_P:		return(T_CHAR);
	case T_INT_P:		return(T_INT);
	case T_SHORT_P:		return(T_SHORT);
	case T_LONG_P:		return(T_LONG);
	case T_U_CHAR_P:	return(T_U_CHAR);
	case T_U_INT_P:		return(T_U_INT);
	case T_U_SHORT_P:	return(T_U_SHORT);
	case T_U_LONG_P:	return(T_U_LONG);
	case T_FLOAT_P:		return(T_FLOAT);
	case T_DOUBLE_P:	return(T_DOUBLE);
	default:		return(T_ERROR);
	}
}

/* is_ptr_type(d) -- to see if DATA_ELEMENT d is of ptr type */

static int is_ptr_type(DATA_ELEMENT d)
{
	if (d.ind) return(1);

	switch (d.type)
	{
	case T_STRING:
	case T_CHAR_P:
	case T_INT_P:
	case T_SHORT_P:
	case T_LONG_P:
	case T_U_CHAR_P:
	case T_U_INT_P:
	case T_U_SHORT_P:
	case T_U_LONG_P:
	case T_FLOAT_P:
	case T_DOUBLE_P:
		return(1);
	default:
		return(0);
	}
}

/* is_ptr_stype(t) -- to see if t is of ptr s_type */

static int is_ptr_stype(S_TYPE t)
{
	switch (t)
	{
	case T_STRING:
	case T_CHAR_P:
	case T_INT_P:
	case T_SHORT_P:
	case T_LONG_P:
	case T_U_CHAR_P:
	case T_U_INT_P:
	case T_U_SHORT_P:
	case T_U_LONG_P:
	case T_FLOAT_P:
	case T_DOUBLE_P:
		return(1);
	default:
		return(0);
	}
}

/* p_shSeIsNumericalType(t) -- to see if t is a numberical type */

int p_shSeIsNumericalType(S_TYPE t)
{
	switch (t)
	{
	case T_INT:
	case T_CHAR:
	case T_SHORT:
	case T_LONG:
	case T_U_CHAR:
	case T_U_INT:
	case T_U_SHORT:
	case T_U_LONG:
	case T_FLOAT:
	case T_DOUBLE:
		return(1);
	default:
		return(0);
	}
}

/* do_assign(a, d) -- assign d to a */

static int do_assign(DATA_ELEMENT a, DATA_ELEMENT d)
{
	S_TYPE t;

	/* special case for general SCHEMA
	 *
	 * if d.type == T_SCHEMA, it means the type is not recognized
	 * by SEval and memcpy is assumed if possible
	 */

	if ((d.type == T_SCHEMA) && (a.type == T_SCHEMA))
	{
		/* check if source and destination are compatible */

		if (data_compatible(a, d))
		{
			memcpy((void *) a.value.hi.ptr,
				(void *) d.value.hi.ptr, d.size);
			return(0);
		}
		else
		{
			warn("source and destination are of uncompatible types");
			return(1);
		}
	}

	if (!is_ptr_type(a))
	{
		warn("wrong address for assignment");
		return(1);
	}

	/* special case for pointers */

	if (a.ind > 1)
	{
		*((void **) a.value.gp) = d.value.gp;
		return(0);
	}

	t = a.type;
	if (adjust_type(&d, t))
		return(1);

	switch (t)
	{
	case T_CHAR:
		*(a.value.cp) = d.value.c;
		break;
	case T_INT:
		*(a.value.ip) = d.value.i;
		break;
	case T_SHORT:
		*(a.value.sp) = d.value.s;
		break;
	case T_LONG:
		*(a.value.lp) = d.value.l;
		break;
	case T_U_CHAR:
		*(a.value.ucp) = d.value.uc;
		break;
	case T_U_INT:
		*(a.value.uip) = d.value.ui;
		break;
	case T_U_SHORT:
		*(a.value.usp) = d.value.us;
		break;
	case T_U_LONG:
		*(a.value.ulp) = d.value.ul;
		break;
	case T_FLOAT:
		*(a.value.fp) = d.value.f;
		break;
	case T_DOUBLE:
		*(a.value.dp) = d.value.d;
		break;
	default:
		return(1);	/* error */
	}

	return(0);
}

/* data_compatible(a, b) -- to see if SCHEMA a and b are compatible
 *				if they are not, it is not safe to
 *				exchange contents
 */

static int data_compatible(DATA_ELEMENT a, DATA_ELEMENT d)
{
	/* assuming it's all right for same schema type */

	if ((a.type == T_SCHEMA) && (d.type == T_SCHEMA) &&
		(a.value.hi.type == d.value.hi.type) &&
		(a.size >= d.size))
	{
		return(1);
	}

	/* a back door for PTR type in the destination */

	if ((a.type == T_SCHEMA) && (d.type == T_SCHEMA) &&
		(a.value.hi.type == shTypeGetFromName("PTR")))
	{
		return(1);
	}

	return(0);
}

int shSEval(Tcl_Interp* interp, char* expression, int start, int end, int debug, int mtime)
{
	extern char* lex_input;
	extern int shSe_yydebug;
	int status;
	long nlc;
	char s[128];

	shSe_yydebug = 0;

	(*seSEvalInit)();

	/* measure time, if necessary */

	if (mtime)
	{
		init_clock();
	}

	if(shSeGenerateCode(expression, interp))
	{
		clean_up_static_strings();
		return 1;
	}

	if (mtime)
	{
		set_clock(&tcomp);
	}

	lc = status = 0;

	if (debug)
	{
		putchar('\n');
		puts(expression);
		putchar('\n');
  		dump_code_store();
		g_debug = 1;
  	}
	else
	{
		g_debug = 0;
	}

	nlc = (ev_length)?ev_length:1;

	if (mtime)
	{
		init_clock();
	}

	for(lc = 0; lc < nlc; lc++)	/* lc is the real loop count */
	{
		status = exec_code();
		(*seFlushTransientObjects)();
        	if (status)
        	{
			status = (status == 2)?0:status;
			break;
		}
	}

	if (mtime)
	{
		set_clock(&texec);
	}

	clean_up_static_strings();
	sprintf(s, "%ld", lc);

	if (mtime)
	{
		report_time(lc);
	}

	Tcl_AppendResult(interp, s, NULL);
	(*seSEvalEnd)();
	return status;
}

/* advance_cursor(pc) -- advance the cursor position at current program
 * code pointed by pc. If the end of list is reached, 1 is returned.
 * otherwise, 0 is returned.
 */

static int advance_cursor(int pc)
{
	switch(code[pc].operand.type)
	{
	case T_CHAIN:
		if (code[pc].operand.value.ci.cep == NULL)
		{
			return(1);	/* end of chain */
		}
		code[pc].operand.value.ci.cep = code[pc].operand.value.ci.cep->pNext;
		break;
	case T_VECTOR:
		if (code[pc].operand.value.vi.n >= code[pc].operand.value.vi.vec->dimen)
		{
			return(1);	/* end of vector */
		}
		code[pc].operand.value.vi.n++;
		break;
	case T_TBLCOL:
		if (code[pc].operand.value.tci.pos >= code[pc].operand.value.tci.array->dim[0])
		{
			return(1);	/* end of array */
		}
		code[pc].operand.value.tci.pos++;
		break;
	default:
		warn("unknown CURSOR type");
	}

	/* everything is fine */

	return(0);
}

/* Function to define the 'object oriented part' of the
   expression evaluator.  If an argument is NULL the previous 
   values is kept */

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
 )
{
  if ( SEvalInit != NULL ) seSEvalInit = SEvalInit;
  if ( SEvalEnd != NULL ) seSEvalEnd = SEvalEnd;
  if ( sdbFindConstructor   != NULL ) seFindConstructor = sdbFindConstructor;
  if ( sdbCallConstructor   != NULL ) seCallConstructor = sdbCallConstructor;
  if ( sdbFindMethod        != NULL ) seFindMethod = sdbFindMethod;
  if ( sdbCallMethod        != NULL ) seCallMethod = sdbCallMethod;
  if ( sdbGetTclHandle      != NULL ) seGetTclHandle = sdbGetTclHandle;
  if ( sdbFindClassFromType != NULL ) seFindClassFromName = sdbFindClassFromType;
	if (realSeRegisterTransientObject != NULL)
		seRegisterTransientObject = realSeRegisterTransientObject;
	if (realSeFlushTransientObjects != NULL)
		seFlushTransientObjects = realSeFlushTransientObjects;
  return SH_SUCCESS;
}

/* shut_up_picky_compiler() -- a dummy function to silence picky
 * 	compilers that complain about unused static functions.
 *	This is never meant to be excuted ...
 */

void shut_up_picky_compiler(void)
{
	OPCODE op;
	DATA_ELEMENT d;
	S_TYPE type;
	int result;

	type = T_INT;
	result = code_top(&op, &d);
	result = dump_eval_stack();
	d = top();
	type = base_type(type);
	type = ptr_type(type);
	fatal("You should not have called this functions!\n");
}

/* static string handleing routines
 *
 * space for storing static string is allocated at compile time
 * a list is maintained to keep track all allocated memory chuncks
 * so that they can be freed properly
 */

typedef struct pe
{
        char *ptr;
        struct pe *next;
} PElement;

static PElement *static_strings = NULL;	/* head of string list */
static PElement *end = NULL;		/* tail of string list */

/* register_string(s) -- register static string */

char *register_string(char *s)
{
	int size;

	size = strlen(s);

	if (end != NULL)
	{
		/* allocate a header first */

		if ((end->next = (PElement *) shMalloc(sizeof(PElement))) == NULL)
		{
			return(NULL);
		}

		end = end->next;
	}
	else
	{
		if ((static_strings = end = (PElement *) shMalloc(sizeof(PElement))) == NULL)
		{
			return(NULL);
		}
	}

	/* now allocate space to store the string */

	end->ptr = (char *) shMalloc(size+1);
	end->next = NULL;
	strcpy(end->ptr, s);

	return(end->ptr);
}

/* clean_up(PElement *p) -- clean up the list recursivly */

static void clean_up(PElement * p)
{
	if (p == NULL) return;
	clean_up(p->next);
	if (p->ptr != NULL) shFree(p->ptr);
	shFree(p);
}

/* clean_up_static_strings() -- cleaning up static strings */

void clean_up_static_strings(void)
{
	clean_up(static_strings);
	static_strings = end = NULL;
}

/* Timing routines -- measuring the speed for improvement */

/* init_clock() -- reset clock */

static void init_clock(void)
{
	times(&t0);
}

/* set the time since last init */

static void set_clock(struct tms *t)
{
	times(t);
	t->tms_utime -= t0.tms_utime;
	t->tms_stime -= t0.tms_stime;
	t->tms_cutime -= t0.tms_cutime;
	t->tms_cstime -= t0.tms_cstime;
}

/* report time */

static void report_time(int n)
{
	printf("\n");
	printf("Compile time   -> user: %d clks, system: %d clks, total: %d clks\n",
		tcomp.tms_utime, tcomp.tms_stime,
		(tcomp.tms_utime + tcomp.tms_stime));
	printf("Execution time -> user: %d clks, system: %d clks, total: %d clks\n",
		texec.tms_utime, texec.tms_stime,
		(texec.tms_utime + texec.tms_stime));
	printf("Each iteration -> user: %8.2f clks, system: %8.2f clks, total: %8.2f clks\n",
		(double) texec.tms_utime / n,
		(double) texec.tms_stime / n,
		(double) (texec.tms_utime + texec.tms_stime) / n);
}

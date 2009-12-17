%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dervish.h>
#include "prvt/seCode.h"

#if defined(_POSIX_SOURCE)
  /*#  define gettxt(FILE,MSG) (MSG)*/
#endif
   
#if defined(NO_CURSES)
#  define gettxt(FILE,MSG) (MSG)
#endif
   
static DATA_ELEMENT NONE = {DUMMY, T_DONTCARE, 0, 0, {0}};

typedef struct hhandle
{
	TYPE type;
	int indirection;
	void *ptr;
}	HHANDLE;

typedef struct symbol_element
{
	S_TYPE type;	/* type info */
	int	att;	/* attribute for the dimension etc */
	int	att2;	/* extra attribute ... */
	int	size;
	union
	{
		long	l;		/* integer */
		double	d;		/* floating point number */
		HHANDLE	h;		/* modified SCHEMA handle */
		char	s[1024];	/* string */
		ARRAY*	a;		/* pointer to an ARRAY */
	} value;
} SYMBOL_ELEMENT;

#define YYSTYPE SYMBOL_ELEMENT

#define MAX_DIM 2048

static DATA_ELEMENT arg_stack[MAX_DIM];
static arg_idx = 0;

extern char yytext[];
int yylex(void);
static int arg_add(DATA_ELEMENT);
static void show_constructor_finding(char *, DATA_ELEMENT *, int);
%}

%token	STRING
%token	INTEGER
%token	FLOAT
%token	IDENTIFIER
%token  CHAIN_HANDLE
%token  VECTOR_HANDLE
%token  OBJECT_HANDLE
%token  SCHEMA_HANDLE
%token  TBLCOL_HANDLE
%left	ARROW
%token	'('
%token	')'
%left	'+' '-'
%left	'*' '/'
%token	'<'
%token	'>'
%token	'['
%token	']'
%token	TT_UNSIGNED
%token	TT_INT
%token	TT_FLOAT
%token	TT_DOUBLE
%token	TT_CHAR
%token	TT_LONG
%token	TT_SHORT
%token	TT_STRING
%token	NEW
%token	F_SIN
%token	F_COS
%token	F_TAN
%token	F_ASIN
%token	F_ACOS
%token	F_ATAN
%token	F_EXP
%token	F_LOG
%token	F_POW
%token	F_ABS
%token	F_ATAN2
%token	F_LOG10
%token	F_SQRT
%token	F_MIN
%token	F_MAX
%token	F_LC
%token	F_STRCPY
%token	F_STRNCPY
%token	F_STRLEN
%token	F_STRCAT
%token	F_STRNCAT
%token	F_STRCMP
%token	F_STRNCMP
%token	F_STRCASECMP
%token	F_STRNCASECMP
%token	F_STROFFSET
%left	'.'
%token	','
%token	'{'
%token	'}'
%token	'='
%token	L_AND
%token	L_OR
%token	L_NOT
%token	IF
%token	THEN
%token	ELSE
%token	ENDIF
%token	C_EQ
%token	C_GT
%token	C_GE
%token	C_LT
%token	C_LE
%token	C_NE
%token	B_SL
%token	B_SR
%token	P_BREAK
%token	P_CONTINUE

%start	expr_string

%%
expr_string
	: statements
	| statements ';'
	;

statements
	: statement
	| statements ';' statement
	;

statement
	: assignment_statement
	| expression
	{
		/* expression always has a value */

		p_shSeCodeGen(IGNORE, NONE);
	}
	;

assignment_statement
	: storage '=' expression
	{
		p_shSeCodeGen(ASSIGN, NONE);
	}
	;

method
	: object_prefix '.' IDENTIFIER '(' arg_list ')'
	{
		DATA_ELEMENT d;
		METHOD_ID mid;

		/* I don't know what I'm doing here ... */

		d = (*seFindMethod)(shNameGetFromType($1.value.h.type),
			$3.value.s, $5.att,
			&arg_stack[arg_idx - $5.att], &mid);
		arg_idx -= $5.att;
	
		if (d.d_class == DUMMY)
		{
			yyerror((char*)shErrStackGetEarliest());
			YYABORT;
		}
	
		$$.type = d.type;
	
		if (d.type == T_OBJECT || d.type == T_SCHEMA)
		{
			$$.value.h.ptr = d.value.han.ptr;
			$$.value.h.type = d.value.han.type;
			$$.value.h.indirection = 0;
			d.size = dervish_type_sizeof(d.value.han.type);
		}
		else
		{
			d.size = p_shStdSize(d.type);
		}
	
		d.d_class = METHOD;
		d.value.md.argc = $5.att;
		d.value.md.mid = mid;
		p_shSeCodeGen(EXEC, d);
	}
	| object_prefix '.' '=' '(' arg_list ')'
	{
		DATA_ELEMENT d;
		METHOD_ID mid;

		/* I don't know what I'm doing here ... */

		d = (*seFindMethod)(shNameGetFromType($1.value.h.type),
			"=", $5.att,
			&arg_stack[arg_idx - $5.att], &mid);
		arg_idx -= $5.att;
	
		if (d.d_class == DUMMY)
		{
			yyerror((char*)shErrStackGetEarliest());
			YYABORT;
		}
	
		$$.type = d.type;
	
		if (d.type == T_OBJECT || d.type == T_SCHEMA)
		{
			$$.value.h.ptr = d.value.han.ptr;
			$$.value.h.type = d.value.han.type;
			$$.value.h.indirection = 0;
			d.size = dervish_type_sizeof(d.value.han.type);
		}
		else
		{
			d.size = p_shStdSize(d.type);
		}
	
		d.d_class = METHOD;
		d.value.md.argc = $5.att;
		d.value.md.mid = mid;
		p_shSeCodeGen(EXEC, d);
	}
	| object_prefix '.' C_EQ '(' arg_list ')'
	{
		DATA_ELEMENT d;
		METHOD_ID mid;

		/* I don't know what I'm doing here ... */

		d = (*seFindMethod)(shNameGetFromType($1.value.h.type),
			"==", $5.att,
			&arg_stack[arg_idx - $5.att], &mid);
		arg_idx -= $5.att;
	
		if (d.d_class == DUMMY)
		{
			yyerror((char*)shErrStackGetEarliest());
			YYABORT;
		}
	
		$$.type = d.type;
	
		if (d.type == T_OBJECT || d.type == T_SCHEMA)
		{
			$$.value.h.ptr = d.value.han.ptr;
			$$.value.h.type = d.value.han.type;
			$$.value.h.indirection = 0;
			d.size = dervish_type_sizeof(d.value.han.type);
		}
		else
		{
			d.size = p_shStdSize(d.type);
		}
	
		d.d_class = METHOD;
		d.value.md.argc = $5.att;
		d.value.md.mid = mid;
		p_shSeCodeGen(EXEC, d);
	}
	| object_reference IDENTIFIER '(' arg_list ')'
	{
		DATA_ELEMENT d;
		METHOD_ID mid;
  
		if ($1.type != T_OBJECT)
		{
			yyerror("method return type not a C++ class instance object");
			YYABORT;
		}

		/* I don't know what I'm doing here ... */

		d = (*seFindMethod)(shNameGetFromType($1.value.h.type),
			$2.value.s, $4.att,
			&arg_stack[arg_idx - $4.att], &mid);
		arg_idx -= $4.att;

		if (d.d_class == DUMMY)
		{
			yyerror((char*)shErrStackGetEarliest());
			YYABORT;
		}

		$$.type = d.type;

		if (d.type == T_OBJECT || d.type == T_SCHEMA)
		{
			$$.value.h.ptr = d.value.han.ptr;
			$$.value.h.type = d.value.han.type;
			$$.value.h.indirection = 0;
			d.size = dervish_type_sizeof(d.value.han.type);
		}
		else
		{
			d.size = p_shStdSize(d.type);
		}

		d.d_class = METHOD;
		d.value.md.argc = $4.att;
		d.value.md.mid = mid;
		p_shSeCodeGen(EXEC, d);
	}
	;

object_prefix
	: object
	| method
	;

object_reference
	: object_prefix ARROW
	{
		DATA_ELEMENT d;
		METHOD_ID mid;

		/* I don't know what I'm doing here ... */

		d = (*seFindMethod)(shNameGetFromType($1.value.h.type), "->",
			0, &arg_stack[arg_idx], &mid);

		if (d.d_class == DUMMY)
		{
			yyerror((char*)shErrStackGetEarliest());
			YYABORT;
		}

		$$.value.h.ptr = d.value.han.ptr;
		$$.value.h.type = d.value.han.type;
		$$.value.h.indirection = 0;
		$$.type = d.type;

		d.size = dervish_type_sizeof(d.value.han.type);
	
		d.d_class = METHOD;
		d.value.md.argc = 0;
		d.value.md.mid = mid;
		p_shSeCodeGen(EXEC, d);
	}
	;

constructor
	: NEW clustering_directive IDENTIFIER '(' arg_list ')'
	{
		DATA_ELEMENT d;
		FUNC_ID fid;
		int persistent;

		if ($2.type == T_LONG)
		{
			persistent = 0;
		}
		else
		{
			persistent = 1;
		}

		d = (*seFindConstructor)($3.value.s, $5.att,
			&arg_stack[arg_idx - $5.att],
			&fid, persistent);
		arg_idx -= $5.att;

		if (d.d_class == DUMMY)
		{
			show_constructor_finding($3.value.s,
				&arg_stack[arg_idx], $5.att);
			yyerror((char*)shErrStackGetEarliest());
			YYABORT;
		}

		$$.type = T_OBJECT;
		$$.value.h.type = d.value.han.type;
		d.value.fn.returnType = d.value.han.type;

		if (persistent)
		{
			d.d_class = FUNCTION;
			d.value.fn.clusteringDirective.ptr = $2.value.h.ptr;
			d.value.fn.clusteringDirective.type = $2.value.h.type;
			d.size = dervish_type_sizeof($2.value.h.type);
		}
		else
		{
			d.d_class = V_FUNCTION;
			d.size = dervish_type_sizeof(d.value.han.type);
		}

		d.value.fn.argc = $5.att;
		d.value.fn.fid = fid;
		p_shSeCodeGen(EXEC, d);

		if (!persistent)
		{
			p_shSeCodeGen(STD, NONE);
		}
	}
	;

clustering_directive
: '(' INTEGER ')'
{
  if ($2.value.l != 0)
    {
      yyerror("illegal clustering directive");
      YYABORT;
    }
  $$.type = T_LONG;
}
| '(' OBJECT_HANDLE ')'   {$$.type = T_OBJECT; $$.value.h = $2.value.h;}
|                  {$$.type = T_OBJECT; $$.value.h.ptr = NULL;}
;

arg_list
	: arg
	{
		DATA_ELEMENT d;

		$$.att = 1;
		d.d_class = LITERAL;
		d.type = $1.type;
		if ($1.type == T_SCHEMA || $1.type == T_OBJECT)
		{
			d.value.han.type = $1.value.h.type;
			d.value.han.ptr = $1.value.h.ptr;
			d.size = dervish_type_sizeof($1.value.h.type);
		}
		else
		{
			d.size = p_shStdSize(d.type);
		}
		if (arg_add(d))
			return(1);
	}
	| arg_list ',' arg
	{
		DATA_ELEMENT d;

		$$.att = $1.att + 1;
		d.d_class = LITERAL;
		d.type = $3.type;
		if ($3.type == T_SCHEMA || $3.type == T_OBJECT)
		{
			d.value.han.ptr = $3.value.h.ptr;
			d.value.han.type = $3.value.h.type;
			d.size = dervish_type_sizeof($3.value.h.type);
		}
		else
		{
			d.size = p_shStdSize(d.type);
		}
		if (arg_add(d))
			return(1);
	}
	|
	{
		$$.att = 0;
	}
	;

cast
	: '(' type ')'
	{
		$$.type = $2.type;
	}
	;


type
	: pointer
	| elementary_type
	;

pointer
	: elementary_type '*'
	{
		switch($1.type)
		{
		case T_CHAR:
			$$.type = T_CHAR_P;
			break;
		case T_INT:
			$$.type = T_INT_P;
			break;
		case T_SHORT:
			$$.type = T_SHORT_P;
			break;
		case T_LONG:
			$$.type = T_LONG_P;
			break;
		case T_FLOAT:
			$$.type = T_FLOAT_P;
			break;
		case T_DOUBLE:
			$$.type = T_DOUBLE_P;
			break;
		case T_U_CHAR:
			$$.type = T_U_CHAR_P;
			break;
		case T_U_INT:
			$$.type = T_U_INT_P;
			break;
		case T_U_SHORT:
			$$.type = T_U_SHORT_P;
			break;
		case T_U_LONG:
			$$.type = T_U_LONG_P;
			break;
		default:
			yyerror("illegal pointer type");
			YYABORT;
			break;
		}
	}
	;

elementary_type
	: gen_type
	| TT_UNSIGNED gen_type
	{
		switch($2.type)
		{
		case T_INT:
			$$.type = T_U_INT;
			break;
		case T_SHORT:
			$$.type = T_U_SHORT;
			break;
		case T_LONG:
			$$.type = T_U_LONG;
			break;
		case T_CHAR:
			$$.type = T_U_CHAR;
			break;
		default:
			yyerror("illegal unsigned type");
			YYABORT;
			break;
		}
	}
	;

gen_type
	: TT_INT      {$$.type = T_INT;}
	| TT_SHORT    {$$.type = T_SHORT;}
	| TT_LONG     {$$.type = T_LONG;}
	| TT_FLOAT    {$$.type = T_FLOAT;}
	| TT_DOUBLE   {$$.type = T_DOUBLE;}
	| TT_CHAR     {$$.type = T_CHAR;}
	;

arg
	: expression
	;

expression
	: conditional_expression
	;

conditional_expression
	: logical_or_expression
	| logical_or_expression if break_or_expression
	{
		p_shSe_code_patch($2.att, p_shSe_code_idx() - $2.att - 1);
		$$.type = $3.type;
	}
	| logical_or_expression if break_or_expression else break_or_expression
	{
		p_shSe_code_patch($2.att, $4.att - $2.att);
		p_shSe_code_patch($4.att, p_shSe_code_idx() - $4.att - 1);
		$$.type = $3.type;	/* what else can I do? */
	}
	;

break_or_expression
	: break
	| continue
	| expression
	;

break
	: P_BREAK
	{
		p_shSeCodeGen(BREAK, NONE);
		$$.type = T_DONTCARE;
	}
	;

continue
	: P_CONTINUE
	{
		p_shSeCodeGen(CONTINUE, NONE);
		$$.type = T_DONTCARE;
	}
	;
	
logical_or_expression
	: logical_and_expression
	| logical_or_expression L_OR logical_and_expression
	{
		p_shSeCodeGen(LOR, NONE);
		$$.type = T_INT;
	}
	;

logical_and_expression
	: bitwise_or_expression
	| logical_and_expression L_AND bitwise_or_expression
	{
		p_shSeCodeGen(LAND, NONE);
		$$.type = T_INT;
	}
	;

bitwise_or_expression
	: bitwise_xor_expression
	| bitwise_or_expression '|' bitwise_xor_expression
	{
		if ($1.type != $3.type)
		{
			yyerror("incompatible types for bitwise or");
			YYABORT;
		}

		switch ($1.type)
		{
		case T_CHAR:
		case T_INT:
		case T_LONG:
		case T_SHORT:
		case T_U_CHAR:
		case T_U_INT:
		case T_U_LONG:
		case T_U_SHORT:
			break;
		default:
			yyerror("bad type for bitwise or");
			YYABORT;
		}

		p_shSeCodeGen(BOR, NONE);
	}
	;

bitwise_xor_expression
	: bitwise_and_expression
	| bitwise_xor_expression '^' bitwise_and_expression
	{
		if ($1.type != $3.type)
		{
			yyerror("incompatible types for bitwise xor");
			YYABORT;
		}

		switch ($1.type)
		{
		case T_CHAR:
		case T_INT:
		case T_LONG:
		case T_SHORT:
		case T_U_CHAR:
		case T_U_INT:
		case T_U_LONG:
		case T_U_SHORT:
			break;
		default:
			yyerror("bad type for bitwise xor");
			YYABORT;
		}

		p_shSeCodeGen(BXOR, NONE);
	}
	;

bitwise_and_expression
	: equality_expression
	| bitwise_and_expression '&' equality_expression
	{
		if ($1.type != $3.type)
		{
			yyerror("incompatible types for bitwise and");
			YYABORT;
		}

		switch ($1.type)
		{
		case T_CHAR:
		case T_INT:
		case T_LONG:
		case T_SHORT:
		case T_U_CHAR:
		case T_U_INT:
		case T_U_LONG:
		case T_U_SHORT:
			break;
		default:
			yyerror("bad type for bitwise and");
			YYABORT;
		}

		p_shSeCodeGen(BAND, NONE);
	}
	;

equality_expression
	: relational_expression
	| equality_expression C_EQ relational_expression
	{
		p_shSeCodeGen(EQ, NONE);
		$$.type = T_INT;
	}
	| equality_expression C_NE relational_expression
	{
		p_shSeCodeGen(NE, NONE);
		$$.type = T_INT;
	}
	;

relational_expression
	: shift_expression
	| relational_expression C_GT shift_expression
	{
		p_shSeCodeGen(GT, NONE);
		$$.type = T_INT;
	}
	| relational_expression C_LT shift_expression
	{
		p_shSeCodeGen(LT, NONE);
		$$.type = T_INT;
	}
	| relational_expression C_GE shift_expression
	{
		p_shSeCodeGen(GE, NONE);
		$$.type = T_INT;
	}
	| relational_expression C_LE shift_expression
	{
		p_shSeCodeGen(LE, NONE);
		$$.type = T_INT;
	}
	;

shift_expression
	: terms
	| shift_expression B_SL terms
	{
		p_shSeCodeGen(SL, NONE);
		$$.type = T_INT;
	}
	| shift_expression B_SR terms
	{
		p_shSeCodeGen(SR, NONE);
		$$.type = T_INT;
	}
	;

if
	: '?'
	{
		$$.att = p_shSeCodeGen(SKIP0, NONE);
	}
	;

else
	: ':'
	{
		$$.att = p_shSeCodeGen(SKIP, NONE);
	}
	;

terms
	: term
	| terms '+' term
	{
		p_shSeCodeGen(ADD, NONE);
		if (($$.type = p_shSeBinaryType($1.type, $3.type)) == T_ERROR)
		{
			yyerror("illegal data type for binary arithmetic operation");
			YYABORT;
		}
	}
	| terms '-' term
	{
		p_shSeCodeGen(SUB, NONE);
		if (($$.type = p_shSeBinaryType($1.type, $3.type)) == T_ERROR)
		{
			yyerror("illegal data type for binary arithmetic operation");
			YYABORT;
		}
	}
	;

term
	: factors
	;

factor
	: abs_factor
	| '-' factor
	{
		p_shSeCodeGen(NEG, NONE); $$.type = $2.type;
	}
	| cast factor
	{
		DATA_ELEMENT d;
  
		$$.type = d.type = $1.type;
		d.d_class = DUMMY;
		d.size = p_shStdSize(d.type);
		p_shSeCodeGen(MDT, d);
	}
	| not_operator factor
	{
		p_shSeCodeGen(LNOT, NONE);
		$$.type = T_INT;
	}
	;

not_operator
	: L_NOT
	| '!'
	;

factors
	: factor
	| factors '*' factor
	{
		p_shSeCodeGen(MUL, NONE);
		if (($$.type = p_shSeBinaryType($1.type, $3.type)) == T_ERROR)
		{
			yyerror("illegal data type for binary arithmetic operation");
			YYABORT;
		}
	}
	| factors '/' factor
	{
		p_shSeCodeGen(DIV, NONE);
		if (($$.type = p_shSeBinaryType($1.type, $3.type)) == T_ERROR)
		{
			yyerror("illegal data type for binary arithmetic operation");
			YYABORT;
		}
	}
	;

abs_factor
	: '(' expression ')'
	{$$.type = $2.type;}
	| number
	| storage
	| method
	| built_in_function
	| object
	| string
	| constructor
	;

storage
	: shandle
	{
		DATA_ELEMENT d;

		$$.type = p_shSeDervishType2ElemType($1.value.h.type);
		if (($$.type == T_CHAR) && ($1.value.h.indirection == 1))
		{
			$$.type = T_CHAR_P;
		}

		if ($1.type == T_CHAIN)
		{
			d.d_class = CURSOR2;
			d.type = T_CHAIN;
			d.size = dervish_type_sizeof($1.value.h.type);
			d.value.ci.cep = (CHAIN_ELEM *) $$.value.h.ptr;
			d.value.ci.offset = 0;
			d.value.ci.type = $1.value.h.type;
			d.value.ci.indirection = $1.value.h.indirection;
			d.value.ci.dim = 0;
			d.value.ci.nelem = NULL;

			p_shSeCodeGen(PUSH, d);
		}
		else if ($1.type == T_SCHEMA)
		{
			d.d_class = HANDLE2;
			d.type = T_SCHEMA;
			d.size = dervish_type_sizeof($1.value.h.type);
			d.value.hi.ptr = $1.value.h.ptr;
			d.value.hi.type = $1.value.h.type;
			d.value.hi.indirection = $1.value.h.indirection;
			d.value.hi.dim = 0;
			d.value.hi.nelem = NULL;

			p_shSeCodeGen(PUSH, d);
		}
		else
		{
			yyerror("unknown handle");
			YYABORT;
		}
	}
	| complex_storage
	{
		DATA_ELEMENT d;

 		$$.type = p_shSeDervishType2ElemType($1.value.h.type);
		if (($$.type == T_CHAR) && ($1.value.h.indirection == 1))
		{
			$$.type = T_CHAR_P;
		}

		if ($1.type == T_CHAIN)
		{
			d.d_class = CURSOR2;
			d.type = T_CHAIN;
			d.size = dervish_type_sizeof($$.value.h.type);
			d.value.ci.cep = (CHAIN_ELEM *) $$.value.h.ptr;
			d.value.ci.offset = $$.att;
			d.value.ci.type = $$.value.h.type;
			d.value.ci.indirection = $$.value.h.indirection;
			d.value.ci.dim = 0;
			d.value.ci.nelem = NULL;

			p_shSeCodeGen(PUSH, d);
		}
		else if ($1.type == T_SCHEMA)
		{
			d.d_class = HANDLE2;
			d.type = T_SCHEMA;
			d.size = dervish_type_sizeof($$.value.h.type);
			d.value.hi.ptr = (char *) $$.value.h.ptr + $$.att;
			d.value.hi.type = $$.value.h.type;
			d.value.hi.indirection = $$.value.h.indirection;
			d.value.hi.dim = 0;
			d.value.hi.nelem = NULL;

			p_shSeCodeGen(PUSH, d);
		}
		else if (($1.type == T_CONT_PERIOD) ||
			 ($1.type == T_CONT_ARROW))
		{
			d.d_class = HANDLE2;
			d.type = $1.type;
			d.size = dervish_type_sizeof($$.value.h.type);
			d.value.chan.offset = $$.att;
			d.value.chan.type = $$.value.h.type;
			d.value.chan.indirection = $$.value.h.indirection;
			d.value.chan.dim = 0;
			d.value.chan.nelem = NULL;

			p_shSeCodeGen(PUSH, d);
		}
		else if ($1.type != T_CONTINUE)
		{
			yyerror("unknown handle");
			YYABORT;
		}
	}
	| vector
	{
		DATA_ELEMENT d;

		d.type = T_VECTOR;
		d.d_class = CURSOR2;
		d.size = p_shStdSize(T_DOUBLE);
		d.value.vi.vec = (VECTOR *) $1.value.h.ptr;
		d.value.vi.n = $1.att;

  		p_shSeCodeGen(PUSH, d);
  		$$.type = T_DOUBLE;
	}
	| tblcol_elem
	{
		DATA_ELEMENT d;

		d.d_class = CURSOR2;
		d.type = T_TBLCOL;
		d.ind = 0;
		d.value.tci.array = $1.value.a;
		d.value.tci.type = $1.type;
		d.value.tci.dim = $1.att;
		d.value.tci.pos = $1.att2;
		d.size = p_shStdSize($1.type);
		p_shSeCodeGen(PUSH, d);
	}
	;

base_storage
	: IDENTIFIER
	| array
	;

array
	: IDENTIFIER indexes
	{
		$$.att = $2.att;
	}
	;

handle_prefix
	: shandle "." base_storage
	{
		const SCHEMA_ELEM *se;
		DATA_ELEMENT d;
		int dims;
		char s[1024];

		/* cache the dervish type */

		int shtSTR = shTypeGetFromName("STR");
		int shtCHAR = shTypeGetFromName("CHAR");

		/* get the schema element */

		se = shSchemaElemGet(shNameGetFromType($1.value.h.type),
			$3.value.s);

		if (se == NULL)
		{
			sprintf(s, "%s is not an element of %s",
				$3.value.s,
				shNameGetFromType($1.value.h.type));
			yyerror(s);
			YYABORT;
		}

		/* calculate the offset and type
		 *
		 * This is stupid! The type recorded in SCHAME_ELEM
		 * should be TYPE, instead of the ascii name of the type
		 * as it is now.
		 *
		 * shcema handling routines should handle the TYPE as
		 * internal TYPE, rather than ascii names of the types.
		 * dealing with internal TYPE directly would make our
		 * life a lot easier and more efficient 
		 */

		$$.att = $1.att + se->offset;
		$$.value.h.type = shTypeGetFromName((char *) se->type);
		$$.value.h.indirection = se->nstar;

		dims = get_dims(se->nelem);

		/* Taking care of char array */

		if (($$.value.h.type == shtCHAR) && (dims == 1) &&
		    ($3.att == 0) && (se->nstar == 0))
		{
			$$.value.h.type = shtSTR;
		}
		else if ($3.att > dims)
		{
			yyerror("too many dimensions");
			YYABORT;
		}

		if (dims) /* array */
		{
			if ($1.type == T_CHAIN)
			{
				d.d_class = CURSOR2;
				d.type = T_CHAIN;
				d.size = se->size;
				d.value.ci.cep = (CHAIN_ELEM *) $$.value.h.ptr;
				d.value.ci.offset = $$.att;
				d.value.ci.type = $$.value.h.type;
				d.value.ci.indirection = se->nstar;
				d.value.ci.dim = $3.att;
				d.value.ci.nelem = se->nelem;
			}
			else if ($1.type == T_SCHEMA)
			{
				d.d_class = HANDLE2;
				d.type = T_SCHEMA;
				d.size = se->size;
				d.value.hi.ptr = (char *) $$.value.h.ptr + $$.att;
				d.value.hi.type = $$.value.h.type;
				d.value.hi.indirection = se->nstar;
				d.value.hi.dim = $3.att;
				d.value.hi.nelem = se->nelem;
			}
			else
			{
				yyerror("unknown handle");
				YYABORT;
			}

			p_shSeCodeGen(PUSH, d);

			/* do something to signify that the head has
			 * been taken care of
			 */

			$$.type = T_CONTINUE;
			$$.att = 0;
		}

	}
	;

shandle
	: SCHEMA_HANDLE
	{
		$$.att = 0;
	}
	| CHAIN_HANDLE
	{
		/* in semantics, CHAIN_HANDLE means the handle to the
		 * the elements in a chain
		 */

		if ((((CHAIN *) $1.value.h.ptr)->pFirst) == NULL)
		{
			yyerror("Empty Chain involved");
			YYABORT;
		}

		/* here, $$.value.h.ptr is pointing to the first
		 * chain element and $$.value.h.type is the dervish type
		 * of that element
		 */

		$$.value.h.ptr = (void *) ((CHAIN *) $1.value.h.ptr)->pFirst;
		$$.value.h.type = ((CHAIN_ELEM *)((CHAIN *) $1.value.h.ptr)->pFirst)->type;
		$$.value.h.indirection = 0;
		$$.att = 0;
	}
	| CHAIN_HANDLE "@" INTEGER
	{
		int i;
		char s[1024];

		/* in semantics, CHAIN_HANDLE means the handle to the
		 * the elements in a chain
		 */

		if ((((CHAIN *) $1.value.h.ptr)->pFirst) == NULL)
		{
			yyerror("Empty Chain involved");
			YYABORT;
		}

		/* here, $$.value.h.ptr is pointing to the n-th
		 * chain element and $$.value.h.type is the dervish type
		 * of that element
		 */

		if (($3.value.l < 0) || ($3.value.l >= ((CHAIN *) $1.value.h.ptr)->nElements))
		{
			sprintf(s,
				"chain index (%d) out of range (0:%d)",
				$3.value.l, ((CHAIN *) $1.value.h.ptr)->nElements - 1);
			yyerror(s);
			YYABORT;
		}

		$$.value.h.ptr = (void *) ((CHAIN *) $1.value.h.ptr)->pFirst;
		for (i = 0; i < $3.value.l; i++)
		{
			$$.value.h.ptr = (void *) ((CHAIN_ELEM *) $$.value.h.ptr)->pNext;
		}

		$$.value.h.type = ((CHAIN_ELEM *)((CHAIN *) $1.value.h.ptr)->pFirst)->type;
		$$.value.h.indirection = 0;
		$$.att = 0;
	}
	;

complex_storage
	: handle_prefix
	| complex_storage "." base_storage
	{
		const SCHEMA_ELEM *se;
		DATA_ELEMENT d;
		int dims;
		char s[1024];

		/* cache the dervish type */

		int shtSTR = shTypeGetFromName("STR");
		int shtCHAR = shTypeGetFromName("CHAR");

		/* get the schema first */

		se = shSchemaElemGet(shNameGetFromType($1.value.h.type),
			$3.value.s);

		if (se == NULL)
		{
			sprintf(s, "%s is not an element of %s",
				$3.value.s,
				shNameGetFromType($1.value.h.type));
			yyerror(s);
			YYABORT;
		}

		if ($1.type == T_CONTINUE)
		{
			$$.type = T_CONT_PERIOD;
		}

		/* calculate the offest */

		$$.att = $1.att + se->offset;
		$$.value.h.type = shTypeGetFromName((char *) se->type);
		$$.value.h.indirection = se->nstar;

		dims = get_dims(se->nelem);

		/* Taking care of char array */

		if (($$.value.h.type == shtCHAR) && (dims == 1) &&
		    ($3.att == 0) && (se->nstar == 0))
		{
			$$.value.h.type = shtSTR;
		}
		else if ($3.att > dims)
		{
			yyerror("too many dimensions");
			YYABORT;
		}

		if (dims) /* array */
		{
			if ($1.type == T_CHAIN)
			{
				d.d_class = CURSOR2;
				d.type = T_CHAIN;
				d.size = se->size;
				d.value.ci.cep = (CHAIN_ELEM *) $$.value.h.ptr;
				d.value.ci.offset = $$.att;
				d.value.ci.type = $$.value.h.type;
				d.value.ci.indirection = se->nstar;
				d.value.ci.dim = $3.att;
				d.value.ci.nelem = se->nelem;
			}
			else if ($1.type == T_SCHEMA)
			{
				d.d_class = HANDLE2;
				d.type = T_SCHEMA;
				d.size = se->size;
				d.value.hi.ptr = (char *) $$.value.h.ptr + $$.att;
				d.value.hi.type = $$.value.h.type;
				d.value.hi.indirection = se->nstar;
				d.value.hi.dim = $3.att;
				d.value.hi.nelem = se->nelem;
			}
			else if (($1.type == T_CONT_PERIOD) ||
				 ($1.type == T_CONTINUE) ||
				 ($1.type == T_CONT_ARROW))
			{
				d.d_class = HANDLE2;
				d.type = ($1.type == T_CONTINUE)?T_CONT_PERIOD:$1.type;
				d.size = se->size;
				d.value.chan.offset = $$.att;
				d.value.chan.type = $$.value.h.type;
				d.value.chan.indirection = se->nstar;
				d.value.chan.dim = $3.att;
				d.value.chan.nelem = se->nelem;
			}
			else
			{
				yyerror("unknown handle");
				YYABORT;
			}

			p_shSeCodeGen(PUSH, d);

			$$.type = T_CONTINUE;
			$$.att = 0;
		}
	}
	| complex_storage ARROW base_storage
	{
		DATA_ELEMENT d;
		const SCHEMA_ELEM *se;
		int dims;
		char s[1024];

		/* cache the dervish type */

		int shtSTR = shTypeGetFromName("STR");
		int shtCHAR = shTypeGetFromName("CHAR");

		/* handle the prefix first */

		if ($1.value.h.indirection != 1)
		{
			/* this is not a pointer */

			yyerror("non-pointer involved in address expression");
			YYABORT;
		}

		if ($1.type == T_CHAIN)
		{
			/* meaing: no array ever involved in the path */

			d.d_class = CURSOR2;
			d.type = T_CHAIN;
			d.size = ($1.value.h.indirection)?(sizeof(void*)):dervish_type_sizeof($1.value.h.type);
			d.value.ci.cep = (CHAIN_ELEM *) $1.value.h.ptr;
			d.value.ci.type = $1.value.h.type;
			d.value.ci.indirection = $1.value.h.indirection;
			d.value.ci.offset = $1.att;
			d.value.ci.dim = 0;
			d.value.ci.nelem = NULL;
			p_shSeCodeGen(PUSH, d);
		}
		else if ($1.type == T_SCHEMA)
		{
			d.d_class = HANDLE2;
			d.type = T_SCHEMA;
			d.size = ($1.value.h.indirection)?(sizeof(void*)):dervish_type_sizeof($1.value.h.type);
			d.value.hi.ptr = (char *) $1.value.h.ptr + $1.att;
			d.value.hi.type = $1.value.h.type;
			d.value.hi.indirection = $1.value.h.indirection;
			d.value.hi.dim = 0;
			d.value.ci.nelem = NULL;
			p_shSeCodeGen(PUSH, d);
		}
		else if (($1.type == T_CONT_PERIOD) ||
			 ($1.type == T_CONT_ARROW))
		{
			/* This is rather tricky */

			d.d_class = HANDLE2;
			d.type = $1.type;
			d.size = ($1.value.h.indirection)?(sizeof(void*)):dervish_type_sizeof($1.value.h.type);
			d.value.chan.offset = $1.att;
			d.value.chan.type = $1.value.h.type;
			d.value.chan.indirection = $1.value.h.indirection;
			d.value.chan.dim = 0;
			d.value.chan.nelem = NULL;
			p_shSeCodeGen(PUSH, d);
		}
		else if ($1.type != T_CONTINUE)
		{
			yyerror("unknown handle");
			YYABORT;
		}

		/* handling the rest */

		se = shSchemaElemGet(shNameGetFromType($1.value.h.type), $3.value.s);

		if (se == NULL)
		{
			sprintf(s, "%s is not an element of %s",
				$3.value.s,
				shNameGetFromType($1.value.h.type));
			yyerror(s);
			YYABORT;
		}

		$$.type = T_CONT_ARROW;

		$$.att = se->offset;
		$$.value.h.type = shTypeGetFromName((char *) se->type);
		$$.value.h.indirection = se->nstar;

		dims = get_dims(se->nelem);

		/* Taking care of char array */

		if (($$.value.h.type == shtCHAR) && (dims == 1) &&
		    ($3.att == 0) && (se->nstar == 0))
		{
			$$.value.h.type = shtSTR;
		}
		else if ($3.att > dims)
		{
			yyerror("too many dimensions");
			YYABORT;
		}

		if (dims) /* array */
		{
			d.d_class = HANDLE2;
			d.type = T_CONT_ARROW;
			d.size = se->size;
			d.value.chan.offset = $$.att;
			d.value.chan.type = $$.value.h.type;
			d.value.chan.indirection = se->nstar;
			d.value.chan.dim = $3.att;
			d.value.chan.nelem = se->nelem;

			p_shSeCodeGen(PUSH, d);

			$$.type = T_CONTINUE;
			$$.att = 0;
		}
	}
	;

built_in_function
	: arithmetic_function
	| string_function
	;

arithmetic_function
	: F_SIN '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(SIN, NONE);
	}
	| F_COS '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(COS, NONE);
	}
	| F_TAN '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(TAN, NONE);
	}
	| F_ASIN '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(ASIN, NONE);
	}
	| F_ACOS '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(ACOS, NONE);
	}
	| F_ATAN '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(ATAN, NONE);
	}
	| F_EXP '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(EXP, NONE);
	}
	| F_LOG '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(LOG, NONE);
	}
	| F_POW '(' expression ',' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(POW, NONE);
	}
	| F_ABS '(' expression ')'
	{
		$$.type = T_INT;
		p_shSeCodeGen(ABS, NONE);
	}
	| F_ATAN2 '(' expression ',' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(ATAN2, NONE);
	}
	| F_LOG10 '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(LOG10, NONE);
	}
	| F_SQRT '(' expression ')'
	{
		$$.type = T_DOUBLE;
		p_shSeCodeGen(SQRT, NONE);
	}
	| F_MIN '(' expression ',' expression ')'
	{
		if (($$.type = p_shSeBinaryType($3.type, $5.type)) == T_ERROR)
		{
			yyerror("illegal data type for min()");
			YYABORT;
		}
		p_shSeCodeGen(MIN, NONE);
	}
	| F_MAX '(' expression ',' expression ')'
	{
		if (($$.type = p_shSeBinaryType($3.type, $5.type)) == T_ERROR)
		{
			yyerror("illegal data type for max()");
			YYABORT;
		}
		p_shSeCodeGen(MAX, NONE);
	}
	| F_LC
	{
		$$.type = T_INT;
		p_shSeCodeGen(LC, NONE);
	}
	;

str_storage
	: storage
	{
		if (($1.type != T_CHAR_P) && ($1.type != T_STRING))
		{
			yyerror("unknown string storage");
			YYABORT;
		}
	}
	| string_function
	{
		if (($1.type != T_CHAR_P) && ($1.type != T_STRING))
		{
			yyerror("unknown string storage");
			YYABORT;
		}
	}
	;

str_arg
	: str_storage
	| string
	| method
	;

string_function
	: F_STRCPY '(' str_storage ',' str_arg ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("first argument of strcpy must be a string or an array of characters");
			YYABORT;
		}

		if (($5.type != T_CHAR_P) && ($5.type != T_STRING))
		{
			yyerror("second argument of strcpy must be a string or an array of characters");
			YYABORT;
		}

		$$.type = T_CHAR_P;
		p_shSeCodeGen(STRCPY, NONE);
	}
	| F_STRNCPY '(' str_storage ',' str_arg ',' expression ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("first argument of strncpy must be a string or an array of characters");
			YYABORT;
		}

		if (($5.type != T_CHAR_P) && ($5.type != T_STRING))
		{
			yyerror("second argument of strncpy must be a string or an array of characters");
			YYABORT;
		}

		if (!p_shSeIsNumericalType($7.type))
		{
			yyerror("third argument of strncpy must be a number");
			YYABORT;
		}

		$$.type = T_CHAR_P;
		p_shSeCodeGen(STRNCPY, NONE);
	}
	| F_STRCAT '(' str_storage ',' str_arg ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("first argument of strcat must be a string or an array of characters");
			YYABORT;
		}

		if (($5.type != T_CHAR_P) && ($5.type != T_STRING))
		{
			yyerror("second argument of strcat must be a string or an array of characters");
			YYABORT;
		}

		$$.type = T_CHAR_P;
		p_shSeCodeGen(STRCAT, NONE);
	}
	| F_STRNCAT '(' str_storage ',' str_arg ',' expression ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("first argument of strncat must be a string or an array of characters");
			YYABORT;
		}

		if (($5.type != T_CHAR_P) && ($5.type != T_STRING))
		{
			yyerror("second argument of strncat must be a string or an array of characters");
			YYABORT;
		}

		if (!p_shSeIsNumericalType($7.type))
		{
			yyerror("third argument of strncat must be a number");
			YYABORT;
		}

		$$.type = T_CHAR_P;
		p_shSeCodeGen(STRNCAT, NONE);
	}
	| F_STRCMP '(' str_arg ',' str_arg ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("first argument of strcmp must be a string or an array of characters");
			YYABORT;
		}

		if (($5.type != T_CHAR_P) && ($5.type != T_STRING))
		{
			yyerror("second argument of strcmp must be a string or an array of characters");
			YYABORT;
		}

		$$.type = T_INT;
		p_shSeCodeGen(STRCMP, NONE);
	}
	| F_STRNCMP '(' str_arg ',' str_arg ',' expression ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("first argument of strncmp must be a string or an array of characters");
			YYABORT;
		}

		if (($5.type != T_CHAR_P) && ($5.type != T_STRING))
		{
			yyerror("second argument of strncmp must be a string or an array of characters");
			YYABORT;
		}

		if (!p_shSeIsNumericalType($7.type))
		{
			yyerror("third argument of strncmp must be a number");
			YYABORT;
		}

		$$.type = T_INT;
		p_shSeCodeGen(STRNCMP, NONE);
	}
	| F_STRCASECMP '(' str_arg ',' str_arg ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("first argument of strcasecmp must be a string or an array of characters");
			YYABORT;
		}

		if (($5.type != T_CHAR_P) && ($5.type != T_STRING))
		{
			yyerror("second argument of strcasecmp must be a string or an array of characters");
			YYABORT;
		}

		$$.type = T_INT;
		p_shSeCodeGen(STRCASECMP, NONE);
	}
	| F_STRNCASECMP '(' str_arg ',' str_arg ',' expression ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("first argument of strncasecmp must be a string or an array of characters");
			YYABORT;
		}

		if (($5.type != T_CHAR_P) && ($5.type != T_STRING))
		{
			yyerror("second argument of strncasecmp must be a string or an array of characters");
			YYABORT;
		}

		if (!p_shSeIsNumericalType($7.type))
		{
			yyerror("third argument of strncasecmp must be a number");
			YYABORT;
		}

		$$.type = T_INT;
		p_shSeCodeGen(STRNCASECMP, NONE);
	}
	| F_STRLEN '(' str_arg ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("argument of strlen must be a string or an array of characters");
			YYABORT;
		}

		$$.type = T_INT;
		p_shSeCodeGen(STRLEN, NONE);
	}
	| F_STROFFSET '(' str_arg ',' expression ')'
	{
		if (($3.type != T_CHAR_P) && ($3.type != T_STRING))
		{
			yyerror("first argument of stroffset must be a string or an array of characters");
			YYABORT;
		}

		if (!p_shSeIsNumericalType($5.type))
		{
			yyerror("third argument of strncasecmp must be a number");
			YYABORT;
		}

		$$.type = T_CHAR_P;
		p_shSeCodeGen(STROFFSET, NONE);
	}
		
	;

tblcol_elem
	: tblcol_field indexes
	{
  		S_TYPE type;
  		ARRAY* array = $1.value.a;
  
		if (array->dimCnt != ($2.att + 1))
		{
			yyerror("incorrect number of indices for TBLCOL");
			YYABORT;
		}

		if (array->nStar > 0)
		{
			yyerror("indirection not allowed in TBLCOL fields");
			YYABORT;
		}

		$$.att = $2.att;

		if ((type = p_shSeDervishType2ElemType(array->data.schemaType))
			== T_ERROR)
		{
			yyerror("TBLCOL fields must contain elementary types");
			YYABORT;
		}

		$$.type = type;
		$$.att2 = $1.att;
	}
	| tblcol_field
	{
		S_TYPE type;

		if ((type = p_shSeDervishType2ElemType($1.value.a->data.schemaType))
			== T_ERROR)
		{
			yyerror("TBLCOL fields must contain elementary types");
			YYABORT;
		}
		$$.type = type;
		$$.att = 0;
		$$.att2 = $1.att;
	}
	;

tblcol_field
	: TBLCOL_HANDLE '.' tblcol_field_name
	{
  		ARRAY* array;
  		TBLFLD* tblFld;
  
  		if (shTblFldLoc((TBLCOL*)$1.value.h.ptr, -1, $3.value.s, 0,
			SH_CASE_INSENSITIVE, &array, &tblFld, NULL) != SH_SUCCESS)
		{
			char s[100];
			sprintf(s, "TBLCOL field '%s' not found", $3.value.s);
			yyerror(s);
			YYABORT;
		}
		$$.type = T_ARRAY;
		$$.value.a = array;
		$$.att = 0;
	}
	| TBLCOL_HANDLE '.' INTEGER
	{
		ARRAY* array;
		TBLFLD* tblFld;
  
		if (shTblFldLoc((TBLCOL*)$1.value.h.ptr, (int) $3.value.l, NULL, 0,
			SH_CASE_INSENSITIVE, &array, &tblFld, NULL) != SH_SUCCESS)
		{
			char s[100];
			sprintf(s, "TBLCOL field '%ld' not found", $3.value.l);
			yyerror(s);
			YYABORT;
		}
		$$.type = T_ARRAY;
		$$.value.a = array;
		$$.att = 0;
	}
	| TBLCOL_HANDLE '@' INTEGER '.' tblcol_field_name
	{
  		ARRAY* array;
  		TBLFLD* tblFld;
  
  		if (shTblFldLoc((TBLCOL*)$1.value.h.ptr, -1, $5.value.s, 0,
			SH_CASE_INSENSITIVE, &array, &tblFld, NULL) != SH_SUCCESS)
		{
			char s[100];
			sprintf(s, "TBLCOL field '%s' not found", $3.value.s);
			yyerror(s);
			YYABORT;
		}
		$$.type = T_ARRAY;
		$$.value.a = array;
		$$.att = $3.value.l;
	}
	| TBLCOL_HANDLE '@' INTEGER '.' INTEGER
	{
		ARRAY* array;
		TBLFLD* tblFld;
  
		if (shTblFldLoc((TBLCOL*)$1.value.h.ptr, (int) $5.value.l, NULL, 0,
			SH_CASE_INSENSITIVE, &array, &tblFld, NULL) != SH_SUCCESS)
		{
			char s[100];
			sprintf(s, "TBLCOL field '%d' not found", $3.value.l);
			yyerror(s);
			YYABORT;
		}
		$$.type = T_ARRAY;
		$$.value.a = array;
		$$.att = $3.value.l;
	}
	;

tblcol_field_name
	: IDENTIFIER
	| STRING
	;

indexes
	: index
	{
		$$.att = 1;
	}
	| indexes index
	{
		/* the indexes in the stack is in reversed order */
		/* the one on the top of the stack is the last one */

		$$.att = $1.att + 1;
	}
	;

index
	: '[' expression ']'
	| '<' expression '>'
	;

string
	: STRING
	{
		DATA_ELEMENT d;
  
		d.type = T_STRING;
		d.d_class = LITERAL;
		d.value.str = register_string($1.value.s);
		if (d.value.str == NULL)	/* something is wrong */
		{
			yyerror("insufficient memory for static strings");
			YYABORT;
		}
		d.size = strlen($1.value.s) + 1;
		p_shSeCodeGen(PUSH, d);
		$$ = $1;
	}
	;

number
	: INTEGER
	{
		DATA_ELEMENT d;
  
		d.type = T_LONG;
		d.d_class = LITERAL;
		d.size = p_shStdSize(d.type);
		d.value.l = $1.value.l;
		p_shSeCodeGen(PUSH, d);
		$$ = $1;
	}
	| FLOAT
	{
		DATA_ELEMENT d;
  
		d.type = T_DOUBLE;
		d.d_class = LITERAL;
		d.size = p_shStdSize(d.type);
		d.value.d = $1.value.d;
		p_shSeCodeGen(PUSH, d);
		$$ = $1;
	}
	;

vector
	: VECTOR_HANDLE
	{
		$$.type = T_DOUBLE;
		$$.att = 0;
	}
	| VECTOR_HANDLE "@" INTEGER
	{
		char s[1024];

		$$.type = T_DOUBLE;
		if (($3.value.l < 0) ||
		($3.value.l >= ((VECTOR *) $1.value.h.ptr)->dimen))
		{
			sprintf(s,
				"vector index (%ld) out of range (0:%d)",
				$3.value.l, ((VECTOR *) $1.value.h.ptr)->dimen - 1);
			yyerror(s);
			YYABORT;
		}
		$$.att = (int) $3.value.l;
	}
	;

object
	: OBJECT_HANDLE
	{
		DATA_ELEMENT d;

		d.type = T_OBJECT;
		d.d_class = HANDLE2;
		d.size = dervish_type_sizeof($1.value.h.type);
		d.value.han.ptr = $1.value.h.ptr;
		d.value.han.type = $1.value.h.type;
		p_shSeCodeGen(PUSH, d);
		$$ = $1;
	}
	;

%%

void p_shSeSymCopyHandle(S_TYPE type, HANDLE value)
{
	yylval.type = type;
	yylval.value.h.type = value.type;
	yylval.value.h.ptr = value.ptr;
	yylval.value.h.indirection = 0;
	yylval.att = 0;
	switch(type)
	{
	case T_CHAIN:
		p_shSeAdjustIterLength(((CHAIN *) value.ptr)->nElements);
		break;
	case T_VECTOR:
		p_shSeAdjustIterLength(((VECTOR *) value.ptr)->dimen);
		break;
	case T_TBLCOL:
		p_shSeAdjustIterLength(((TBLCOL *) value.ptr)->rowCnt);
		break;
	default:
		break;
	}
}

void p_shSeSymCopyLong(long value)
{
	yylval.type = T_LONG;
	yylval.value.l = value;
	yylval.att = 0;
}

void p_shSeSymCopyDouble(double value)
{
	yylval.type = T_DOUBLE;
	yylval.value.d = value;
	yylval.att = 0;
}

void p_shSeSymCopyIdentifier(const unsigned char* value)
{
	yylval.type = T_DONTCARE;
	yylval.att = 0;
	strcpy(yylval.value.s, (char*)value);
}

void p_shSeSymCopyString(const unsigned char* value)
{
	int n;

	n = strlen(value);

	yylval.type = T_STRING;
	strncpy(yylval.value.s, (char*)value+1, (n-2));
	yylval.value.s[n-2] = '\0';
}

/* To remove the dependency on yacc library */

void yyerror(char *s1)
{
	char s[2048];
	char s2[2048];
	int i;
	extern char *lex_input, *lex_input_buffer;
	extern char yytext[];
	void reset_yy_lex(void);

	for (i = 0; i < 2048; i++)
	{
		s[i] = (iscntrl(lex_input_buffer[i])?lex_input_buffer[i]:' ');
		if (s[i] == '\0')
		break;
	}
  
	strcpy(s + (lex_input - lex_input_buffer), "^");
/*	strcat(s, s1); */
	strcat(s, " at ");
	strcat(s, yytext);
	strcpy(s2, s1);
	shErrStackClear();
/*
	fprintf(stderr, "%s\n%s\n", lex_input_buffer, s);
*/
	shErrStackPush(s);
	shErrStackPush(lex_input_buffer);
	shErrStackPush(s2);
/*	shErrStackPush("%s\n%s", lex_input_buffer, s); */

	/* reset yylex() */

	reset_yy_lex();
}

/* This is another stupid function */

static int get_dims(char *nelem)
{
	int i;

	if (nelem == NULL)	return(0);
	i = 0;

	while (*nelem != '\0')
	{
		while ((*nelem != '\0') && isdigit(*nelem)) nelem++;
		while ((*nelem != '\0') && !isdigit(*nelem)) nelem++;
		i++;
	}

	return(i);
}

/* dervish_type_sizeof(TYPE) -- return the size of the type */

int dervish_type_sizeof(TYPE type)
{
	const	SCHEMA *se;

	se = shSchemaGet(shNameGetFromType(type));

	/* prevent ill-fated pointers */

	if (se == NULL)
	{
		return(0);
	}
	else
	{
		return(se->size);
	}
}

/* arg_add(t) -- add one more argument type */

static int arg_add(DATA_ELEMENT d)
{
	if (arg_idx >= MAX_DIM)
	{
		puts("too many arguments");
		return(1);
	}

	arg_stack[arg_idx++] = d;

	return(0);
}

/* show_constructor_finding(name, argv, argc) -- print the contents */

static void show_constructor_finding(char *name, DATA_ELEMENT *argv, int argc)
{
	int i;
	char s[1024];

	printf("Fail to find this constructor:\n%s(", name);

	for (i = 0; i < argc; i++)
	{
		show_type(s, argv[i].type);
		printf("%s", s);
		if (i < argc - 1) printf(",");
		else printf(")\n");
	}
}

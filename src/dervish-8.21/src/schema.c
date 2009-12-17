/*
 * this file manipulates schema definitions
 *
 * Robert Lupton (rhl@astro.princeton.edu)
 *
 * Wei Peng 04/10/94 added routines to deal with enum schemas 
 * Don Holmgren  1/17/96 added shSchemaNew to allow schema creation on the fly
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <float.h>
#include "shCAssert.h"
#include "shCUtils.h"
#include "shCSchema.h"
#include "shCErrStack.h"
#include "shCHash.h"
#include "shCGarbage.h"
#include "shC.h"

/*
 * Schema Data
 */
extern SCHEMA **p_shSchema;		/* list of all known schema */
extern int p_shNSchema;			/* number of known schema */

SHHASHTAB *schemaTable;

/* Chris S.  Use FLT_DIG and DBL_DIG to define the number of digits.
   Previously, exprGet_precsion was set to 6, which is FLT_DIG,
   but exprGet_Dprecision was 12, while DBL_DIG is 15. */

static int exprGet_precision = FLT_DIG;	  /* float precision for exprGet */
static int exprGet_Dprecision = DBL_DIG;  /* double precision for exprGet */

/*****************************************************************************/
/*
 * Set exprGet_D?precision, returning old value
 */
int
shExprGetSetPrecision(int precision,	/* desired precision */
		      int set_float)	/* set float (not double) precision */
{
   int old;

   if(set_float) {
      old = exprGet_precision;
      exprGet_precision = precision;
   } else {
      old = exprGet_Dprecision;
      exprGet_Dprecision = precision;
   }

   return(old);
}

/*****************************************************************************/
/*
 * Allocate and return a SCHEMA of size nelems.  Returned structure is
 * stuffed with zeros, except for SCHEMATYPE of UNKNOWN.
 * If we can't allocate enough memory, shFatal is called.
 * If nelems is negative, shFatal is called.
 *
 * Note: eventually the user will use p_shSchemaLoad to put his new
 * schema into the system tables.  p_shSchemaLoad assumes an array of
 * SCHEMA's is input with the last entry having a null name (as generated
 * by diskio).  Therefore we allocate an extra byte at the end of our SCHEMA,
 * and fill it with a zero.  The allocated structure, after filling by the
 * user, can successfully be added to the system tables with p_shSchemaLoad.
 *
 */
SCHEMA *
shSchemaNew(int nelems)
{
  SCHEMA *schema;
  int i;

  if (nelems < 0) {
    shFatal("shSchemaNew: Negative nelem's specified");
  }

  /* Malloc storage - 1 SCHEMA plus 1 byte; nelem SCHEMA_ELEM's */
  schema = (SCHEMA *)malloc(sizeof(SCHEMA)+1);
  if (schema == NULL) {
    shFatal("shSchemaNew: Couldn't allocate memory for SCHEMA");
  }
  if (nelems > 0) {
    schema->elems = (SCHEMA_ELEM *)malloc(nelems*sizeof(SCHEMA_ELEM));
    if (schema->elems == NULL) {
      shFatal("shSchemaNew: Couldn't allocate memory for SCHEMA_ELEM's");
    }
  }
  else {
    schema->elems = (SCHEMA_ELEM *)NULL;
  }

/* Stuff SCHEMA - mark trailing byte as a zero for p_shSchemaLoad */
  schema[1].name[0] = (char)0;
  schema->name[0] = (char)0;
  schema->type = UNKNOWN;
  schema->size = 0;
  schema->nelem = nelems;
  schema->construct = (void *)NULL;
  schema->destruct = (void *)NULL;
  schema->read = (void  *)NULL;
  schema->write = (void *)NULL;
  schema->lock = SCHEMA_NOLOCK;
  schema->id = UNKNOWN_SCHEMA;

/* Stuff SCHEMA_ELEMS */
  if (nelems > 0) {
    for (i=0; i<nelems; i++) {
      (schema->elems[i]).name[0] = (char)0;
      (schema->elems[i]).type[0] = (char)0;
      (schema->elems[i]).value = 0;
      (schema->elems[i]).nstar = 0;
      (schema->elems[i]).size = 0;
      (schema->elems[i]).nelem = (char *)NULL;
      (schema->elems[i]).i_nelem = 0;
      (schema->elems[i]).offset = 0;
#if SCHEMA_ELEMS_HAS_TTYPE
      (schema->elems[i]).ttype = UNKNOWN_SCHEMA;
#endif
      
    }
  }

  return(schema);
}

/*
 * Do NOT call this function if you've called p_shSchemaLoad! 
 */
void
shSchemaDel(SCHEMA *schema)
{
   if(schema == NULL) return;

   free(schema->elems);
   free(schema);
}

/*****************************************************************************/
/*
 * return a schema definition, given a type name
 * Deal with all unsigned and const type conversions here.
 */
const SCHEMA *
shSchemaGet(char *name)
{
   int i;
   int is_struct;			/* name is of form "struct foo" */
   char uname[100];			/* uppercase version of name */
   char *ptr;
   char *uptr;
   int ustart;
   SCHEMA *schemaPtr=NULL;

/* const, signed and unsigned types are treated here.  There are too many
 * gotcha's otherwise.  All name-based schema fetching comes through here
 */
   if(strncmp(name,"const ",6) == 0) {
      name += 6;
   }
/*
 * Deal with struct tags
 */
   is_struct = 0;
   if(strncmp(name,"struct ",7) == 0) {
      name += 7;
      is_struct = 1;
   }
/*
 * now deal with qualifiers that have meaning to us
 */
   ptr = name;

   if(strncmp(ptr,"unsigned ",9) == 0) {
      uptr = ptr + 9;
      ustart = 1;
      uname[0] = 'U';
   } else if (strncmp(ptr, "signed ", 7) == 0)  { 
      uptr = ptr + 7;
      ustart = 1;
      uname[0] = 'S';
   } else {
      uptr = ptr;
      ustart=0;
   }

   for(i = 0; uptr[i] != '\0'; i++) {    /* set uname from name */
      uname[i+ustart] = toupper(uptr[i]);
   }
   uname[i+ustart] = '\0';

   /* Well we have to do a little playing around here.  For schema that are not
      of type PRIM, only the name that is exactly equivalent to the typedef
      (usually all uppercase) is legal, after removing const qualifiers.
      For PRIM types, the names may be of mixed case.
      For names of type "struct ident", we allow either ident or IDENT.
      The hash table only contains the all uppercase versions for
      both PRIM and other types.  Therefore the following must be done:
	 o get the schema using uname (uppercase version of schema name)
	 o if type is PRIM, we are done return the schemaPtr
	 o if we have an explicit "struct ", allow either case
	 o if type is not PRIM, make sure that the name that was entered was
	   originally all uppercase.  If not, it is an error.
      
      These rules have been lifted! Mixed case are allower everywhere 
      (especially for c++ objects !!).  However the comparaison is 
      case INSENSITIVE!

   */
   if (shHashGet(schemaTable, uname, (void **)&schemaPtr)
						  == SH_HASH_ENTRY_NOT_FOUND) {
      return(NULL);
   } else {
     return schemaPtr;
     /*
       if(schemaPtr->type == PRIM) {
       return(schemaPtr);
       } else if(is_struct) {
       return(schemaPtr);
       } else if(strcmp(uname, name) != 0) {
       return(NULL);
       } else {
       return(schemaPtr);
       }
       */
   }
}

/* 
------------------------------------------------------------------------
 *
 * return a schema definition, given a type Id (numeric TYPE).
 * Deal with potential unknown types here.
 */
const SCHEMA *
shSchemaGetFromType(TYPE type)
{
  if( (type >= 0) && (type <= p_shNSchema) ) {
    return(p_shSchema[(int)type]);
  } else {
    return NULL;
  }
}

/*****************************************************************************/
/*
 * get the description of an member ELEM of the schema
 *
 * Return values:
 *	The desired SCHEMA_ELEM if possible, else NULL
 */
const SCHEMA_ELEM *
shSchemaElemGetFromSchema(const SCHEMA* schema, char *elem)
{
  int i;
  if( schema == NULL) {
    return(NULL);
  }
  for(i = 0;i < schema->nelem;i++) {
    if(strcmp(elem,schema->elems[i].name) == 0) {
      return(&schema->elems[i]);
    }
  }
  return NULL;
}

/*****************************************************************************/
/*
 * get the description of an member ELEM of type id TYPE
 *
 * Return values:
 *	The desired SCHEMA_ELEM if possible, else NULL
 */
const SCHEMA_ELEM *
shSchemaElemGetFromType(TYPE type, char *elem)
{
  const SCHEMA* schema;
  if((schema = shSchemaGetFromType(type)) == NULL) {
    shErrStackPush("shSchemaElemGetFromType: Can't find schema for %d\n",type);
    return(NULL);
  } else {
    return (shSchemaElemGetFromSchema( schema, elem ));
  }
}

/*****************************************************************************/
/*
 * get the description of an member ELEM of type TYPE
 *
 * Return values:
 *	The desired SCHEMA_ELEM if possible, else NULL
 */
const SCHEMA_ELEM *
shSchemaElemGet(char *type, char *elem)
{
  const SCHEMA* schema;
  if ((schema = shSchemaGet(type)) == NULL) {
    shErrStackPush("shSchemaElemGetFromType: Can't find schema for %s\n",type);
    return(NULL);
  } else {
    return (shSchemaElemGetFromSchema( schema, elem ));
  }
}

/*****************************************************************************/
/*
 * given a SCHEMA_ELEM, return a TYPE
 */
TYPE
shTypeGetFromSchema(SCHEMA_ELEM *sch_el)
{
   char *ptr;
   TYPE type;

#if SCHEMA_ELEMS_HAS_TTYPE
   if(sch_el->ttype != UNKNOWN_SCHEMA) {
      return(sch_el->ttype);
   }
#endif

   ptr = sch_el->type;

/* Note that unsigned char is not printed as a character string - is this
** the desired behavior?
** PCG: Yes it is.
*/
   if(shStrcmp(ptr,"char",SH_CASE_INSENSITIVE) == 0 && sch_el->nstar == 1) {
      type = shTypeGetFromName("STR");
   } else if(sch_el->nstar > 0) {
      type = shTypeGetFromName("PTR");
   } else {
      type = shTypeGetFromName(ptr);
   }

#if SCHEMA_ELEMS_HAS_TTYPE
   sch_el->ttype = type;
#endif
   
   return(type);
}

/*****************************************************************************/
/*
 * return the total number of elements <n> in a SCHEMA_ELEM.nelem string
 */
RET_CODE
p_shSchemaNelemGet(const SCHEMA_ELEM *sch_el,int *n)
{
#define BUFFSIZE 100
   char buff[BUFFSIZE];			/* temp space for new nelem string */
   char *bptr = buff;			/* pointer to buff */
   char *ptr;				/* pointer to nelem string */
   int m;

   *n = 1;
   if(sch_el->nelem != NULL) {
      ptr = sch_el->nelem;

      for(;;) {
	 while(isspace(*ptr)) ptr++;
	 if(*ptr == '\0') {
	    break;
	 }
/*
 * parse a string containing no white space of the form
 *	123+12+...
 * No other operators, including parens, are allowed
 */
	 m = 0;
	 for(;;) {
	    int val;
	    if(!isdigit(*ptr)) {
	       shError("Expected a digit in dimension of %s\n",sch_el->name);
	       *n = 0;
	       return(SH_GENERIC_ERROR);
	    }
	    val = atoi(ptr);
	    while(isdigit(*ptr)) ptr++;
	    
	    m += val;
	    if(*ptr == '\0' || isspace(*ptr) ) {
	       break;
	    } else if(*ptr == '+') {		/* OK, #+# */
	       ptr++;
	    } else {
	       shError("Expected a + in dimension of %s\n",sch_el->name);
	       *n = 0;
	       return(SH_GENERIC_ERROR);
	    }
	 }
/*
 * accumulate a new nelem string in buff
 */
	 if(bptr != buff) {
	    *bptr++ = ' ';
	 }
	 sprintf(bptr,"%d",m);
	 while(isdigit(*bptr)) bptr++;

	 *n *= m;
	 if ( (bptr-buff) > BUFFSIZE ) {
	   shError("p_shSchemaNelemGet has too short of a string buffer");
	 }
      }
/*
 * save new nelem if it is different; there's a slight memory leak here as
 * the old value was a static string
 * we use malloc here because this is the case for all schema stuff (and
 * we don't want this to show up as a memory leak since this is NOT freeable
 * at the tcl level.
 */
      if(strcmp(buff,sch_el->nelem) != 0) {
	 ((SCHEMA_ELEM *)sch_el)->nelem = (char *)malloc(strlen(buff) + 1);
	 strcpy(sch_el->nelem,buff);
      }
   }
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * return a pointer to an element in a structure thing, described by
 * the SCHEMA_ELEM sch_el; the type is returned in *type, if type isn't NULL
 *
 * If the type is actually an array type (sch_el->nelem != NULL) the type
 * returned is the _base_ type --- this is arguably wrong but is forced
 * upon us by the impoverishment on the TYPE system. 
 *
 * You will still have to cast the returned pointer as needed, probably
 * based on the returned type
 *
 * Return values:
 *	The desired pointer if possible, else NULL
 */
void *
shElemGet(void *thing, SCHEMA_ELEM *sch_el, TYPE *type)
{
   if(type != NULL) {
      *type = shTypeGetFromSchema(sch_el);
   }
   
/* Negative offsets were treated as errors.
 * However, Versant vstrs have negative offsets for their counts, it is
 * desirable to be able to access their fields in the normal fashion
*/

/*   if(thing == NULL || sch_el->offset < 0) { */
   if(thing == NULL) {
      return(NULL);
   }
   return((char *)thing + sch_el->offset);
}

/*****************************************************************************/
/*
 * Set the value of an element in a structure thing, described by
 * the SCHEMA_ELEM sch_el. The value is passed as a string
 *
 * Return values:
 *	SH_GENERIC_ERROR	Can't convert a value
 *	SH_SUCCESS		Otherwise
 */
#include <math.h>
#include <float.h>

RET_CODE
shElemSet(void *thing, SCHEMA_ELEM *sch_el, char *value)
{
   int ival,len;
   double dval;
   long lval;
   unsigned long uval;
   LOGICAL bval;
   char *ptr;
   void *elem = (char *)thing + sch_el->offset; /* the desired element */

   TYPE type = shTypeGetFromSchema(sch_el);
   char *type_name = shNameGetFromType(type);

   /* Since the TYPE and type_name are Now strictly equivalent (08/96)
      we can cache these types because we know that they are necessary 
      defined (use of make_io) */

   static TYPE LOGICAL_ID, CHAR, UCHAR, SCHAR;
   static TYPE SHORT, USHORT, INT, UINT;
   static TYPE LONG, ULONG, FLOAT, DOUBLE;
   static TYPE STR, PTR;
   static TYPE TYPE_ID;
   static int init = 0;
   
   if (!init) {
     LOGICAL_ID = shTypeGetFromName("LOGICAL");
     CHAR = shTypeGetFromName("CHAR");
     UCHAR = shTypeGetFromName("UCHAR");
     SCHAR = shTypeGetFromName("SCHAR");
     SHORT = shTypeGetFromName("SHORT");
     USHORT = shTypeGetFromName("USHORT");
     INT = shTypeGetFromName("INT");
     UINT = shTypeGetFromName("UINT");
     LONG = shTypeGetFromName("LONG");
     ULONG = shTypeGetFromName("ULONG");
     FLOAT = shTypeGetFromName("FLOAT");
     DOUBLE = shTypeGetFromName("DOUBLE");
     STR = shTypeGetFromName("STR");
     PTR = shTypeGetFromName("PTR");
     TYPE_ID = shTypeGetFromName("TYPE");
     init = 1;
   }

   shDebug(10,"shElemSet: schema %s (%s) at "PTR_FMT" with %s",
	   sch_el->type,type_name,elem,value);

   if( type == TYPE_ID ) {
      ival = shTypeGetFromName(value);
      /*      if(strcmp(shNameGetFromType(ival), value) != 0) { */
      if ( ival == UNKNOWN_SCHEMA ) {
         shErrStackPush("Unknown type %s\n",value);
         return(SH_GENERIC_ERROR);
      }
      *(int *)elem = (int)ival;
   }
   else if(p_shTypeIsEnum(type)) 
   {
     /* Wei Peng 04/14/94 added enum capabilities */

      ptr=value;
      len = strlen(value);
      while(isdigit(*ptr)) {ptr++; if(ptr >= value+len) break;}
      if(ptr >= value+len) 
      {
         /* OK, passed is a value, do the old stuff */

         ival = (int)strtol(value,&ptr,0);
         if(ptr == value) {
	    shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	   return(SH_GENERIC_ERROR);
         }
         ptr = shEnumNameGetFromValue(type_name, (int) ival);
         if(ptr==NULL)
         {
            shErrStackPush("%d is not a supported value of %s", *value, type_name);
            return SH_GENERIC_ERROR;
          }
         *(int *)elem = (int)ival;
      }
      else
      {     /* passed is a string that might be a member in enum */
            /* get values associated with the string */

         ptr=shValueGetFromEnumName(type_name, value, (int*) &ival);
         if(ptr==NULL)
         {
            shErrStackPush("%s is not a legal %s value", value, type_name);
            return SH_GENERIC_ERROR;
         }
         *(int *)elem = (int)ival;
      }
   }

   else if( type == SHORT ) {
       ival = (int)strtol(value,&ptr,0);
       if(ptr == value) {
	    shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	   return(SH_GENERIC_ERROR);
        }
        *(short *)elem = (short)ival;
   }
 
   else if( type == USHORT ) {
       uval = strtoul(value,&ptr,0);
       if(ptr == value) {
	    shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	   return(SH_GENERIC_ERROR);
        }
        *(unsigned short *)elem = (unsigned short)uval;
   }
 
   else if( type == INT ) {
       ival = (int)strtol(value,&ptr,0);
       if(ptr == value) {
	    shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	   return(SH_GENERIC_ERROR);
        }
        *(int *)elem = (int)ival;
   }
 
   else if( type == UINT ) {
       uval = strtoul(value,&ptr,0);
       if(ptr == value) {
	    shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	   return(SH_GENERIC_ERROR);
        }
        *(unsigned int *)elem = (unsigned int)uval;
   } 

   else if( type == LONG ) {
      lval = strtol(value,&ptr,0);
      if(ptr == value) {
	 shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	 return(SH_GENERIC_ERROR);
      }
      *(long *)elem = lval;
   } 

   else if( type == ULONG ) {
      uval = strtoul(value,&ptr,0);
      if(ptr == value) {
	 shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	 return(SH_GENERIC_ERROR);
      }
      *(unsigned long *)elem = uval;
   }

   else if( type == FLOAT ) {
      dval = strtod(value,&ptr);
      if(ptr == value) {
	 shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	 return(SH_GENERIC_ERROR);
      }
      /* We need to be sure that the float is in the correct range
	 to avoid any core dumping! */
      /* We may want to set some error number ... next time */
      if ( fabs(dval) <= FLT_MAX && fabs(dval) >= FLT_MIN ) {
	*(float *)elem = dval;
      } else {
	if ( fabs(dval) >= FLT_MIN ) {
	  *(float *)elem = (dval/fabs(dval))*FLT_MAX;
	} else {
	  *(float *)elem = 0;
	};
      }
   }

   else if( type == DOUBLE ) {
      dval = strtod(value,&ptr);
      if(ptr == value) {
	 shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	 return(SH_GENERIC_ERROR);
      }
      *(double *)elem = dval;
   }

   else if( type == LOGICAL_ID ) {
      if (shBooleanGet (value, &bval) != SH_SUCCESS) {
         shErrStackPush("Can't convert %s to type %s\n",value,type_name);
         return(SH_GENERIC_ERROR);
      }
      *(LOGICAL *)elem = bval;

   } else if( type == UCHAR ) {
       uval = strtoul(value,&ptr,0);
       if(ptr == value) {
	    shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	   return(SH_GENERIC_ERROR);
        }
      *(unsigned char *)elem = (unsigned char)uval;

   } else if( type == SCHAR ) {
       ival = (int)strtol(value,&ptr,0);
       if(ptr == value) {
	    shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	   return(SH_GENERIC_ERROR);
        }
      *(signed char *)elem = (signed char)ival;

   } else if( type == CHAR ) {
      *(char *)elem = *value;

   } else if( type == STR ) {
      if ( (*(void**)elem!=NULL) && !shIsShMallocPtr(*(void **)elem)) {
	 shError("shElemSet: I cannot set a non-shMalloced string\n");
	 return(SH_GENERIC_ERROR);
      }

      if (*(void**)elem!=NULL) shFree(*(void **)elem);
      *(void **)elem = shMalloc(strlen(value) + 1);
      strcpy(*(char **)elem,value);
   } else {
      lval = strtol(value,&ptr,0);
      if(ptr == value) {
	 shErrStackPush("Can't convert %s to type %s\n",value,type_name);
	 return(SH_GENERIC_ERROR);
      }
      *(long *)elem = lval;
   }

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Convert types (now defined as indexes into schema lists) to strings
 */
char *
shNameGetFromType(TYPE type)
{
   shAssert(p_shSchema != NULL);	/* i.e. they called shSchemaInit() */
   
   if((int)type < 0 || (int)type >= p_shNSchema) {
      return("UNKNOWN");
   } else {
      return(p_shSchema[(int)type]->name);
   }
}

/*****************************************************************************/
/*
 * Convert character string type to numerical value.
 * Special cases:
 *   UNKNOWN:   p_shNSchema
 *   PTR:       p_shNSchema+1
 *   STR:       p_shNSchema+2
 *
 * Users are allowed to save types and pass them to functions that expect
 * names of type, e.g.
 *   TYPE r = shTypeGetFromName("REGION");
 *   ...
 *   shChainElementAddByPos(...,(char *)r,...);
 * instead of
 *   shChainElementAddByPos(...,"REGION",...);
 * This works by treating all string addresses <= p_shNSchema as TYPEs
 *    
 */

TYPE
shTypeGetFromName(char *name)
{
   const SCHEMA *schema;

   if (p_shSchema == NULL) return -1;

   /* Porting warning: The following test is potentially dangerous
      if a char* append to be different from a long you could get too many
      or too few bytes to do an accurate comparaison.  We also rely on the
      fact that in Unix the data 'segment' usually has addresses very much
      superior to 0 */
   if((unsigned long int)name <= p_shNSchema
      ||(signed long int)name == UNKNOWN_SCHEMA) {	/* it's a TYPE, return it */
     return(*(TYPE *)&name);
   }

/* Translate the name using shSchemaGet to deal with upper-casing woes */

   schema = shSchemaGet(name);

   if (schema != NULL) {
/*printf("%s %s : ",name,schema->name);*/
     name = (char *)schema->name;
     return schema->id;
/*printf("%d, ",schema->id);*/
   }

   /* schema not found ! */
   return  UNKNOWN_SCHEMA;
}

/*****************************************************************************/
/*
 * Is this type an enum?
 */
int
p_shTypeIsEnum(TYPE type)
{
   if((int)type < 0 || (int)type >= p_shNSchema) {
      return(0);
   } else {
      return(p_shSchema[(int)type]->type == ENUM ? 1 : 0);
   }
}

/*****************************************************************************/
/*
 * Return a printed representation of a value given by a pointer and a type
 */
char *
shPtrSprint(void *ptr, TYPE type)
{
   static char buff[1000];
   static char buff2[2000];
   const SCHEMA *sch;
   /*   char *type_name = shNameGetFromType(type);  */
   char* enum_name=NULL;                /* Wei Peng, 04/14/94 */
   int   i, j, bufflen;

   /* Since the TYPE and type_name are Now strictly equivalent (08/96)
      we can cache these types because we know that they are necessary 
      defined (use of make_io) */

   static TYPE LOGICAL_ID, CHAR, UCHAR, SCHAR;
   static TYPE SHORT, USHORT, INT, UINT;
   static TYPE LONG, ULONG, FLOAT, DOUBLE;
   static TYPE STR, PTR;
   static TYPE TYPE_ID, FUNCTION_ID;
   static int init = 0;
   
   if (!init) {
     LOGICAL_ID = shTypeGetFromName("LOGICAL");
     CHAR = shTypeGetFromName("CHAR");
     UCHAR = shTypeGetFromName("UCHAR");
     SCHAR = shTypeGetFromName("SCHAR");
     SHORT = shTypeGetFromName("SHORT");
     USHORT = shTypeGetFromName("USHORT");
     INT = shTypeGetFromName("INT");
     UINT = shTypeGetFromName("UINT");
     LONG = shTypeGetFromName("LONG");
     ULONG = shTypeGetFromName("ULONG");
     FLOAT = shTypeGetFromName("FLOAT");
     DOUBLE = shTypeGetFromName("DOUBLE");
     STR = shTypeGetFromName("STR");
     PTR = shTypeGetFromName("PTR");
     TYPE_ID = shTypeGetFromName("TYPE");
     FUNCTION_ID = shTypeGetFromName("FUNCTION");
     init = 1;
   }

   if(p_shTypeIsEnum(type)) {
      if((sch = (SCHEMA *)shSchemaGetFromType(type)) == NULL) {
	 sprintf(buff,"(unknown enum)%d",*(int *)ptr);
      } else {
            enum_name=shEnumNameGetFromValueFromType(type, *(int*) ptr); /* WP */
            if(enum_name != NULL) sprintf(buff, "(enum) %s", enum_name);
	    else sprintf(buff,"(enum) %d (non-supported value)",*(int *)ptr);
      }
      
   }  

   /* Wei Peng 05/06/94 moved the lines related to TYPE to here */
   /* Got rid of leading / for octal representation */

   else if( type == TYPE_ID ) { /* a very special case */
	    sprintf(buff,"%s",shNameGetFromType(*(int *)ptr));
   }
   else if( type == CHAR ) {
      char c = *(char *)ptr;
      if(isprint(c)) {
	 sprintf(buff,"%c",c);
      } else {
	 sprintf(buff,"%03o",c);
      }

     /* Wei Peng 05/05/94 added lines for all the unsigned types */

   } else if( type == UCHAR ){  
      char c = *(unsigned char*) ptr;
      if(isprint(c)) {
         sprintf(buff, "%c", c);
      } else {
         sprintf(buff, "%03o",c);
      }
   } else if( type == SCHAR ){  /* Signed char is by 
						    'dervish standard' a number! */
      char c = *(signed char*) ptr;
      sprintf(buff,"%d",c);

/* Steve Kent - do a proper job on unsigned integer quantities.  Nobody seems
 * to ever get this right */
   } else if( type == UINT ) { 
      sprintf(buff,"%u",*(unsigned int *)ptr);
   } else if( type == ULONG ) {
      sprintf(buff,"%lu",*(unsigned long *)ptr);
   } else if( type == USHORT ) {
      sprintf(buff,"%u",*(unsigned short *)ptr);
   } else if( type == SHORT ) {
      sprintf(buff,"%d",*(short *)ptr);
   } else if( type == INT ) {
      sprintf(buff,"%d",*(int *)ptr);
   } else if( type == LONG ) {
      sprintf(buff,"%ld",*(long *)ptr);
   } else if( type == FLOAT ) {
      sprintf(buff,"%.*g", exprGet_precision, *(float *)ptr);
   } else if( type == DOUBLE ) {
      sprintf(buff,"%.*g", exprGet_Dprecision, *(double *)ptr);
   } else if( type == FUNCTION_ID ) {
      sprintf(buff,PTR_FMT,ptr);
   } else if( type == STR ) {
      sprintf(buff,"%s",*(char **)ptr); 

      /* Check if there's any double-quotes (") in the string */
      if (strchr(buff, '"') != NULL) {
        /* This may not be the most efficient way of doing this but we
           don't anticipate following this path very often */
        bufflen = strlen(buff);
        j = 0;
        for (i=0; i < bufflen; i++) {
          if (buff[i] == '"') {
            buff2[j++] = '\\';
            buff2[j++] = '"';
          } else {
            buff2[j++] = buff[i];
          }
        } 
        buff2[j] = (char) 0;
        strncpy(buff, buff2, sizeof(buff));
      }
      bufflen = sizeof(buff) - 1;
      buff[bufflen] = (char) 0;		/* Just in case */

   } else {
      sprintf(buff,PTR_FMT,*(void **)ptr);
   }
   return(buff);
}



/*************************************************************************
 *  Routines to convert enum value to an enum string (vice versa).
 *  An enum name means the member name that has an integer value.
 *  An enum type is the actualy type defined by typedef enum.
 *
 *  Wei Peng, 04/10/94
 *
 *************************************************************************/

/************************************************************************
 given value, return an enum name 
 */


char* shEnumNameGetFromValueFromSchema(const SCHEMA* schema, int value)
{
    int i=0;

    if(schema==NULL || schema->type!=ENUM) return NULL;
    for(i=0; i < schema->nelem; i++)
    {
       if(schema->elems[i].value == value) return schema->elems[i].name;
    }
    return NULL;
}

char* shEnumNameGetFromValue(char* type, int value)
{
    const SCHEMA* schema;

    if(type==NULL) return NULL;
    schema=shSchemaGet(type);
    return ( shEnumNameGetFromValueFromSchema( schema, value) );
}

char* shEnumNameGetFromValueFromType(TYPE type, int value)
{
  return (shEnumNameGetFromValueFromSchema( shSchemaGetFromType(type), value) );
}

/************************************************************************
 * given a string, find its value. Return name if successful, 
 * NULL if no match is found.
 */

char* shValueGetFromEnumNameFromSchema(const SCHEMA* schema, char* name, int* value)
{
    int i=0;

    if(value==NULL || name==NULL ) return NULL;

    if(schema==NULL || schema->type!=ENUM) return NULL;
    for(i=0; i < schema->nelem; i++)
    {
       if(strcmp(schema->elems[i].name, name)!=0) continue;
       *value = schema->elems[i].value;
       return schema->elems[i].name;
    }
    return NULL;
}

char* shValueGetFromEnumName(char* type, char* name, int* value)
{
    const SCHEMA* schema;

    if( type==NULL ) return NULL;
    schema=shSchemaGet(type);
    return (shValueGetFromEnumNameFromSchema( schema, name, value));
}

char* shValueGetFromEnumNameFromType(TYPE type, char* name, int* value)
{
    return (shValueGetFromEnumNameFromSchema( shSchemaGetFromType(type), name, value));
}









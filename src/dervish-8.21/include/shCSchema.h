#if !defined(SHCSCHEMA_H)
#define SHCSCHEMA_H
/*
 * structures and prototypes associated with the schema of datastructures
 */

#include <limits.h>			/* For UCHAR_MAX ... */

#include <stddef.h>
#include <stdio.h>
#include "dervish_msg_c.h"

/*
 * Primitive data types.
 */

typedef	unsigned char	LOGICAL;	/* Boolean value.  This   M U S T  be */
					/* 1 byte in size in order for TBLCOL */
					/* stuff to work properly.            */

#if (UCHAR_MAX != 255U)
#error LOGICAL type   M U S T   be 1 byte in size
#endif

/*
 * If SCHEMA_ELEMS_HAS_TTYPE is true, an element is added to SCHEMA_ELEM
 * which is used to cache TYPE information; this is a significant time
 * sink in dervish-based code. The code works, but requires that all products
 * that use make_io be rebuilt. When it is deemed that the time has come to
 * do this, remove the SCHEMA_ELEMS_HAS_TTYPE tests everywhere.
 */
#define SCHEMA_ELEMS_HAS_TTYPE 0

#define SIZE 500			/* generic size of things */

/* 
 * These defined are used by SCHEMA.lock 
 * These are bit values.
 */ 
#define SCHEMA_NOLOCK      0
#define SCHEMA_LOCK        1
#define SCHEMA_LOCK_NOWARN 2
#define SCHEMA_LOCKED_OUT  4
#define SCHEMA_REPLACE     8

typedef int TYPE;			/* describe an object's type */

/* Define the unknown schema ID */
/* It has to be noted that to actually get an empty schema
   you can find one at the highest existing ID.  Id based
   function to access schema are taking this into account.
   */
#define UNKNOWN_SCHEMA (TYPE)(-1)

typedef struct {
   char name[SIZE];			/* name of argument */
   char type[SIZE];			/* type of argument */
   int value;                           /* value of the elem if an enum */
                                        /* added by Wei Peng, 04/10/94 */
   int nstar;				/* no. of indirections */
   int size;				/* sizeof this type */
   char *nelem;				/* dimension[s] if an array */
   int i_nelem;				/* total no. of elements */
   int offset;				/* offset of data in struct */
#if SCHEMA_ELEMS_HAS_TTYPE
   TYPE ttype;				/* ==  shTypeGetFromSchema(this) */
#endif
} SCHEMA_ELEM;

typedef enum {
   UNKNOWN,
   PRIM,
   ENUM,
   STRUCT
} SCHEMATYPE; 				/* what sort of thing? */

typedef struct {
   char name[SIZE];			/* name of type */
   SCHEMATYPE type; 			/* what sort of thing? */
   int size;				/* sizeof this type */
   int nelem;				/* no. of elements */
   SCHEMA_ELEM *elems;			/* list of elements */
   
   void *(*construct)(void);		/* constructor for this type */
   void *(*destruct)(void *ptr);	/* destructor for this type */
   RET_CODE (*read)(FILE *fil, void *obj); /* read one of these from a dump */
   RET_CODE (*write)(FILE *fil, void *obj); /* write one of these to a dump */
   char lock;
   int id;
} SCHEMA;

/*
 * Schema Data
 */
/*extern SCHEMA **p_shSchema;*/		/* list of all known schema */
/*extern int p_shNSchema;*/			/* number of known schema */

/*****************************************************************************/
/*
 * now for prototypes
 */
#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */
   
RET_CODE shDumpSchemaElemRead(FILE *fil,char *parent_type, 
			      void *thing,SCHEMA_ELEM *sch_el);
RET_CODE shDumpSchemaElemWrite(FILE *fil,char *parent_type, 
			      void *thing,SCHEMA_ELEM *sch_el);
SCHEMA *shSchemaNew(int nelems);
void shSchemaDel(SCHEMA *schema);

void *shElemGet(void *thing, SCHEMA_ELEM *sch_el, TYPE *type);
RET_CODE shElemSet(void *thing, SCHEMA_ELEM *sch_el, char *value);

const SCHEMA *shSchemaGet(char *type);
const SCHEMA *shSchemaGetFromType(TYPE type);

const SCHEMA_ELEM *shSchemaElemGet(char *type, char *elem);
const SCHEMA_ELEM *shSchemaElemGetFromSchema(const SCHEMA* schema, char *elem);
const SCHEMA_ELEM *shSchemaElemGetFromType(TYPE type, char *elem);

RET_CODE p_shSchemaNelemGet(const SCHEMA_ELEM *sch_el,int *n);
TYPE shTypeGetFromSchema(SCHEMA_ELEM *sch_el);
char *shPtrSprint(void *ptr, TYPE type);
RET_CODE p_shSchemaLoad(SCHEMA *schema_in);
RET_CODE p_shDumpSchemaWrite(FILE *fil, SCHEMA *schema);
RET_CODE p_shDumpSchemaRead(FILE *fil, SCHEMA **schema);
RET_CODE p_shDumpSchemaRestore(FILE *outfil, SCHEMA ***sch, int *nschema);
RET_CODE p_shDumpSchemaSave(FILE *outfil, SCHEMA **sch, int nschema);

char* shEnumNameGetFromValue(char* type, int value);
char* shEnumNameGetFromValueFromSchema(const SCHEMA* schema, int value);
char* shEnumNameGetFromValueFromType(TYPE type, int value);
char* shValueGetFromEnumName(char* type, char* name, int* value);
char* shValueGetFromEnumNameFromSchema(const SCHEMA* schema, char* name, int* value);
char* shValueGetFromEnumNameFromType(TYPE type, char* name, int* value);

int shExprGetSetPrecision(int precision, int set_float);
/*
 * functions to deal with data types
 */
char *shNameGetFromType(TYPE type);
TYPE shTypeGetFromName(char *name);
int p_shTypeIsEnum(TYPE type);

/*
 * function called by shMainTcl_Declare in cmdMain
 */

RET_CODE shSchemaLoadFromDervish(void);   /* load the DERVISH schema definitions */

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif

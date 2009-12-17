/*
 * Dervish utility functions connected with disk I/O
 *
 * External Symbols
 *
 * shDumpOpen		public  Open a dump file
 * shDumpReopen		public  Reopen a dump (don't init data structures)
 * shDumpClose		public	Close a dump
 * shDumpPtrsResolve	public  Resolve pointer ids
 * shDumpDateDel	public	Replace a dump's date string with Xs
 * shDumpDateGet	public	Return a dump's date string
 * shDumpTypeGet	public	Return the TYPE of the next item in a dump
 *		functions to read/write/skip something in a dump
 * shDumpCharRead	public	Read a char
 * shDumpCharWrite	public
 * shDumpUcharRead	public	Read an unsigned char
 * shDumpUcharWrite	public
 * shDumpFloatRead	public	Read a float
 * shDumpFloatWrite	public
 * shDumpDoubleRead	public	Read a double
 * shDumpDoubleWrite	public
 * shDumpIntRead	public	Read an int
 * shDumpIntWrite	public
 * shDumpUintRead	public	Read an unsigned int
 * shDumpUintWrite	public
 * shDumpShortRead	public  Read a short
 * shDumpShortWrite	public
 * shDumpUshortRead	public  Read an unsigned short
 * shDumpUshortWrite	public
 * shDumpLongRead	public	Read a long
 * shDumpLongWrite	public
 * shDumpUlongRead	public	Read an unsigned long
 * shDumpUlongWrite	public
 * shDumpLogicalRead	public	Read a logical value
 * shDumpLogicalWrite	public
 * shDumpMaskRead	public	Read a MASK
 * shDumpMaskWrite	public
 * shDumpPtrRead	public	Read a pointer
 * shDumpPtrWrite	public
 * shDumpHdrRead	public	Read a Header
 * shDumpHdrWrite	public
 * shDumpRegRead	public	Read a REGION
 * shDumpRegWrite	public
 * shDumpStrRead	public	Read a string
 * shDumpStrWrite	public
 * shThingNew		public	make a new THING
 * shThingDel		public	delete a THING
 * shDumpChainWrite     public  Write a CHAIN
 * shDumpChainRead      public  
 * shDumpChain_elemWrite  public   Write a CHAIN_ELEM
 * shDumpChain_elemRead   public   
 * shDumpScharRead      public  Read a signed character
 * shDumpScharWrite     public  Write a signed character
 *
 *		extern functions that are really only for friends
 * p_shDumpStructsReset	private	Reset the structs in a dump file
 * p_shPtrIdSave	private
 *
 *		extern variables
 *
 * p_shDumpReadShallow  private If true, shDump...Read can skip over parts
 *				of things being read
 *
 * ------------------------------------------------------------
 * LISTs/LINKs MODIFICATIONS NOTES:
 *
 * Dumping code uses Lists/Links heavily for internal state keeping. As such,
 * it is probably a good idea not to change the internal use of Links/Lists to
 * Chains due to two good reasons (in order of importance):
 *
 *   a) All the public APIs, except one - shDumpRead(), use lists/links
 *      internally; i.e. they are not visible to the caller of the API. In
 *      that case, why bother changing the internal state of the code at
 *      all. We obviously have to change the shDumpRead() API to return a
 *      CHAIN pointer instead of a LIST pointer. That is easily accomplished
 *      by renaming shDumpRead() and writing a jacket routine around it to
 *      convert the returned LIST to a CHAIN before proceeding further.
 *
 *   b) Dumps are not in widespread use yet. As such, it makes little sense
 *      to spend an inordinate amount of time changing the internals from
 *      lists/links to chains.
 *
 * So, in the end I have decided to put a jacket routine around the API that
 * returns a LIST pointer. The jacket routine will convert the LIST pointer
 * to a CHAIN pointer, which in turn will be returned. 
 *
 * vijay (05/04/95)
 *
 * ------------------------------------------------------------
 * PORT TO OSF1 / Alpha processor MODIFICATIONS NOTES:
 * 
 * OSF1/Alpha platform and IRIX/Mips platform have two major differences:
 *
 *    1) the memory system is little-endian on OSF1 vs big-endian on IRIX.
 *
 *    2) The Alpha processor is a 64 bits processor. In particular long are 
 *       64 bits wide versus only 32 bits in usual SGI.
 *
 * To allow a cross-platform use of the Dump file we store in the file a big-
 * endian version of the memory.  In addition we make the assumption that 
 * the 'int's are 4 bytes wide on any platform (Sky Survey Standard).
 * We dont make any assumption on 'long' size and thus store there size on disk
 * as well.  The behavior of reading a 8 bytes long on 4 bytes long -platform
 * is as of now UNDECIDED!
 *
 * Also most of the integer types used to be written as long, we changed it
 * to int.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "shCAssert.h"
#include "shCUtils.h"
#include "region.h"
#include "shCHdr.h"
#include "shCFitsIo.h"
#include "shCDiskio.h"
#include "shCSchema.h"
#include "shTclTree.h"
#include "shCList.h"
#include "shChain.h"
#include "shCHash.h"
#include "shCGarbage.h"
#include "prvt/region_p.h"
#include "prvt/utils_p.h"
#include "libfits.h"
#include "shC.h"

static LIST *p_shDumpRead(FILE *, int);

int p_shDumpReadShallow = 0;		/* used to enable shallow reads */
/*
 * A file header to protect us against protocol changes in dump files
 */
#define HEADER "Dervish, Dump format 1.0"
static char buf[] = HEADER;

/*
 * static functions in this module
 */
static RET_CODE thingDelKeepPointers(THING *elem);
static TNODE *rebuild(TNODE *root);
static void resolve_ids(TNODE *node);
static int write_scheduled_ptrs(FILE *fil);
static int write_date(FILE *fil);

/*
 * Data
 */
SCHEMA **p_shSchema = NULL;		/* list of all known schema */
int p_shNSchema = 0;			/* number of known schema */
#define ALIASES_INIT_VALUE 0
#define ALIASES_OTHER_VALUE 1
static int aliasesAdded=ALIASES_INIT_VALUE; /* records whether the aliases have
					       been added to the hash table. */

extern SHHASHTAB *schemaTable;

/*
 * addr_root and id_root are used to store addresses by id and vice versa.
 * If an object is in one of these trees it means that we have seen it;
 * addr_root is used while writing, and id_root while reading dumps.
 *
 * if we see an id that we can't resolve (by looking it up in the id-tree),
 * we put it onto unknown_root's tree. The value is the address of the pointer;
 * the pointer is set to the id temporarily, but is replaced (`revolved')
 * when we know the value of the corresponding address (i.e. it's been
 * placed on the id_root tree).
 *
 * anon_list is a list of anonymous structures read from a dump file. Addresses
 * of pointers on the id-tree can point into this list, so it must not be
 * freed while there are unresolved references. Believe me -- I've tried it!
 *
 * ptr_root is used to store structures such as MASKs and REGIONs that have
 * been pointed at but not yet read/written.
 *
 * struct_list is used to store submasks/regions with unresolved pointers
 */
static TNODE *addr_root = NULL;
static TNODE *id_root = NULL;
static LIST *anon_list;			/* list of anonymous objects read */
static TNODE *ptr_root = NULL;
static LIST *struct_list = NULL;
static TNODE *unknown_root = NULL;

/* we verify if the int are the correct length */
#include <limits.h>
/* #include <netinet/in.h> */
#if  ( INT_MAX != 2147483647 ) 
#error INT should be 4 Bytes longs
/*   INT should be defined on 4 bytes.; */
#endif

/*****************************************************************************/
/*
 * Convert longs and image data to or from net byte order.
 *
 * Note that for longs these functions/macros are equivalent to
 * those defined in <netinit/in.h>
 *
 * The 'net byte order' is defined as 'big endian' and is used as a cross
 * platform oder.  So we have actually to convert to and from 'net byte
 * order' only when we are on a 'little endian' platform. 
 *
 * Currently only data type up to 8 bytes long are covered. (To modify that
 * juste change the #define MAX_LENGTH )
 *
 * ROUTINES        FUNCTIONS
 * X sh_htonX (X)     translate a host representation to a 'net byte order'
 * X sh_ntohX (X)     translate a 'net byte order' to the host representation.
 * void sh_hton(*D,S) Actually set the data pointed by D to the 'net byte order'
 * void sh_ntoh(*D,S) Actually set the data pointed by D to the host order
 */

/* define INTEL_BYTEORDER */
#if defined(SDSS_LITTLE_ENDIAN)

#define MAX_LENGTH 8

/* The following union is used to 'shuffle' the data representation
   from little endian to big endian and vice and versa
*/
   
union swab {
  short s;
  int i;
  long l;
  float f;
  double d;
  char c[MAX_LENGTH];
};

/*
 * ROUTINE: Switch (void *s, char l)
 *
 * DESCRIPTION:
 *      Reverse the byte order of the data positioned in the memory 
 *      location pointed by s.  The data must have size l.
 *
*/

static void
Switch ( void * data, char l)
{
   int i;
   char t;
   char *s = (char*) data;

   if (l>MAX_LENGTH) 
     shError("Switch (for Dump files): can't handle data of size greater than %d",MAX_LENGTH);
   for (i=0;i<l/2 ;i++)
     {
       t = s[i]; s[i] = s[l-1-i]; s[l-1-i] = t;
     };
} 

/*
 * ROUTINE: sh_htonX 
 *
 * DESCRIPTION:
 *      Take a value a type X and returns it reversed byte order
 *      counterpart.
 *
*/

static short 
sh_htons(short s)
{
  union swab u;
  u.s = s;
  Switch ( &(u.s), sizeof(s));
  return u.s;
}

static int
sh_htoni(int i) 
{
  union swab u;
  u.i = i;
  Switch ( &(u.i), sizeof(i));
  return u.i;
}

static long
sh_htonl(long l) 
{
  union swab u;
  u.l = l;
  Switch ( &(u.l), sizeof(l));
  return u.l;
}

static float
sh_htonf(float f) 
{
  union swab u;
  u.f = f;
  Switch ( &(u.f), sizeof(f));
  return u.f;
}

static double
sh_htond(double d) 
{
  union swab u;
  u.d = d;
  Switch ( &(u.d), sizeof(d));
  return u.d;
}

#  define sh_ntohs(S) sh_htons(S)
#  define sh_ntohi(I) sh_htoni(I)
#  define sh_ntohl(L) sh_htonl(L)
#  define sh_ntohf(F) sh_htonf(F)
#  define sh_ntohd(D) sh_htond(D)

#  define sh_hton(D,S) Switch(D,S)
#  define sh_ntoh(D,S) sh_hton  (D,S)
#else
#  define sh_hton(D,S)
#  define sh_ntoh(D,S)

#  define sh_htons(S) S
#  define sh_htoni(I) I
#  define sh_htonl(L) L
#  define sh_htonf(F) F
#  define sh_htond(D) D

#  define sh_ntohs(S) S
#  define sh_ntohi(I) I
#  define sh_ntohl(L) L
#  define sh_ntohf(F) F
#  define sh_ntohd(D) D
#endif

/****************************************************************************/
/*
 * Open an SDSS data dump file.
 *
 * Valid modes are "a", "r", or "w" for append, read, or write. If you
 * capitalise the mode, closeDump() will simply close the file, without any
 * attempt to map id's to pointers ("r") or write referenced but
 * unwritten structures ("a" or "w").
 *
 * If mode is "a" or "r" check to see if it has a valid header; if "r"
 * leave the file pointer just after the header; if "a" leave it at the
 * end of the file after updating the date.
 *
 * The usual call is shDumpOpen, if you want to avoid the usual cleanup
 * of data structures call shDumpReopen instead.
 */

#define CTIME_LEN 25L			/* length of ctime() string */

/*
 * This had to be changed from READ and WRITE to SHREAD and SHWRITE to not
 * conflict with a READ and WRITE in libfits.h
 */
static enum { SHREAD, SHWRITE } open_type;   /* what was file opened for? */
static int no_cleanup;			/* should closeDump cleanup data
					   structures? */

FILE *
shDumpOpen(char *name, char *mode)
{
   p_shDumpStructsReset(1);		/* cleanup leftover data structures */

   return(shDumpReopen(name,mode));
}
   
FILE *
shDumpReopen(char *name, char *mode)
{
   char date[CTIME_LEN + 1];
   FILE *fil;

   no_cleanup = isupper(*mode) ? 1 : 0;	/* cleanup things on close? */
   
   if(strcmp(mode,"a") == 0 || strcmp(mode,"A") == 0) {
      open_type = SHWRITE;
      mode = "r+b";
      if((fil = fopen(name,mode)) == NULL) {
	 shError("shDumpReopen: can't open \"%s\" in mode \"%s\"",name,mode);
	 return(NULL);
      }
      if(fread(buf,sizeof(HEADER),1,fil) != 1) {
         shError("shDumpReopen: can't read header for \"%s\"",name);
         fclose(fil);
         return(NULL);
      }
      if(strcmp(buf,HEADER) != 0) {
         buf[sizeof(HEADER) - 1] = '\0';
         shError("shDumpReopen: header for \"%s\" is invalid",name);
         shError("   It is:        \"%s\"",buf);
         shError("   It should be: \"%s\"",HEADER);
         fclose(fil);
         return(NULL);
      }
      fseek(fil,0L,1);			/* change from read to write */
      if(write_date(fil) < 0) {
	 shError("shDumpReopen: can't update date string");
         fclose(fil);
         return(NULL);
      }
      fseek(fil,0L,2);
   } else if(strcmp(mode,"r") == 0 || strcmp(mode,"R") == 0) {
      open_type = SHREAD ;
      mode = "rb";
      if((fil = fopen(name,mode)) == NULL) {
	 shError("shDumpReopen: can't open \"%s\" in mode \"%s\"",name,mode);
	 return(NULL);
      }
      if(fread(buf,sizeof(HEADER),1,fil) != 1) {
         shError("shDumpReopen: can't read header for \"%s\"",name);
         fclose(fil);
         return(NULL);
      }
      if(strcmp(buf,HEADER) != 0) {
         buf[sizeof(HEADER) - 1] = '\0';
         shError("shDumpReopen: header for \"%s\" is invalid",name);
         shError("   It is:        \"%s\"",buf);
         shError("   It should be: \"%s\"",HEADER);
         fclose(fil);
         return(NULL);
      }
      if(fread(date,(unsigned int )CTIME_LEN,1,fil) == 0) {
         shError("shDumpReopen: can't read date for \"%s\"",name);
         fclose(fil);
         return(NULL);
      }
   } else if(strcmp(mode,"w") == 0 || strcmp(mode,"W") == 0) {
      open_type = SHWRITE;
      mode = "wb";
      if((fil = fopen(name,mode)) == NULL) {
	 shError("shDumpReopen: can't open \"%s\" in mode \"%s\"",name,mode);
	 return(NULL);
      }
      if(fwrite(HEADER,sizeof(HEADER),1,fil) != 1) {
         shError("shDumpReopen: can't write header for \"%s\"",name);
         fclose(fil);
         return(NULL);
      }
      if(write_date(fil) < 0) {
	 shError("shDumpReopen: can't update date string");
         fclose(fil);
         return(NULL);
      }
   } else {
      shError("shDumpReopen: Unknown mode: %s",mode);
      return(NULL);
   }
   
   return(fil);
}

/*
 * write the date into a file's header
 * (here no big-endiand/little-endian trouble: Only ascii is written)
 */
static int
write_date(FILE *fil)
{
   char *ct;
   time_t t;

   time(&t);
   ct = ctime(&t);
   ct[CTIME_LEN - 1] = '\0';
   return(fwrite(ct,(unsigned int )CTIME_LEN,1,fil) == 1 ? 0 : -1);
}

/*
 * Close a dump file.
 *
 * If the file was opened for read, see if there is any extra stuff
 * at the end of the file; if there is, read it. Then try to convert
 * all remaining pointer ids to addresses, and set up subregion pointers
 *
 * If the file was opened for write (or append) there may still be
 * referenced structures that haven't been written. This is a problem;
 * we can (and do) write them out, but when read back they'll be
 * anonymous (albeit with valid pointers to them somewhere)
 *
 * These actions can be disabled by setting the no_cleanup flag; this is
 * done by opening the file with a capitalised mode string (e.g. "R")
 */
RET_CODE
shDumpClose(FILE *fil)
{
   RET_CODE ret = SH_SUCCESS;		/* return code */
   
   if(!no_cleanup) {
      if(open_type == SHREAD) {
	 if(p_shDumpAnonRead(fil,p_shDumpReadShallow) != SH_SUCCESS) {
	    shError("closeDump: error reading anonymous structures");
	    ret = SH_GENERIC_ERROR;
	 }
	 
	 if(shDumpPtrsResolve() == SH_GENERIC_ERROR) {
	    shError("closeDump: failed to resolve all pointer ids");
	    ret = SH_GENERIC_ERROR;
	 }
/*
 * we've succesfully resolved all pointer ids, so it is OK to free the
 * list of anonymous objects
 */
	 shListDel(anon_list,(RET_CODE(*)(LIST_ELEM *))thingDelKeepPointers);
	 anon_list = NULL;
      } else if(open_type == SHWRITE) {
	 if(write_scheduled_ptrs(fil) == SH_GENERIC_ERROR) {
	    shError("closeDump: failed to write all pending pointerss");
	    ret = SH_GENERIC_ERROR;
	 }
	 addr_root = shTreeDel(addr_root);
      }
   }
   p_shDumpStructsReset(0);		/* this is benign and can save space */

   shListDel(struct_list,(RET_CODE(*)(LIST_ELEM *ptr))shThingDel);
   struct_list = NULL;

   if(fil != NULL && fclose(fil) == EOF) {
      shError("closeDump: EOF from fclose");
   }

   return(ret);
}

/*****************************************************************************/
/*
 * return the date string from a dump file, leave the file pointer
 * unchanged
 */
char *
shDumpDateGet(FILE *fil)
{
   static char date[CTIME_LEN + 1];
   long here = ftell(fil);

   if(fseek(fil,sizeof(HEADER),0) != 0) {
      shError("getDate: can't skip header");
      return(NULL);
   }
   if(fread(date,(unsigned int )CTIME_LEN,1,fil) == 0) {
      shError("getDate: can't read date");
      return(NULL);
   }
   date[CTIME_LEN] = '\0';

   if(fseek(fil,here,0) != 0) {
      shError("getDate: can't return to initial position");
      return(NULL);
   }
   
   return(date);
}

/*****************************************************************************/
/*
 * Find the type of the next thing in a dump file. Don't move the file pointer,
 * and don't print any messages if we fail (we use this function to see if
 * there are any more structures to read)
 */
RET_CODE
shDumpTypeGet(FILE *outfil, TYPE *type)
{
   long here = ftell(outfil);

   if(shDumpTypeRead(outfil,type) == SH_GENERIC_ERROR) {
      *type = shTypeGetFromName("UNKNOWN");
      fseek(outfil,here,0);
      return(SH_GENERIC_ERROR);
   }
   fseek(outfil,here,0);
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Replace a date string with Xs; useful for comparing dump files
 */
void
shDumpDateDel(char *name)
{
   FILE *fil;
   char str[CTIME_LEN];
   
   if((fil = shDumpOpen(name,"r")) == NULL) {
      shError("null_date: Can't open %s",name);
      return;
   }
   (void)fseek(fil,sizeof(HEADER),0);
   memset(str,'X',(unsigned int )CTIME_LEN);
   fwrite(str,(unsigned int )CTIME_LEN,1,fil);
   fclose(fil);
}


/*****************************************************************************/
/*
 * Load a set of schema defined in schema_in into p_shSchema, which is
 * allocated as needed. This function is, as the p_ indicates,
 * a private function; it is called by the code generated by make_io
 */
RET_CODE
p_shSchemaLoad(SCHEMA *schema_in)
{
   RET_CODE rstatus = SH_SUCCESS;	/* function return value */
   int i,j;
   int n;				/* number of array elements, i_nelem */
   int n_nsch;				/* number of schema in schema_in */
   int n_new;                           /* number of 'new' schema in schema_in */
   int error;                           /* True if we try to redefine a schema that is lock and
					   do no have SCHEMA_LOCK_NOWARN */
   const SCHEMA *sch;			/* schema for an element */
   char *type_name;			/* name of a type */
   const SCHEMA *schemaScratch;
   SCHEMA *unknown = NULL;		/* the UNKNOWN schema */
 
   /* Count the valid schemas */
   for(i = 0,error=0,n_new=0;*schema_in[i].name != '\0';i++) {
     if (    (schemaTable!= NULL)
	     /* Use shSchemaGet instead of shHashGet to deal with
		name and case woes */
	  && ( (schemaScratch = shSchemaGet (schema_in[i].name) )
              != NULL ) ) {
       if ( (schemaScratch->lock & SCHEMA_LOCK) != 0 ) {
	 schema_in[i].lock |= SCHEMA_LOCKED_OUT;
	 if ( (schemaScratch->lock & SCHEMA_LOCK_NOWARN) == 0 ) {
	   error++;
	 }
       } else {
	 schema_in[i].lock |= SCHEMA_REPLACE;
	 schema_in[i].id = schemaScratch->id;
       }
     } else { 
       n_new ++;
     }
   }
   n_nsch = i;

   if(p_shSchema == NULL) {
     p_shNSchema = 0;
     if((p_shSchema = (SCHEMA **)malloc((n_nsch + 1)*sizeof(SCHEMA *)))
								     == NULL) {
       fprintf(stderr,"Can't allocate storage for p_shSchema\n");
       return(SH_GENERIC_ERROR);
     }
   } else {
     unknown = p_shSchema[p_shNSchema - 1]; /* the unknown schema */
     p_shNSchema--;			/* overwrite UNKNOWN */
     
     if((p_shSchema = (SCHEMA **)realloc((char *)p_shSchema,
				(p_shNSchema + n_new + 1)*sizeof(SCHEMA *)))
								     == NULL) {
       fprintf(stderr,"Can't reallocate storage for p_shSchema\n");
       return(SH_GENERIC_ERROR);
     }
   }
/*
 * load the schema defined in this file into p_shSchema.
 */
   for(i = 0,j = 0;i < n_nsch;i++) {
     if ( (schema_in[i].lock & SCHEMA_LOCKED_OUT) == 0 ) {
       if ( (schema_in[i].lock & SCHEMA_REPLACE) == 0 ) {
	 p_shSchema[p_shNSchema + j] = &schema_in[i];
	 p_shSchema[p_shNSchema + j]->id = p_shNSchema + j;
	 j++;
       } else {
	  p_shSchema[schema_in[i].id] = &schema_in[i];
       }
     } 
   }
   p_shNSchema += j;
/*
 * include the UNKNOWN schema at the end
 */
   if(unknown == NULL) {
      unknown = &schema_in[n_nsch];	/* present in all make_io generated
					   schemas */
   }
   shAssert(unknown->type == UNKNOWN);
   p_shSchema[p_shNSchema++] = unknown;
/*
 * Initialise the hash table
 */
   if(schemaTable == NULL) {
      schemaTable = shHashTableNew(1<<9); /* nbucket MUST be a power of 2 */
   }
   /*
    * Fill in the hash table
    *
    * Silently ignore the schema definition if a schema of the same name is
    * already defined! This is clearly a bug
    */
   for(i = 0;i < n_nsch;i++) {
     if (schema_in[i].name[0] != '\0' &&
	 (schema_in[i].lock & SCHEMA_LOCKED_OUT) == 0) {
	rstatus = shHashAdd(schemaTable, schema_in[i].name,
			    (void *)(&schema_in[i]));
     } 
     
     /* Now see if the schema_in[i].name has an alias.  If it does the alias
	needs to be added to the hash table too. All of the schemas with
	aliases are of type PRIM.  So check this type first so we do not
	have to do all the strcmp's. Only add the aliases once. */
     if (aliasesAdded == ALIASES_INIT_VALUE)
       {
	 if (schema_in[i].type == PRIM) {
	   /* We have found a PRIM type, see if it has an alias */
	   if (strcmp(schema_in[i].name, "UCHAR") == 0) { 
	     rstatus = shHashAdd(schemaTable, "U8", (void *)(&schema_in[i])); 
	   } else if (strcmp(schema_in[i].name, "USHORT") == 0) {
	     rstatus = shHashAdd(schemaTable, "U16", (void *)(&schema_in[i]));
	   } else if (strcmp(schema_in[i].name, "UINT") == 0) { 
	     rstatus = shHashAdd(schemaTable, "U32", (void *)(&schema_in[i]));
	   } else if (strcmp(schema_in[i].name, "SHORT") == 0) {
	     rstatus = shHashAdd(schemaTable, "S16", (void *)(&schema_in[i]));
	     rstatus = shHashAdd(schemaTable, "SSHORT",(void *)(&schema_in[i]));
	   } else if (strcmp(schema_in[i].name, "INT") == 0) {
	     rstatus = shHashAdd(schemaTable, "S32", (void *)(&schema_in[i]));
	     rstatus = shHashAdd(schemaTable, "SINT", (void *)(&schema_in[i]));
	   } else if (strcmp(schema_in[i].name, "FLOAT") == 0) {
	     rstatus = shHashAdd(schemaTable, "FL32",(void *)(&schema_in[i]));
	   } else if (strcmp(schema_in[i].name, "DOUBLE") == 0) {
	     rstatus = shHashAdd(schemaTable, "FL64",(void *)(&schema_in[i]));
	   } else if (strcmp(schema_in[i].name, "LONG") == 0) {
	     rstatus = shHashAdd(schemaTable,"SLONG",(void *)(&schema_in[i]));
	   }
	 }
       }
     if (rstatus == SH_HASH_TAB_IS_FULL)
       {
	 return(rstatus);
       }   
   }

   /* 
    * We have added the aliases to the table, do not do it again.
    */
   aliasesAdded = ALIASES_OTHER_VALUE;

   /*
    * Fill out the `convenience' fields i_nelem and size. We could
    * calculate these values every time that we use this schema, but why
    * not do it now?
    */
   for(i = 0;i < n_nsch;i++) {
     /*
      * find the total number of elements, either 1 for a scalar, or the product
      * of all dimensions for an array
      */
     for(j = 0;j < schema_in[i].nelem;j++) {
       if(p_shSchemaNelemGet(&schema_in[i].elems[j],&n) != SH_SUCCESS) {
	 shError("problem loading schema definitions for %s",
		 schema_in[i].name);
       }
       schema_in[i].elems[j].i_nelem = n;
       /*
	* and the size of this type (or one element if an array)
	*/
       type_name = shNameGetFromType(
				     shTypeGetFromSchema(&schema_in[i].elems[j]));
       if((sch = shSchemaGet(type_name)) == NULL) {
	 shError("p_shSchemaLoad: in schema %s, can't retrieve schema for \
element %s:  type is %s",
		 schema_in[i].name, schema_in[i].elems[j].name,
		 schema_in[i].elems[j].type);
	 schema_in[i].elems[j].size = 0;
       } else {
	 schema_in[i].elems[j].size = sch->size;
       }
     }
   }
   
   if (error) {
     return(SH_SCHEMA_LOCKED);
   } else {
     return(SH_SUCCESS);
   }
}

/*****************************************************************************/
/*
 * Dump schema to a file. This can't be done using the usual dump code
 * as the schema pointers are used to identify objects in the dumps.
 */
RET_CODE
p_shDumpSchemaRead(FILE *outfil, SCHEMA **sch)
{
   *sch = (SCHEMA *)shMalloc(sizeof(SCHEMA));

   if(shDumpStrRead(outfil,(*sch)->name,SIZE) == SH_GENERIC_ERROR) {
      shError("p_shDumpSchemaRead: reading name");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,(int *)&(*sch)->type) == SH_GENERIC_ERROR) {
      shError("p_shDumpSchemaRead: reading type");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&(*sch)->nelem) == SH_GENERIC_ERROR) {
      shError("p_shDumpSchemaRead: reading nelem");
      return(SH_GENERIC_ERROR);
   }
/*
 * don't dump elems for now
 */
   return(SH_SUCCESS);
}

RET_CODE
p_shDumpSchemaWrite(FILE *outfil, SCHEMA *sch)
{
   if(shDumpStrWrite(outfil,sch->name) == SH_GENERIC_ERROR) {
      shError("p_shDumpSchemaWrite: writing name");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,(int *)&sch->type) == SH_GENERIC_ERROR) {
      shError("p_shDumpSchemaWrite: writing type");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&sch->nelem) == SH_GENERIC_ERROR) {
      shError("p_shDumpSchemaWrite: writing nelem");
      return(SH_GENERIC_ERROR);
   }
/*
 * don't dump elems for now
 */
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Save or restore all the schema to/from a dump file
 */
RET_CODE
p_shDumpSchemaSave(FILE *outfil, SCHEMA **sch, int nschema)
{
   int i;
   
   if(shDumpIntWrite(outfil,&nschema) == SH_GENERIC_ERROR) {
      shError("p_shDumpSchemaSave: writing nschema");
      return(SH_GENERIC_ERROR);
   }
   
   for(i = 0;i < nschema;i++) {
      if(p_shDumpSchemaWrite(outfil,sch[i]) == SH_GENERIC_ERROR) {;
         fprintf(stderr,"p_shDumpSchemaSave: can't write %dth schema\n",i);
         return(SH_GENERIC_ERROR);
      }
   }
   return(SH_SUCCESS);
}


RET_CODE
p_shDumpSchemaRestore(FILE *outfil, SCHEMA ***sch, int *nschema)
{
   int i;
   SCHEMA *sch_p;
   
   if(shDumpIntRead(outfil,nschema) == SH_GENERIC_ERROR) {
      shError("p_shDumpSchemaRestore: reading nschema");
      return(SH_GENERIC_ERROR);
   }
   
   (*sch) = (SCHEMA **)shMalloc(*nschema*sizeof(SCHEMA *));
   
   for(i = 0;i < *nschema;i++) {
      if(p_shDumpSchemaRead(outfil,&sch_p) == SH_GENERIC_ERROR) {;
         fprintf(stderr,"p_shDumpSchemaRestore: can't read %dth schema\n",i);
         return(SH_GENERIC_ERROR);
      }
      (*sch)[i] = sch_p;
   }
   
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * read a dump file, returning a chain of THINGs. This is a jacket routine
 * around p_shDumpRead(), which returns a pointer to a LIST of things. Since
 * links/lists are no longer supported in public dervish APIs, this jacket
 * routine converts the pointer to LIST to a pointer to a CHAIN. See also
 * the comments at the top of this file.
 */
CHAIN *
shDumpRead(FILE *outfil, int shallow_read)
{
   LIST  *pList;
   CHAIN *pChain;
   void  *pData, 
         *pWalkState;

   pList  = p_shDumpRead(outfil, shallow_read);
   pData  = pWalkState = NULL;
   pChain = shChainNew("THING");

   /*
    * Transfer the data from the LIST to a CHAIN
    */
   while ((pData = shListWalk(pList, &pWalkState)))
       shChainElementAddByPos(pChain, pData, "THING", TAIL, AFTER);

   /*
    * Now delete the list and the things on it - we don't need it any more
    */
   shListDel(pList,(RET_CODE(*)(LIST_ELEM *ptr))shThingDel);

   return pChain;
}

/*****************************************************************************/
/*
 * read a dump file, returning a list of THINGs
 */
static LIST *
p_shDumpRead(FILE *outfil, int shallow_read)
{
   char *date;
   LIST *list = shListNew(shTypeGetFromName("THING"));
   THING *thing;

   p_shDumpReadShallow = shallow_read;
   
   if((date = shDumpDateGet(outfil)) == NULL) {
      date = "";
   }
   thing = shThingNew(shMalloc(strlen(date)+1),shTypeGetFromName("STR"));
   strcpy((char *)thing->ptr,date);
   shListAdd(list,thing,ADD_BACK);
   
   while((thing = shDumpNextRead(outfil,p_shDumpReadShallow)) != NULL) {
      shListAdd(list,thing,ADD_BACK);
   }
   
   if(!p_shDumpReadShallow) p_shDumpStructsReset(1);
   return(list);
}

/*****************************************************************************/
/*
 * Read the next thing from a dump file
 */
THING *
shDumpNextRead(FILE *outfil, int level)
{
   void *ptr;				/* where to read into */
   const SCHEMA *sch;
   THING *thing;
   char *type_name;
   TYPE type;
   int didshMalloc;
   
   p_shDumpReadShallow = level;
   
   if(shDumpTypeGet(outfil,&type) != SH_SUCCESS) {
      shError("shDumpNextRead: can't get next type");
      return(NULL);
   }
   
   if(type == shTypeGetFromName("UNKNOWN")) {
      shError("shDumpNextRead: file contains an object of type UNKNOWN");
      return(NULL);
   } else if(type == shTypeGetFromName("STR")) {
      thing = shThingNew(shMalloc(sizeof(char **)),shTypeGetFromName("STR"));
      *(char **)thing->ptr = (char *)shMalloc(SIZE);
      if(shDumpStrRead(outfil,*(char **)thing->ptr,SIZE) == SH_GENERIC_ERROR){
         shError("shDumpNextRead: reading STR");
         shFree(thing->ptr);
         shThingDel(thing);
         return(NULL);
      }
   } else {
      type_name = shNameGetFromType(type);
      if((sch = (SCHEMA *)shSchemaGet(type_name)) == NULL) {
	 shError("shDumpNextRead: Unable to get schema for type %s",type_name);
	 return(NULL);
      }
      
      if(sch->read == NULL) {
	 shError("shDumpNextRead: Type %s is not dumpable",type_name);
	 return(NULL);
      } else if(sch->read == (RET_CODE (*)(FILE *, void *))shDumpGenericRead) {
	 thing = shThingNew(NULL,type);
	 if(shDumpGenericRead(outfil,type_name,&thing->ptr)
							 == SH_GENERIC_ERROR) {
	    shError("shDumpNextRead: reading %s",type_name);
	    shThingDel(thing);
	    return(NULL);
	 }
      } else {
	 thing = shThingNew(NULL,(TYPE)-1);
	 didshMalloc = 1;            /* Assume will do the shMalloc */
	 if(p_shTypeIsEnum(type) || strcmp(type_name,"INT") == 0) {
	    ptr = thing->ptr = shMalloc(sizeof(int));
	    thing->type = shTypeGetFromName("INT");
	 } else if(strcmp(type_name,"UINT") == 0) {
	    ptr = thing->ptr = shMalloc(sizeof(unsigned int));
	    thing->type = shTypeGetFromName("UINT");
	 } else if(strcmp(type_name,"LONG") == 0) {
	    ptr = thing->ptr = shMalloc(sizeof(long));
	    thing->type = shTypeGetFromName("LONG");
	 } else if(strcmp(type_name,"ULONG") == 0) {
	    ptr = thing->ptr = shMalloc(sizeof(unsigned long));
	    thing->type = shTypeGetFromName("ULONG");
	 } else if(strcmp(type_name,"FLOAT") == 0) {
	    ptr = thing->ptr = shMalloc(sizeof(float));
	    thing->type = shTypeGetFromName("FLOAT");
	 } else if(strcmp(type_name,"DOUBLE") == 0) {
	    ptr = thing->ptr = shMalloc(sizeof(double));
	    thing->type = shTypeGetFromName("DOUBLE");
	 } else if(strcmp(type_name,"LOGICAL") == 0) {
	    ptr = thing->ptr = shMalloc(sizeof(LOGICAL));
	    thing->type = shTypeGetFromName("LOGICAL");
	 } else {
	    didshMalloc = 0;
	    ptr = &thing->ptr;
	    thing->type = type;
	 }
	 
	 if((*sch->read)(outfil,ptr) == SH_GENERIC_ERROR) {
	    shError("shDumpNextRead: reading %s",type_name);
	    if (didshMalloc == 1) {shFree(ptr);}
	    shThingDel(thing);
	    return(NULL);
	 }
      }
   }
   
   return(thing);
}

/*****************************************************************************/
/*
 * read all the anonymous objects at the end of a file.
 *
 * The objects are put onto anon_list. Nota bene: it is NOT safe to free
 * this list until after all of the pointers are resolved.
 */
RET_CODE
p_shDumpAnonRead(FILE *outfil, int shallow_read)
{
   THING *thing;

   if(anon_list == NULL) {
      anon_list = shListNew(shTypeGetFromName("THING"));
   }
   
   p_shDumpReadShallow = shallow_read;
   
   while((thing = shDumpNextRead(outfil,p_shDumpReadShallow)) != NULL) {
      shListAdd(anon_list,thing,ADD_BACK);
   }

   return(SH_SUCCESS);
}


/*****************************************************************************/
/*
 * Write a THING to a history file; this is sort-of the opposite of
 * shDumpNextRead, hence the name
 */
RET_CODE
shDumpNextWrite(FILE *outfil, THING *thing)
{
   const SCHEMA *sch;
   char *type_name = shNameGetFromType(thing->type);

   if(strcmp(type_name,"UNKNOWN") == 0) {
      shError("shDumpNextWrite: attempting to write an UNKNOWN type");
      return(SH_GENERIC_ERROR);
   } else if(strcmp(type_name,"PTR") == 0) {
      if(shDumpPtrWrite(outfil,thing->ptr,shTypeGetFromName("PTR"))
							 == SH_GENERIC_ERROR) {
	 shError("shDumpNextWrite: writing thing->ptr");
	 return(SH_GENERIC_ERROR);
      }
      return(SH_SUCCESS);
   } else if(p_shTypeIsEnum(thing->type) || strcmp(type_name,"INT") == 0) {
      if(shDumpIntWrite(outfil,(int *)thing->ptr) == SH_GENERIC_ERROR) {
	 shError("shDumpNextWrite: writing *(int *)thing->ptr");
	 return(SH_GENERIC_ERROR);
      }
   } else if(strcmp(type_name,"LONG") == 0) {
      if(shDumpLongWrite(outfil,(long *)thing->ptr) == SH_GENERIC_ERROR) {
	 shError("shDumpNextWrite: writing *(long *)thing->ptr");
	 return(SH_GENERIC_ERROR);
      }
   } else if(strcmp(type_name,"FLOAT") == 0) {
      if(shDumpFloatWrite(outfil,(float *)thing->ptr) == SH_GENERIC_ERROR){
	 shError("shDumpNextWrite: writing *(float *)thing->ptr");
	 return(SH_GENERIC_ERROR);
      }
   } else if(strcmp(type_name,"DOUBLE") == 0) {
      if(shDumpDoubleWrite(outfil,(double *)thing->ptr)==SH_GENERIC_ERROR){
	 shError("shDumpNextWrite: writing *(double *)thing->ptr");
	 return(SH_GENERIC_ERROR);
      }
   } else {
      if((sch = (SCHEMA *)shSchemaGet(type_name)) == NULL) {
	 shError("shDumpNextWrt: Unable to get schema for type %s",type_name);
	 return(SH_GENERIC_ERROR);
      }
      if(sch->write == NULL) {
	 shError("shDumpNextWrite: Type %s is not dumpable",type_name);
	 return(SH_GENERIC_ERROR);
      } else if(sch->write == (RET_CODE (*)(FILE*, void*))shDumpGenericWrite) {
	 if(shDumpGenericWrite(outfil,type_name,thing->ptr)
							 == SH_GENERIC_ERROR) {
	    shError("shDumpNextWrite: writing %s",type_name);
	    return(SH_GENERIC_ERROR);
	 }
      } else {
	 if((*sch->write)(outfil,thing->ptr) == SH_GENERIC_ERROR) {
	    shError("shDumpNextWrite: writing %s",type_name);
	    return(SH_GENERIC_ERROR);
	 }
      }
   }
   
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * read something, given its schema and its parents address
 */
RET_CODE
shDumpSchemaElemRead(FILE *outfil,char *parent_type,
                          void *thing,SCHEMA_ELEM *sch_el)
{
   int i;
   void *iptr;				/* initial value of ptr */
   void *ptr;
   TYPE type;
   const SCHEMA *sch;
   
   if((ptr = shElemGet(thing,sch_el,&type)) == NULL) {
      return(SH_GENERIC_ERROR);
   }
   if(p_shTypeIsEnum(type)) {
      type = shTypeGetFromName("INT");
   }
   
   for(iptr = ptr,i = 0;i < sch_el->i_nelem;i++) {
      ptr = (void *)((char *)iptr + i*sch_el->size); /* it may be an array */

      if(type == shTypeGetFromName("STR")) {
	 char i_name[SIZE];
	 shDebug(1,"shMallocing space for ptr\n");
	       
	 if(shDumpStrRead(outfil,i_name,SIZE) == SH_GENERIC_ERROR) {
	    shError("shDumpSchemaElemRead: reading i_name");
	    return(SH_GENERIC_ERROR);
	 }
	 *(char **)ptr = (char *)shMalloc(strlen(i_name) + 1);
	 strcpy(*(char **)ptr,i_name);
      } else if(type == shTypeGetFromName("PTR") || sch_el->nstar > 0) {
	 if(shDumpPtrRead(outfil,(void **)ptr) == SH_GENERIC_ERROR) {
	    shError("shDumpSchemaElemRead: reading ptr");
	    return(SH_GENERIC_ERROR);
	 }
      } else {
	 char *type_name = shNameGetFromType(type);
	 if((sch = (SCHEMA *)shSchemaGet(type_name)) == NULL) {
	    shError("shDumpSchemaElemRead: Unable to get schema for type %s",
		    type_name);
	    return(SH_GENERIC_ERROR);
	 }
	 if(sch->read == NULL) {
	    shError("shDumpSchemaElemRead: Type %s is not dumpable",type_name);
	    return(SH_GENERIC_ERROR);
	 } else if(sch->read == (RET_CODE (*)(FILE*, void*))shDumpGenericRead){
	    if(shDumpGenericRead(outfil,type_name,(void **)ptr)
                                           == SH_GENERIC_ERROR) {
	       shError("shDumpSchemaElemRead: reading %s",type_name);
	       return(SH_GENERIC_ERROR);
	    }
	 } else {
	    if((*sch->read)(outfil,ptr) == SH_GENERIC_ERROR) {
	       shError("shDumpSchemaElemRead: reading %s",type_name);
	       return(SH_GENERIC_ERROR);
	    }
	 }
      }
   }
   return(SH_SUCCESS);
}

/*
 * Write something given an base address and a schema element
 */
RET_CODE
shDumpSchemaElemWrite(FILE *outfil,char *parent_type,
		      void *thing,SCHEMA_ELEM *sch_el)
{
   int i;
   void *iptr;				/* initial value of ptr */
   void *ptr;
   TYPE type;
   char *type_name;
   const SCHEMA *sch;

   if((ptr = shElemGet(thing,sch_el,&type)) == NULL) {
      return(SH_GENERIC_ERROR);
   }

   type_name = shNameGetFromType(type);
   
   for(iptr = ptr, i = 0;i < sch_el->i_nelem;i++) {
      ptr = (void *)((char *)iptr + i*sch_el->size); /* it may be an array */

      if(p_shTypeIsEnum(type) || strcmp(type_name,"INT") == 0) {
	 if(shDumpIntWrite(outfil,(int *)ptr) == SH_GENERIC_ERROR) {
	    shError("shDumpSchemaElemWrite: writing *(int *)ptr");
	    return(SH_GENERIC_ERROR);
         }
      } else if(strcmp(type_name,"LONG") == 0) {
	 if(shDumpLongWrite(outfil,(long *)ptr) == SH_GENERIC_ERROR) {
	    shError("shDumpSchemaElemWrite: writing *(long *)ptr");
	    return(SH_GENERIC_ERROR);
	 }
      } else if(strcmp(type_name,"FLOAT") == 0) {
	 if(shDumpFloatWrite(outfil,(float *)ptr) == SH_GENERIC_ERROR) {
	    shError("shDumpSchemaElemWrite: writing *(float *)ptr");
	    return(SH_GENERIC_ERROR);
	 }
      } else if(strcmp(type_name,"DOUBLE") == 0) {
	 if(shDumpDoubleWrite(outfil,(double *)ptr) == SH_GENERIC_ERROR) {
	    shError("shDumpSchemaElemWrite: writing *(double *)ptr");
	    return(SH_GENERIC_ERROR);
	 }
      } else if(strcmp(type_name,"LOGICAL") == 0) {
	 if(shDumpLogicalWrite(outfil,(LOGICAL *)ptr) == SH_GENERIC_ERROR) {
	    shError("shDumpSchemaElemWrite: writing *(LOGICAL *)ptr");
	    return(SH_GENERIC_ERROR);
         }
      } else if(strcmp(type_name,"STR") == 0) {
	 if(shDumpStrWrite(outfil,*(char **)ptr) == SH_GENERIC_ERROR) {
	    shError("shDumpSchemaElemWrite: writing *(char **)ptr");
	    return(SH_GENERIC_ERROR);
	 }
      } else if(strcmp(type_name,"PTR") == 0) {
	 TYPE ptype;				/* type of `*PTR' */

	 if(sch_el->nstar > 1) {
	    ptype = type;		/* i.e. PTR */
	 } else {
	    ptype = shTypeGetFromName(sch_el->type);
	 }

	 if(shDumpPtrWrite(outfil,*(void **)ptr,ptype) == SH_GENERIC_ERROR) {
	    shError("shDumpSchemaElemWrite: writing ptr");
	    return(SH_GENERIC_ERROR);
	 }
      } else {
	 if((sch = shSchemaGet(type_name)) == NULL) {
	    shError("shDumpSchemaElemWrite: Unable to get schema for type %s",
		    type_name);
	    return(SH_GENERIC_ERROR);
	 }
	 if(sch->write == NULL) {
	    shError("shDumpSchemaElemWrite: Type %s is not dumpable",
		    type_name);
	    return(SH_GENERIC_ERROR);
	 } else if(sch->write ==
		   	    (RET_CODE (*)(FILE *, void *))shDumpGenericWrite) {
	    if(shDumpGenericWrite(outfil,type_name,ptr) == SH_GENERIC_ERROR) {
	       shError("shDumpSchemaElemWrite: writing %s",type_name);
	       return(SH_GENERIC_ERROR);
	    }
	 } else {
	    if((*sch->write)(outfil,ptr) == SH_GENERIC_ERROR) {
	       shError("shDumpSchemaElemWrite: reading %s",type_name);
	       return(SH_GENERIC_ERROR);
	    }
	 }
      }
   }
   
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Support code for THINGs
 */
THING *
shThingNew(void *ptr, TYPE type)
{
   THING *newThing;
   
   if((newThing = (THING *)shMalloc(sizeof(THING))) == NULL) {
      shFatal("newThing: can't alloc for new THING");
   }
   newThing->ltype = shTypeGetFromName("THING");
   newThing->next = newThing->prev = NULL;
   newThing->ptr = ptr;
   if(shIsShMallocPtr(newThing->ptr)) {
      p_shMemRefCntrIncr(newThing->ptr);
   }
   newThing->type = type;

   return(newThing);
}

RET_CODE
shThingDel(THING *elem)
{
   if(elem != NULL) {
      shAssert(elem->next == NULL && elem->prev == NULL);
      if(shIsShMallocPtr(elem->ptr)) {
	 p_shMemRefCntrDecr(elem->ptr);
      }
      shFree((char *)elem);
   }
   return(SH_SUCCESS);
}

/*
 * like shThingDel, but don't delete what is pointed at
 */
static RET_CODE
thingDelKeepPointers(THING *elem)
{
   if(elem != NULL) {
      shFree((char *)elem);
   }
   return(SH_SUCCESS);
}

/****************************************************************************/
/*
 * Code to read, write, or skip over things
 *
 * First read/write TYPEs
 */
RET_CODE
shDumpTypeRead(FILE *fil, TYPE *t)
{
   int i = 0L;

   if(fread((void *)&i,sizeof(int),1,fil) < 1) {
      return(SH_GENERIC_ERROR);
   }
   *t = (TYPE)sh_ntohi(i);
   return(SH_SUCCESS);
}

RET_CODE
shDumpTypeWrite(FILE *fil, TYPE *t)
{
   int i = sh_htoni((int)*t);
   
   return(fwrite((void *)&i,sizeof(int),1,fil) == 1
	  ? SH_SUCCESS : SH_GENERIC_ERROR);
}

/*****************************************************************************/
/*
 * read/write type info to/from a file
 */
RET_CODE
p_shDumpTypecheckWrite(FILE *outfil,char *type)
{
   TYPE t = shTypeGetFromName(type);
   
   return(shDumpTypeWrite(outfil,&t));
}

/*
 * check if a type in a dump is correct
 */
int
p_shDumpTypeCheck(FILE *outfil,char *expected_type)
{
   TYPE et = shTypeGetFromName(expected_type);
   TYPE type;
   
   if(shDumpTypeRead(outfil,&type) == SH_GENERIC_ERROR) {
      shError("p_shDumpTypeCheck: reading type");
      return(-1);
   }
   if(type != et) {
      shError("p_shDumpTypeCheck: expecting a %s; found a %s",
	      expected_type,shNameGetFromType(type));
      return(-1);
   }
   return(0);
}

/*****************************************************************************/
/*
 * Then the primitive C types
 */
RET_CODE
shDumpCharRead(FILE *fil, char *c)
{
   if(p_shDumpTypeCheck(fil,"CHAR") != 0) {
      shError("shDumpCharRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   return(fread((void *)c,sizeof(char),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpCharWrite(FILE *fil, char *c)
{
   if(p_shDumpTypecheckWrite(fil,"CHAR") != SH_SUCCESS) {
      shError("shDumpCharWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)c,sizeof(char),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpUcharRead(FILE *fil, unsigned char *uc)
{
   if(p_shDumpTypeCheck(fil,"UCHAR") != 0) {
      shError("shDumpUcharRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   return(fread((void *)uc,sizeof(unsigned char),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpUcharWrite(FILE *fil, unsigned char *uc)
{
   if(p_shDumpTypecheckWrite(fil,"UCHAR") != SH_SUCCESS) {
      shError("shDumpUcharWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)uc,sizeof(unsigned char),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpScharRead(FILE *fil, signed char *sc)
{
   if(p_shDumpTypeCheck(fil,"SCHAR") != 0) {
      shError("shDumpScharRead: invalid type on read\n");
      return(SH_GENERIC_ERROR);
   }
   return(fread((void *)sc,sizeof(signed char),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpScharWrite(FILE *fil, signed char *sc)
{
   if(p_shDumpTypecheckWrite(fil,"SCHAR") != SH_SUCCESS) {
      shError("shDumpScharWrite: invalid type on write\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)sc,sizeof(signed char),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpFloatRead(FILE *fil, float *x)
{
   float f = 0.0;

   if(p_shDumpTypeCheck(fil,"FLOAT") != 0) {
      shError("shDumpFloatRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   
   if (fread((void *)&f,sizeof(float),1,fil) < 1 )
     return SH_GENERIC_ERROR;

   *x = sh_ntohf( f);
   return SH_SUCCESS;
}

RET_CODE
shDumpFloatWrite(FILE *fil, float *x)
{
   float f = sh_htonf( *x);

   if(p_shDumpTypecheckWrite(fil,"FLOAT") != SH_SUCCESS) {
      shError("shDumpFloatWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)&f,sizeof(float),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpDoubleRead(FILE *fil, double *x)
{
   double d = 0.0;

   if(p_shDumpTypeCheck(fil,"DOUBLE") != 0) {
      shError("shDumpDoubleRead: checking type");
      return(SH_GENERIC_ERROR);
   }

   if (fread((void *)&d,sizeof(double),1,fil) < 1)
     return SH_GENERIC_ERROR;

   *x = sh_ntohd( d);
   return SH_SUCCESS;
}

RET_CODE
shDumpDoubleWrite(FILE *fil, double *x)
{
   double d = sh_htond( *x);

   if(p_shDumpTypecheckWrite(fil,"DOUBLE") != SH_SUCCESS) {
      shError("shDumpDoubleWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)&d,sizeof(double),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

/*****************************************************************************/
/*
 * Old: Note that ints are actually written as longs, in case ints have
 * different sizes on different machines
 * New: ints have a fixed size for Sky Survey Software (4 bytes)
 *      whereas longs do NOT have the same size
 *      (PGC 08/28/95)
 */

RET_CODE
shDumpIntRead(FILE *fil, int *i)
{
   int j = 0L;
   
   if(p_shDumpTypeCheck(fil,"INT") != 0) {
      shError("shDumpIntRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(fread((void *)&j,sizeof(int),1,fil) < 1) {
      return(SH_GENERIC_ERROR);
   }

   *i = (int )sh_ntohi(j);
   return(SH_SUCCESS);
}

RET_CODE
shDumpIntWrite(FILE *fil, int *i)
{
   int j = *i;
   
   j = sh_htoni(j);
   if(p_shDumpTypecheckWrite(fil,"INT") != SH_SUCCESS) {
      shError("shDumpIntWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)&j,sizeof(int),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpUintRead(FILE *fil, unsigned int *ui)
{
   unsigned int uj = 0L;
   
   if(p_shDumpTypeCheck(fil,"UINT") != 0) {
      shError("shDumpUintRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(fread((void *)&uj,sizeof(unsigned int),1,fil) < 1) {
      return(SH_GENERIC_ERROR);
   }

   *ui = (unsigned int )sh_ntohi(uj);
   return(SH_SUCCESS);
}

RET_CODE
shDumpUintWrite(FILE *fil, unsigned int *ui)
{
   unsigned int uj = *ui;
   
   uj = sh_htoni(uj);
   if(p_shDumpTypecheckWrite(fil,"UINT") != SH_SUCCESS) {
      shError("shDumpUintWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)&uj,sizeof(unsigned int),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpShortRead(FILE *fil, short *s)
{
   if(p_shDumpTypeCheck(fil,"SHORT") != 0) {
      shError("shDumpShortRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(fread((void *)s,sizeof(short),1,fil) < 1) {
      return(SH_GENERIC_ERROR);
   }
   *s = sh_ntohs(*s);
   return(SH_SUCCESS);
}

RET_CODE
shDumpShortWrite(FILE *fil, short *s)
{
   short ls = sh_htons(*s);

   if(p_shDumpTypecheckWrite(fil,"SHORT") != SH_SUCCESS) {
      shError("shDumpShortWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)&ls,sizeof(short),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpUshortRead(FILE *fil, unsigned short *us)
{
   if(p_shDumpTypeCheck(fil,"USHORT") != 0) {
      shError("shDumpUshortRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(fread((void *)us,sizeof(unsigned short),1,fil) < 1) {
      return(SH_GENERIC_ERROR);
   }
   *us = sh_ntohs(*us);
   return(SH_SUCCESS);
}

RET_CODE
shDumpUshortWrite(FILE *fil, unsigned short *us)
{
   unsigned short lus = sh_htons(*us);

   if(p_shDumpTypecheckWrite(fil,"USHORT") != SH_SUCCESS) {
      shError("shDumpUshortWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)&lus,sizeof(unsigned short),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

/* 
 * In order to provide for being able to read/write long of 
 * different size. We had a long_size field in the Dump file.
 *
 * When we read a long with a size different from this of the host:
 *     o if the read size is too wide for the host, we return
 *       the minimun between LONG_MAX (for the host) and the
 *       actual value read.
 *     o if the read size is too narrow we read it at the 'left' of
 *       the (host's) long.
 *       then if the host is big endian we move the bits to the right to 
 *       align the two numbers.
 *       if the host is little endian we just have to reverse the byte order
 *       of the read value and we are fine.
 * 
 */

RET_CODE
shDumpLongRead(FILE *fil, long *l)
{
   char long_size;
   
   if(p_shDumpTypeCheck(fil,"LONG") != 0) {
      shError("shDumpLongRead: checking type");
      return(SH_GENERIC_ERROR);
   }

   shDumpCharRead(fil, &long_size);
   
   if ( long_size == sizeof(long) ) {
     if(fread((void *)l,sizeof(long),1,fil) < 1) {
       return(SH_GENERIC_ERROR);
     };
     *l = sh_ntohl(*l);
   } else {
     if ( long_size < sizeof(long) ) {
       if(fread((void *)l,sizeof(long_size),1,fil) < 1) {
	 return(SH_GENERIC_ERROR);
       };
#      if defined(SDSS_LITTLE_ENDIAN)
          sh_ntoh((void*)*l,long_size);
#      else
          *l = sh_ntohl ( *l >> 8 * (sizeof(long)-long_size) );
#      endif
     }
     else {
       int k,overflow;
       
       printf( "WARNING:\nThis Dump file has long of %d bits while your system only has %d bits.\nThe obtained value is:min(LONG_MAX,read value).\n",
	      8*long_size,8*sizeof(long));
       
       overflow =0;
       for (k=0;k< long_size/sizeof(long);k++){
	   if(fread((void *)l,sizeof(long),1,fil) < 1) {
	     return(SH_GENERIC_ERROR);
	   };
	   sh_ntoh((void*)*l,sizeof(long));
	   if ((*l!=0)&&(k< long_size/sizeof(long)-1) ) {
	     overflow = 1; 
	   };
       };
       if (overflow) *l = LONG_MAX;
     };
   };

   return(SH_SUCCESS);
}

RET_CODE
shDumpLongWrite(FILE *fil, long *l)
{
   long ll = sh_htonl(*l);
   char long_size = (char) sizeof(long);

   if(p_shDumpTypecheckWrite(fil,"LONG") != SH_SUCCESS) {
      shError("shDumpLongWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   shDumpCharWrite(fil, &long_size);
   return(fwrite((void *)&ll,sizeof(long),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpUlongRead(FILE *fil, unsigned long *ul)
{
   char long_size;

   if(p_shDumpTypeCheck(fil,"ULONG") != 0) {
      shError("shDumpUlongRead: checking type");
      return(SH_GENERIC_ERROR);
   }

   shDumpCharRead(fil, &long_size);
   
   if ( long_size == sizeof(unsigned long) ) {
     if(fread((void *)ul,sizeof(unsigned long),1,fil) < 1) {
       return(SH_GENERIC_ERROR);
     };
     *ul = sh_ntohl(*ul);
   } else {
     if ( long_size < sizeof(unsigned long) ) {
       if(fread((void *)ul,sizeof(long_size),1,fil) < 1) {
	 return(SH_GENERIC_ERROR);
       };
#      if defined(SDSS_LITTLE_ENDIAN)
          sh_ntoh((void*)*ul,long_size);
#      else
          *ul = sh_ntohl ( *ul >> 8 * (sizeof(unsigned long)-long_size) );
#      endif
     }
     else {
       int k,overflow;
       
       printf( "WARNING:\nThis Dump file has long of %d bits while your system only has %d bits.\nThe obtained value is:min(ULONG_MAX,read value).\n",
	      8*long_size,8*sizeof(unsigned long));

       overflow =0;
       for (k=0;k< long_size/sizeof(unsigned long);k++) {
	   if (fread((void *)ul,sizeof(unsigned long),1,fil) < 1) {
	     return(SH_GENERIC_ERROR);
	   };
	   if ((*ul!=0)&&(k< long_size/sizeof(unsigned long)-1 )) {
	     overflow = 1; 
	   };
       };
       if (overflow) *ul = ULONG_MAX;
     };  
   }
   *ul = sh_ntohl(*ul);
   return(SH_SUCCESS);
}

RET_CODE
shDumpUlongWrite(FILE *fil, unsigned long *ul)
{
   unsigned long lul = sh_htonl(*ul);
   char long_size = (char) sizeof(long);

   if(p_shDumpTypecheckWrite(fil,"ULONG") != SH_SUCCESS) {
      shError("shDumpUlongWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   shDumpCharWrite(fil, &long_size);

   return(fwrite((void *)&lul,sizeof(unsigned long),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

/*****************************************************************************/
/*
 * Logicals are handled just as ints are.
 */

RET_CODE
shDumpLogicalRead(FILE *fil, LOGICAL *i)
{
   int j = 0L;
   
   if(p_shDumpTypeCheck(fil,"LOGICAL") != 0) {
      shError("shDumpLogicalRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(fread((void *)&j,sizeof(int),1,fil) < 1) {
      return(SH_GENERIC_ERROR);
   }

   *i = (LOGICAL)sh_ntohi(j);
   return(SH_SUCCESS);
}

RET_CODE
shDumpLogicalWrite(FILE *fil, LOGICAL *i)
{
   int j = (*i) ? 1 : 0;
   
   j = sh_htoni(j);
   if(p_shDumpTypecheckWrite(fil,"LOGICAL") != SH_SUCCESS) {
      shError("shDumpLogicalWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   return(fwrite((void *)&j,sizeof(int),1,fil) == 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

/*****************************************************************************/
/*
 * Read an id and attempt to convert it to an address; if this fails schedule
 * the address for later conversion when we've seen the desired object
 *
 * Writing is easier; simply write the id
 */
RET_CODE
shDumpPtrRead(FILE *fil, void **ptr)
{
   IPTR addr;
   int id;
   
   if(shDumpIntRead(fil,&id) == SH_GENERIC_ERROR) {
      return(SH_GENERIC_ERROR);
   }
   if(id == 0) {
      *ptr = NULL;
   } else if((addr = shTreeValFind(id_root,id)) >= 0) {
      *ptr = (void *)addr;
      shDebug(1,"Read known id: %d --> "PTR_FMT,id,*ptr);
   } else {
      shDebug(1,"Read unknown id: %d",id);
      unknown_root = shTreeMkeyInsert(unknown_root,id,TO_IPTR(ptr));
      *ptr = (void *)id;
   }
   return(SH_SUCCESS);
}

RET_CODE
shDumpPtrWrite(FILE *fil, void *ptr, TYPE type)
{
   int id;

   /*
    * In order to make dumps work with the new memory manager, this 
    * function was the only one that needed modification. Basically,
    * there were two modifications in order:
    * 1) check if ptr == NULL, if so set id to 0. In the old memory
    *    manager, shIsShMallocPtr(NULL) returned true (!?!), so id
    *    got set accordingly. In the new memory manager, shIsShMallocPtr(0)
    *    return false, so we have to handle the case when ptr is NULL 
    *    as the very first if statement
    * 2) In the old memory manager, if shIsShMallocPtr(address) returned
    *    false, then p_shMemAddrIntern(address) was called to "internalize"
    *    the address. In the new memory manager, p_shMemAddrIntern() does
    *    nothing. It's there to be used for successful linking, but beyond
    *    that, it does nothing useful. So, if we come across an address
    *    that has not been shMalloc()'ed, we inform the user and abort.
    *    If anyone has problems with this, we'll have to work something
    *    out.
    * - vijay (Nov 15, 1994)
    */
   if (ptr == NULL) 
       id = 0;
   else if(shIsShMallocPtr(ptr)) {
      id = p_shMemRefCntrDel(p_shMemIdFind(TO_IPTR(ptr)));
      shAssert(id != -1);		/* i.e. a valid address */
   } else {				/* this is not a shMalloced address */
      shFatal("shDumpPtrWrite: all pointers to be dumped _must_ be obtained"
              " via a call to\nshMalloc(). Pointers obtained from a call to"
              " malloc() or any other memory\nallocation routine are not"
              " dumpable");
   }

   if(ptr != NULL) {
/*
 * See if we need to schedule this ptr to be written
 */
      if(type != shTypeGetFromName("PTR")) { /* we know what it is */
	 if(shTreeValFind(addr_root,TO_IPTR(ptr)) < 0 && /* neither written */
	    shTreeValFind(ptr_root,TO_IPTR(ptr)) < 0) { /* nor scheduled */
      
	    shDebug(1,"Scheduling %s %ld to be written",
		    shNameGetFromType(type),id);
	    ptr_root = shTreeKeyInsert(ptr_root,TO_IPTR(ptr),type);
	 }
      }
   }

   return(shDumpIntWrite(fil,&id));
}

/*****************************************************************************/
/*
 * Now deal with strings; write a length and then the string (including
 * the NUL that isn't included in the length)
 */
RET_CODE
shDumpStrRead(FILE *fil, char *s, int n)
{
   int len;
   
   if(p_shDumpTypeCheck(fil,"STR") != 0) {
      shError("shDumpStrRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(fil,&len) == SH_GENERIC_ERROR) {
      shError("readStr: Can't read length of string");
      return(SH_GENERIC_ERROR);
   }
   if(len == 0) {
      *s = '\0';			/* it was NULL; now it's empty */
      return(SH_SUCCESS);
   } else if(len >= n) {
      shError("readStr: Insufficient space for string");
      return(SH_GENERIC_ERROR);
   }
   return(fread((void *)s,1,(unsigned int )(len + 1),fil) == len + 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

RET_CODE
shDumpStrWrite(FILE *fil, char *s)
{
   int len;

   if(p_shDumpTypecheckWrite(fil,"STR") != SH_SUCCESS) {
      shError("shDumpStrWrite: writing TYPE\n");
      return(SH_GENERIC_ERROR);
   }
   if(s == NULL) {
      len = 0;
   } else {
      len = strlen(s);
   }
   if(shDumpIntWrite(fil,&len) == SH_GENERIC_ERROR) {
      shError("writeStr: Can't write length of string \"%s\"",s);
      return(SH_GENERIC_ERROR);
   }
   if(len == 0) {			/* a NULL pointer */
      return(SH_SUCCESS);
   }
   return(fwrite((void *)s,1,(unsigned int )(len + 1),fil) == len + 1 ?
	  SH_SUCCESS : SH_GENERIC_ERROR);
}

/*****************************************************************************/
/*
 * Read/Write some generic type, given only the name of the type
 */
RET_CODE
shDumpGenericRead(
		  FILE *outfil,		/* input file */
		  char *type_name,	/* name of type, e.g. FOO */
		  void **home		/* where the new object should go */
		  )
{
   int i;
   int id;
   void *newObj;
   const SCHEMA *sch;
/*
 * find out what we know about our type
 */
   if((sch = (SCHEMA *)shSchemaGet(type_name)) == NULL) {
      shError("Can't retrieve schema for %s",type_name);
      return(SH_GENERIC_ERROR);
   }
/*
 * read header
 */
   if(p_shDumpTypeCheck(outfil,type_name) != 0) {
      shError("shDumpGenericRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&id) == SH_GENERIC_ERROR) {
      shError("shDumpGenericRead: reading id");
      return(SH_GENERIC_ERROR);
   }
   if(id == 0) {
      *home = NULL;
      return(SH_SUCCESS);
   } else if(p_shIsInternedId(id)) {	/* no need to malloc space */
      newObj = home;
   } else {
/*
 * Now make the newObj thing
 */
      shAssert(sch->construct != NULL);	/* we need a constructor */
      *home = newObj = (*sch->construct)();
   }
   p_shPtrIdSave(id,newObj);
/*
 * read elements of structure
 */
   for(i = 0;i < sch->nelem;i++) {
      shDebug(2,"shDumpGenericRead: reading %s",sch->elems[i].name);
      
      if(shDumpSchemaElemRead(outfil,type_name,newObj,&sch->elems[i]) != 
								  SH_SUCCESS) {
	 shError("shDumpGenericRead: reading %s",sch->elems[i].name);
      }
   }
   return(SH_SUCCESS);
}

RET_CODE
shDumpGenericWrite(
		   FILE *outfil,	/* input file */
		   char *type_name,	/* name of type, e.g. FOO */
		   void *obj		/* pointer to object */
		   )
{
   int i;
   const SCHEMA *sch;
/*
 * find out what we know about our type
 */
   if((sch = (SCHEMA *)shSchemaGet(type_name)) == NULL) {
      shError("Can't retrieve schema for %s",type_name);
      return(SH_GENERIC_ERROR);
   }
/*
 * write object header
 */
   if(p_shDumpTypecheckWrite(outfil,type_name) != SH_SUCCESS) {
      shError("shDumpGenericWrite: Write type\n");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,obj,shTypeGetFromName("PTR")) == SH_GENERIC_ERROR){
      shError("shDumpFooWrite: writing obj");
      return(SH_GENERIC_ERROR);
   }
/*
 * write elements of structure
 */
   for(i = 0;i < sch->nelem;i++) {
      shDebug(2,"shDumpGenericWrite: writing %s",sch->elems[i].name);

      if(shDumpSchemaElemWrite(outfil,type_name,obj,&sch->elems[i]) !=
								  SH_SUCCESS) {
	 shError("shDumpGenericWrite: writing %s",sch->elems[i].name);
      }
   }

   p_shDumpStructWrote(obj);
   
   return(SH_SUCCESS);
}

/****************************************************************************/
/*
 * and now for SDSS types. Almost all the code is generated
 * automatically by make_io, the only exceptions being MASKs and
 * REGIONs which are a bit weird due to submasks/regions
 */

/*
 * Read a mask; note that we have to do some readahead before we can
 * create the new mask
 */
static int
readMask_p(FILE *fil, struct mask_p *prvt)
{
   if(shDumpIntRead(fil,&prvt->flags) == SH_GENERIC_ERROR) {
      shError("readMask_p: reading prvt->flags");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(fil,&prvt->crc) == SH_GENERIC_ERROR) {
      shError("readMask_p: reading prvt->crc");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(fil,&prvt->nchild) == SH_GENERIC_ERROR) {
      shError("readMask_p: reading prvt->nchild");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrRead(fil,(void **)&prvt->parent) == SH_GENERIC_ERROR) {
      shError("readMask_p: reading prvt->parent");
      return(SH_GENERIC_ERROR);
   }
   return(SH_SUCCESS);
}

RET_CODE
shDumpMaskRead(FILE *outfil, MASK **home)
{
   int i;
   int id;
   char name[SIZE];
   MASK *newMask;
   int xsize,ysize;
   
   if(p_shDumpTypeCheck(outfil,"MASK") != 0) {
      shError("shDumpMaskRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&id) == SH_GENERIC_ERROR) {
      shError("readMask: reading id");
      return(SH_GENERIC_ERROR);
   }
   if(id == 0) {
      *home = NULL;
      return(SH_SUCCESS);
   }

   if(shDumpStrRead(outfil,name,SIZE) == SH_GENERIC_ERROR) {
      shError("readMask: reading newMask->name");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&xsize) == SH_GENERIC_ERROR) {
      shError("readMask: reading newMask->nrow");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&ysize) == SH_GENERIC_ERROR) {
      shError("readMask: reading newMask->ncol");
      return(SH_GENERIC_ERROR);
   }
   *home = newMask = shMaskNew(name,xsize,ysize);
   p_shPtrIdSave(id,newMask);

   if(shDumpPtrRead(outfil,(void **)&newMask->prvt->parent)
                                       == SH_GENERIC_ERROR) {
      shError("readMask: reading newMask->prvt->parent");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&newMask->row0) == SH_GENERIC_ERROR) {
      shError("readMask: reading newMask->row0");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&newMask->col0) == SH_GENERIC_ERROR) {
      shError("readMask: reading newMask->col0");
      return(SH_GENERIC_ERROR);
   }
   if(readMask_p(outfil,newMask->prvt) == SH_GENERIC_ERROR) {
      shError("readMask: reading newMask->prvt");
      return(SH_GENERIC_ERROR);
   }
/*
 * deal with image data
 */
   if(shDumpIntRead(outfil,&id) == SH_GENERIC_ERROR) {
      shError("readMask: rereading id");
      return(SH_GENERIC_ERROR);
   }
   if(id == 0) {
      if(p_shDumpReadShallow) {		/* skip the data */
	 if(fseek(outfil,newMask->nrow*newMask->ncol,1) != 0) {
	    shError("shDumpMaskRead: can't seek over data");
	    return(SH_GENERIC_ERROR);
	 }
      } else {				/* really read it */
	 for(i = 0;i < newMask->nrow;i++) {
/*
 * One can read the Mask directly (i.e. with translation to 'network byte
 * order' since the 'elements' of the mask array are 1 byte long only
 * (There is though to grow them to two bytes and that case a translation
 *  will probably be necessary)
 */
	    if(fread((void *)newMask->rows[i],1,newMask->ncol,outfil)
                                                     < newMask->ncol) {
	       shError("readMask: can't read data");
	       return(SH_GENERIC_ERROR);
	    }
	 }
      }
   } else if(shTreeValFind(id_root,id) >= 0) {
      shDebug(1,"Setting image pointers in submask");
      for(i = 0;i < newMask->nrow;i++) {
	 newMask->rows[i] = &newMask->prvt->parent->rows[newMask->row0 + i][newMask->col0];
      }
   } else {
      THING *elem;
      
      shDebug(1,"We need to set mask %ld's pointers",
	      p_shMemRefCntrDel(p_shMemIdFind(TO_IPTR(newMask))));
      elem = shThingNew(newMask,shTypeGetFromName("MASK"));
      shListAdd(struct_list,elem,ADD_ANY);
   }

   return(SH_SUCCESS);
}

static int
writeMask_p(FILE *outfil, struct mask_p *prvt)
{
   if(shDumpIntWrite(outfil,&prvt->flags) == SH_GENERIC_ERROR) {
      shError("writeMask_p: can't write prvt->flags");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&prvt->crc) == SH_GENERIC_ERROR) {
      shError("writeMask_p: can't write prvt->crc");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&prvt->nchild) == SH_GENERIC_ERROR) {
      shError("writeMask_p: can't write prvt->nchild");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,prvt->parent,shTypeGetFromName("MASK"))
							 == SH_GENERIC_ERROR) {
      shError("writeMask_p: can't write prvt->parent");
      return(SH_GENERIC_ERROR);
   }
   return(SH_SUCCESS);
}

RET_CODE
shDumpMaskWrite(FILE *outfil, MASK *mask)
{
   if(p_shDumpTypecheckWrite(outfil,"MASK") != SH_SUCCESS) {
      shError("writeMask: writing MASK\n");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,mask,shTypeGetFromName("PTR")) ==SH_GENERIC_ERROR){
      shError("writeMask: writing mask\n");
      return(SH_GENERIC_ERROR);
   }
/*
 * Now write the MASK
 */
   if(mask == NULL) {
      return(SH_SUCCESS);
   }

   if(shDumpStrWrite(outfil,mask->name) == SH_GENERIC_ERROR) {
      shError("writeMask: can't write mask->name");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&mask->nrow) == SH_GENERIC_ERROR) {
      shError("writeMask: can't write mask->nrow");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&mask->ncol) == SH_GENERIC_ERROR) {
      shError("writeMask: can't write mask->ncol");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,mask->prvt->parent,shTypeGetFromName("PTR"))
							 == SH_GENERIC_ERROR) {
      shError("writeMask: can't write mask->parent");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&mask->row0) == SH_GENERIC_ERROR) {
      shError("writeMask: can't write mask->row0");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&mask->col0) == SH_GENERIC_ERROR) {
      shError("writeMask: can't write mask->col0");
      return(SH_GENERIC_ERROR);
   }
   if(writeMask_p(outfil,mask->prvt) == SH_GENERIC_ERROR) {
      shError("writeMask: can't write mask->prvt");
      return(SH_GENERIC_ERROR);
   }
/*
 * deal with image data
 *
 * Yes, we do write mask->prvt->parent a second time. This enables
 * us to tell if it has been resolved when reading the data back
 */
   if(shDumpPtrWrite(outfil,mask->prvt->parent,shTypeGetFromName("PTR"))
							 == SH_GENERIC_ERROR) {
      shError("writeMask: can't write mask->prvt->parent");
      return(SH_GENERIC_ERROR);
   }
   if(mask->prvt->parent == NULL) {
      int i;
      char *data;

      if((data = (char *)shMalloc(mask->ncol)) == NULL) {
	 shFatal("writeMask: can't allocate buffer for image data");
      }
      for(i = 0;i < mask->nrow;i++) {

/*
 * One can write the Mask directly (i.e. with translation to 'network byte
 * order' since the 'elements' of the mask array are 1 byte long only
 * (There is though to grow them to two bytes and that case a translation
 *  will probably be necessary)
 */

	 if(fwrite((void *)mask->rows[i],1,mask->ncol,outfil) != mask->ncol) {
	    shError("writeMask: can't write image data");
	    shFree((char *)data);
	    return(SH_GENERIC_ERROR);
	 }
      }
      shFree((char *)data);
   } else {
      ;				/* no need to write anything */
   }

   p_shDumpStructWrote(mask);
   return(SH_SUCCESS);
}

/*****************************************************************************/
RET_CODE
readHeader(FILE *outfil, HDR **header)
{
   int  lineCount, i;
   char tempHdrLine[HDRLINESIZE];

/*
 * Get a new header to fill in if we need it.  We may only need to get the
 * vector array.
 */
   if (*header == NULL) {
      *header = shHdrNew();
   } else {
      p_shHdrMallocForVec(*header);
   }
/*
 * Now read in the modification count
 */
   if(shDumpUintRead(outfil,&((*header)->modCnt)) == SH_GENERIC_ERROR) {
      shError("readHeader: reading header modification count\n");
      return(SH_GENERIC_ERROR);
   }
/*
 * Read in the number of header lines in the dump file.
 */
   if(shDumpIntRead(outfil, &lineCount) == SH_GENERIC_ERROR) {
      shError("readHeader: reading number of header lines in dump\n");
      return(SH_GENERIC_ERROR);
   }
/*
 * Read in each of the header lines as a simple string.  The space for each
 * line needs to be malloced here as well, so use the libfits routines.
 */
   for (i = 0; i < lineCount; ++i) {
      if(shDumpStrRead(outfil, tempHdrLine, HDRLINESIZE) == SH_GENERIC_ERROR) {
         shError("readHeader: reading header line number %d\n", i);
	 return(SH_GENERIC_ERROR);
      }
      f_hlins((*header)->hdrVec, MAXHDRLINE, i, tempHdrLine);
   }

   return(SH_SUCCESS);
}

/*
 * Read a header
 */
RET_CODE
shDumpHdrRead(FILE *outfil, HDR **header)
{
   char func[SIZE];			/* name of this function */
   int id;

   sprintf(func,"shDumpHdrRead");
   
   if(p_shDumpTypeCheck(outfil,"HDR") != 0) {
      shError("%s: checking type\n", func);
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&id) == SH_GENERIC_ERROR) {
      shError("%s: reading id\n",func);
      return(SH_GENERIC_ERROR);
   }
   if(id == 0) {
      *header = NULL;
      return(SH_SUCCESS);
   }
/*
 * Read in the header.  The pointer we are passing, points to nothing
 * currently, so set it to NULL.
 */
   *header = NULL;
   if (readHeader(outfil, header) != SH_SUCCESS) {
      shError("%s: reading header\n",func);
      return(SH_GENERIC_ERROR);
   }

   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Read a region; note that we have to do some readahead before we can
 * create the new region
 */
static int
readRegion_p(FILE *fil, struct region_p *prvt)
{
   if(shDumpIntRead(fil,(int *)&prvt->type) == SH_GENERIC_ERROR) {
      shError("readRegion_p: reading prvt->type");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(fil,(int *)&prvt->ModCntr) == SH_GENERIC_ERROR) {
      shError("readRegion_p: reading prvt->ModCntr");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(fil,&prvt->flags) == SH_GENERIC_ERROR) {
      shError("readRegion_p: reading prvt->flags");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(fil,&prvt->crc) == SH_GENERIC_ERROR) {
      shError("readRegion_p: reading prvt->crc");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(fil,&prvt->nchild) == SH_GENERIC_ERROR) {
      shError("readRegion_p: reading prvt->nchild");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrRead(fil,(void **)&prvt->parent) == SH_GENERIC_ERROR) {
      shError("readRegion_p: reading prvt->parent");
      return(SH_GENERIC_ERROR);
   }
/*
 * don't bother to dump the rest of the physical region stuff
 */

   return(SH_SUCCESS);
}
   
RET_CODE
shDumpRegRead(FILE *outfil, REGION **home)
{
   int data_size;
   char func[SIZE];			/* name of this function */
   int i,j;
   int id;
   char name[SIZE];
   PIXDATATYPE pixtype = 0;		/* keep officious compilers happy */
   REGION *newRegion;
   HDR  *hdrPtr;
   void **rows;
   const SCHEMA *sch;
   int xsize,ysize;

   sprintf(func,"shDumpRegRead");
/*
 * find out what we know about our type
 */
   if((sch = (SCHEMA *)shSchemaGet("REGION")) == NULL) {
      shError("Can't retrieve schema for REGION");
      return(SH_GENERIC_ERROR);
   }
   
   if(p_shDumpTypeCheck(outfil,"REGION") != 0) {
      shError("shDumpRegRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&id) == SH_GENERIC_ERROR) {
      shError("%s: reading id",func);
      return(SH_GENERIC_ERROR);
   }
   if(id == 0) {
      *home = NULL;
      return(SH_SUCCESS);
   }
/*
 * Now the first hand-coded part
 */
   if(shDumpStrRead(outfil,name,SIZE) == SH_GENERIC_ERROR) {
      shError("shDumpRegRead: reading newRegion->name");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&ysize) == SH_GENERIC_ERROR) {
      shError("shDumpRegRead: reading newRegion->nrow");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&xsize) == SH_GENERIC_ERROR) {
      shError("shDumpRegRead: reading newRegion->ncol");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,(int *)&pixtype) == SH_GENERIC_ERROR) {
      shError("shDumpRegRead: reading newRegion->type");
      return(SH_GENERIC_ERROR);
   }
   *home = newRegion = shRegNew(name,ysize,xsize,pixtype);
   p_shPtrIdSave(id,newRegion);
/*
 * Ready to read elements of structure. First check that the ones that
 * we just read were present in the schema
 */
   i = -1;
   if(strcmp(sch->elems[++i].name,"name") != 0 ||
      strcmp(sch->elems[++i].name,"nrow") != 0 ||
      strcmp(sch->elems[++i].name,"ncol") != 0 ||
      strcmp(sch->elems[++i].name,"type") != 0) {
      shError("%s: structure mismatch at REGION.%s",func,sch->elems[i].name);
      return(SH_GENERIC_ERROR);
   }

/*
 * Now read the header
 */
   hdrPtr = &(newRegion->hdr);
   if(readHeader(outfil, &hdrPtr) == SH_GENERIC_ERROR) {
      shError("readRegion: can't read region->hdr");
      return(SH_GENERIC_ERROR);
   }

   for(i++;i < sch->nelem;i++) {
      shDebug(2,"%s: reading %s",func,sch->elems[i].name);
      if(strncmp(sch->elems[i].name,"rows",4) == 0) {
	 shDebug(2,"%s: not reading %s",func,sch->elems[i].name);
      } else if(strcmp(sch->elems[i].name,"hdr") == 0) {
	 shDebug(2,"%s: not reading %s",func,sch->elems[i].name);
      } else if(strcmp(sch->elems[i].type,"struct region_p") == 0) {
	 if(readRegion_p(outfil,newRegion->prvt) == SH_GENERIC_ERROR) {
	    shError("%s: reading newRegion->prvt",func);
	    return(SH_GENERIC_ERROR);
	 }
      } else {
	 if(shDumpSchemaElemRead(outfil,"",newRegion,&sch->elems[i])
                                                      != SH_SUCCESS) {
	    shError("%s: reading %s",func,sch->elems[i].name);
	 }
      }
   }
/*
 * That's the end of the generic part. Now deal with the image data.
 * Start by reading region->prvt->parent
 */
   if(shDumpIntRead(outfil,&id) == SH_GENERIC_ERROR) {
      shError("shDumpRegRead: rereading id");
      return(SH_GENERIC_ERROR);
   }
   if(id == 0) {			/* no parent */
      if(p_shDumpReadShallow) {		/* skip over image data */
	 (void)p_shRegPtrGet(newRegion,&data_size);
	 if(data_size == 0) {
	    shError("shDumpRegRead: Unknown PIXDATATYPE: %d",
                    (int)newRegion->type);
	    shRegDel(newRegion);
	    return(SH_GENERIC_ERROR);
	 }
	 if(fseek(outfil,data_size*newRegion->nrow*newRegion->ncol,1) != 0) {
	    shError("shDumpRegRead: can't seek over image data");
	    return(SH_GENERIC_ERROR);
	 }
      } else {				/* really read it */
	 if((rows = p_shRegPtrGet(newRegion,&data_size)) == NULL) {
	    shError("shDumpRegRead: Unknown PIXDATATYPE: %d",
                    (int)newRegion->type);
	    shRegDel(newRegion);
	    return(SH_GENERIC_ERROR);
	 }
	 
	 for(i = 0;i < newRegion->nrow;i++) {
	    for(j = 0;j < newRegion->ncol;j++) {
/*
 * We dont know the actual type of the elements of the rows
 * but we know their size expressed in bytes (data_size)
 * So to get the element j of the rows i 
 * ((char*)rows[i])+j * datasize
 * gives us the element "rows[i][j]"
 * ('cos a char is one byte long)
 */
	      if(fread(((char*)rows[i])+j*data_size,data_size,1,outfil)
                                            < 1) {
	         shError("shDumpRegRead: can't read image data");
	         return(SH_GENERIC_ERROR);
	      }
	      sh_ntoh(((char*)rows[i])+j*data_size,data_size);
	    };
	 }
      }
   } else if(shTreeValFind(id_root,id) >= 0) {
      void **prows;			/* rows array for parent */
      REGION *parent = newRegion->prvt->parent;
      
      shDebug(1,"Setting image pointers in subregion");
      shAssert(newRegion->type == parent->type);

      if((rows = p_shRegPtrGet(newRegion,&data_size)) == NULL) {
	 shError("shDumpRegRead: Unknown PIXDATATYPE: %d",
		 (int)newRegion->type);
	 shRegDel(newRegion);
	 return(SH_GENERIC_ERROR);
      }
      prows = p_shRegPtrGet(parent,&data_size); /* this can't fail if the
						   previous call succeeded */
      for(i = 0;i < newRegion->nrow;i++) {
	 rows[i] = (char *)prows[newRegion->row0 + i]
                           + data_size*newRegion->col0;
      }
   } else {
      THING *elem;
      
      shDebug(1,
	   "We need to set region %ld's pointers",
           p_shMemRefCntrDel(p_shMemIdFind(TO_IPTR(newRegion))));
      elem = shThingNew(newRegion,shTypeGetFromName("REGION"));
      shListAdd(struct_list,elem,ADD_ANY);
   }

   return(SH_SUCCESS);
}

RET_CODE
writeHeader(FILE *outfil, HDR *header)
{
   int lineCount, i=0;

   if(header == NULL) {
      return(SH_SUCCESS);
   }
/*
 * Now write out the modification count
 */
   if(shDumpUintWrite(outfil, &header->modCnt) != SH_SUCCESS) {
      shError("writeHeader: could not write the modification count\n");
      return(SH_GENERIC_ERROR);
   }
/*
 * Figure out how many lines we have in the header to write
 */
   if(shHdrGetLineTotal(header, &lineCount) != SH_SUCCESS) {
      shError("writeHeader: could not get number of lines in header\n");
      return(SH_GENERIC_ERROR);
   }
/*
 * Save this in the dump file.
 */
   if(shDumpIntWrite(outfil, &lineCount) != SH_SUCCESS) {
      shError("writeHeader: could not write line count %d\n", i);
      return(SH_GENERIC_ERROR);
   }

/* 
 * Now write out each line as a simple string
 */
   for (i = 0;i < lineCount;++i) {
      if(shDumpStrWrite(outfil, header->hdrVec[i]) != SH_SUCCESS) {
         shError("writeHeader: could not write line number %d\n", i);
	 return(SH_GENERIC_ERROR);
      }
   }

   return(SH_SUCCESS);
}

RET_CODE
shDumpHdrWrite(FILE *outfil, HDR *header)
{
   if(p_shDumpTypecheckWrite(outfil,"HDR") != SH_SUCCESS) {
      shError("writeHeader: writing HDR\n");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,header,shTypeGetFromName("PTR"))
							 == SH_GENERIC_ERROR) {
      shError("writeHeader: writing header\n");
      return(SH_GENERIC_ERROR);
   }
/*
 * Now write the Header
 */
   if (writeHeader(outfil, header) != SH_SUCCESS) {
      shError("writeHeader: writing header\n");
      return(SH_GENERIC_ERROR);
   }

p_shDumpStructWrote(header);
return(SH_SUCCESS);
}

static int
writeRegion_p(FILE *outfil, struct region_p *prvt)
{
   if(shDumpIntWrite(outfil,(int *)&prvt->type) == SH_GENERIC_ERROR) {
      shError("writeRegion_p: writing prvt->type");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,(int *)&prvt->ModCntr) == SH_GENERIC_ERROR) {
      shError("writeRegion_p: writing prvt->ModCntr");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&prvt->flags) == SH_GENERIC_ERROR) {
      shError("writeRegion_p: can't write prvt->flags");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&prvt->crc) == SH_GENERIC_ERROR) {
      shError("writeRegion_p: can't write prvt->crc");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&prvt->nchild) == SH_GENERIC_ERROR) {
      shError("writeRegion_p: can't write prvt->nchild");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,(void *)prvt->parent,shTypeGetFromName("REGION"))
      							 == SH_GENERIC_ERROR) {
      shError("writeRegion_p: writing prvt->parent");
      return(SH_GENERIC_ERROR);
   }
/*
 * we don't bother to dump the rest of the physical region
 */
   return(SH_SUCCESS);
}

RET_CODE
shDumpRegWrite(FILE *outfil, REGION *region)
{
   if(region->prvt->type != SHREGVIRTUAL) {
      shError("shDumpRegWrite: Attempt to dump a non-virtual REGION");
      return(SH_GENERIC_ERROR);
   }

   if(p_shDumpTypecheckWrite(outfil,"REGION") != SH_SUCCESS) {
      shError("writeRegion: writing REGION\n");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,region,shTypeGetFromName("PTR"))
							 == SH_GENERIC_ERROR) {
      shError("writeRegion: writing region\n");
      return(SH_GENERIC_ERROR);
   }
/*
 * Now write the REGION
 */
   if(region == NULL) {
      return(SH_SUCCESS);
   }

   if(shDumpStrWrite(outfil,region->name) == SH_GENERIC_ERROR) {
      shError("writeRegion: can't write region->name");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&region->nrow) == SH_GENERIC_ERROR) {
      shError("writeRegion: can't write region->nrow");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&region->ncol) == SH_GENERIC_ERROR) {
      shError("writeRegion: can't write region->ncol");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,(int *)&region->type) == SH_GENERIC_ERROR) {
      shError("writeRegion: writing newRegion->type");
      return(SH_GENERIC_ERROR);
   }
   if(writeHeader(outfil,&(region->hdr)) == SH_GENERIC_ERROR) {
      shError("writeRegion: can't write region->hdr");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,region->mask,shTypeGetFromName("MASK"))
							 == SH_GENERIC_ERROR) {
      shError("writeRegion: can't write region->mask");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&region->row0) == SH_GENERIC_ERROR) {
      shError("writeRegion: can't write region->row0");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntWrite(outfil,&region->col0) == SH_GENERIC_ERROR) {
      shError("writeRegion: can't write region->col0");
      return(SH_GENERIC_ERROR);
   }
   if(writeRegion_p(outfil,region->prvt) == SH_GENERIC_ERROR) {
      shError("writeRegion: can't write region->prvt");
      return(SH_GENERIC_ERROR);
   }
/*
 * deal with image data
 *
 * Yes, we do write region->prvt->parent. This enables us to tell if the
 * data really belongs to this REGION, and hence was written
 */
   if(shDumpPtrWrite(outfil,region->prvt->parent,shTypeGetFromName("PTR"))
							 == SH_GENERIC_ERROR) {
      shError("writeRegion: can't write region->prvt->parent");
      return(SH_GENERIC_ERROR);
   }
   if(region->prvt->parent == NULL) {
      void *data;
      int data_size;
      int i,j;
      void **rows;
      
      if((rows = p_shRegPtrGet(region,&data_size)) == NULL) {
	 shError("shDumpRegWrite: Unknown PIXDATATYPE: %d",
		 (int)region->type);
	 shRegDel(region);
	 return(SH_GENERIC_ERROR);
      }
      if((data = shMalloc(region->ncol*data_size)) == NULL) {
	 shFatal("shDumpRegWrite: can't allocate buffer for image data");
      }
      for(i = 0;i < region->nrow;i++) {
	 memcpy(data,rows[i],region->ncol*data_size);
	 for(j = 0;j < region->ncol;j++) {
	   sh_hton( (void*)( ((char*)data)+j*data_size), data_size);

/*
 * We dont know the actual type of the elements of the rows
 * but we know their size expressed in bytes (data_size)
 * So to get the element j of the rows i 
 * ((char*)rows[i])+j * datasize
 * gives us the element "rows[i][j]"
 * ('cos a char is one byte long)
 */
	   if(fwrite(((char*)data)+j*data_size,data_size,1,outfil) != 1) {
	      shError("shDumpRegWrite: can't write image data");
	      shFree((char *)data);
	      return(SH_GENERIC_ERROR);
	   }
	 }
      }
      shFree(data);
   } else {
      ;				/* no need to write anything */
   }

   p_shDumpStructWrote(region);
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Save the mapping of an id to an address
 */
void
p_shPtrIdSave(int id, void *addr)
{
   shDebug(1,"New id: %d --> "PTR_FMT,id,addr);
   if(addr != NULL) {
      id_root = shTreeKeyInsert(id_root,id,TO_IPTR(addr));
   }
}

/*****************************************************************************/
/*
 * Deal with pointers and pointer IDs
 *
 * Label a structure obj as safely written to disk
 */
void
p_shDumpStructWrote(void *obj)
{
   int id = p_shMemRefCntrDel(p_shMemIdFind(TO_IPTR(obj)));
   addr_root = shTreeKeyInsert(addr_root,TO_IPTR(obj),id);
}

/*****************************************************************************/
/*
 * Free any i/o data structures that we built. If all went well, they'll
 * be empty. If they are not all empty, only free them if force is true
 */
void
p_shDumpStructsReset(int force_free)
{
   if((struct_list != NULL && struct_list->first != NULL) ||
	 addr_root != NULL || id_root != NULL || ptr_root != NULL ||
							unknown_root != NULL) {
      if(!force_free) {
	 return;
      }
   }

   shListDel(struct_list,(RET_CODE(*)(LIST_ELEM *ptr))shThingDel);
   addr_root = shTreeDel(addr_root);
   id_root = shTreeDel(id_root);
   ptr_root = shTreeDel(ptr_root);
   unknown_root = shTreeDel(unknown_root);
   
   addr_root = shTreeKeyInsert(addr_root,TO_IPTR(NULL),0);
   struct_list = shListNew(shTypeGetFromName("THING"));
}

/*****************************************************************************/
/*
 * Write the regions that were scheduled to be written later 
 */
static void p_sh_write_scheduled_ptrs(TNODE *root, FILE *fil);
static int n_anon;

static int
write_scheduled_ptrs(FILE *fil)
{
   int v;				/* verbosity level for shDebug */
   TNODE *old_ptr_root;			/* ptr_root while doing scheduled
					   writes */
   
   if(ptr_root != NULL) {
      shDebug(1,"Performing scheduled writes");
   }
   
   v = 1;
   do {
      n_anon = 0;
      old_ptr_root = ptr_root;
      ptr_root = NULL;
      p_sh_write_scheduled_ptrs(old_ptr_root,fil);
      old_ptr_root = rebuild(old_ptr_root);

      if(n_anon > 0) {
	 shDebug(v,"write_scheduled_ptrs: writing anonymous objects to file");
      }
      v = 2;
   } while(n_anon > 0);
   
   if(old_ptr_root != NULL) {
      shError("write_scheduled_ptrs: Can't write all scheduled objects");
      return(SH_GENERIC_ERROR);
   }
   return(SH_SUCCESS);
}

static void
p_sh_write_scheduled_ptrs(TNODE *node,FILE *fil)
{
   THING thing;
   if(node == NULL) return;

   p_sh_write_scheduled_ptrs(node->left,fil);
   p_sh_write_scheduled_ptrs(node->right,fil);
   if(shTreeValFind(addr_root,TO_IPTR(node->key)) >= 0) { /* already written */
      shDebug(1,"data-object %ld is already written",
	      p_shMemRefCntrDel(p_shMemIdFind(TO_IPTR(node->key))));
      node->val = 0;
      return;
   }

   thing.ptr = (void *)node->key;
   thing.type = (TYPE)node->val;
   if(shDumpNextWrite(fil,&thing) == SH_SUCCESS) {
      shDebug(1,"Wrote %s %ld",shNameGetFromType((TYPE)node->val),
	      p_shMemRefCntrDel(p_shMemIdFind(TO_IPTR(node->key))));
      n_anon++;
      node->val = 0;
   } else {
      shError("write_scheduled_ptrs: Can't write object %ld",
	      p_shMemRefCntrDel(p_shMemIdFind(TO_IPTR(node->key))));
   }
}

/*****************************************************************************/
/*
 * traverse the unknown tree, trying to resolve unknown ids, replacing
 * them by real addresses. Then, if all pointers are resolved, go through
 * the list of REGIONs and MASKs dealing with submasks/subregions
 */
RET_CODE
shDumpPtrsResolve(void)
{
   int i;
   MASK *mask;
   REGION *region;
   THING *ptr, *next = NULL;		/* initialise for officious compilers*/
   char *type_name;
/*
 * First pointer ids that haven't been converted to real pointers yet
 */
   resolve_ids(unknown_root);
   if((unknown_root = rebuild(unknown_root)) != NULL) {
      shError("shDumpPtrsResolve: Can't resolve all pointer ids");
      return(SH_GENERIC_ERROR);
   }
/*
 * Now set sub[mask|region] pointers
 */
   for(ptr = (THING *)struct_list->first;ptr != NULL;ptr = next) {
      type_name = shNameGetFromType(ptr->type);
      if(strcmp(type_name,"MASK") == 0) {
	 mask = (MASK *)ptr->ptr;
	 shDebug(1,"Setting image pointers for mask %ld",
		 p_shMemRefCntrDel(p_shMemIdFind(TO_IPTR(mask))));
	 for(i = 0;i < mask->nrow;i++) {
	    mask->rows[i] =
	      &mask->prvt->parent->rows[mask->row0 + i][mask->col0];
	 }
      } else if(strcmp(type_name,"REGION") == 0) {
	 int data_size;			/* sizeof one pixel */
	 int pdata_size;		/* ditto for the parent */
	 void **rows;			/* pointer to row pointer */
	 void **prows;			/* ditto for the parent */
	 
	 region = (REGION *)ptr->ptr;
	 shDebug(1,"Setting image pointers for region %ld",
		 p_shMemRefCntrDel(p_shMemIdFind(TO_IPTR(region))));
/*
 * this is complicated by the need to support lots of types of pixel data
 */
	 shAssert(region->type == region->prvt->parent->type);
	 if((rows = p_shRegPtrGet(region,&data_size)) == NULL) {
	    shError("shDumpPtrsResolve: Unknown PIXDATATYPE: %d",
		    					    (int)region->type);
	    shRegDel(region);
	    return(SH_GENERIC_ERROR);
	 }
	 prows = p_shRegPtrGet(region->prvt->parent,&pdata_size);
	 shAssert(prows != NULL);	/* the types are known to be the same*/

	 for(i = 0;i < region->nrow;i++) {
	    rows[i] = (char *)prows[region->row0 + i] + data_size*region->col0;
	 }
      } else {
	 shFatal("shDumpPtrsResolve: list contains an object of type %s",
		    type_name);
      }

      next = ptr->next;
      shThingDel((THING *)shListRem(struct_list,ptr));
   }
   return(struct_list->first == NULL ? SH_SUCCESS : SH_GENERIC_ERROR);
}

static void
resolve_ids(TNODE *node)
{
   IPTR addr;

   if(node == NULL) return;

   resolve_ids(node->left);
   resolve_ids(node->right);
   if(node->val == 0) {			/* already resolved */
      ;
   } else if((addr = shTreeValFind(id_root,node->key)) >= 0) {
      shDebug(1,"Resolving id %ld --> "PTR_FMT,node->key,(void *)addr);
      *(void **)node->val = (void *)addr;
      node->val = 0;
   } else {
      shError("resolve_ids: Can't resolve id %ld",node->key);
   }
}

/*****************************************************************************/
/*
 * Rebuild a tree, deleting nodes with value 0 (i.e. that have been processed)
 */
static TNODE *i_rebuild(TNODE *node, TNODE *new_root);

static TNODE *
rebuild(TNODE *root)
{
   return(i_rebuild(root,NULL));
}

static TNODE *
i_rebuild(TNODE *node, TNODE *new_root)
{
   if(node == NULL) return(new_root);

   new_root = i_rebuild(node->left,new_root);
   new_root = i_rebuild(node->right,new_root);
   if(node->val != 0) {
      new_root = shTreeMkeyInsert(new_root,node->key,node->val);
   }
   free((char *)node);
   return(new_root);
}


   /* 
    * the following functions tell the "dump" code how to read/write a
    * CHAIN from/to a dumpfile.  
    */

RET_CODE
shDumpChainRead(FILE *outfil, CHAIN **chain)
{
   int id;
   int i;
   CHAIN *c;
   CHAIN_ELEM *c_e;
   TYPE type;

   if (p_shDumpTypeCheck(outfil,"CHAIN") != 0) {
      shError("shDumpChainRead: Checking type\n");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&id) == SH_GENERIC_ERROR) {
       shError("shDumpChainRead: reading id");
      return(SH_GENERIC_ERROR);
   }
   if (id == 0) {
      *chain = NULL;
      return(SH_SUCCESS);
   }
/*
 * now see what type of chain this is
 */
   if (shDumpTypeRead(outfil,&type) == SH_GENERIC_ERROR) {
      shError("shDumpChainRead: reading type");
      return(SH_GENERIC_ERROR);
   }
   *chain = c = shChainNew(shNameGetFromType(type));
   p_shPtrIdSave(id,c);

   if (shDumpUintRead(outfil, &(c->nElements)) == SH_GENERIC_ERROR) {
      shError("shDumpChainRead: reading chain->nElements");
      return(SH_GENERIC_ERROR);
   }
   if (shDumpPtrRead(outfil,(void **)&(c->pFirst)) == SH_GENERIC_ERROR) {
      shError("shDumpChainRead: reading chain->pFirst");
      return(SH_GENERIC_ERROR);
   }
   if (shDumpPtrRead(outfil,(void **)&(c->pLast)) == SH_GENERIC_ERROR) {
      shError("shDumpChainRead: reading chain->pLast");
      return(SH_GENERIC_ERROR);
   }

   /* don't bother to read/write cursor information ... */

   /* 
    * now, we read each chain element in the chain.  The elements will
    * all be linked in order when pointers are resolved, after all
    * data has been read from the dump file.
    */
   for (i = 0; i < c->nElements; i++) {
      if (shDumpChain_elemRead(outfil, &c_e) != SH_SUCCESS) {
	 shError("shDumpChainRead: reading chain element");
	 return(SH_GENERIC_ERROR);
      }
   }

   return(SH_SUCCESS);
}

   /*
    * write a CHAIN into a dump file 
    */

RET_CODE
shDumpChainWrite(FILE *outfil, CHAIN *chain)
{
   CHAIN_ELEM *ptr;
   TYPE t = shTypeGetFromName("CHAIN");
 
   if (shDumpTypeWrite(outfil,&t) == SH_GENERIC_ERROR) {
      shError("shDumpChainWrite: writing CHAIN");
      return(SH_GENERIC_ERROR);
   }
   if (shDumpPtrWrite(outfil,chain,shTypeGetFromName("PTR")) ==
                                                    SH_GENERIC_ERROR){
      shError("shDumpChainWrite: writing chain");
      return(SH_GENERIC_ERROR);
   }
   if (chain == NULL) {
      return(SH_SUCCESS);
   }
   if (shDumpTypeWrite(outfil,&(chain->type)) == SH_GENERIC_ERROR) {
      shError("shDumpChainWrite: writing chain->type");
      return(SH_GENERIC_ERROR);
   }
   if (shDumpUintWrite(outfil, &(chain->nElements)) == SH_GENERIC_ERROR) {
      shError("shDumpChainWrite: writing chain->nElements");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,chain->pFirst,shTypeGetFromName("PTR"))
                                                         == SH_GENERIC_ERROR) {
      shError("shDumpChainWrite: writing chain->pFirst");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,chain->pLast,shTypeGetFromName("PTR"))
                                                         == SH_GENERIC_ERROR) {
      shError("shDumpChainWrite: writing chain->pLast");
      return(SH_GENERIC_ERROR);
   }

   /* we do not write the pCacheElem value, or the cacheElemPos value */

   /*
    * now go through the list, writing out a pointer to each chain element,
    * AND a pointer to each structure that the chain element points to.
    */
   if (chain->pFirst != NULL) {
      for (ptr = chain->pFirst; ptr != NULL; ptr = ptr->pNext) {
	 if (shDumpChain_elemWrite(outfil, ptr) != SH_SUCCESS) {
	    shError("shDumpChainWrite: writing chain element");
	    return(SH_GENERIC_ERROR);
	 }
      }
   }

   return(SH_SUCCESS);
}

   /* 
    * the following functions tell the "dump" code how to read/write a
    * CHAIN_ELEM from/to a dumpfile.  
    */

RET_CODE
shDumpChain_elemRead(FILE *outfil, CHAIN_ELEM **chain_elem)
{
   int id;
   CHAIN_ELEM *c_e;
   THING *thing;
   TYPE type;

   if (p_shDumpTypeCheck(outfil,"CHAIN_ELEM") != 0) {
      shError("shDumpChain_elemRead: Checking type\n");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpIntRead(outfil,&id) == SH_GENERIC_ERROR) {
       shError("shDumpChain_elemRead: reading id");
      return(SH_GENERIC_ERROR);
   }
   if (id == 0) {
      *chain_elem = NULL;
      return(SH_SUCCESS);
   }

   if (shDumpTypeRead(outfil,&type) == SH_GENERIC_ERROR) {
      shError("shDumpChain_elemRead: reading chain_elem->type");
      return(SH_GENERIC_ERROR);
   }
   *chain_elem = c_e = shChain_elemNew(shNameGetFromType(type));
   p_shPtrIdSave(id,c_e);

   /* 
    * now, we read the _ID number_ for the object to which this chain
    * element points, and create a new THING to hold the ID number;
    * we place the new THING into a list called "anon_list".  
    * We will resolve all
    * the ID numbers into pointers after we've read in all data from the
    * dump file... 
    */
   if (shDumpPtrRead(outfil,(void **)&(c_e->pElement)) == SH_GENERIC_ERROR) {
      shError("shDumpChain_elemRead: reading chain_elem->pElement");
      return(SH_GENERIC_ERROR);
   }
   if (c_e->pElement != NULL) {
      if (anon_list == NULL) {
	 anon_list = shListNew(shTypeGetFromName("THING"));
      }
      thing = shThingNew(NULL, type);
      thing->ptr = (void *) (c_e->pElement);
      shListAdd(anon_list,thing,ADD_BACK);
   }

   if (shDumpPtrRead(outfil,(void **)&(c_e->pPrev)) == SH_GENERIC_ERROR) {
      shError("shDumpChain_elemRead: reading chain_elem->pPrev");
      return(SH_GENERIC_ERROR);
   }
   if (shDumpPtrRead(outfil,(void **)&(c_e->pNext)) == SH_GENERIC_ERROR) {
      shError("shDumpChain_elemRead: reading chain_elem->pNext");
      return(SH_GENERIC_ERROR);
   }


   return(SH_SUCCESS);
} 


RET_CODE
shDumpChain_elemWrite(FILE *outfil, CHAIN_ELEM *chain_elem)
{
   TYPE t = shTypeGetFromName("CHAIN_ELEM");
 
   if (shDumpTypeWrite(outfil,&t) == SH_GENERIC_ERROR) {
      shError("shDumpChain_elemWrite: writing CHAIN_ELEM");
      return(SH_GENERIC_ERROR);
   }
   if (shDumpPtrWrite(outfil, chain_elem, shTypeGetFromName("PTR")) ==
                                                    SH_GENERIC_ERROR){
      shError("shDumpChain_elemWrite: writing chain_elem");
      return(SH_GENERIC_ERROR);
   }
   if (chain_elem == NULL) {
      return(SH_SUCCESS);
   }

   if (shDumpTypeWrite(outfil,&(chain_elem->type)) == SH_GENERIC_ERROR) {
      shError("shDumpChain_elemWrite: writing chain_elem->type");
      return(SH_GENERIC_ERROR);
   }
   if (shDumpPtrWrite(outfil, chain_elem->pElement,
				       chain_elem->type) == SH_GENERIC_ERROR) {
      shError("shDumpChain_elemWrite: writing chain_elem->pElement");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,(chain_elem)->pPrev,shTypeGetFromName("PTR"))
                                                         == SH_GENERIC_ERROR) {
      shError("shDumpChain_elemWrite: writing chain_elem->pPrev");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(outfil,(chain_elem)->pNext,shTypeGetFromName("PTR"))
                                                         == SH_GENERIC_ERROR) {
      shError("shDumpChain_elemWrite: writing chain_elem->pNext");
      return(SH_GENERIC_ERROR);
   }

   return(SH_SUCCESS);
}

/*
 * <INTRO> 
 * This file implements FITS binary tables for photo types that cannot afford
 * the overhead of going through the dervish sequence of conversion to a TBLCOL,
 * and then being written to disk.
 *
 * The way that it works is that you first declare a SCHEMATRANS for each
 * structure you are going to write; in most cases only the src and dest
 * field names matter (see examples below; note that malloced arrays must
 * be declared as heap. REGIONs are special; the proper type of rows pointer
 * will be written when you include rows in a SCHEMATRANS).
 *
 * If you have an array in your struct (e.g. REGION **arr), declare it as
 * "-dimen $size".  You can also use -dimen to write out only the start
 * of arrays declared as, e.g., REGION *arr[5].
 *
 * You then open the FITS file, and open a table for the desired type. Once
 * that's done, you can read or write the table row by row.
 *
 * If you are reading from a table, the object that you are reading MUST
 * preexist (you'll get warnings if they don't). REGIONs and MASKs are
 * treated specially, and must be passed in with a NULL rows pointer (most
 * easily achieved by creating them with nrow == ncol == 0). Structs such
 * as OBJCs that have internal arrays must have those arrays properly
 * initialised (see below for how to do this).
 *
 * The basic procedure to write a file of OBJECT1s and then one of REGIONs
 * is as follows; I've removed error recovery. First declare the parts of
 * the schema that you want written:
 *
 *   obj1_trans = shSchemaTransNew();
 *   shSchemaTransEntryAdd(obj1_trans,CONVERSION_BY_TYPE,"FILTER","filter",
 *                       "",NULL,NULL,NULL,NULL,0.0,-1);
 *   shSchemaTransEntryAdd(obj1_trans,CONVERSION_BY_TYPE,"NPIX","npix",
 *                       "",NULL,NULL,NULL,NULL,0.0,-1);
 *   shSchemaTransEntryAdd(obj1_trans,CONVERSION_BY_TYPE,"REGION","region",
 *                       "",NULL,NULL,NULL,NULL,0.0,-1);
 *   phFitsBinDeclareSchemaTrans(obj1_trans,"OBJECT1");
 * 
 *   region_trans = shSchemaTransNew();
 *   shSchemaTransEntryAdd(region_trans,CONVERSION_BY_TYPE,"RNAME","name",
 *                       "heap","char","-1",NULL,NULL,0.0,-1);
 *   shSchemaTransEntryAdd(region_trans,CONVERSION_BY_TYPE,"RNROW","nrow",
 *                       "",NULL,NULL,NULL,NULL,0.0,-1);
 *   shSchemaTransEntryAdd(region_trans,CONVERSION_BY_TYPE,"RNCOL","ncol",
 *                       "",NULL,NULL,NULL,NULL,0.0,-1);
 *   shSchemaTransEntryAdd(region_trans,CONVERSION_BY_TYPE,"RROWS","rows",
 *                       "heap","char","-1",NULL,NULL,0.0,-1);
 *   phFitsBinDeclareSchemaTrans(region_trans,"REGION");
 *
 * Then actually write the file:
 *
 *   fd = phFitsBinTblOpen(file,1,NULL);
 *
 *   phFitsBinTblHdrWrite(fd,"OBJECT1");
 *   phFitsBinTblRowWrite(fd,obj1);
 *   phFitsBinTblRowWrite(fd,obj2);
 *   phFitsBinTblEnd(fd);
 *
 *   phFitsBinTblHdrWrite(fd,"REGION");
 *   phFitsBinTblRowWrite(fd,reg1);
 *   phFitsBinTblRowWrite(fd,reg2);
 *   phFitsBinTblEnd(fd);
 *
 *   phFitsBinTblClose(fd);
 *
 * Let us pretend that you only want to read back the REGIONs, skipping the
 * first table
 *
 *   fd = phFitsBinTblOpen(file,0,NULL);
 *
 *   phFitsBinTblHdrRead(fd, NULL, NULL, NULL);
 *   phFitsBinTblEnd(fd);

 *   phFitsBinTblHdrRead(fd, REGION, NULL, &nrow);
 *
 *   reg1 = shRegNew("",0,0,TYPE_PIX);
 *   phFitsBinTblRowRead(fd,reg1);
 *   reg2 = shRegNew("",0,0,TYPE_PIX);
 *   phFitsBinTblRowRead(fd,reg2);
 *   phFitsBinTblEnd(fd);
 *
 *   phFitsBinTblClose(fd);
 *
 * (The type TYPE_PIX is irrelevant; the true type's read from the file)
 *
 * How about a more complex type, such as an OBJC? Writing it is easy, just
 * proceed as above, specifying a -dimen to the SchemaTrans (e.g.
 * fitsBinSchemaTransEntryAdd $trans color color -dimen $ncolor), but
 * reading it back requires us to know the value of ncolor. This can be
 * done as follows:
 *
 *   objc = phObjcNew(10);
 * 
 *   phFitsBinTblRowRead(fd,objc);
 *   shErrStackClear();
 *   phFitsBinTblRowUnread(fd);
 *   ncolor = objc->ncolor; phObjcDel(objc);
 * 
 * and then you are away. You can read the first object as:
 *
 *   objc = phObjcNew(ncolor);
 *   for(j = 0;j < ncolor;j++) {
 *      objc->color[j] = phObject1New();
 *      objc->color[j]->region = shRegNew("",0,0,TYPE_PIX);
 *   }
 *  
 *   phFitsBinTblRowRead(fd,objc);
 *
 *
 * The code is (almost) re-entrant; this means that you can be writing two
 * or more files simultaneously. The only restriction is that a given type
 * can have only one SCHEMATRANS defined at a time (in fact, once you've
 * written the table header you are free to change the SCHEMATRANS -- but
 * this is getting a little tricky)
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "dervish.h"
#include "phPhotoFitsIO.h"
#include "phSpanUtil.h"
#include "phUtils.h"
#include "phObjc.h"
#include "phDataIo.h"
#include "prvt/region_p.h"		/* for p_shRegPtrGet() */

#if defined(SDSS_LITTLE_ENDIAN)
#   define DEC_BYTE_ORDER
#endif

#define BYTE1 1
#define BYTE2 2
#define BYTE4 4
#define BYTE8 8

/*
 * Buffered file i/o for heap
 */
static void hbclose(int fd);
static void hbflush(int fd);
static int hblseek(int fd, off_t n, int w);
static int hbread(int fd, void *data, int n, int byte_data);
static int hbwrite(int fd, void *data, int n, int byte_data);


#define FITSIZE 2880			/* size of FITS record
					   Note that we need FITSIZE+1
					   in write_card() */
#define NTYPE 20			/* maximum number of types that can be
					   declared to phFitsBinTblHdrWrite */

/*****************************************************************************/
/*
 * We are going to build a simple machine with two stacks to actually
 * write the file. It supports a limited number of operations on an address
 * P and if it's in a state S, the operators are:
 *	FITSB_STOP			We are finished
 *	FITSB_NOP			Don't do anything
 *	FITSB_WRITE			write(,P,S.i); P += S.i; S++;
 *	FITSB_SEEK			seek(,P,S.i); P += S.i; S++;
 *	FITSB_HEAP			n = ipop();
 *					write_heap(,P,(j=ipop()),n);
 *					write(,j,4); write(,ipop(),4);
 *					P += S.i; S++;
 *					Write: nbyte = ?, heap_offset = ?
 *	FITSB_HEAP_MAIN			write(,ipop(),4); write(,ipop(),4);
 *					S++;
 *					Write: nbyte = ?, heap_offset = ?
 *	FITSB_HEAP_SEEK			read(&heap_nelem,n,4);
 *					read(&heap_offset,n,4);
 *					heap_seek(heap_offset,n,0); S++;
 *	FITSB_INCR			P += S.i; S++;
 *	FITSB_DEREF			P = *P; S++;
 *	FITSB_SUB			push(S + 1); S = S.i;
 *	FITSB_RET			S = pop();
 *      FITSB_LOOP			push(S + 1); S++;
 *      FITSB_ENDLOOP			if(bottom-of-istack-- > 0) {
 *					   S = bottom-of-stack;
 *					} else {
 *					   pop(); ipop(); S++;
 *					}
 *	FITSB_SAVE			push(P); S++;
 *	FITSB_RESTORE			P = pop(); S++;
 * Manipulate the integer stack
 *      FITSB_ICONST                    ipush(S.i); S++;
 *      FITSB_HEAP_SIZE                 ipush(heapsize); S++;
 * Used for byte swapping:
 *	FITSB_SWAB			(swap S.i bytes ABCD --> BADC); S++;
 *	FITSB_SWAB4			(swap S.i bytes ABCD --> DCBA); S++;
 *	FITSB_SWAB8			(swap S.i bytes ABCDEFGH --> HGFEDCBA);
 *                                                                         S++;
 * Special ops for special types
 *      FITSB_ATLAS_IMAGE		Write an ATLAS_IMAGE to the heap
 *      FITSB_MASK_ROWS			Write a MASK->rows to the heap
 *      FITSB_OBJMASK_SPAN		Write a OBJMASK->s to the heap
 *      FITSB_REGION_ROWS		Write a REGION->ROWS to the heap
 *      FITSB_STRING                    Write a string to the heap
 *
 * Note that there are two stacks. One is used to store both the data
 * pointer (P) and the "program counter" (S), and is implemented as an
 * array of (void *)s. The other is used to store integers, primarily
 * for heap calculations (although it is also used in constructing unique
 * TTYPEs).
 *
 * Note that FITSB_HEAP writes data to the heap and to the main table,
 * whereas FITSB_HEAP_MAIN only writes to the main table. Both expect that
 * the istack will contain a heap offset and a number of bytes
 *
 * Both stacks are in the PHFITSFILE structure, although they contain no
 * state information (why? because that way we know that they're big enough)
 *
 * To execute a block n times, set up states as:
 *	{ FITSB_ICONST, n}
 *	{ FITSB_LOOP,   }
 *	do things
 *	{ FITSB_ENDLOOP,}
 * You can, of course, use other istack ops instead of ICONST
 *
 * We need a struct, MACHINE, to handle this. It's private to this file,
 * and has to be a named struct so as appear as an opaque pointer in
 * phPhotoFitsIO.h. 
 */
#define FITSB_MISSING_IN_FILE 0x1	/* field is missing in file */

typedef struct {
   enum fitsb_op {
      FITSB_NOP,
      FITSB_SUB, FITSB_RET,
      FITSB_LOOP, FITSB_ENDLOOP,
      FITSB_SAVE, FITSB_RESTORE,
      FITSB_INCR, FITSB_DEREF,
      FITSB_ICONST, FITSB_HEAP_SIZE,
      FITSB_WRITE, FITSB_SEEK, FITSB_HEAP, FITSB_HEAP_MAIN, FITSB_HEAP_SEEK,
      FITSB_MASK_ROWS, FITSB_OBJMASK_SPAN, FITSB_REGION_ROWS,
      FITSB_ATLAS_IMAGE,
      FITSB_STRING,
#if defined(DEC_BYTE_ORDER)
      FITSB_SWAB, FITSB_SWAB4, FITSB_SWAB8,
#endif
      FITSB_STOP
   } op;				/* machine operations */
   int i;				/* parameter for op (if needed) */
   int flags;				/* modify action of state */
   const char *name;			/* name of element of struct */
   const char *src;			/* name of FITS column */
   const SCHEMA *sch;			/* schema for name */
   const SCHEMA_ELEM *sch_elem;		/* SCHEMA_ELEM for name */
} FITSB_STATE;

typedef struct phfitsb_machine {
   SCHEMATRANS *trans;			/* The schema trans used to build it */
   char type[SIZE];			/* the type the machine serves */
   int nstate;				/* number of states in machine */
   int nalloc;				/* number of allocated states */
   int nbyte;				/* number of bytes this machine
					   writes to main table */
   FITSB_STATE *states;			/* the states of the machine */
} MACHINE;

/*
 * Managing types such as REGIONs that have data that must be written to the
 * heap is hard; we give up and special case them. Why is it hard? Because
 * the pointer to the data (as well as the dimension) are specified elsewhere
 * in the structure.
 *
 * Additionally, if elem is NULL, type names a type such as CHAR that
 * may need to be written to the heap
 *
 * All these operators expect data to hold the address of the object
 * being written/read and that the heap file pointer points to the end of the
 * processed part of the heap. The number of elements to read is available as
 * the variable heap_nelem (you'll have to multiply by sizeof(elem) to get the
 * number of bytes to read)
 *
 * They leave the number of bytes written/read to/from heap on the istack
 * (but the actual heap i/o is the responsibility of the special operator)
 */
static struct {
   char *type;				/* type (e.g. MASK) */
   char *elem;				/* element (e.g. rows) */
   enum fitsb_op op;			/* operator */
   int maxsize;				/* max size of field */
} heap_capable[] = {
   {"ATLAS_IMAGE", "ncolor",	FITSB_ATLAS_IMAGE, 0},
   {"MASK",	"rows",		FITSB_MASK_ROWS, 0},
   {"OBJMASK",	"s",		FITSB_OBJMASK_SPAN, 0},
   {"REGION",	"rows_u16",	FITSB_REGION_ROWS, 0},
   {"REGION",	"rows_fl32",	FITSB_REGION_ROWS, 0},
   {"CHAR",	NULL,		FITSB_STRING, 100}, /*must go after MASK.rows*/
   {NULL, NULL, FITSB_NOP, 0}
};

/*****************************************************************************/

static void swap(char *sdata, const char *data, int n, int bytepix);
static void parse_card( const char *card );

#if 0
static int check_card_d(int fd, char *card, const char *keyword, int val);
#endif
static int check_card_s(const HDR *hdr, char *card,
			const char *keyword, const char *val);

static int set_card(int fd, int ncard, char *record, const char *card);

static int write_card_d(int fd, int n, char *record,
			     const char *keyword, int val, const char *commnt);
#if 0
static int write_card_f(int fd, int n, char *record,
			const char *keyword, float val, const char *commnt);
#endif
static int write_card_l(int fd, int n, char *record,
			const char *keyword, const char *val,
			const char *commnt);
static int write_card_s(int fd, int n, char *record,
			const char *keyword, const char *val,
			const char *commnt);
static int hdr_insert_line(HDR *hdr, const char *card);
/*
 * For backwards compatibility, if this is true then all heap data will be
 * assumed to be written with size read_all_heap_as_bytes; this is cleared
 * when a file is opened, and set if a Suspicious Card is seen in a header
 */
static int read_all_heap_as_bytes = 0;

/*****************************************************************************/
/*
 * Statics used by the card-parsing routines while processing headers
 */
static char *key;
static char *value;
static char *comment = "";
static char ccard[81];

static char record[FITSIZE + 1];
/*
 * Statics used to generate headers and write data
 */
static MACHINE *machine[NTYPE + 1] = {
   (MACHINE *)1
};					/* machines to handle types*/

/*****************************************************************************/
/*
 * Make and destroy state machines
 */
static MACHINE * 
new_machine(char *type, int nstate)
{
   int i;
   MACHINE *new = shMalloc(sizeof(MACHINE));
/*
 * check that machine[] array is initialised. 
 */
   if(machine[0] == (MACHINE *)1) {
      for(i = 0; i < NTYPE; i++) {
	 machine[i] = NULL;
      }
   }

   strncpy(new->type,type,SIZE);
   new->trans = NULL;
   new->nstate = 0;
   new->nalloc = nstate;
   new->nbyte = 0;
   if(nstate == 0) {
      new->states = NULL;
   } else {
      new->states = shMalloc(nstate*sizeof(FITSB_STATE));
      for(i = 0;i < nstate;i++) {
	 new->states[i].op = FITSB_STOP;
	 new->states[i].flags = 0;
	 new->states[i].i = -1;
	 new->states[i].name = new->states[i].src = "";
	 new->states[i].sch = NULL;
	 new->states[i].sch_elem = NULL;
      }
   }

   return(new);
}

static void
del_machine(MACHINE *mac)
{
   if(mac != NULL) {
      if(mac->trans != NULL) shSchemaTransDel(mac->trans);
      if(mac->nalloc != 0) shFree(mac->states);
      shFree(mac);
   }
}

/*****************************************************************************/
/*
 * Build a state machine for a given SCHEMA and SCHEMATRANS
 *
 * First a utility function to add a state to a machine just after state,
 * moving other states down as needed. The new state is a copy of state,
 * but with its op and val set as specified.
 *
 * Returns new state
 */
static FITSB_STATE *
add_state(MACHINE *machine, FITSB_STATE *state, enum fitsb_op op, int val)
{
   int off = state - machine->states;
   
   if(machine->nstate == machine->nalloc - 1) {	/* -1 so can copy to state+1 */
      machine->nalloc *= 2;
      machine->states = shRealloc(machine->states,
				  	  machine->nalloc*sizeof(FITSB_STATE));
      state = machine->states + off;
   }

   if(off < machine->nstate) {		/* not at end of array */
      memmove(state + 2, state + 1,
	      (machine->nstate - off)*sizeof(FITSB_STATE));
   }
   *(state + 1) = *state;		/* copy name etc. to next state */
   state->op = op;
   state->i = val;

   machine->nstate++;
   return(state + 1);
}

/*
 * and now the code to actually build a machine from a SCHEMATRANS and SCHEMA
 */
static MACHINE *
build_machine(MACHINE *machine,		/* the machine to build */
	      const SCHEMA *sch,	/* for this schema */
	      SCHEMATRANS *trans)	/* using this trans */
{
   int nbyte;				/* size of element or array thereof */
   int i,j,k;
   const SCHEMA *elem_sch;		/* schema of an element */
   int offset;
   FITSB_STATE *state = machine->states;

   if(machine->trans != NULL) {
      shSchemaTransDel(machine->trans);
   }
   machine->trans = trans;
   
   offset = 0;
   machine->nbyte = 0;
   for(i = 0;i < sch->nelem;i++) {
      if((j = p_shSpptEntryFind(trans,sch->elems[i].name)) < 0) {
	 ;				/* presumably ignored on purpose */
      } else {
	 if((elem_sch = shSchemaGet(sch->elems[i].type)) == NULL) {
#if 1					/* work around a dervish deficiency */
	    if(strcmp(sch->elems[i].type,"U8") == 0) {
	       elem_sch = shSchemaGet("UCHAR");
	    } else if(strcmp(sch->elems[i].type,"S8") == 0) {
	       elem_sch = shSchemaGet("CHAR");
	    } else if(strcmp(sch->elems[i].type,"U16") == 0) {
	       elem_sch = shSchemaGet("USHORT");
	    } else if(strcmp(sch->elems[i].type,"S16") == 0) {
	       elem_sch = shSchemaGet("SHORT");
	    } else if(strcmp(sch->elems[i].type,"U32") == 0) {
	       elem_sch = shSchemaGet("UINT");
	    } else if(strcmp(sch->elems[i].type,"S32") == 0) {
	       elem_sch = shSchemaGet("INT");
	    } else if(strcmp(sch->elems[i].type,"F32") == 0) {
	       elem_sch = shSchemaGet("FLOAT");
	    } else if(strcmp(sch->elems[i].type,"F64") == 0) {
	       elem_sch = shSchemaGet("DOUBLE");
	    } else {
	       shError("Cannot look up schema for %s",sch->elems[i].name);
	       continue;
	    }
#else
	    shError("Cannot look up schema for %s",sch->elems[i].name);
	    continue;
#endif
	 }
/*
 * If the elements being written aren't contiguous, due to gaps or padding,
 * we'll have to increment the data pointer when writing the table's rows
 */
	 if(sch->elems[i].offset != offset) {
	    state = add_state(machine,state,FITSB_INCR,
			      			sch->elems[i].offset - offset);
	    offset = sch->elems[i].offset;
	 }
/*
 * Now see what we are dealing with
 */
	 state->name = sch->elems[i].name;
	 state->src = trans->entryPtr[j].src;
	 state->sch = elem_sch;
	 state->sch_elem = &sch->elems[i];

	 if(trans->entryPtr[j].dstDataType == DST_HEAP) {
/*
 * We only know how to write a limited number of types to the heap; if this
 * isn't one of them, give up. There are two possibilities; a type such as
 * CHAR with a NULL elem, or a type such as MASK with an elem such as rows
 */
	    if(trans->entryPtr[j].heaptype != NULL) {
	       const SCHEMA *tmp = shSchemaGet(trans->entryPtr[j].heaptype); 
	       if(tmp != NULL) {
		  state->sch = tmp;
	       }
	    }

	    for(k = 0;heap_capable[k].type != NULL;k++) {
	       if((strcmp(state->sch->name,heap_capable[k].type) == 0 &&
		   			       heap_capable[k].elem == NULL) ||
		  (strcmp(sch->name,heap_capable[k].type) == 0 &&
		   	      strcmp(state->name,heap_capable[k].elem) == 0)) {
		  state = add_state(machine,state,FITSB_HEAP_SIZE,0);
		  state = add_state(machine,state,FITSB_SAVE,0);
		  state = add_state(machine,state,FITSB_INCR,
				    			-sch->elems[i].offset);
		  state = add_state(machine,state,FITSB_HEAP_SEEK,
				    		      heap_capable[k].maxsize);
		  state = add_state(machine,state,heap_capable[k].op,
				    			 sch->elems[i].offset);
		  state = add_state(machine,state,FITSB_HEAP_MAIN,
				    		      heap_capable[k].maxsize);
		  state = add_state(machine,state,FITSB_RESTORE,0);
		  state = add_state(machine,state,FITSB_INCR,
				    			   sch->elems[i].size);

		  offset += sch->elems[i].size;
		  machine->nbyte += 8;
		  break;
	       }
	    }
	    if(heap_capable[k].type == NULL) {
	       shError("Unable to read/write heap data for %s->%s",
			      sch->name,state->name);
	    }
	 } else if(elem_sch->type == ENUM) {
	    nbyte = sch->elems[i].size*sch->elems[i].i_nelem;
	    state = add_state(machine,state,FITSB_WRITE,nbyte);
#if defined(DEC_BYTE_ORDER)
	    state = add_state(machine,state,FITSB_SWAB4,nbyte);
#endif
	    offset += nbyte;
	    machine->nbyte += nbyte;
	 } else if(elem_sch->type == PRIM) {
	    const char *type_name = elem_sch->name;
	    nbyte = sch->elems[i].size*sch->elems[i].i_nelem;
	    if(strcmp(type_name,"CHAR") == 0 ||
	       strcmp(type_name,"UCHAR") == 0 ||
	       strcmp(type_name,"SHORT") == 0 ||
	       strcmp(type_name,"USHORT") == 0 ||
	       strcmp(type_name,"INT") == 0 ||
	       strcmp(type_name,"UINT") == 0 ||
	       strcmp(type_name,"LONG") == 0 ||
	       strcmp(type_name,"ULONG") == 0 ||
	       strcmp(type_name,"FLOAT") == 0 ||
	       strcmp(type_name,"DOUBLE") == 0) {
	       if(sch->elems[i].nstar == 0) { /* an inline array */
		  state = add_state(machine,state,FITSB_WRITE,nbyte);
#if defined(DEC_BYTE_ORDER)
		  if(strcmp(type_name,"SHORT") == 0 ||
		     strcmp(type_name,"USHORT") == 0) {
		     state = add_state(machine,state,FITSB_SWAB,nbyte);
		  } else if(strcmp(type_name,"INT") == 0 ||
			    strcmp(type_name,"UINT") == 0 ||
			    strcmp(type_name,"LONG") == 0 ||
			    strcmp(type_name,"ULONG") == 0 ||
			    strcmp(type_name,"FLOAT") == 0) {
		     state = add_state(machine,state,FITSB_SWAB4,nbyte);
		  } else if(strcmp(type_name,"DOUBLE") == 0) {
		     state = add_state(machine,state,FITSB_SWAB8,nbyte);
		  } else {
		     ;			/* don't swap CHARs */
		  }
#endif
		  offset += nbyte;
		  machine->nbyte += nbyte;
	       } else {
		  shError("Ignoring pointer to %s: %s->%s",type_name,
			  				sch->name,state->name);
		  continue;
	       }
	    } else if(strcmp(type_name,"LOGICAL") == 0 ||
		      strcmp(type_name,"STR") == 0 ||
		      strcmp(type_name,"TYPE") == 0) {
	       shError("Ignoring %s: %s->%s",type_name,sch->name,state->name);
	       continue;
	    } else {
	       shError("Saw an unknown PRIM type %s for %s->%s",
					 type_name,sch->name,state->name);
	       continue;
	    }
	 } else if(elem_sch->type == STRUCT) {
	    state = add_state(machine,state,FITSB_SAVE,0);

	    if(sch->elems[i].i_nelem > 1 || trans->entryPtr[j].size != NULL) {
	       int nelem = sch->elems[i].i_nelem;
	       if(trans->entryPtr[j].size != NULL) {
		  nelem = trans->entryPtr[j].num[0];
	       }

	       if(sch->elems[i].i_nelem > 1) { /* a FOO *array[]; */
		  ;
	       } else {			/* a FOO **array */
		  state = add_state(machine,state,FITSB_DEREF,0);
	       }
/*
 * loop over the elements of the array at run time
 */
	       state = add_state(machine,state,FITSB_ICONST,nelem);
	       state = add_state(machine,state,FITSB_LOOP,0);
	       state = add_state(machine,state,FITSB_SAVE,0);
	       state = add_state(machine,state,FITSB_DEREF,0);
	       state = add_state(machine,state,FITSB_SUB,-1); /*not yet known*/
	       state = add_state(machine,state,FITSB_RESTORE,0);
	       state = add_state(machine,state,FITSB_INCR,sch->elems[i].size);
	       state = add_state(machine,state,FITSB_ENDLOOP,0);
	    } else {
	       state = add_state(machine,state,FITSB_DEREF,0);
	       state = add_state(machine,state,FITSB_SUB,-1);
	    }

	    state = add_state(machine,state,FITSB_RESTORE,0);
	 } else {
	    shError("I don't know what to do with UNKNOWN type for "
			   		     "%s's schema",state->name);
	    continue;
	 }
      }
   }
/*
 * Terminate the finite state machine
 */
   state = add_state(machine,state,FITSB_STOP,0);

   shAssert(machine->nstate <= machine->nalloc);
   
   return(machine);
}

/*****************************************************************************/
/*
 * A simple peephole optimiser on the state machine. We generate a new
 * state machine with the optimisation applied (in the future we'll generate
 * a new machine using a simplified version of FITSB_STATE)
 */
static MACHINE *
optimise(MACHINE *machine)
{
   FITSB_STATE *state;			/* state of input machine */
   MACHINE *omachine;			/* output machine */
   FITSB_STATE *ostate;			/* state of output machine */

   omachine = new_machine(machine->type,machine->nstate);
   omachine->nbyte = machine->nbyte;
   ostate = omachine->states - 1;
   state = machine->states - 1;
   do {
      *++ostate = *++state;
/*
 * Remove some no-ops
 */
      if(state->op == FITSB_NOP) {
	 ostate--;
	 continue;
      }
      if(state->op == FITSB_INCR && state->i == 0) {
	 ostate--;
	 continue;
      }
/*
 * Can we amalgamate writes?
 */
      if(state->op == FITSB_WRITE && (state + 1)->op == state->op ) {
	 while((state + 1)->op == ostate->op) {
	    if((state->flags & FITSB_MISSING_IN_FILE) ||
	       (state + 1)->flags & FITSB_MISSING_IN_FILE) {
	       break;
	    }
	    state++;
	    ostate->i += state->i;
	 }
	 continue;
      }
/*
 * How about pairs of swaps-and-writes? If this is a SWAB, we know that it must
 * have been preceeded by a WRITE, so it's safe to look at state - 2.
 */
#if defined(DEC_BYTE_ORDER)
      if(state->op == FITSB_WRITE &&
	 ((state + 1)->op == FITSB_SWAB || (state + 1)->op == FITSB_SWAB4 ||
	  (state + 1)->op == FITSB_SWAB8)) {
	 if((state + 2)->op == state->op &&
	    (state + 2)->flags == state->flags &&
	    				  (state + 3)->op == (state + 1)->op) {
	    *(ostate + 1) = *(state + 1);
	    while((state + 2)->op == ostate->op &&
		  (state + 2)->flags == state->flags &&
		  (state + 3)->op == (ostate + 1)->op) {
	       state += 2;
	       ostate->i += state->i;
	       (ostate + 1)->i += (state + 1)->i;
	    }
	    state++; ostate++;		/* advance to the SWAB */
	 }
	 continue;
      }
#endif
   } while(state->op != FITSB_STOP);

   omachine->nstate = (ostate - omachine->states) + 1;

   return(omachine);
}

/*****************************************************************************/
/*
 * `Link' the machines, i.e. set the FITSB_SUB parameters properly.
 */
static void
link_machines(void)
{
   int i,j;
   FITSB_STATE *state;			/* state of machine */
   
   for(i = 0; i < NTYPE; i++) {
      if(machine[i] == NULL) continue;
      
      for(state = machine[i]->states;state->op != FITSB_STOP;state++) {
	 if(state->op == FITSB_SUB) {
	    for(j = 0;machine[j] != NULL;j++) {
	       if(strcmp(state->sch->name,machine[j]->type) == 0) {
		  state->i = j;
		  break;
	       }
	    }
	    if(machine[j] == NULL) {
	       shError("Failed to find machine for %s (type: %s)",
		       state->name, state->sch->name);
	       state->op = FITSB_NOP;
	    }
	 }
      }
   }
}
     
/*****************************************************************************/
/*
 * Trace a machine's operation. If fil is non-NULL, use optimised machine
 * from that file pointer
 */
int
phFitsBinTblMachineTrace(char *type,	/* TYPE of interest */
			 int exec,	/* execute machine? */
			 PHFITSFILE *fil) /* File descriptor, can be NULL */
{
   int i;
   int istack[1];			/* a fake istack */
   MACHINE *start;			/* starting machine */
   int list;				/* just list known types */
   MACHINE **machines = NULL;		/* machines to use */
   int opt = (fil == NULL) ? 0 : 1;	/* run an optimised machine? */
   FITSB_STATE *state;
   int slev = 0;			/* stack level */
   void **stack;
   int stacksize;

   if(fil != NULL && (fil->omachine == NULL || fil->omachine[0] == NULL)) {
      shErrStackPush("phFitsBinTblMachineTrace:"
	      " You must write a header before printing an optimised machine");
      return(-1);
   }

   list = (strcmp(type, "list") == 0 || strcmp(type, "LIST") == 0) ? 1 : 0;
   
   start = NULL;
   for(i = 0; i < NTYPE; i++) {
      if(machine[i] == NULL) {
	 continue;
      }
      
      if(list) {
	 printf("%s\n", machine[i]->type);
	 continue;
      }

      if(strcmp(type,machine[i]->type) == 0) {
	 if(opt) {
	    if((machines = fil->omachine) == NULL) {
	       printf("No optimised machine is available; use "
		      	  " fitsBinTblHdrWrite before printing the machine\n");
	       machines = machine;
	    }
	 } else {
	    if(!exec || fil->omachine[i] == NULL) {
	       machines = machine;
	    } else {
	       printf("using optimised machines\n");
	       machines = fil->omachine;
	    }
	 }
	 start = machines[i];
	 break;
      }
   }
   if(list) {
      return(0);
   }
   
   if(start == NULL) {
      printf("Machine %s doesn't exist\n",type);
      return(-1);
   }

   stacksize = 10;
   stack = shMalloc(stacksize*sizeof(void *));
   state = start->states;
      
   while(state->op != FITSB_STOP) {
      switch (state->op) {
       case FITSB_STOP:
	 shFatal("phFitsBinTblMachineTrace: you cannot get here");
	 break;
       case FITSB_NOP:
	 printf("(%d)%*s NOP\n",slev,slev,"");
	 break;
       case FITSB_WRITE:
	 printf("(%d)%*s WRITE       %d%s\n", slev, slev, "", state->i,
		(state->flags&FITSB_MISSING_IN_FILE) ? " (not in file)" : "");
	 break;
       case FITSB_SEEK:
	 printf("(%d)%*s SEEK        %d\n",slev,slev,"",state->i);
	 break;
       case FITSB_HEAP:
	 printf("(%d)%*s HEAP\n",slev,slev,"");
	 break;
       case FITSB_HEAP_MAIN:
	 printf("(%d)%*s HEAP_MAIN\n",slev,slev,"");
	 break;
       case FITSB_HEAP_SEEK:
	 printf("(%d)%*s HEAP_SEEK\n",slev,slev,"");
	 break;
       case FITSB_INCR:
	 printf("(%d)%*s INCR        %d\n",slev,slev,"",state->i);
	 break;
       case FITSB_DEREF:
	 printf("(%d)%*s DEREF\n",slev,slev,"");
	 break;
       case FITSB_SUB:
	 if(state->i < 0) {
	    printf("(%d)%*s SUB         (unresolved)\n",slev,slev,"");
	 } else {
	    printf("(%d)%*s SUB         %s\n",slev,slev,"",
	   				      	     machines[state->i]->type);
	    if(slev >= stacksize) {
	       stacksize *= 2;
	       stack = shRealloc(stack,stacksize*sizeof(void *));
	    }
	    if(exec) {
	       stack[slev++] = state + 1;
	       state = machines[state->i]->states;
	       continue;
	    }
	 }
	 break;
       case FITSB_RET:
	 printf("(%d)%*s RET\n",slev,slev,"");
	 if(slev == 0) {
	    printf("Saw RET at outermost stack level; stopping\n");
	    break;
	 }
	 shAssert(slev > 0);
	 if(exec) {
	    state = stack[--slev];
	    continue;
	 } else {
	    slev--;
	 }
	 break;
       case FITSB_LOOP:
	 printf("(%d)%*s LOOP\n",slev,slev,"");
	 if(slev >= stacksize) {
	    stacksize *= 2;
	    stack = shRealloc(stack,stacksize*sizeof(void *));
	 }
	 if(exec) {
	    istack[0] = 2;		/* twice round the loop */
	    stack[slev++] = state + 1;
	 } else {
	    slev++;
	 }
	 break;
       case FITSB_ENDLOOP:
	 printf("(%d)%*s ENDLOOP\n",slev - 1,slev - 1,"");
	 shAssert(slev > 0);
	 if(exec) {
	    if(--istack[0] > 0) {
	       state = stack[slev - 1];
	       continue;
	    }
	    slev--;
	 } else {
	    slev--;
	 }
	 break;
       case FITSB_SAVE:
	 printf("(%d)%*s SAVE\n",slev,slev,"");
	 if(slev >= stacksize) {
	    stacksize *= 2;
	    stack = shRealloc(stack,stacksize*sizeof(void *));
	 }
	 slev++;
	 break;
       case FITSB_RESTORE:
	 printf("(%d)%*s RESTORE\n",slev - 1,slev - 1,"");
	 slev--;
	 break;
       case FITSB_ICONST:
	 printf("(%d)%*s ICONST %d\n",slev,slev,"",state->i);
	 break;
       case FITSB_STRING:
	 printf("(%d)%*s STRING\n",slev,slev,"");
	 break;
       case FITSB_HEAP_SIZE:
	 printf("(%d)%*s HEAP_SIZE\n",slev,slev,"");
	 break;
       case FITSB_ATLAS_IMAGE:
	 printf("(%d)%*s FITSB_ATLAS_IMAGE\n",slev,slev,"");
	 break;
       case FITSB_MASK_ROWS:
	 printf("(%d)%*s MASK_ROWS\n",slev,slev,"");
	 break;
       case FITSB_OBJMASK_SPAN:
	 printf("(%d)%*s OBJMASK_SPAN\n",slev,slev,"");
	 break;
       case FITSB_REGION_ROWS:
	 printf("(%d)%*s REGION_ROWS\n",slev,slev,"");
	 break;
#if defined(DEC_BYTE_ORDER)
       case FITSB_SWAB:
	 printf("(%d)%*s SWAB        %d%s\n",slev,slev,"",state->i,
		(state->flags&FITSB_MISSING_IN_FILE) ? " (not in file)" : "");
	 break;
       case FITSB_SWAB4:
	 printf("(%d)%*s SWAB4       %d%s\n",slev,slev,"",state->i,
		(state->flags&FITSB_MISSING_IN_FILE) ? " (not in file)" : "");
	 break;
       case FITSB_SWAB8:
	 printf("(%d)%*s SWAB8       %d%s\n",slev,slev,"",state->i,
		(state->flags&FITSB_MISSING_IN_FILE) ? " (not in file)" : ""); 
	 break;
#endif
      }
      state++;
   }

   printf("(%d)%*s STOP\n",slev,slev,"");
   shFree(stack);

   return(0);
}

/*****************************************************************************/
/*
 * Declare types and SCHEMATRANSs to the fits binary code; we use these
 * to construct machines to generate the tables.
 */
static int
fitsBinMachineSet(MACHINE *mac)		/* a MACHINE to add to machine[] */
{
   int i;

   shAssert(mac != NULL && mac->type != NULL);
/*
 * look for slot for input machine
 */
   for(i = 0;i < NTYPE;i++) {
      if(machine[i] == NULL) {
	 machine[i] = mac;
	 return(0);
      } else if(strcmp(mac->type,machine[i]->type) == 0) {
	 del_machine(machine[i]);
	 machine[i] = mac;
	 return(0);
      }
   }
   
   shErrStackPush("Too many schema specified (max %d)",NTYPE);
   machine[NTYPE] = NULL;
   return(-1);
}

int
phFitsBinDeclareSchemaTrans(SCHEMATRANS *trans, char *type)
{
   MACHINE *mac;
   const SCHEMA *sch;			/* schema for type */

   if((sch = shSchemaGet(type)) == NULL) {
      shErrStackPush("Cannot find schema for %s",type);
      return(-1);
   }
/*
 * build the machine, and add it to machine[]
 */   
   mac = build_machine(new_machine(type,30),sch,trans);
   if(fitsBinMachineSet(mac) < 0) {
      del_machine(mac);
      return(-1);
   }

   return(0);
}

/*****************************************************************************/
/*
 * Forget a SCHEMATRANS declared with phFitsBinDeclareSchemaTrans(). If the
 * specified type is NULL, forget all machines
 */
void
phFitsBinForgetSchemaTrans(char *type)
{
   int i;
   
   for(i = 0;i < NTYPE;i++) {
      if(machine[i] == NULL || machine[i] == (MACHINE *)1) {
	 continue;
      }
      
      if(type == NULL) {
	 del_machine(machine[i]); machine[i] = NULL;
      } else if(strcmp(type,machine[i]->type) == 0) {
	 del_machine(machine[i]); machine[i] = NULL;
	 break;
      }
   }
}

/*****************************************************************************/
/*
 * Return 1 iff the type has a SCHEMATRANS registered
 */
int
phFitsBinSchemaTransIsDefined(const char *type)
{
   int i;
   
   for(i = 0;i < NTYPE;i++) {
      if(machine[i] == NULL || machine[i] == (MACHINE *)1) {
	 continue;
      }
      
      if(type != NULL && strcmp(type,machine[i]->type) == 0) {
	  return 1;
      }
   }

   return 0;
}

/*****************************************************************************/
/*
 * Allocate a new PHFITSFILE
 */
static PHFITSFILE *
new_fitsfile(char *file,		/* File to open */
	     int flag,			/* open for read or write? */
	     int nstack			/* size of stack */
	     )
{
   int fd;
   int heapfd;
   int i;
   PHFITSFILE *new = shMalloc(sizeof(PHFITSFILE));

#if 0
   shAssert(sizeof(off_t) == sizeof(long)); /* we use long not off_t in
					       PHFITSFILE.old_{fd,heapfd}_pos
					       for the sake of make_io */
#else
   if(sizeof(off_t) != sizeof(long)) { /* we use long not off_t in
					       PHFITSFILE.old_{fd,heapfd}_pos
					       for the sake of make_io */
      static int warned = 0;
      if(!warned) {			/* NOTREACHED ---
					   quieten confused compaq compilers */
	 shError("sizeof(off_t) != sizeof(long) in %s line %d",
							    __FILE__,__LINE__);
	 warned = 1;
      }
   }
#endif

   if(flag == 0) {			/* open for read */
      if((fd = open(file,0)) < 0) {
	 shErrStackPush("phFitsBinTblOpen: Can't open %s",file);
	 return(NULL);
      }
      if((heapfd = open(file,0)) < 0) {
	 shErrStackPush("phFitsBinTblOpen: Can't open %s's heap",file);
	 return(NULL);
      }
   } else if(flag == 1) {		/* open for write */
      if((fd = open(file,O_RDWR|O_CREAT|O_TRUNC,0666)) < 0) {
	 shErrStackPush("phFitsBinTblOpen: Can't open %s",file);
	 return(NULL);
      }
      heapfd = -1;
   } else if(flag == 2) {		/* open for append */
      flag = 1;
      if((fd = open(file,O_RDWR|O_CREAT,0666)) < 0) {
	 shErrStackPush("phFitsBinTblOpen: Can't open %s",file);
	 return(NULL);
      }
      heapfd = -1;
   } else {
      shErrStackPush("phFitsBinTblOpen: unknown flag %d",flag);
      return(NULL);
   }

   new->type = shTypeGetFromName("UNKNOWN");
   new->mode = flag;
   new->fd = fd;
   new->heapfd = heapfd;
   new->old_fd_pos = new->old_heapfd_pos = -1;
   new->hdr_start = new->heap_start = -1;
   new->hdr_end = new->heap_end = -1;
   new->row = NULL;
   new->nrow = 0;

   new->machine = NULL;
   new->omachine = shMalloc(NTYPE*sizeof(MACHINE *));
   for(i = 0;i <= NTYPE;i++) {
      new->omachine[i] = NULL;
   }
   new->start_ind = -1;
   new->stack = shMalloc(nstack*sizeof(void *));
   new->istack = shMalloc(nstack*sizeof(int));
   new->stacksize = new->istacksize = nstack;

   return(new);
}

static void
del_fitsfile(PHFITSFILE *fil)
{
   if(fil != NULL) {
      close(fil->fd);
      hbclose(fil->heapfd);
      ;					/* don't delete fil->machine,
					   we don't own the memory */
      if(fil->omachine != NULL) {
	 int i;
	 for(i = 0;i < NTYPE;i++) {
	    del_machine(fil->omachine[i]); 
	 }
	 shFree(fil->omachine); fil->omachine = NULL;
      }
      shFree(fil->stack);
      shFree(fil->istack);
      shFree(fil);
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Open a file containing fits binary tables. If the HDR is non-NULL, it
 * will be written to the file's primary header
 *
 * Return a PHFITSFILE *, or NULL in case of error.
 */
PHFITSFILE *
phFitsBinTblOpen(char *file,		/* file to open */
		 int flag,		/* open for read (flag == 0),
					   write (flag == 1; => creat)
					   append (flag == 2; => creat) */
		 HDR *hdr)		/* additional header info */
{
   char cbuff[82];			/* full filename/card buffer */
   PHFITSFILE *fil;			/* descriptor for table */
   int i;
   int ncard;				/* number of cards in this record */

   read_all_heap_as_bytes = 0;		/* are all datatypes written to the
					   heap as bytes? */

   if((fil = new_fitsfile(file,flag,10)) == NULL) {
      return(NULL);
   }
   if(flag == 2 && lseek(fil->fd, 0, SEEK_END) == 0) { /* go to end of file */
      flag = 1;				/* nothing to append to; write PDU */
   }
   
   if(flag == 0) {			/* open for read */
      for(ncard = 0;read(fil->fd,cbuff,80) == 80;ncard++) {
	 parse_card(cbuff);
	 
	 if(strcmp(key,"SIMPLE") == 0) {
	    if(strcmp(value,"T") != 0) {
	       shErrStackPush("File isn't \"SIMPLE = T\" FITS");
	       del_fitsfile(fil);
	       return(NULL);
	    }
	 } else if(strcmp(key,"BITPIX") == 0) {
#if 0					/* this is not a requirement for the PDU */
	    if(atoi(value) != 8) {
	       shErrStackPush("FITS binary tables must have BITPIX == 8");
	       del_fitsfile(fil);
	       return(NULL);
	    }
#endif
	 } else if(strcmp(key,"NAXIS") == 0) {
	    if(atoi(value) != 0) {
	       shErrStackPush("FITS binary tables must have NAXIS == 0");
	       del_fitsfile(fil);
	       return(NULL);
	    }
	 } else if(strcmp(key,"EXTEND") == 0) {
	    ;
	 } else if(strcmp(key,"END") == 0) {
	    break;
	 } else {
	    if(hdr != NULL) {
	       shHdrInsertAscii(hdr, key, value, comment);
	    }
	 }
      }
      while(++ncard % 36 != 0) {	/* read rest of header */
	 if(read(fil->fd,cbuff,80) != 80) {
	    shErrStackPush("Failed to read card %d",ncard + 1);
	    return(NULL);
	 }
      }
   } else if(flag == 1) {		/* open for write */
/*
 * The file's open, so fill out the header
 */
      ncard = 0;
      ncard = write_card_l(fil->fd,ncard,record,"SIMPLE","T","");
      ncard = write_card_d(fil->fd,ncard,record,"BITPIX",8,"");
      ncard = write_card_d(fil->fd,ncard,record,"NAXIS",0,"");
      ncard = write_card_l(fil->fd,ncard,record,"EXTEND","T","");
      if(hdr != NULL) {
	 for(i = 0;hdr->hdrVec[i] != NULL;i++) {
	    ncard = set_card(fil->fd,ncard,record,hdr->hdrVec[i]);
	 }
      }
      ncard = write_card_s(fil->fd,ncard,record,"END","","");
      
      while(ncard != 0) {
	 ncard = write_card_s(fil->fd,ncard,record,"","","");
      }
   } else if(flag == 2) {		/* open for append */
      ;
   } else {
      shErrStackPush("phFitsBinTblOpen: Unknown flag value %d",flag);
      return(NULL);
   }

   return(fil);
}

/*****************************************************************************/
/*
 * Read a fits header and define a type's schema from it
 *
 * First a struct to remember what the header says about a table column
 */
typedef struct fits_col {
/*
 * fields in the header cards
 */
   char tdim[40];			/* value for TDIM card */
   char tform[40];			/* value for TFORM card */
   char ttype[40];			/* value for TTYPE card */
   char tzero[40], tscal[40];		/* TZERO and TSCAL values */

   int nel;				/* number of elements if an array */
   int size;				/* size of element of member */

   struct fits_col *next;
} FITS_COL;

static FITS_COL *
fitsColNew(void)
{
   FITS_COL *newcol = shMalloc(sizeof(FITS_COL));
   *newcol->tdim = *newcol->tform = *newcol->ttype = '\0';
   *newcol->tzero = *newcol->tscal = '\0';
   newcol->next = NULL;

   return(newcol);
}

static void
fitsColDel(FITS_COL *col)
{
   shFree(col);
}

static int
define_schema_from_header(const char *schema_name, /* the desired schema */
			  int tfields,		   /* the number of columns */
			  const HDR *hdr)	   /* header with keywords */
{
   char cbuff[82];			/* full filename/card buffer */
   FITS_COL *col;			/* the current column */
   int colno;				/* number of current column */
   char *dimen_str;			/* dimension of array in schema */
   int i, j;
   int nel;				/* number of elements in an column */
   int offset;				/* offset of col's element in struct */
   char *ptr;				/* scratch pointer to strings */
   SCHEMA *schema;			/* our new schema */
   int size;				/* sizeof a column's type */
   FITS_COL *table;			/* all the columns in the table */
   char *type_str;			/* name of type in schema */

   table = col = NULL;

   for(colno = 1; ; colno++) {
      char keyword[40];
      sprintf(keyword, "TTYPE%d", colno);

      if (shHdrGetLine(hdr, keyword, cbuff) != SH_SUCCESS) {
	 break;
      }
      parse_card(cbuff);
/*
 * Allocate a new FITS_COL
 */
      if(table == NULL) {
	 col = table = fitsColNew();
      } else {
	 col->next = fitsColNew();
	 col = col->next;
      }
/*
 * And fill it out
 */
      strcpy(col->ttype, value);
      if((ptr = strchr(col->ttype, ' ')) != NULL) *ptr = '\0';

      sprintf(keyword, "TFORM%d", colno);
      if (shHdrGetLine(hdr, keyword, cbuff) == SH_SUCCESS) {
	 parse_card(cbuff);
	 strcpy(col->tform, value);	 
	 if((ptr = strchr(col->tform, ' ')) != NULL) *ptr = '\0';
      } else {
	 shError("define_schema_from_header: "
		 "Failed to find %s", keyword);
      }
      
      sprintf(keyword, "TDIM%d", colno);
      if (shHdrGetLine(hdr, keyword, cbuff) == SH_SUCCESS) {
	 parse_card(cbuff);
	 strcpy(col->tdim, value);
      }

      sprintf(keyword, "TSCAL%d", colno);
      if (shHdrGetLine(hdr, keyword, cbuff) == SH_SUCCESS) {
	 parse_card(cbuff);
	 strcpy(col->tscal, value);
      }
      
      sprintf(keyword, "TZERO%d", colno);
      if (shHdrGetLine(hdr, keyword, cbuff) == SH_SUCCESS) {
	 parse_card(cbuff);
	 strcpy(col->tzero, value);
      }
   }
/*
 * Allocate a SCHEMA and set the elements to match this table
 */
   colno--;				/* started at 1, not 0 */
   if (colno != tfields) {
      shError("define_schema_from_header: "
	      "Saw %d columns (TFIELDS = %d)", colno, tfields);
   }
   
   schema = shSchemaNew(colno);

   offset = 0;
   for(i = 0, col = table; i < colno; i++, col = col->next) {
/*
 * Find dimension of any arrays
 */
      if (!isdigit(col->tform[0])) {	/* default is one */
	 nel = 1;
      } else {
	 nel = atoi(col->tform);	/* number of elements in array */
      }
      
      if(nel <= 1) {			/* a scalar */
	 shAssert(nel == 1);
	 dimen_str = NULL;
      } else {				/* an n-dimensional array (n >= 1) */
	 int dims[10];			/* we have to reverse the order of
					   the dimensions, so save them here */
	 if(col->tdim[0] == '\0') {
	    dims[0] = nel; j = 1;
	 } else {
	    strcpy(cbuff, col->tdim);
	    ptr = strchr(cbuff, ')');
	    shAssert(*cbuff == '(' && ptr != NULL);

	    *ptr = ','; ptr = &cbuff[1];
	    for(j = 0; j < 10; j++) {
	       dims[j] = atoi(ptr);
	       if((ptr = strchr(ptr,',')) == NULL) {
		  break;
	       }
	       ptr++;
	    }
	 }

	 cbuff[0] = '\0';
	 for(j--; j >= 0; j--) {
	    sprintf(&cbuff[strlen(cbuff)],"%d ",dims[j]);
	 }
	 shAssert(strlen(cbuff) < sizeof(cbuff));

	 j = strlen(cbuff);
	 dimen_str = malloc(j);	/* use malloc not shMalloc as this
					   string will end up in SCHEMA */
	 strncpy(dimen_str, cbuff, j - 1); dimen_str[j - 1] = '\0';
      }
/*
 * Now deal with the type; it starts with a dimension which we must skip
 */
      for(ptr = col->tform; ptr != '\0' && isdigit(*ptr); ptr++) continue;

      switch (*ptr) {
       case 'A':			/* string */
	 type_str = "CHAR";
	 size = sizeof(char);		/* == 1 */
	 break;
       case 'B':			/* char (not string) */
	 if(col->tzero[0] != '\0') {	/* tzero signals unsigned */
	    shAssert(atoi(col->tzero) == (1 << 7));
	    type_str = "UCHAR";
	 } else {
	    type_str = "CHAR";
	 }
	 size = sizeof(char);		/* == 1 */
	 break;
       case 'E':			/* float */
	 type_str = "FLOAT";
	 size = sizeof(float);
	 break;
       case 'D':			/* double */
	 type_str = "DOUBLE";
	 size = sizeof(double);
	 break;
       case 'I':			/* short int */
	 if(col->tzero[0] != '\0') {	/* tzero signals unsigned */
	    shAssert(atoi(col->tzero) == (1 << 15));
	    type_str = "USHORT";
	 } else {
	    type_str = "SHORT";
	 }
	 size = sizeof(short);
	 break;
       case 'J':			/* int or long */
	 if(col->tzero[0] != '\0') {	/* tzero signals unsigned */
	    shAssert(atoi(col->tzero) == (1 << 31));
	    type_str = "UINT";
	 } else {
	    type_str = "INT";
	 }
	 size = sizeof(int);
	 break;
       case 'P':			/* Heap */
	 shErrStackPush("I cannot define a schema for types including heap: "
			"%s %s", col->ttype, col->tform);
/*
 * clean up
 */
	 col = table;
	 while(col != NULL) {
	    FITS_COL *tmp = col->next;
	    fitsColDel(col);
	    col = tmp;
	 }
	 for(; i >= 0; i--) {
	    if(schema->elems[i].nelem != NULL) {
	       free(schema->elems[i].nelem);
	    }
	 }
#if DERVISH_VERSION >= 7 || (DERVISH_VERSION == 7 && DERVISH_MINOR_VERSION >= 8)
	 shSchemaDel(schema);
#endif

	 return(-1);
       default:
	 shError("Unknown type %s for field %s in binary table; skipping",
		 col->tform, col->ttype);

	 col = table;
	 while(col != NULL) {
	    FITS_COL *tmp = col->next;
	    fitsColDel(col);
	    col = tmp;
	 }
	 return(-1);
      }
/*
 * set the SCHEMA_ELEM
 */
      if(offset%size != 0) {	/* need some padding */
	 offset += size - offset%size;
      }

      strcpy(schema->elems[i].name, col->ttype);
      strcpy(schema->elems[i].type, type_str);
      schema->elems[i].nelem = dimen_str;
      schema->elems[i].offset = offset;

      offset += nel*size;
   }
/*
 * Tell dervish and descendants about that schema
 */
   if(offset%8 != 0) {			/* pad to 8-byte boundary */
      offset += 8 - offset%8;
   }

   strcpy(schema->name, schema_name);
   schema->type = STRUCT;
   schema->size = offset;

   p_shSchemaLoad(schema);
/*
 * free table
 */
   col = table;
   while(col != NULL) {
      FITS_COL *tmp = col->next;
      fitsColDel(col);
      col = tmp;
   }

   return(0);
}

/*****************************************************************************/
/*
 * Return the proper character for a TFORM card
 */
static char
get_tform_char(const char *type_name)
{
   if(strcmp(type_name,"SHORT") == 0 || strcmp(type_name,"USHORT") == 0) {
      return('I');
   } else if(strcmp(type_name,"INT") == 0 ||
	     strcmp(type_name,"UINT") == 0 ||
	     strcmp(type_name,"LONG") == 0 ||
	     strcmp(type_name,"ULONG") == 0) {
      return('J');
   } else if(strcmp(type_name,"FLOAT") == 0) {
      return('E');
   } else if(strcmp(type_name,"DOUBLE") == 0) {
      return('D');
   } else if(strcmp(type_name,"CHAR") == 0) {
      return('A');
   } else if(strcmp(type_name,"UCHAR") == 0) {
      return('B');
   } else {
      shFatal("get_tform_char: unknown type %s", type_name);
      return('\0');			/* NOTREACHED */
   }
}

static int
get_sizeof_from_tform(const char tform)
{
   switch (tform) {
    case 'A':
    case 'B':
      return(1);
    case 'E':
      return(4);
    case 'I':
      return(2);
    case 'J':
      return(4);
    case 'D':
      return(8);
    default:
      shFatal("get_sizeof_from_tform: unknown type %c", tform);
      return(0);			/* NOTREACHED */
   }

   return(0);			/* NOTREACHED */
}

/*****************************************************************************/
/*
 * shHdrDel() fails to check for NULL. Grrr
 */
static void
myHdrDel(HDR *hdr)
{
   if (hdr != NULL) {
      shHdrDel(hdr);
   }
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Read a FITS table header; we've already read the file header
 *
 * If type is NULL, skip the entire table. In this case all the cards
 * in the header will be returned in hdr (if provided), otherwise
 * only those before the TFORM1 card
 *
 * If the type isn't known it'll be defined from the header (heap fields
 * are not acceptable); you'll still have to define a schematrans before
 * you can read the table
 *
 * Return:
 *	0	on success
 *	1	on successful completion with warnings
 *     -1	on failure
 */
int
phFitsBinTblHdrRead(PHFITSFILE *fil,	/* file descriptor for (opened) file */
		    char *type,		/* the type we'll be reading */
		    HDR *hdr,		/* a header to write to the file */
		    int *nrow,		/* number of rows in table
					   (can be NULL) */
		    int quiet)		/* don't print informational messages*/
{
   int colno;				/* number of current column */
   char cbuff[82];			/* full filename/card buffer */
   int i;
   char keyword[20];			/* keyword on a card */
   int naxis1 = 0, naxis2 = 0;		/* from header */
   int pcount = 0;			/*  "    "  "  */
   int theap = 0;			/*  "    "  "  */
   HDR *myhdr = NULL;			/* a header to save the FITS keys to if
					   hdr == NULL */
   int nwrite[NTYPE];			/* how many times has this struct
					   been written? */
   int ncard;				/* number of cards in this record */
   int n_mismatch;			/* number of mismatches in header */
   int rowlen;				/* number of bytes in a row */
   int slev = 0;			/* stack level */
   int islev = 0;			/* integer stack level */
   char *ptr;				/* scratch pointer */
   char scr[40];			/* a scratch buffer */
   int schema_mismatch;			/* the file doesn't match the schema */
   FITSB_STATE *saved_state = NULL;	/* real state if state == &stop */
   FITSB_STATE *state;			/* state of machine */
   FITSB_STATE stop;			/* state with op == FITSB_STOP */
   int state_ind;			/* index in machine[], nwrite[], etc.
					   of the current machine */
   char suffix[10];			/* suffix for TTYPE keys */
   char tdim_value[40];			/* value for TDIM card */
   int tfields = -1;			/* value of TFIELDS */
   unsigned int tzero;			/* TZERO, only used for unsigned data*/
   char val[40];			/* value of a card */
   int warning = 0;			/* should we return a warning status?*/

   stop.op = FITSB_STOP;
/*
 * Check that we know about the type we are being given
 */
   if(type != NULL) {
      fil->start_ind = -1;
      for(i = 0; i < NTYPE; i++) {
	 if(machine[i] == NULL || machine[i] == (MACHINE *)1) continue;

	 if(strcmp(machine[i]->type,type) == 0) {
	    fil->start_ind = i;
	    break;
	 }
      }
      fil->type = shTypeGetFromName(type);
      if(fil->start_ind == -1) {
	 shErrStackPush("No SCHEMATRANS is known for %s",type);
	 if(fil->type == -1) {
	    shErrStackPush("I'll define %s for you, but you'll "
			   "have to provide a SCHEMATRANS to read the data",
			   type);
	 } else {
	    return(-1);
	 }

	 fil->type = 0;
      }
   }
/*
 * We need a header to save FITS keys/values for later perusal; if
 * they didn't give us one, allocate our own
 */
   if (hdr == NULL && type != NULL) {
       hdr = myhdr = shHdrNew();
   }
/*
 * Read the header, looking for required keywords. They really have to
 * be in order, but I shall not bother checking this for now.
 *
 * set the start of the header; used in phFitsBinTblEnd
 */
   if((fil->hdr_start = lseek(fil->fd,0,SEEK_CUR)) == -1) {
      shErrStackPushPerror("phFitsTblHdrRead: "
			   "cannot find where file pointer is");
      myHdrDel(myhdr);
      return(-1);
   }


   for(ncard = 0;read(fil->fd,cbuff,80) == 80 && ++ncard;) {
      parse_card(cbuff);

      if(hdr != NULL) {
	 if(hdr_insert_line(hdr, cbuff) != SH_SUCCESS) {
	    shErrStackPush("phFitsBinTblHdrRead: "
			   "Failed to copy %s card to header line %d",
			   key,ncard);
	    myHdrDel(myhdr);
	    return(-1);
	 }
      }
      
      if(strcmp(key,"XTENSION") == 0) {
	 if(strcmp(value,"BINTABLE") != 0) {
	    shErrStackPush("Extension type is \"%s\" not \"BINTABLE\"",value);
	    myHdrDel(myhdr);
	    return(-1);
	 }
      } else if(strcmp(key,"BITPIX") == 0) {
	 if(atoi(value) != 8) {
	    shErrStackPush("FITS binary tables must have BITPIX == 8");
	    myHdrDel(myhdr);
	    return(-1);
	 }
      } else if(strcmp(key,"NAXIS") == 0) {
	 if(atoi(value) != 2) {
	    shErrStackPush("FITS binary tables must have NAXIS == 2");
	    myHdrDel(myhdr);
	    return(-1);
	 }
      } else if(strcmp(key,"NAXIS1") == 0) {
	 naxis1 = atoi(value);
      } else if(strcmp(key,"NAXIS2") == 0) {
	 naxis2 = atoi(value);
      } else if(strcmp(key,"PCOUNT") == 0) {
	 pcount = atoi(value);
      } else if(strcmp(key,"GCOUNT") == 0) {
	 if(atoi(value) != 1) {
	    shErrStackPush("FITS binary tables must have GCOUNT == 1");
	    myHdrDel(myhdr);
	    return(-1);
	 }
      } else if(strcmp(key,"TFIELDS") == 0) {
	 tfields = atoi(value);
	 
	 break;				/* last of required keywords */
      } else {
	 shErrStackPush("Saw keyword \"%s\" in midst of required keywords",
									  key);
	 myHdrDel(myhdr);
	 return(-1);
      }
   }
   shAssert(comment != NULL);		/* this test makes compilers happy */
/*
 * Now read the rest of the header
 */
   for(;read(fil->fd,cbuff,80) == 80;ncard++) { /* find end of header */
       parse_card(cbuff);
       
       if(strcmp(key,"END") == 0) {
	   break;
       } else if(strcmp(key,"THEAP") == 0) {
	   theap = atoi(value);
       }
       
       if(hdr != NULL) {
	   if(hdr_insert_line(hdr, cbuff) != SH_SUCCESS) {
	       shErrStackPush("phFitsBinTblHdrRead: "
			      "Failed to copy %s card to header line %d",
			      key,ncard);
	       return(-1);
	   }
       }
   }
   while(++ncard % 36 != 0) {		/* read header padding */
       if(read(fil->fd,cbuff,80) != 80) {
	   shErrStackPushPerror("Failed to read card %d",ncard + 1);
	   return(-1);
       }
   }
/*
 * If type's NULL, skip this table unless hdr is non-NULL, in which case
 * simply read the header and leave the read pointers in heap and main
 * table undisturbed
 */
   if(type == NULL) {
      if(hdr != NULL) {
	 if(lseek(fil->fd, fil->hdr_start, SEEK_SET) == -1) {
	    shErrStackPushPerror("phFitsTblHdrRead: "
				 "failed to seek back to start of header");
	    return(-1);
	 }
      } else {
	 if((fil->heap_end = lseek(fil->fd,naxis1*naxis2 + pcount,SEEK_CUR))
								       == -1) {
	    shErrStackPushPerror("Failed to skip table");
	    return(-1);
	 }
	 if(fil->heap_end%FITSIZE != 0) {
	    int extra = FITSIZE - fil->heap_end%FITSIZE;
	    if((fil->heap_end = lseek(fil->fd, extra, SEEK_CUR)) == -1) {
	       shErrStackPushPerror("Failed to skip to start of next table");
	       return(-1);
	    }
	 }
      }
      
      return(0);
   }
/*
 * Set the end of the header
 */
   if((fil->hdr_end = lseek(fil->fd,0,SEEK_CUR)) == -1) {
      shErrStackPushPerror("Cannot set seek pointer to end of header");
      myHdrDel(myhdr);
      return(-1);
   }
/*
 * We are ready to process the header and see if it agrees with the schema
 * that we are trying to read into; if we didn't recognise the type we'll
 * define it from the header
 */
   if(fil->type == 0) {
      if(define_schema_from_header(type, tfields, hdr) < 0) {
	 shErrStackPushPerror("phFitsTblHdrRead: "
			      "Error defining schema for %s from header",type);
	 myHdrDel(myhdr);
	 return(-1);
      }
/*
 * return to start of header
 */
      if(lseek(fil->fd, fil->hdr_start, SEEK_SET) == -1) {
	 shErrStackPushPerror("phFitsTblHdrRead: "
			      "failed to seek back to start of header");
      }
      myHdrDel(myhdr);
      return(-1);
   }
/*
 * `Link' the machines, i.e. set the FITSB_SUB parameters properly.
 * For this first stage (reading the headers) we interpret STOP
 * instructions as RETs
 */
   for(i = 0; i < NTYPE; i++) {
      nwrite[i] = 0;			/* number of times this type's been
					   written */
   }
   
   link_machines();
/*
 * Now use those machines to check the fits table header. Because we may
 * have written the same structure several times, we were careful to make
 * the TTYPE values unique by appending a number to the string
 * specified in the SCHEMATRANS, and incrementing it every time that we
 * wrote the structure. We have to reproduce this as we read the header.
 * The number of uses is stored in the array nwrite.
 *
 * There is a further problem, in that we need to keep track of which machine
 * we are currently in. We do this by pushing the current machine's index
 * onto the integer stack.
 */
   colno = 1;
   n_mismatch = 0;
   rowlen = 0;
   state_ind = fil->start_ind;	/* index of current state */
   suffix[0] = '\0';
   
   state = machine[state_ind]->states;
   
   while(!(slev == 0 && state->op == FITSB_STOP)) {
      switch(state->op) {
       case FITSB_NOP:
	 state++;
	 break;
       case FITSB_SUB:
	 shAssert(state->i >= 0);
	 nwrite[state->i]++;		/* we're reading another instance of a
					   type, so increment the use counter*/
	 if(nwrite[state->i] > 1) {
	    sprintf(suffix,"%d",nwrite[state->i]);
	 } else {
	    suffix[0] = '\0';
	 }
	 
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,
				   fil->stacksize*sizeof(void *));
	 }
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,
				    fil->istacksize*sizeof(int));
	 }
	 
	 fil->istack[islev++] = state_ind;
	 fil->stack[slev++] = state + 1;
	 state_ind = state->i;
	 state = machine[state->i]->states;
	 break;
       case FITSB_STOP:		/* a STOP, but treat as a RET */
       case FITSB_RET:
	 shAssert(slev > 0 && islev > 0);
	 state = fil->stack[--slev];
	 state_ind = fil->istack[--islev];
	 if(nwrite[state_ind] > 1) {
	    sprintf(suffix,"%d",nwrite[state_ind]);
	 } else {
	    suffix[0] = '\0';
	 }
	 break;
       case FITSB_SEEK:
	 state++;
	 break;
       case FITSB_WRITE:
       case FITSB_HEAP:
       case FITSB_HEAP_MAIN:
/*
 * See what we are dealing with
 */
	 tzero = 0;
	 if(state->op == FITSB_HEAP || state->op == FITSB_HEAP_MAIN) {
	    if(state->op == FITSB_HEAP) {
	       shAssert(islev > 0);
	       islev--;
	    }
	    if(read_all_heap_as_bytes != 1 && state->sch->type == PRIM) {
	       sprintf(val,"1P%c(%d)",
				   get_tform_char(state->sch->name), state->i);
	    } else {
	       sprintf(val,"1PB(%d)",state->i);
	    }
	    rowlen += 8;
	 } else if(state->sch->type == ENUM) {
	    sprintf(val,"%dJ",state->i/4);
	    rowlen += state->i;
	 } else if(state->sch->type == PRIM) {
	    const char *type_name = state->sch->name; /* canonical form */
	    
	    if(strcmp(type_name,"SHORT") == 0 ||
	       strcmp(type_name,"USHORT") == 0) {
	       
	       if(*type_name == 'U') {
		  tzero = (unsigned short)(1 << (15));
	       }
	       
	       sprintf(val,"%dI",state->i/2);
	       rowlen += state->i;
	    } else if(strcmp(type_name,"INT") == 0 ||
		      strcmp(type_name,"UINT") == 0 ||
		      strcmp(type_name,"LONG") == 0 ||
		      strcmp(type_name,"ULONG") == 0) {
	       if(*type_name == 'U') {
		  tzero = 0x80000000;
	       }
	       
	       sprintf(val,"%dJ",state->i/4);
	       rowlen += state->i;
	    } else if(strcmp(type_name,"FLOAT") == 0) {
	       sprintf(val,"%dE",state->i/4);
	       rowlen += state->i;
	    } else if(strcmp(type_name,"DOUBLE") == 0) {
	       sprintf(val,"%dD",state->i/8);
	       rowlen += state->i;
	    } else if(strcmp(type_name,"CHAR") == 0 ||
		      strcmp(type_name,"UCHAR") == 0) {
	       if(*type_name == 'U') {
		  sprintf(val,"%dB",state->i);
	       } else {
		  if(state->i == 1) {	/* an 8-bit signed number */
		     tzero = (unsigned short)(1 << (8));
		     sprintf(val,"%dB",state->i);
		  } else {
		     sprintf(val,"%dA",state->i);
		  }
	       }
	       rowlen += state->i;
	    } else if(strcmp(type_name,"STR") == 0 ||
		      strcmp(type_name,"LOGICAL") == 0) {
	       shErrStackPush("Ignoring %s: %s",type_name,state->name);
	       warning = 1;
	       state++;
	       continue;
	    } else {
	       shErrStackPush("Saw an unknown PRIM type %s for %s",
			      type_name, state->name);
	       warning = 1;
	       state++;
	       continue;
	    }
	 } else if(state->sch->type == STRUCT) {
	    shFatal("STRUCT %s cannot appear as a WRITE op",state->name);
	 } else {
	    shErrStackPush("I don't know what to do with UNKNOWN type for "
			   "%s's schema",state->name);
	    warning = 1;
	    state++;
	    continue;
	 }
	 
	 schema_mismatch = 0;
	 tdim_value[0] = '\0';		/* we haven't seen it yet */
	 sprintf(keyword,"TFORM%d",colno);

	 strncpy(scr, &val[1], 2);	/* allow a missing number and (0) */
	 if(check_card_s(hdr,cbuff,keyword,val) == 0 &&
	    check_card_s(hdr,cbuff,keyword,scr) == 0) {
/*
 * See if it's a heap entry written as bytes; if so, and the schema expected
 * a type, accept it anyway
 */
	    int n = 1; char type_c;
	    if(sscanf(value, "%dP%c", &n, &type_c) == 2 ||
	       sscanf(value, "P%c", &type_c) == 1) {
	       read_all_heap_as_bytes = get_sizeof_from_tform(type_c);
		  
	       if(!quiet) {
		  shError("phFitsBinTblHdrRead: saw %s [expected %s]:\n"
			  "   Assuming that all heap data were written %s",
			  value, val, (read_all_heap_as_bytes == 1 ?
				    "as bytes" : "with incorrect header"));
	       }
	    } else {
	       schema_mismatch++;
	    }
	 }
	 ncard++;

	 if(schema_mismatch && strcmp(key, "END") == 0) {
	    n_mismatch++;
	    saved_state = state;
	    state = &stop;
	    break;
	 }
/*
 * Then the TTYPE card
 */
	 sprintf(keyword,"TTYPE%d",colno);
	 sprintf(val,"%s%s",state->src,suffix);
	 if(check_card_s(hdr,cbuff,keyword,val) == 0) {
	    schema_mismatch++;
	 }
	 
	 if(schema_mismatch && strcmp(key, "END") == 0) {
	    saved_state = state;
	    state = &stop;
	    break;
	 }
/*
 * Does the schema match? If not, try to patch the machine. If there are
 * missing fields in the file replace those states in the machine with NOPs;
 * if there are extra fields in the file add a state to seek over them.
 *
 * The case where there are fields in the schema which don't correspond
 * to any in the file is currently unfixable, and we give up. XXX Sort of works
 */
	 if(schema_mismatch) {
	    const int nreread = (tzero == 0 ? 2 : 4); /* number of cards
							 to read again */
	    FITSB_STATE *saved = state;
	    FITSB_STATE *synched = NULL; /* re-synched state */
	    n_mismatch++;
	    /*
	     * try searching forward for the desired element
	     */
	    for(; state->op != FITSB_STOP; state++) {
	       if(strcmp(state->src, value) == 0) { /* found it */
		  synched = state;
		  break;
	       }
	    }
	    state = saved;
	    /*
	     * did we find the desired schema?
	     */
	    if(synched != NULL) {	/* extra fields in schema */
	       shAssert(state->op == FITSB_WRITE);
	       shAssert(state != synched);

	       rowlen -= state->i;
	       do {
		  if(state->op == FITSB_WRITE) {
		     state->flags |= FITSB_MISSING_IN_FILE;
		  }
#if defined(DEC_BYTE_ORDER)
		  else if(state->op == FITSB_SWAB ||
			  state->op == FITSB_SWAB4 ||
			  state->op == FITSB_SWAB8) {
		     state->flags |= FITSB_MISSING_IN_FILE;
		  }
#endif
	       } while(++state != synched);
		  
	       ncard -= nreread;
	       if(lseek(fil->fd,-nreread*80,SEEK_CUR) == -1) {
		  shErrStackPushPerror("Cannot skip back over card");
		  myHdrDel(myhdr);
		  return(-1);
	       }
	    } else {			/* extra data in the file
					   or extra field in schema */
	       if(rowlen > naxis1) {	/* out of fields in the file */
		  state = add_state(machine[state_ind], state,
				    FITSB_STOP, state->i);
	       } else if(naxis1 > rowlen) { /* too many fields in file*/
		  ;
	       }
	       state = add_state(machine[state_ind], state,
				 FITSB_SEEK, state->i);

	       colno++;
	    }
	    
	    continue;
	 }
	 
	 if(state->sch_elem->nelem != NULL) {
	    int dims[10];		/* we have to reverse the order of
					   the dimensions, so save them here */
	    ptr = strcpy(val,state->sch_elem->nelem);
	    for(i = 0;i < 10;i++) {
	       dims[i] = atoi(ptr);
	       if((ptr = strchr(ptr,' ')) == NULL) {
		  i++;			/* we need to -- it */
		  break;
	       }
	       ptr++;
	    }
	    if(i > 1) {			/* skip 1-d arrays */
	       strcpy(val,"(");
	       for(i--;i >= 0;i--) {
		  sprintf(&val[strlen(val)],"%d%s",dims[i],
			  (i > 0 ? "," : ""));
	       }
	       strcat(val,")");
	       shAssert(strlen(val) < sizeof(val));
		  
	       sprintf(keyword,"TDIM%d",colno);
	       if(tdim_value[0] == '\0') { /* not read yet */
		  check_card_s(hdr,cbuff,keyword,val);
		  ncard++;
	       } else {
		  for(i = strlen(tdim_value) - 1;
		      i >= 0 && tdim_value[i] == ' '; i--) {
		     tdim_value[i] = '\0';
		  }
		  if(strcmp(tdim_value, val) != 0) {
		     shError("Expected value \"%s\" for keyword "
			     "\"%s\", saw \"%s\"", val, tdim_value, value);
		  }
	       }
	    }
	 }
	 
	 colno++;
	 state++;
	 break;
       case FITSB_ICONST:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,
				    fil->istacksize*sizeof(int));
	 }
	 fil->istack[islev++] = state->i;
	 state++;
	 break;
       case FITSB_LOOP:
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,
				   fil->stacksize*sizeof(void *));
	 }
	 fil->stack[slev++] = state + 1;
	 state++;
	 break;
       case FITSB_ENDLOOP:
	 shAssert(slev > 0 && islev > 0);
	 if(--fil->istack[islev - 1] > 0) {
	    state = fil->stack[slev - 1];
	    continue;
	 }
	 slev--; islev--;
	 state++;
	 break;
       default:
	 state++;	 
	 break;
      }
   }
   if(n_mismatch != 0) {
      if(state == &stop) {		/* a forced STOP */
	 int nextra = naxis1 - rowlen;
	 state = saved_state;
	 shErrStackPush("Header disagrees with that derived from "
			"schema for %s", type);
	 if(nextra > 0) {
	    shErrStackPush("There are %d extra bytes per row in the file "
			   "for type %s",
			   nextra, type);
	    state = add_state(machine[state_ind], state, FITSB_SEEK, nextra);
	 } else {
	    shErrStackPush("There are %d extra bytes per object "
			   "in the schema for type %s", -nextra, type);
	    if(state->i == -nextra) {
	       state->flags |= FITSB_MISSING_IN_FILE; /* we can handle this */
	    } else {
	       myHdrDel(myhdr);
	       return(-1);		/* too much work to handle */
	    }
	 }
	 rowlen += nextra;	       
	 
	 state = &stop;			/* resignal emergency stop */
      } else {
	 shErrStackPush("Header disagrees with that derived from "
			"schema for %s", type);
	 shErrStackPush("Patched schema for %s to match file; continuing",
			type);
      }
      if(!quiet) {
	 shErrStackPrint(stderr);
	 shErrStackClear();
      }
   }
/*
 * We opened two file descriptors for the file, so now we can set one of them
 * to the start of the heap
 */
   if(fil->heapfd < 0) {
      shErrStackPush("phFitsTblHdrRead: heap file descriptor is not open");
      myHdrDel(myhdr);
      return(-1);
   }

   if((fil->heap_start = lseek(fil->fd,0,SEEK_CUR)) == -1) {
      shErrStackPushPerror("phFitsTblHdrRead: "
			   "cannot find where file pointer is");
      myHdrDel(myhdr);
      return(-1);
   }
   fil->heap_end = fil->heap_start + naxis1*naxis2 + pcount; /* end of heap */

   if(fil->heap_end%FITSIZE != 0) {
      int extra = FITSIZE - fil->heap_end%FITSIZE;
      fil->heap_end += extra;
   }

   fil->heap_start += theap;		/* advance to start of heap */
   if(hblseek(fil->heapfd,fil->heap_start,SEEK_SET) == -1) {
      shErrStackPushPerror("phFitsTblHdrRead: cannot seek to start of heap");
      myHdrDel(myhdr);
      return(-1);
   }
/*
 * We're done with reading the header, so we are free to optimise, and
 * replace STOPs by RETs for all but the starting type
 */
   for(i = 0; i < NTYPE; i++) {
      if(machine[i] == NULL) continue;

      if(fil->omachine[i] != NULL) {
	 del_machine(fil->omachine[i]);
      }
      
      fil->omachine[i] = optimise(machine[i]);
      fil->omachine[i]->states[fil->omachine[i]->nstate - 1].op =
				(i == fil->start_ind) ? FITSB_STOP : FITSB_RET;
   }

   fil->machine = fil->omachine;
/*
 * We processed the header OK, so allocate the row buffer
 */
   if(naxis1 != rowlen) {
      shErrStackPush("Table has %d bytes per row; schema predicts %d",
								naxis1,rowlen);
      warning = 1;
   }
   fil->rowlen = rowlen;

   fil->row = shMalloc(rowlen);
   fil->nrow = naxis2;

   if(nrow != NULL) {
      *nrow = naxis2;
   }

   myHdrDel(myhdr);
   return(warning ? 1 : 0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Start a FITS binary table; the header to the file should already have
 * been written, and now we have to figure out and write the header to the
 * table
 *
 * If the HDR is non-NULL, it will be written to the file's
 *
 * Return:
 *	0	on success
 *	1	on successful completion with warnings
 *     -1	on failure
 */
int
phFitsBinTblHdrWrite(
		     PHFITSFILE *fil,	/* file descriptor for (opened) file */
		     char *type,	/* the type we'll be writing */
		     HDR *hdr		/* a header to write to the file */
		     )
{
   int colno;				/* number of current column */
   int i;
   FILE *heap;				/* FILE for temporary heap file */
   char keyword[20];			/* keyword on a card */
   int ncard;				/* number of cards in this record */
   int nwrite[NTYPE];			/* how many times has this struct
					   been written? */
   int rowlen;				/* number of bytes in a row */
   int slev = 0;			/* stack level */
   int islev = 0;			/* integer stack level */
   char *ptr;				/* scratch pointer */
   FITSB_STATE *state;			/* state of machine */
   int state_ind;			/* index in machine[], nwrite[], etc.
					   of the current machine */
   char suffix[10];			/* suffix for TTYPE keys */
   unsigned tzero;			/* TZERO, only used for unsigned data*/
   char val[40];			/* value of a card */
   int warning = 0;			/* should we return a warning status?*/
/*
 * Check that we know about the type we are being given
 */
   fil->start_ind = -1;
   for(i = 0; i < NTYPE; i++) {
      if(machine[i] == NULL || machine[i] == (MACHINE *)1) continue;

      if(strcmp(machine[i]->type,type) == 0) {
	 fil->start_ind = i;
	 break;
      }
   }
   if(fil->start_ind == -1) {
      shErrStackPush("No SCHEMATRANS is known for %s",type);
      return(-1);
   }
   fil->type = shTypeGetFromName(type);
/*
 * Open the temporary heap file
 */
   if(fil->heapfd >= 0) hbclose(fil->heapfd);

   if((heap = phTmpfile()) == NULL) {
      shErrStackPushPerror("phFitsTblHdrWrite: "
			   "Can't open temporary heap file");
      return(-1);
   } else {
      fil->heapfd = dup(fileno(heap));	/* we dup it, so that we can close */
      fclose(heap);			/*        the FILE and keep the fd */
   }
/*
 * Write header
 */
   if((fil->hdr_start = lseek(fil->fd,0,SEEK_CUR)) == -1) {
      shErrStackPushPerror("Cannot set seek pointer to start of header");
      return(-1);
   }

   ncard = 0;
   ncard = write_card_s(fil->fd,ncard,record,"XTENSION","BINTABLE","");
   ncard = write_card_d(fil->fd,ncard,record,"BITPIX",8,"");
   ncard = write_card_d(fil->fd,ncard,record,"NAXIS",2,"");
   ncard = write_card_d(fil->fd,ncard,record,"NAXIS1",0,"");
   ncard = write_card_d(fil->fd,ncard,record,"NAXIS2",0,"");
   ncard = write_card_d(fil->fd,ncard,record,"PCOUNT",0,"");
   ncard = write_card_d(fil->fd,ncard,record,"GCOUNT",1,"");
   ncard = write_card_d(fil->fd,ncard,record,"TFIELDS",0,"");
   ncard = write_card_d(fil->fd,ncard,record,"THEAP",0,"");
   if(hdr != NULL) {
      for(i = 0;hdr->hdrVec[i] != NULL;i++) {
	 ncard = set_card(fil->fd,ncard,record,hdr->hdrVec[i]);
      }
   }
/*
 * `Link' the machines, i.e. set the FITSB_SUB parameters properly.
 * For this first stage (writing the headers) we interpret STOP
 * instructions as RETs
 */
   for(i = 0; i < NTYPE; i++) {
      nwrite[i] = 0;			/* number of times this type's been
					   written */
   }

   link_machines();
/*
 * Now use those machines to generate fits table headers. Because we may
 * write the same structure several times, we have to be careful to make
 * the TTYPE values unique. We do this by appending a number to the string
 * specified in the SCHEMATRANS, and incrementing it every time that we
 * write the structure. The number of uses is stored in the array nwrite.
 *
 * There is a further problem, in that we need to keep track of which machine
 * we are currently in. We do this by pushing the current machine's index
 * onto the integer stack.
 */
   colno = 1;
   rowlen = 0;
   state_ind = 0;			/* index of current state */
   suffix[0] = '\0';

   state = machine[fil->start_ind]->states;
   while(!(slev == 0 && state->op == FITSB_STOP)) {
      switch(state->op) {
       case FITSB_NOP:
	 state++;
	 break;
       case FITSB_SUB:
	 shAssert(state->i >= 0);
	 nwrite[state->i]++;		/* we're writing another instance of a
					   type, so increment the use counter*/
	 if(nwrite[state->i] > 1) {
	    sprintf(suffix,"%d",nwrite[state->i]);
	 } else {
	    suffix[0] = '\0';
	 }
	 
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,fil->stacksize*sizeof(void *));
	 }
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }
	 
	 fil->istack[islev++] = state_ind;
	 fil->stack[slev++] = state + 1;
	 state_ind = state->i;
	 state = machine[state->i]->states;
	 break;
       case FITSB_STOP:			/* a STOP, but treat as a RET */
       case FITSB_RET:
	 shAssert(slev > 0 && islev > 0);
	 state = fil->stack[--slev];
	 state_ind = fil->istack[--islev];
	 if(nwrite[state_ind] > 1) {
	    sprintf(suffix,"%d",nwrite[state_ind]);
	 } else {
	    suffix[0] = '\0';
	 }
	 break;
       case FITSB_SEEK:
	 state++;
	 break;
       case FITSB_WRITE:
       case FITSB_HEAP:
       case FITSB_HEAP_MAIN:
/*
 * See what we are dealing with
 */
	 tzero = 0;
	 if(state->sch->type == ENUM) {
	    sprintf(val,"%dJ",state->i/4);
	 } else if(state->sch->type == PRIM) {
	    const char *type_name = state->sch->name; /* canonical form */

	    if(strcmp(type_name,"SHORT") == 0 ||
	       				     strcmp(type_name,"USHORT") == 0) {
	       
	       if(*type_name == 'U') {
		  tzero = (unsigned short)(1 << (15));
	       }

	       sprintf(val,"%dI",state->i/2);
	    } else if(strcmp(type_name,"INT") == 0 ||
		      strcmp(type_name,"UINT") == 0 ||
		      strcmp(type_name,"LONG") == 0 ||
		      strcmp(type_name,"ULONG") == 0) {
	       if(*type_name == 'U') {
		  tzero = 0x80000000;
	       }
	       
	       sprintf(val,"%dJ",state->i/4);
	    } else if(strcmp(type_name,"FLOAT") == 0) {
	       sprintf(val,"%dE",state->i/4);
	    } else if(strcmp(type_name,"DOUBLE") == 0) {
	       sprintf(val,"%dD",state->i/8);
	    } else if(strcmp(type_name,"CHAR") == 0) {
	       sprintf(val,"%dA",state->i);
	    } else if(strcmp(type_name,"UCHAR") == 0) {
	       sprintf(val,"%dB",state->i);
	    } else if(strcmp(type_name,"STR") == 0 ||
		      strcmp(type_name,"LOGICAL") == 0) {
	       shErrStackPush("Ignoring %s: %s",type_name,state->name);
	       warning = 1;
	       state++;
	       continue;
	    } else {
	       shErrStackPush("Saw an unknown PRIM type %s for %s",
			      type_name, state->name);
	       warning = 1;
	       state++;
	       continue;
	    }
	 } else {
	    if(state->op != FITSB_HEAP && state->op != FITSB_HEAP_MAIN) {
	       if(state->sch->type == STRUCT) {
		  shFatal("STRUCT %s cannot appear as a WRITE op",state->name);
	       } else {
		  shErrStackPush("I don't know what to do with UNKNOWN "
				 "type for %s's schema", state->name);
		  warning = 1;
		  state++;
		  continue;
	       }
	    }
	 }
/*
 * Is the data destined for the heap?
 */
	 if(state->op == FITSB_HEAP || state->op == FITSB_HEAP_MAIN) {
	    if(state->op == FITSB_HEAP) {
	       shAssert(islev > 0);
	       islev--;
	    }

	    if(state->sch->type == PRIM) {
	       sprintf(val,"1P%c(%d)",
		       get_tform_char(state->sch->name), state->i);
	    } else {
	       sprintf(val,"1PB(%d)", state->i);
	    }
	    rowlen += 8;
	    tzero = 0;			/* no TZERO card even if unsigned */
	 } else {
	    rowlen += state->i;
	 }
	   
	 sprintf(keyword,"TFORM%d",colno);
	 ncard = write_card_s(fil->fd,ncard,record,
			      			 keyword,val,state->sch->name);
	 if(tzero != 0) {
	    sprintf(keyword,"TZERO%d",colno);
	    ncard = write_card_d(fil->fd,ncard,record,
					     keyword,tzero,"Data is unsigned");
	    sprintf(keyword,"TSCAL%d",colno);
	    ncard = write_card_d(fil->fd,ncard,record,keyword,1,"");
	 }
/*
 * Then the TTYPE card
 */
	 sprintf(keyword,"TTYPE%d",colno);
	 sprintf(val,"%s%s",state->src,suffix);
	 ncard = write_card_s(fil->fd,ncard,record,keyword,val,state->name);
/*
 * And maybe the TDIM card
 */
	 if(state->sch_elem->nelem != NULL) {
	    int dims[10];		/* we have to reverse the order of
					   the dimensions, so save them here */
	    ptr = strcpy(val,state->sch_elem->nelem);
	    for(i = 0;i < 10;i++) {
	       dims[i] = atoi(ptr);
	       if((ptr = strchr(ptr,' ')) == NULL) {
		  i++;			/* we need to -- it */
		  break;
	       }
	       ptr++;
	    }
	    if(i > 1) {			/* no TDIM card for 1-d arrays */
	       strcpy(val,"(");
	       for(i--;i >= 0;i--) {
		  sprintf(&val[strlen(val)],"%d%s",dims[i],(i > 0 ? "," : ""));
	       }
	       strcat(val,")");
	       shAssert(strlen(val) < sizeof(val));
	       
	       sprintf(keyword,"TDIM%d",colno);
	       ncard = write_card_s(fil->fd,ncard,record,keyword,val,"");
	    }
	 }

	 colno++;
	 state++;
	 break;
       case FITSB_ICONST:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }
	 fil->istack[islev++] = state->i;
	 state++;
	 break;
       case FITSB_LOOP:
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,fil->stacksize*sizeof(void *));
	 }
	 fil->stack[slev++] = state + 1;
	 state++;
	 break;
       case FITSB_ENDLOOP:
	 shAssert(slev > 0 && islev > 0);
	 if(--fil->istack[islev - 1] > 0) {
	    state = fil->stack[slev - 1];
	    continue;
	 }
	 slev--; islev--;
	 state++;
	 break;
       default:
	 state++;	 
	 break;
      }
   }
/*
 * And finally the END card
 */
   ncard = write_card_s(fil->fd,ncard,record,"END","","");
   while(ncard%36 != 0) {
      ncard = write_card_s(fil->fd,ncard,record,"","","");
   }
/*
 * We're done with writing the header, so we are free to optimise, and
 * replace STOPs by RETs for all but the starting type
 */
   for(i = 0; i < NTYPE; i++) {
      if(machine[i] == NULL) continue;

      if(fil->omachine[i] != NULL) {
	 del_machine(fil->omachine[i]);
      }

      fil->omachine[i] = optimise(machine[i]);
      fil->omachine[i]->states[fil->omachine[i]->nstate - 1].op =
				(i == fil->start_ind) ? FITSB_STOP : FITSB_RET;
   }

   fil->machine = fil->omachine;
/*
 * We know the length of a row, so allocate an i/o buffer
 */
   fil->row = shMalloc(rowlen);
   fil->rowlen = rowlen;
/*
 * We know the number of fields; go back and fix the header
 */
   if((fil->hdr_start = lseek(fil->fd,fil->hdr_start,SEEK_SET)) == -1) {
      shErrStackPushPerror("Cannot seek to start of header");
      return(-1);
   }

   if(read(fil->fd,record,FITSIZE) != FITSIZE) {
      shErrStackPushPerror("Cannot read header record");
      return(-1);
   }
   if((fil->hdr_start = lseek(fil->fd,fil->hdr_start,SEEK_SET)) == -1) {
      shErrStackPushPerror("Cannot seek to start of header a second time");
      return(-1);
   }
   
   ncard = 0;
   ncard++;				/* XTENSION = BINTABLE */
   ncard++;				/* BITPIX = 8 */
   ncard++;				/* NAXIS = 2 */
   ncard = write_card_d(fil->fd,ncard,record,"NAXIS1",rowlen,"");
   ncard++;				/* NAXIS2 = ?? */
   ncard++;				/* PCOUNT */
   ncard++;				/* GCOUNT */
   ncard = write_card_d(fil->fd,ncard,record,"TFIELDS",colno - 1,"");

   if(write(fil->fd,record,FITSIZE) != FITSIZE) {
      shErrStackPush("Cannot write header record");
      warning = 1;
   }
/*
 * and seek to end of the file
 */
   if((fil->hdr_end = lseek(fil->fd,0,SEEK_END)) == -1) {
      shErrStackPushPerror("Cannot seek to end of header");
      return(-1);
   }

   return(warning ? 1 : 0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Seek in a FITS table. The offset is measured in rows, and the whence
 * variable can have the values:
 *	0	Seek to the nth row of the table (0-indexed)
 *      1       Seek forward n rows
 *	2	Seek to the nth row beyond the end of the table (n <= 0)
 *
 * Return 0, or -1 in case of error
 */
int
phFitsBinTblRowSeek(
		    PHFITSFILE *fil,	/* file descriptor for (opened) file */
		    int n,		/* how much to seek */
		    int whence		/* where to seek from */
		    )
{
   off_t current;			/* current position in file */
   
   if(fil == NULL) {
      shErrStackPushPerror("phFitsBinTblRowSeek: File is not opened");
      return(-1);
   }

   fil->old_fd_pos = fil->old_heapfd_pos = -1; /* invalidate */

   if(fil->mode != 0) {
      shErrStackPushPerror("phFitsBinTblRowSeek: "
			   "file must be opened for read");
      return(-1);
   }

   if(whence == 2) {
      n = fil->nrow - n;
      whence = 0;
   }

   if(whence == 0) {
      if(n > fil->nrow) {
	 shErrStackPushPerror("phFitsBinTblRowSeek: "
			      "Attempt to seek beyond end of table (%d > %d)",
			      n, fil->nrow);
	 return(-1);
      }
      if(lseek(fil->fd,fil->hdr_end + n*fil->rowlen,SEEK_SET) == -1) {
	 shErrStackPushPerror("phFitsBinTblRowSeek: "
			      "cannot restore seek to row %d",n);
	 return(-1);
      }
   } else if(whence == 1) {
      current = lseek(fil->fd, 0, SEEK_CUR);

      if(current == -1) {
	 shErrStackPushPerror("phFitsBinTblRowSeek: "
			      "cannot seek to find current position");
	 return(-1);
      }

      if(current + n*fil->rowlen < fil->hdr_end) {
	 shErrStackPushPerror("phFitsBinTblRowSeek: cannot seek %d rows "
			      "(file pointer would be in header)",n);
	 return(-1);
      }
      if(lseek(fil->fd,n*fil->rowlen,SEEK_CUR) == -1) {
	 shErrStackPushPerror("phFitsBinTblRowSeek: "
			      "cannot seek %d rows",n);
	 return(-1);
      }
   } else {
      shErrStackPushPerror("phFitsBinTblRowSeek: "
			      "unknown value for whence: %d",whence);
	 return(-1);
   }

   current = lseek(fil->fd, 0, SEEK_CUR);
   if(current == -1) {
      shErrStackPushPerror("phFitsBinTblRowSeek: "
			   "cannot seek to find current position");
      return(-1);
   }

   return((current - fil->hdr_end)/fil->rowlen);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Read a row from a FITS binary table
 */
int
phFitsBinTblRowRead(
		    PHFITSFILE *fil,	/* file descriptor for (opened) file */
		    void *vdata		/* where to put the line read */
		    )
{
   int byte_data;			/* number of bytes per data item;
					   needed to byteswap heap */
   char *data = vdata;			/* we need a char * pointer
					   so that pointer operations work */
   const int hstart = fil->heap_start;	/* start of heap */
   int heap_nelem;			/* number of elements of data in heap*/
   int heap_offset;			/* starting offset of data in heap */
   int nbyte;				/* no. of bytes to write to heap */
   char *rptr = fil->row;		/* just a pointer to row */
   FITSB_STATE *state = fil->machine[fil->start_ind]->states;
   int slev = 0;			/* stack level */
   int islev = 0;			/* integer stack level */
   int warning = 0;			/* should we return a warning status?*/

   shAssert(fil->row != NULL);
/*
 * save the file pointers at the start of this row, then read it
 */
   if((fil->old_fd_pos = lseek(fil->fd,0,SEEK_CUR)) == -1) {
      shErrStackPushPerror("phFitsBinTblRowRead: "
		     "cannot save main table file position");
      return(-1);
   }
   if((fil->old_heapfd_pos = lseek(fil->heapfd,0,SEEK_CUR)) == -1) {
      shErrStackPushPerror("phFitsBinTblRowRead: "
			   "cannot save heap file position");
      return(-1);
   }
   
   if(read(fil->fd,fil->row,fil->rowlen) != fil->rowlen) {
      shErrStackPushPerror("phFitsBinTblRowRead: cannot read row");
      return(-1);
   }

   for(;;) {
      switch (state->op) {
       case FITSB_STOP:			/* all done */
	 shAssert(rptr == fil->row + fil->rowlen);
	 return(warning ? 1 : 0);
       case FITSB_NOP:
	 state++;
	 break;
       case FITSB_HEAP:
/*
 * The heap offsets are in the main part of the table
 */
#if defined(DEC_BYTE_ORDER)
	 swap(rptr, rptr, 2*sizeof(int), BYTE4);
#endif
	 memcpy(&heap_nelem,rptr,sizeof(int)); rptr += sizeof(int);
	 memcpy(&heap_offset,rptr,sizeof(int)); rptr += sizeof(int);

	 shAssert(islev > 2);
	 byte_data = fil->istack[--islev]; /* bytes per data item */
	 if(read_all_heap_as_bytes) {
	    nbyte = heap_nelem*read_all_heap_as_bytes;
	 } else {
	    nbyte = heap_nelem*byte_data;
	 }

	 islev -= 2;			/* drop the (unnecessary) offsets */
	 if(nbyte > 0) {
	    if(data == NULL) {
	       shErrStackPush("phFitsBinTblRowRead: "
			      "attempt to read NULL pointer");
	       warning = 1;
	       nbyte = 0;
	    } else {
	       heap_offset += fil->heap_start;
	       if(hblseek(fil->heapfd,heap_offset,SEEK_SET) == -1) {
		  shErrStackPushPerror("phFitsBinTblRowRead: "
				 "error seeking in heap");
		  return(-1);
	       }
	       if(hbread(fil->heapfd,data,nbyte,byte_data) != nbyte) {
		  shErrStackPushPerror("phFitsBinTblRowRead: "
				 "error reading %dbytes",nbyte);
		  return(-1);
	       }
	    }
	 }

	 state++;
	 break;
       case FITSB_HEAP_MAIN:
	 shAssert(islev > 1);
	 islev -= 2;
	 state++;
	 break;
       case FITSB_HEAP_SEEK:
#if defined(DEC_BYTE_ORDER)
	 swap(rptr, rptr, 2*sizeof(int), BYTE4);
#endif
	 memcpy(&heap_nelem,rptr,sizeof(int)); rptr += sizeof(int);
	 memcpy(&heap_offset,rptr,sizeof(int)); rptr += sizeof(int);

	 heap_offset += fil->heap_start;
	 if(hblseek(fil->heapfd,heap_offset,SEEK_SET) == -1) {
	    shErrStackPushPerror("phFitsBinTblRowRead: error seeking in heap");
	    return(-1);
	 }
	 
	 state++;
	 break;
       case FITSB_SEEK:
	 rptr += state->i;
	 state++;
	 break;
       case FITSB_WRITE:
	 if(data == NULL) {		/* we cannot read, but we can skip
					   the space in the file */
	    shErrStackPush("phFitsBinTblRowRead: "
			   "attempt to read into NULL pointer");
	    warning = 1;
	    if(!(state->flags & FITSB_MISSING_IN_FILE)) {
	       rptr += state->i;
	    }
	 } else {
	    if(!(state->flags & FITSB_MISSING_IN_FILE)) {
	       memcpy(data,rptr,state->i); rptr += state->i;
	    }
	    data += state->i;
	 }
	 state++;
	 break;
       case FITSB_INCR:
	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowRead: "
			   "attempt to increment NULL pointer");
	    warning = 1;
	 } else {
	    data += state->i;
	 }
	 state++;
	 break;
       case FITSB_DEREF:
	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowRead: "
			   "attempt to dereference a NULL pointer");
	    warning = 1;
	 } else {
	    data = *(char **)data;
	 }
	 state++;
	 break;
       case FITSB_SUB:
	 shAssert(state->i >= 0);
	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowRead: NULL pointer for SUB %s",
			   state->name);
	    warning = 1;
	 }
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,fil->stacksize*sizeof(void*));
	 }
	 fil->stack[slev++] = state + 1;
	 state = fil->machine[state->i]->states;
	 break;
       case FITSB_RET:
	 state = fil->stack[--slev];
	 break;
       case FITSB_LOOP:
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,fil->stacksize*sizeof(void *));
	 }
	 fil->stack[slev++] = state + 1;
	 state++;
	 break;
       case FITSB_ENDLOOP:
	 shAssert(slev > 0 && islev > 0);
	 if(--fil->istack[islev - 1] > 0) {
	    state = fil->stack[slev - 1];
	    continue;
	 }
	 slev--; islev--;
	 state++;
	 break;
       case FITSB_SAVE:
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,fil->stacksize*sizeof(void *));
	 }
	 fil->stack[slev++] = data;
	 state++;
	 break;
       case FITSB_RESTORE:
	 data = fil->stack[--slev];
	 state++;
	 break;
       case FITSB_ICONST:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }
	 fil->istack[islev++] = state->i;
	 state++;
	 break;
       case FITSB_HEAP_SIZE:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }
	 fil->istack[islev++] = hblseek(fil->heapfd,0,SEEK_CUR) - hstart;
	 state++;
	 break;
       case FITSB_STRING:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }
	 data += state->i;		/* back to the proper address */
	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowRead: attempt to process a "
			   				       	"NULL string");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    if(*(char **)data != NULL) {
	       shFree(*(char **)data);
	    }
	    heap_nelem *= BYTE1;	/* OK, so this is a NOP */
	    *(char **)data = shMalloc(heap_nelem + 1);
	    data = *(char **)data;
	    data[heap_nelem] = '\0';

	    if(hbread(fil->heapfd,data,heap_nelem,BYTE1) != heap_nelem) {
	       shErrStackPushPerror("phFitsBinTblRowRead: "
				    "error reading %dbytes",heap_nelem);
	       return(-1);
	    }
	    fil->istack[islev++] = heap_nelem;
	 }
	 state++;
	 break;
       case FITSB_ATLAS_IMAGE:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }

	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowRead: attempt to process a "
						   "(ATLAS_IMAGE *)NULL");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    ATLAS_IMAGE *ai = (ATLAS_IMAGE *)data;
	    unsigned char *buff;	/* buffer for flattened atlas image */
	    int nbyte;

	    heap_nelem *= BYTE1;	/* OK, so this is a NOP */
	    buff = shMalloc(heap_nelem);
	    if(hbread(fil->heapfd,buff,heap_nelem,BYTE1) != heap_nelem) {
	       shErrStackPushPerror("phFitsBinTblRowRead: "
				    "error reading %dbytes", heap_nelem);
	       shFree(buff);
	       return(-1);
	    }
	    nbyte = heap_nelem;
	    (void)phAtlasImageInflate(ai, buff, &nbyte);
	    shFree(buff);

	    fil->istack[islev++] = heap_nelem;
	 }
	 state++;
	 break;
       case FITSB_MASK_ROWS:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }

	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowRead: attempt to process a "
							 "(MASK *)NULL->rows");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    int ir;
	    MASK *mask = (MASK *)data;
	    int ncol;
/*
 * We have to allocate a large enough mask. We can do this
 * with a few private MASK calls
 */
	    shAssert(mask->rows == NULL);

	    ncol = mask->ncol;		/* trashed by p_shMaskVectorGet() */
	    p_shMaskVectorGet(mask, mask->nrow);
	    mask->ncol = ncol;
	    p_shMaskRowsGet(mask, mask->nrow, mask->ncol);

	    heap_nelem *= BYTE1;	/* OK, so this is a NOP */
	    shAssert(heap_nelem == mask->nrow*mask->ncol);
	    for(ir=0;ir < mask->nrow;ir++) {
	       if(hbread(fil->heapfd,mask->rows[ir],mask->ncol,BYTE1)
							       != mask->ncol) {
		  shErrStackPushPerror("phFitsBinTblRowRead: "
				       "error reading %dbytes", mask->ncol);
		  return(-1);
	       }
	    }
	    fil->istack[islev++] = mask->ncol*mask->nrow;
	 }
	 state++;
	 break;
       case FITSB_OBJMASK_SPAN:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }

	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowRead: attempt to process a "
							 "(OBJMASK *)NULL->s");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    int nbyte;
	    OBJMASK *om = (OBJMASK *)data;
	    phObjmaskRealloc(om,om->nspan);

	    nbyte = om->nspan*sizeof(om->s[0]);
	    shAssert(nbyte == heap_nelem);
	    if(hbread(fil->heapfd,om->s,nbyte,BYTE2) != nbyte) {
	       shErrStackPushPerror("phFitsBinTblRowRead: "
				    "error reading %dbytes", nbyte);
	       return(-1);
	    }
	    fil->istack[islev++] = nbyte;
	 }
	 state++;
	 break;
       case FITSB_REGION_ROWS:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }

	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowRead: attempt to process a "
						       "(REGION *)NULL->ROWS");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    int ir;
	    int ncol;
	    REGION *reg = (REGION *)data;
	    void **rows;		/* rows of data */
	    int size = 0;		/* size of a data item */
/*
 * We have to allocate a large enough region, of the given type. We can do this
 * with a few private REGION calls
 */
	    shAssert(p_shRegPtrGet(reg,&size) == NULL);

	    ncol = reg->ncol;		/* it's trashed by p_shRegVectorGet()*/
	    p_shRegVectorGet(reg, reg->nrow, reg->type);
	    reg->ncol = ncol;
	    p_shRegRowsGet(reg, reg->nrow, reg->ncol, reg->type);

	    rows = p_shRegPtrGet(reg,&size);
	    byte_data = size;
	    size *= reg->ncol;

#if 0
	    shAssert(heap_nelem == \
		     reg->nrow*reg->ncol*\
		     (read_all_heap_as_bytes == 1 ? byte_data : 1));
#endif
	    for(ir=0;ir < reg->nrow;ir++) {
	       if(hbread(fil->heapfd,rows[ir],size,byte_data) != size) {
		  shErrStackPushPerror("phFitsBinTblRowRead: "
						 "error reading %dbytes",size);
		  return(-1);
	       }
	    }
	    fil->istack[islev++] = heap_nelem;
	 }
	 state++;
	 break;
#if defined(DEC_BYTE_ORDER)
/*
 * Both of these operators swap the data just _before_ the current position
 * of rptr, so first emit the i/o op, then the SWAP instruction
 */
       case FITSB_SWAB:
	 if(data == NULL || (state->flags & FITSB_MISSING_IN_FILE)) {
	    ;				/* NULL data is already reported */
	 } else {
	    char *ptr = data - state->i; /* start of buffer to swap */
	    char tmp;
   
	    for(;ptr < data - 1;ptr += 2) {
	       tmp = ptr[0]; ptr[0] = ptr[1]; ptr[1] = tmp;
	    }
	 }
	 state++;
	 break;
       case FITSB_SWAB4:
	 if(data == NULL || (state->flags & FITSB_MISSING_IN_FILE)) {
	    ;				/* NULL data is already reported */
	 } else {
	    char *ptr = data - state->i; /* start of buffer to swap */
	    char tmp;
   
	    for(;ptr < data - 3;ptr += 4) {
	       tmp = ptr[0]; ptr[0] = ptr[3]; ptr[3] = tmp;
	       tmp = ptr[1]; ptr[1] = ptr[2]; ptr[2] = tmp;
	    }
	 }
	 state++;
	 break;
       case FITSB_SWAB8:
	 if(data == NULL || (state->flags & FITSB_MISSING_IN_FILE)) {
	    ;				/* NULL data is already reported */
	 } else {
	    char *ptr = data - state->i; /* start of buffer to swap */
	    char tmp;
   
	    for(;ptr < data - 7;ptr += 8) {
	       tmp = ptr[0]; ptr[0] = ptr[7]; ptr[7] = tmp;
	       tmp = ptr[1]; ptr[1] = ptr[6]; ptr[6] = tmp;
	       tmp = ptr[2]; ptr[2] = ptr[5]; ptr[5] = tmp;
	       tmp = ptr[3]; ptr[3] = ptr[4]; ptr[4] = tmp;
	    }
	 }
	 state++;
	 break;
#endif
      }
   }

   return(0);				/* NOTREACHED */
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Seek back over the last row read
 */
int
phFitsBinTblRowUnread(
		      PHFITSFILE *fil	/* file descriptor for (opened) file */
		    )
{
   if(fil->old_fd_pos == -1 || fil->old_heapfd_pos == -1) {
      shErrStackPush("phFitsBinTblRowUnread: no saved file position");
      return(-1);
   }
   if(lseek(fil->fd,fil->old_fd_pos,SEEK_SET) == -1) {
      shErrStackPushPerror("phFitsBinTblRowUnread: "
		     "cannot restore main table file position");
      return(-1);
   }
   if(lseek(fil->heapfd,fil->old_heapfd_pos,SEEK_SET) == -1) {
      shErrStackPushPerror("phFitsBinTblRowUnread: "
		     "cannot restore main table file position");
      return(-1);
   }

   fil->old_fd_pos = fil->old_heapfd_pos = -1;
   
   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Write a row to a FITS binary table
 */
int
phFitsBinTblRowWrite(
		     PHFITSFILE *fil,	/* file descriptor for file */
		     void *vdata
		     )
{
   int byte_data = 0;			/* number of bytes per data item;
					   needed to byteswap heap */
   char *data = vdata;			/* we need a char * pointer
					   so that pointer operations work */
   int heap_offset;			/* starting offset of data in heap */
   int nbyte;				/* no. of bytes to write to heap */
   int heap_nelem;			/* no. of elements to write to heap */
   char *rptr = fil->row;		/* just a pointer to row */
   FITSB_STATE *state = fil->machine[fil->start_ind]->states;
   int slev = 0;			/* stack level */
   int islev = 0;			/* integer stack level */
   int warning = 0;			/* should we return a warning status?*/

   shAssert(fil->row != NULL);

   for(;;) {
      switch (state->op) {
       case FITSB_STOP:			/* all done; write row */
	 shAssert(rptr == fil->row + fil->rowlen);
	 
	 if(fil->heapfd >= 0) hbflush(fil->heapfd);
	 
	 fil->nrow++;
	 if(write(fil->fd,fil->row,fil->rowlen) != fil->rowlen) {
	    shErrStackPushPerror("Error writing row %d",fil->nrow);
	    return(-1);
	 }
	 return(warning ? 1 : 0);
       case FITSB_NOP:
	 state++;
	 break;
       case FITSB_HEAP:
       case FITSB_HEAP_MAIN:
/*
 * The bottom of the integer stack contains the number of bytes to write
 * to the heap; we'll do that first (unless we are only writing the main
 * table part as directed by HEAP_MAIN)
 */
	 if(state->op == FITSB_HEAP) {
	    shAssert(islev > 0);
	    byte_data = fil->istack[--islev]; /* bytes per data item */
	 }
	 shAssert(islev > 1);
	 heap_nelem = fil->istack[--islev];
	 heap_offset = fil->istack[--islev];
	 if(state->op == FITSB_HEAP && heap_nelem > 0) {
	    nbyte = heap_nelem*byte_data;
	    if(data == NULL) {
	       shErrStackPush("phFitsBinTblRowWrite: "
			      "attempt to write NULL pointer");
	       warning = 1;
	       heap_nelem = 0;
	    } else {
	       if(hbwrite(fil->heapfd,data,nbyte,byte_data) != nbyte) {
		  shErrStackPushPerror("phFitsBinTblRowWrite: "
				       "error writing %dbytes",nbyte);
		  return(-1);
	       }
	    }
	 }
/*
 * Now write the heap offsets into the main part of the table
 */
	 memcpy(rptr,&heap_nelem,sizeof(int));
	 memcpy(rptr + sizeof(int),&heap_offset,sizeof(int));
#if defined(DEC_BYTE_ORDER)
	 swap(rptr, rptr, 2*sizeof(int), BYTE4);
#endif
	 rptr += 2*sizeof(int);

	 state++;
	 break;
       case FITSB_HEAP_SEEK:
	 state++;
	 break;
       case FITSB_SEEK:
	 state++;
	 break;
       case FITSB_WRITE:
	 if(data == NULL) {		/* we cannot write, but we can use
					   up the space in the file */
	    shErrStackPush("phFitsBinTblRowWrite: "
			   "attempt to write NULL pointer");
	    warning = 1;
	    memset(rptr,'\0',state->i); rptr += state->i;
	 } else {
	    memcpy(rptr,data,state->i); rptr += state->i;
	    data += state->i;
	 }
	 state++;
	 break;
       case FITSB_INCR:
	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowWrite: "
			   "attempt to increment NULL pointer");
	    warning = 1;
	 } else {
	    data += state->i;
	 }
	 state++;
	 break;
       case FITSB_DEREF:
	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowWrite: attempt to dereference a "
			   				       "NULL pointer");
	    warning = 1;
	 } else {
	    data = *(char **)data;
	 }
	 state++;
	 break;
       case FITSB_SUB:
	 shAssert(state->i >= 0);
	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowWrite: NULL pointer for SUB %s",
			   state->name);
	    warning = 1;
	 }
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,fil->stacksize*sizeof(void*));
	 }
	 fil->stack[slev++] = state + 1;
	 state = fil->machine[state->i]->states;
	 break;
       case FITSB_RET:
	 state = fil->stack[--slev];
	 break;
       case FITSB_LOOP:
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,fil->stacksize*sizeof(void *));
	 }
	 fil->stack[slev++] = state + 1;
	 state++;
	 break;
       case FITSB_ENDLOOP:
	 shAssert(slev > 0 && islev > 0);
	 if(--fil->istack[islev - 1] > 0) {
	    state = fil->stack[slev - 1];
	    continue;
	 }
	 slev--; islev--;
	 state++;
	 break;
       case FITSB_SAVE:
	 if(slev >= fil->stacksize) {
	    fil->stacksize *= 2;
	    fil->stack = shRealloc(fil->stack,fil->stacksize*sizeof(void *));
	 }
	 fil->stack[slev++] = data;
	 state++;
	 break;
       case FITSB_RESTORE:
	 data = fil->stack[--slev];
	 state++;
	 break;
       case FITSB_ICONST:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }
	 fil->istack[islev++] = state->i;
	 state++;
	 break;
       case FITSB_HEAP_SIZE:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }
	 fil->istack[islev++] = hblseek(fil->heapfd,0,SEEK_END);
	 state++;
	 break;
       case FITSB_STRING:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }
	 data += state->i;		/* back to the proper address */
	 if(data == NULL || (data = *(char **)data) == NULL) {
	    shErrStackPush("phFitsBinTblRowWrite: attempt to process a "
			   				       	"NULL string");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    int len;			/* length of string */

	    len = strlen(data) + 1;
	    len = 4*((len - 1)/4 + 1);	/* round up to multiple of 4 bytes */
	    if(hbwrite(fil->heapfd,data,len,BYTE1) != len) {
	       shErrStackPushPerror("phFitsBinTblRowWrite: "
				    "error writing %dbytes", len);
	       return(-1);
	    }
	    fil->istack[islev++] = len;
	 }
	 state++;
	 break;
       case FITSB_ATLAS_IMAGE:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }

	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowWrite: attempt to process"
							"(ATLAS_IMAGE *)NULL");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    ATLAS_IMAGE *ai = (ATLAS_IMAGE *)data;
	    unsigned char *buff;	/* buffer to flatten ai into */
/*
 * call phAtlasImageFlatten twice; the first call returns an upper limit to the
 * size; the second the number of bytes actually required (after compression)
 */
	    nbyte = phAtlasImageFlatten(ai, NULL, 0);
	    buff = shMalloc(nbyte);
	    nbyte = phAtlasImageFlatten(ai, buff, nbyte);
	    heap_nelem = nbyte/BYTE1;	/* OK, so it's a NOP */

	    if(hbwrite(fil->heapfd, buff, nbyte, BYTE1) != nbyte) {
	       shErrStackPushPerror("phFitsBinTblRowWrite: "
					       "error writing %dbytes", nbyte);
	       shFree(buff);
	       return(-1);
	    }
	    shFree(buff);

	    fil->istack[islev++] = heap_nelem;
	 }
	 state++;
	 break;
       case FITSB_MASK_ROWS:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }

	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowWrite: attempt to process a "
			   				    "NULL MASK->rows");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    MASK *mask = (MASK *)data;
	    int row;

	    nbyte = mask->ncol*mask->nrow;
	    heap_nelem = nbyte/BYTE1;	/* OK, so it's a NOP */
	    for(row=0;row < mask->nrow;row++) {
	       if(hbwrite(fil->heapfd,mask->rows[row],mask->ncol,BYTE1) !=
		  						  mask->ncol) {
		  shErrStackPushPerror("phFitsBinTblRowWrite: "
				       "error writing %dbytes", mask->ncol);
		  return(-1);
	       }
	    }
	    fil->istack[islev++] = heap_nelem;
	 }
	 state++;
	 break;
       case FITSB_OBJMASK_SPAN:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }

	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowWrite: attempt to process a "
			   				  "NULL OBJMASK->s");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    OBJMASK *om = (OBJMASK *)data;

	    heap_nelem = nbyte = om->nspan*sizeof(om->s[0]);
	    
	    if(hbwrite(fil->heapfd,om->s,nbyte,BYTE2) != nbyte) {
	       shErrStackPushPerror("phFitsBinTblRowWrite: "
				       "error writing %dbytes", nbyte);
	       return(-1);
	    }
	    fil->istack[islev++] = heap_nelem;
	 }
	 state++;
	 break;
       case FITSB_REGION_ROWS:
	 if(islev >= fil->istacksize) {
	    fil->istacksize *= 2;
	    fil->istack = shRealloc(fil->istack,fil->istacksize*sizeof(int));
	 }

	 if(data == NULL) {
	    shErrStackPush("phFitsBinTblRowWrite: attempt to process a "
			   				  "NULL REGION->ROWS");
	    warning = 1;
	    fil->istack[islev++] = 0;
	 } else {
	    REGION *reg = (REGION *)data;
	    int row;
	    void **rows;		/* rows of data */

	    rows = p_shRegPtrGet(reg,&byte_data);
	    nbyte = reg->ncol*byte_data; /* size of one row of data */
	    heap_nelem = reg->nrow*reg->ncol;

	    for(row=0;row < reg->nrow;row++) {
	       if(hbwrite(fil->heapfd,rows[row],nbyte,byte_data) != nbyte) {
		  shErrStackPushPerror("phFitsBinTblRowWrite: "
				       "error writing %dbytes", nbyte);
		  return(-1);
	       }
	    }
	    fil->istack[islev++] = heap_nelem;
	 }
	 state++;
	 break;
#if defined(DEC_BYTE_ORDER)
/*
 * Both of these operators swap the data just _before_ the current position
 * of rptr, so first emit the WRITE then the SWAP instruction
 */
       case FITSB_SWAB:
	 {
	    char *ptr = rptr - state->i; /* start of buffer to swap */
	    char tmp;
   
	    for(;ptr < rptr - 1;ptr += 2) {
	       tmp = ptr[0]; ptr[0] = ptr[1]; ptr[1] = tmp;
	    }
	 }
	 state++;
	 break;
       case FITSB_SWAB4:
	 {
	    char *ptr = rptr - state->i; /* start of buffer to swap */
	    char tmp;
   
	    for(;ptr < rptr - 3;ptr += 4) {
	       tmp = ptr[0]; ptr[0] = ptr[3]; ptr[3] = tmp;
	       tmp = ptr[1]; ptr[1] = ptr[2]; ptr[2] = tmp;
	    }
	 }
	 state++;
	 break;
       case FITSB_SWAB8:
	 {
	    char *ptr = rptr - state->i; /* start of buffer to swap */
	    char tmp;
   
	    for(;ptr < rptr - 7;ptr += 8) {
	       tmp = ptr[0]; ptr[0] = ptr[7]; ptr[7] = tmp;
	       tmp = ptr[1]; ptr[1] = ptr[6]; ptr[6] = tmp;
	       tmp = ptr[2]; ptr[2] = ptr[5]; ptr[5] = tmp;
	       tmp = ptr[3]; ptr[3] = ptr[4]; ptr[4] = tmp;
	    }
	 }
	 state++;
	 break;
#endif
      }
   }

   return(0);				/* NOTREACHED */
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * We could do this at the TCL level, but it's too slow
 */
int
phFitsBinTblChainWrite(
		       PHFITSFILE *fil,	/* file descriptor for file */
		       CHAIN *chain
		       )
{
   void *elem;				/* something stored on the chain */
   CURSOR_T curs = shChainCursorNew(chain);
   
   while((elem = shChainWalk(chain,curs,NEXT)) != NULL) {
      phFitsBinTblRowWrite(fil,elem);
   }

   shChainCursorDel(chain,curs);

   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * Update a FITS binary table header
 */
int
phFitsBinTblEnd(
		PHFITSFILE *fil		/* file descriptor for (opened) file */
	       )
{
   char buff[8192];
   long endmain;			/* the end of the main table */
   int ncard;				/* number of cards in this record */
   int i;
   int n;
   long theap;				/* the start of the heap */

   if(fil->mode == 0) {			/* opened for read */
/*
 * skip over the heap
 */
      if(lseek(fil->fd,fil->heap_end,SEEK_SET) == -1) {
	 shErrStackPushPerror("Failed to skip table");
	 return(-1);
      }
   } else if(fil->mode == 1) {			/* opened for write */
      if((endmain = lseek(fil->fd,0L,SEEK_END)) == -1) {
	 shErrStackPushPerror("Cannot seek to end of file");
	 return(-1);
      }
/*
 * copy the heap into the main file. Note that we seeked (sook?) on the heap
 * file descriptor, so it's been flushed and it's safe to use the
 * regular unix read/write to copy the heap, saving an memcpy()
 */
      theap = endmain;
      if(fil->heapfd >= 0) {		/* there's a heap to worry about */
	 if((n = theap%4) != 0) {	/* align heap on 4-byte boundary */
	    if(hblseek(fil->heapfd,n - 1,SEEK_END) == -1) {
	       shErrStackPushPerror("Cannot seek beyond end of main data");
	       return(-1);
	    }
	    if(hbwrite(fil->heapfd,"",1,BYTE1) != 1) {
	       shErrStackPushPerror("Cannot write a byte to end of main data");
	       return(-1);
	    }
	    theap += (4 - n);
	 }
	 
	 if(hblseek(fil->heapfd,0L,SEEK_SET) == -1) {
	    shErrStackPushPerror("Cannot seek to start of heap");
	    return(-1);
	 }
	 while((n = read(fil->heapfd,buff,8192)) > 0) {
	    if((i = write(fil->fd,buff,n)) != n) {
	       shErrStackPushPerror("error copying heap (%d not %d) ", i, n);
	       return(-1);
	    }
	 }
	 hbclose(fil->heapfd); fil->heapfd = -1;
      }
/*
 * Pad out to a multiple of FITSIZE bytes. FITS. Pah!
 * n will be the total number of bytes in the file
 */
      if((n = lseek(fil->fd,0,SEEK_END)) == -1) {
	 shErrStackPushPerror("cannot seek to end of file");
	 return(-1);
      }
   
      if(n%FITSIZE != 0) {
	 int dn = FITSIZE - n%FITSIZE;
	 memset(buff,'\0',dn);
	 if(write(fil->fd,buff,dn) != dn) {
	    shErrStackPushPerror("error extending heap to a "
				 "fits record boundary");
	    return(-1);
	 }
	 n += dn;
      }
/*
 * And adjust the header now that we've written the entire file.
 */
      if((fil->hdr_start = lseek(fil->fd,fil->hdr_start,SEEK_SET)) == -1) {
	 shErrStackPushPerror("Cannot seek to start of header");
	 return(-1);
      }

      if(read(fil->fd,record,FITSIZE) != FITSIZE) {
	 shErrStackPushPerror("Cannot read header record");
	 return(-1);
      }
      if(lseek(fil->fd,fil->hdr_start,SEEK_SET) == -1) {
	 shErrStackPushPerror("Cannot seek to start of header a second time");
	 return(-1);
      }
      
      ncard = 0;
      ncard++;				/* XTENSION = BINTABLE */
      ncard++;				/* BITPIX = 8 */
      ncard++;				/* NAXIS = 2 */
      ncard++;				/* NAXIS1 = ?? */
      ncard = write_card_d(fil->fd,ncard,record,"NAXIS2",fil->nrow,"");
      ncard = write_card_d(fil->fd,ncard,record,"PCOUNT",n - endmain,"");
      ncard++;				/* GCOUNT */
      ncard++;				/* TFIELDS */
      ncard = write_card_d(fil->fd,ncard,record,"THEAP",
						      theap - fil->hdr_end,"");
      
      if(write(fil->fd,record,FITSIZE) != FITSIZE) {
	 shErrStackPushPerror("Cannot write header record");
	 return(-1);
      }
      fil->hdr_start = -1;		/* we've done with that header */
      fil->nrow = 0;
/*
 * set the file pointer to the EOF; the last thing to do to the file
 */
      if(lseek(fil->fd,0,SEEK_END) == -1) {
	 shErrStackPushPerror("Cannot set seek pointer to start of header");
	 return(-1);
      }
   } else {
      shFatal("phFitsBinTblEnd: Unknown mode %d",fil->mode);
   }
/*
 * We have finished writing tables of that rowlength, so free the i/o buffer
 */
   shFree(fil->row); fil->row = NULL;
/*
 * Free the optimised machines. We'll have to rebuild them anyway if
 * we write another table
 */
   fil->machine = NULL;			/* it's one of the optimised ones */
   for(i = 0;i < NTYPE;i++) {
      if(machine[i] != NULL) {
	 del_machine(fil->omachine[i]);
	 fil->omachine[i] = NULL;
      }
   }
/*
 * There's no longer a type associated with this table
 */
   fil->type = shTypeGetFromName("UNKNOWN");
   
   return(0);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 * close a table
 */
int
phFitsBinTblClose(
		  PHFITSFILE *fil		/* file descriptor for file */
		  )
{
   int ret = 0;
   if(fil->hdr_start > 0)	{		/* there's a header to fixup */
      ret = phFitsBinTblEnd(fil);
   }
   close(fil->fd);
   del_fitsfile(fil);

   return(ret);
}

/*****************************************************************************/
/*
 * FITS card utilities
 *
 * parse_card sets the globals key, value, and comment
 */
static void
parse_card(const char *card)
{
   char *ptr;
   
   strncpy(ccard,card,80); ccard[80] = '\0';

   key = ccard;
   for(ptr = ccard;!isspace(*ptr);ptr++) {
      if(*ptr == '=') {
	 break;
      }
   }
   *ptr++ = '\0';
   
   while(isspace(*ptr)) ptr++;
   if(*ptr == '=') {
      ptr++;
      while(isspace(*ptr)) ptr++;
   }

   value = ptr;
   if(*ptr == '\'') {			/* string valued */
      ptr++; value++;
      for(;*ptr != '\0';ptr++) {
	 if(*ptr == '\'') {
	    if(*(ptr + 1) == '\'') {	/* escaped ' */
	       ptr++;
	    } else {
	       break;
	    }
	 }
      }
   } else {
      while(*ptr != '\0' && !isspace(*ptr)) ptr++;
   }
   *ptr++ = '\0';

   while(isspace(*ptr)) ptr++;
   if(*ptr == '/') {
      ptr++;
      while(isspace(*ptr)) ptr++;
   }
   comment = ptr;
}

/*****************************************************************************/
/*
 * Check that a fits card from a file has the desired keyword and value
 *
 * Return -1 in case of error, 0 if a mismatch is found, and 1 if all is OK
 *
 * Various calls for character and int
 *
 * Check a string value
 */
static int
check_card_s(const HDR *hdr,
	     char *card,
	     const char *keyword,
	     const char *val)
{
   static char buff[81];		/* white-space stripped value */
   int i;
   int len;				/* length of non-blank part of value */

   if (shHdrGetLine(hdr, keyword, buff) != SH_SUCCESS) {
      shErrStackPush("Failed to find keyword \"%s\"",keyword);
      return(0);
   }
   parse_card(buff);
/*
 * There may be trailing white space in value
 */
   if(val == NULL) {			/* don't bother checking it */
      return(1);
   }
   
   value = strncpy(buff,value,80);
   for(len = strlen(value) - 1;len >= 0;len--) {
      if(!isspace(value[len])) {
	 value[len + 1] = '\0';
	 break;
      }
   }
/*
 * here's a case-insensitive version of strcmp
 */
   for(i = 0; i < len; i++) {
      if(toupper(val[i]) != toupper(value[i])) {
	 shErrStackPush("Expected value \"%s\" for keyword \"%s\", saw \"%s\"",
								val,key,value);
	 return(0);
      }
   }

   return(1);
}   

/*****************************************************************************/
/*
 * Check an integer (%d)
 */
#if 0					/* no longer used */
static int
check_card_d(int fd,
	     char *card,
	     const char *keyword,
	     int val)
{
   if(read(fd,card,80) != 80) {
      shErrStackPushPerror("Failed to read card");
      return(-1);
   }
   parse_card(card);
   
   if(strcmp(key,keyword) != 0) {
      shErrStackPush("Expected keyword \"%s\", saw \"%s\"",keyword,key);
      return(0);
   }

   if(val != atoi(value)) {
      shErrStackPush("Expected value \"%d\" for keyword, saw \"%d\"",
							  val,key,atoi(value));
      return(0);
   }

   return(1);
}   
#endif

/*****************************************************************************/
/*
 * Write a FITS card image. Various calls for character, int, logical, float
 *
 *
 * Simply copy a formatted card to the output record
 */
static int
set_card(
	 int fd,
	 int ncard,
	 char *record,
	 const char *card
	 )
{
   strncpy(&record[80*ncard],card,80);

   if(++ncard == 36) {
      if(write(fd,record,FITSIZE) != FITSIZE) {
	 shError("Cannot write header record");
      }
      ncard = 0;
   }
   
   return(ncard);
}

/*****************************************************************************/
/*
 * Write a string value
 */
static int
write_card_s(
	     int fd,
	     int ncard,
	     char *record,
	     const char *keyword,
	     const char *val,
	     const char *commnt
	     )
{
   char *card = &record[80*ncard];
   char value[20];			/* blank-padded val, if needed */

   if(strlen(val) < 8) {		/* FITS requires at least 8 chars */
      sprintf(value, "%-8s", val);
      val = value;
   }

   if(*keyword == '\0' || !strcmp(keyword,"COMMENT") || !strcmp(keyword,"END")
		       || !strcmp(keyword,"HISTORY")) {
      if(commnt[0] != '\0') {
	 shError("You can't add a comment to a COMMENT, END, or HISTORY card");
      }
      sprintf(ccard,"%-8.8s%-72s",keyword,val);
   } else {
      sprintf(ccard,"%-8.8s= '%s' %c%-*s", keyword, val,
	      (commnt[0] == '\0' ? ' ' : '/'),
	      (80 - 14 - (int)strlen(val)), commnt);
   }
   shAssert(strlen(ccard) == 80);
   memcpy(card,ccard,80);
/*
 * Write record if full
 */
   if(++ncard == 36) {
      if(write(fd,record,FITSIZE) != FITSIZE) {
	 shError("Cannot write header record");
      }
      ncard = 0;
   }
   
   return(ncard);
}   

/*****************************************************************************/
/*
 * Write an integer (%d)
 */
static int
write_card_d(
	     int fd,
	     int ncard,
	     char *record,
	     const char *keyword,
	     int val,
	     const char *commnt
	     )
{
   char *card = &record[80*ncard];

   sprintf(ccard,"%-8.8s= %20d %c%-48s",keyword,val,
	   		(commnt[0] == '\0' ? ' ' : '/'),commnt);
   strncpy(card,ccard,80);
/*
 * Write record if full
 */
   if(++ncard == 36) {
      if(write(fd,record,FITSIZE) != FITSIZE) {
	 shError("Cannot write header record");
      }
      ncard = 0;
   }
   
   return(ncard);
}   

/*****************************************************************************/
/*
 * Write a logical value
 */
static int
write_card_l(
	     int fd,
	     int ncard,
	     char *record,
	     const char *keyword,
	     const char *val,
	     const char *commnt
	     )
{
   char *card = &record[80*ncard];

   if(strcmp(val,"T") != 0 && strcmp(val,"F") != 0) {
      shError("Invalid logical %s for keyword %s",val,keyword);
      val = "?";
   }
   sprintf(ccard,"%-8.8s= %20s %c%-48s",keyword,val,
	   		(commnt[0] == '\0' ? ' ' : '/'),commnt);
   strncpy(card,ccard,80);
/*
 * Write record if full
 */
   if(++ncard == 36) {
      if(write(fd,record,FITSIZE) != FITSIZE) {
	 shError("Cannot write header record");
      }
      ncard = 0;
   }
   
   return(ncard);
}   

/*****************************************************************************/
/*
 * Insert a line into a header.  We cannot simply call shHdrInsertLine()
 * as it (or rather the libfits routine f_hlins()) fails to check that
 * the card is 80 or fewer characters (PR XXX)
 */
static int
hdr_insert_line(HDR *hdr,		/* the header to fill out */
		const char *card)	/* card to insert */
{
   char ccard[81];
   int ncard;				/* number of cards already present */

   for (ncard = 0; hdr->hdrVec[ncard] != NULL; ncard++) continue;

   strncpy(ccard, card, 80); ccard[80] = '\0';

   return(shHdrInsertLine(hdr, ncard, ccard));
}

/*****************************************************************************/
/*
 * Write a floating value
 */
#if 0
static int
write_card_f(
	     int fd,
	     int ncard,
	     char *record,
	     const char *keyword,
	     float val,
	     const char *commnt
	     )
{
   char *card = &record[80*ncard];

   sprintf(ccard,"%-8.8s= %20g %c%-48s",keyword,val,
	   		(commnt[0] == '\0' ? ' ' : '/'),commnt);
   strncpy(card,ccard,80);
/*
 * Write record if full
 */
   if(++ncard == 36) {
      if(write(fd,record,FITSIZE) != FITSIZE) {
	 shError("Cannot write header record");
      }
      ncard = 0;
   }
   
   return(ncard);
}   
#endif

/*****************************************************************************/
/*
 * byte swap data; data and sdata may be the same
 */
static void
swap(char *sdata, const char *data, int n, int bytepix)
{
   const char *end = data + n;
#if defined(DEC_BYTE_ORDER)
   const int do_swap = (bytepix == 1) ? 0 : 1;
#else
   const int do_swap = 0;
#endif
   char tmp;
   
   if(!do_swap) {			/* no swapping required */
      if(sdata == data) {		/* not even a copy required */
	 ;
      } else if((data < sdata && data + n <= sdata) || 
		(sdata + n <= data)) {	/* no overlap */
	 memcpy(sdata,data,n);
      } else {				/* data overlaps */
	 memmove(sdata,data,n);
      }
   } else if(bytepix == 2) {
      for(;data < end - 1;data += 2, sdata += 2) {
	 tmp = data[0]; sdata[0] = data[1]; sdata[1] = tmp;
      }
   } else if(bytepix == 4) { 
      for(;data < end - 3;data += 4, sdata += 4) {
	 tmp = data[0]; sdata[0] = data[3]; sdata[3] = tmp;
	 tmp = data[1]; sdata[1] = data[2]; sdata[2] = tmp;
      }
   } else if(bytepix == 8) { 
      for(;data < end - 7;data += 8, sdata += 8) {
	 tmp = data[0]; sdata[0] = data[7]; sdata[7] = tmp;
	 tmp = data[1]; sdata[1] = data[6]; sdata[6] = tmp;
	 tmp = data[2]; sdata[2] = data[5]; sdata[5] = tmp;
	 tmp = data[3]; sdata[3] = data[4]; sdata[4] = tmp;
      }
   } else {
      shFatal("photoFits: I cannot byteswap for bytepix == %d",bytepix);
   }
}


/*****************************************************************************/
/*
 * Buffered I/O for the heap. These functions are identical in calling
 * sequence to read and write.
 *
 * Using fread/fwrite doesn't seem to be much of an efficiency gain over
 * simply using read/write, and at least on a SGI running irix 5.2 isn't
 * fast enough, so we have to do it ourselves.
 */
#define BSIZE 8192
static char hbuff[BSIZE];
static char *hbptr = hbuff, *hbend = hbuff + BSIZE;

static void
hbflush(int fd)
{
   if(hbptr != hbuff) {
      write(fd,hbuff,hbptr - hbuff);
      hbptr = hbuff;
   }
}

static void
hbclose(int fd)
{
   hbflush(fd);
   close(fd);
}

static int
hblseek(int fd, off_t n, int w)
{
   hbflush(fd);
   return(lseek(fd,n,w));
}

static int
hbread(int fd, void *data, int n,
       int byte_data)			/* NOTUSED */ /* if !DEC_BYTE_ORDER */
{
   int nread;

   nread = read(fd,data,n);
#if defined(DEC_BYTE_ORDER)
   if(nread > 0 && byte_data != 1) {
      swap(data,data,nread,byte_data);
   }
#endif

   return(nread);
}

static int
hbwrite(int fd, void *data, int n, int byte_data)
{
   int n2go = n;			/* number of bytes still to write */

   if (n <= 0) {
       return n;
   }

   if(byte_data == 1) {			/* no need to swap */
      if(hbptr + n2go >= hbend) {
	 hbflush(fd);
	 while(n2go > BSIZE) {
	    int nw;
	    if((nw = write(fd,data,BSIZE)) != BSIZE) {
	       return(n - n2go + nw);
	    }
	    data = (char *)data + BSIZE;
	    n2go -= BSIZE;
	 }
      }
      memcpy(hbptr,data,n2go);		/* we know that hbuff != data */
      hbptr += n2go;
   } else {
      if(hbptr + n2go >= hbend) {
	 hbflush(fd);
	 while(n2go > BSIZE) {
	    int nw;
	    swap(hbuff,data,BSIZE,byte_data);
	    if((nw = write(fd,hbuff,BSIZE)) != BSIZE) {
	       return(n - n2go + nw);
	    }
	    data = (char *)data + BSIZE;
	    n2go -= BSIZE;
	 }
      }
      swap(hbptr,data,n2go,byte_data);
      hbptr += n2go;
   }
   
   return(n);
}

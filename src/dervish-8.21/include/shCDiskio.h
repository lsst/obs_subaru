#ifndef SHCDISKIO_H
#define SHCDISKIO_H
/*
 * read/write datatypes from/to disk
 */
#include "shCSchema.h"
#include "shCList.h"
#include "region.h"
#include "shChain.h"

/*
 * A type used to maintain lists of SDSS data objects
 */
typedef struct struct_thing {
   TYPE ltype;				/* used by LIST stuff */
   struct struct_thing *next, *prev;
   void *ptr;
   TYPE type;
} THING;

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

RET_CODE shThingDel(THING *elem);
THING *shThingNew(void *body, TYPE type);

/*
 * If p_shDumpReadShallow is true, DumpRead functions are allowed to skip
 * over parts of the file, if they feel like it. This is probably mostly
 * useful for reading Regions/Masks.
 *
 * It is set as an argument to shDump[Next]Read -- don't touch it yourself
 */
extern int p_shDumpReadShallow;
/*
 * Manipulate dump files
 *
 * shDumpOpen, shDumpReopen, and shDumpClose open, reopen or close a dump
 * shDumpTypeGet returns the TYPE of the next item in the dump.
 * shDumpNextRead reads the next item, returning it as a THING
 * shDumpNextWrite writes a THING to the dump file
 * shDumpRead reads the rest of the dump, returning it as a LIST of THINGSs
 * shDumpDateGet() returns the date string from a dump
 * shDumpDateDel() is used to null out the time stamp in a dump file
 * p_shDumpAnonRead is called by closeDump;
 */
RET_CODE shDumpClose(FILE *fil);
FILE *shDumpOpen(char *name, char *mode);
FILE *shDumpReopen(char *name, char *mode);
RET_CODE shDumpPtrsResolve(void);
RET_CODE shDumpTypeGet(FILE *outfil, TYPE *type);
THING *shDumpNextRead(FILE *fil, int level);
RET_CODE shDumpNextWrite(FILE *fil, THING *thing);
CHAIN *shDumpRead(FILE *fil, int level);
char *shDumpDateGet(FILE *fil);
void shDumpDateDel(char *name);
RET_CODE p_shDumpAnonRead(FILE *outfil, int shallow_read);
/*
 * declare that a structure has been written to the dump
 */
void p_shDumpStructWrote(void *str);
/*
 * Resolve unknown references, and define id <--> address mapping
 */
void p_shPtrIdSave(int id, void *addr);
void p_shDumpStructsReset(int force_free);
/*
 * writing types to the dump file; these are like shDumpIntRead/Write,
 * but don't record what type is being written (which would lead to
 * infinite data files). p_shDumpTypeCheckWrite writes type info to a file,
 * while p_shDumpTypeCheck reads the info, and checks if it is correct
 */
RET_CODE shDumpTypeRead(FILE *fil, TYPE *t);
RET_CODE shDumpTypeWrite(FILE *fil, TYPE *t);
int p_shDumpTypeCheck(FILE *outfil,char *expected_type);
RET_CODE p_shDumpTypecheckWrite(FILE *outfil,char *type);
/*
 * dump generic types, based on their schema
 */
RET_CODE shDumpGenericRead(FILE *fil, char *type, void **h);
RET_CODE shDumpGenericWrite(FILE *fil, char *type, void *p);
/*
 * then deal with C primitive types
 */
RET_CODE shDumpCharRead(FILE *fil, char *c);
RET_CODE shDumpUcharRead(FILE *fil, unsigned char *uc);
RET_CODE shDumpDoubleRead(FILE *fil, double *x);
RET_CODE shDumpFloatRead(FILE *fil, float *x);
RET_CODE shDumpIntRead(FILE *fil, int *i);
RET_CODE shDumpUintRead(FILE *fil, unsigned int *ui);
RET_CODE shDumpShortRead(FILE *fil, short *i);
RET_CODE shDumpUshortRead(FILE *fil, unsigned short *ui);
RET_CODE shDumpLongRead(FILE *fil, long *i);
RET_CODE shDumpUlongRead(FILE *fil, unsigned long *ui);
RET_CODE shDumpLogicalRead(FILE *fil, LOGICAL *i);
RET_CODE shDumpPtrRead(FILE *fil, void **ptr);
RET_CODE shDumpStrRead(FILE *fil, char *str, int n);
RET_CODE shDumpChainRead(FILE *fil, CHAIN **chain);
RET_CODE shDumpChain_elemRead(FILE *fil, CHAIN_ELEM **chain_elem);
RET_CODE shDumpCharWrite(FILE *fil, char *c);
RET_CODE shDumpUcharWrite(FILE *fil, unsigned char *uc);
RET_CODE shDumpDoubleWrite(FILE *fil, double *x);
RET_CODE shDumpFloatWrite(FILE *fil, float *x);
RET_CODE shDumpIntWrite(FILE *fil, int *i);
RET_CODE shDumpUintWrite(FILE *fil, unsigned int *ui);
RET_CODE shDumpShortWrite(FILE *fil, short *i);
RET_CODE shDumpUshortWrite(FILE *fil, unsigned short *ui);
RET_CODE shDumpLongWrite(FILE *fil, long *i);
RET_CODE shDumpUlongWrite(FILE *fil, unsigned long *ui);
RET_CODE shDumpLogicalWrite(FILE *fil, LOGICAL *i);
RET_CODE shDumpPtrWrite(FILE *fil, void *ptr, TYPE type);
RET_CODE shDumpStrWrite(FILE *fil, char *str);
RET_CODE shDumpChainWrite(FILE *fil, CHAIN *chain);
RET_CODE shDumpChain_elemWrite(FILE *fil, CHAIN_ELEM *chain_elem);
RET_CODE shDumpScharRead(FILE *, signed char *);
RET_CODE shDumpScharWrite(FILE *, signed char *);
/*
 * Dervish types
 */
RET_CODE shDumpHdrRead(FILE *file, HDR **header);
RET_CODE shDumpMaskRead(FILE *file, MASK **mask);
RET_CODE shDumpRegRead(FILE *file, REGION **home);

RET_CODE shDumpHdrWrite(FILE *file, HDR *header);
RET_CODE shDumpMaskWrite(FILE *file, MASK *mask);
RET_CODE shDumpRegWrite(FILE *file, REGION *region);

#ifdef __cplusplus
}
#endif	/* ifdef cpluplus */

#endif

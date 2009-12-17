/*
 * Support for two types used to test diskio and lists, FOO and BAR
 *
 * FOOs are pretty simple; all that we have to do is declare them and
 * let make_io know that they exist and we can use them from tcl; we
 * can create new instances bound to handles and and read/write them
 * to disk with no further work on our part.
 *
 * BARs are more complicated, as they contain a malloced array of BARs
 * The support code given here shows that even this is not impossible,
 * or even very hard. The TCL code (to define barNew and barDel) is in
 * tclFoo.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shCAssert.h"
#include "shCUtils.h"
#include "shCSchema.h"
#include "shCList.h"
#include "shCDiskio.h"
#include "foo.h"

static void init_goo(GOO *g);		/* initialise a GOO */

FOO *
tsFooNew(void)
{
   int i,j;
   FOO *new = shMalloc(sizeof(FOO));
   
   new->type = shTypeGetFromName("FOO");
   new->next = new->prev = NULL;

   new->name = shMalloc(1);
   *new->name = '\0';
   new->f = new->i = new->l = 0;

   for(i = 0;i < FSIZE;i++) {
      new->arr[i] = 0;
   }

   for(j = 0;j < 3;j++) {
      for(i = 0;i < FSIZE;i++) {
	 new->arr2[j][i] = 0;
      }
   }

   for(j = 0;j < NGOO;j++) {
      for(i = 0;i < NGOO;i++) {
	 init_goo(&new->goo[j][i]);
      }
   }

   return(new);
}

RET_CODE
tsFooDel(FOO *foo)
{
   shAssert(foo->next == NULL && foo->prev == NULL);
   if(foo->name != NULL) {
      shFree(foo->name);
   }
   shFree(foo);
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * Now for GOOs
 */
GOO *
tsGooNew(void)
{
   GOO *new = shMalloc(sizeof(GOO));
   init_goo(new);
   return(new);
}

static void
init_goo(GOO *g)
{
   g->f[0] = g->f[1] = 1.0;
}

RET_CODE
tsGooDel(GOO *goo)
{
   shFree(goo);
   return(SH_SUCCESS);
}

/*****************************************************************************/
/*
 * And now for a type BAR. This one is a bit more complicated, and
 * requires us to hand-craft our dump code, as well as writing the
 * TCL interface ourselves
 *
 * First the consructor/destructor
 */
BAR *
tsBarNew(int nfoo)
{
   int i;
   BAR *bar = shMalloc(sizeof(BAR));

   bar->nfoo = nfoo;
   bar->foo = shMalloc(nfoo*sizeof(FOO));
   for(i = 0;i < nfoo;i++) {
      bar->foo[i] = tsFooNew();
      bar->foo[i]->name = shMalloc(6);	/* be lazy! */
      sprintf(bar->foo[i]->name,"foo%-2d",i);
   }
   bar->f1 = bar->f2 = bar->i1 = bar->i2 = 0;

   return(bar);
}

RET_CODE
tsBarDel(BAR *bar)
{
   int i;

   if(bar != NULL) {
      for(i = 0;i < bar->nfoo;i++) {
	 tsFooDel(bar->foo[i]);
      }
      shFree(bar);
   }
   return(SH_SUCCESS);
}

/*
 * Read/Write a BAR to/from disk
 *
 * Writing first
 */
RET_CODE
tsDumpBarWrite(
	       FILE *fil,		/* input file */
	       BAR *bar			/* pointer to our BAR */
	       )
{
   int i;
   long id;
   void *newObj;
   const SCHEMA *sch;
/*
 * find out what we know about our type
 */
   if((sch = shSchemaGet("BAR")) == NULL) {
      shError("Can't retrieve schema for BAR");
      return(SH_GENERIC_ERROR);
   }
/*
 * Start by making sure that the current schema agrees with our prejudices
 */
   i = -1;
   if(strcmp(sch->elems[++i].name,"nfoo") != 0 ||
      strcmp(sch->elems[++i].name,"foo") != 0) {
      shError("tsDumpBarWrite: structure mismatch at BAR.%s",
	      sch->elems[i].name);
      return(SH_GENERIC_ERROR);
   }
/*
 * write object header. This is boilerplate
 */ 
   if(p_shDumpTypecheckWrite(fil,"BAR") != SH_SUCCESS) {
      shError("tsDumpBarWrite: Write type\n");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpPtrWrite(fil,bar,shTypeGetFromName("PTR")) == SH_GENERIC_ERROR) {
      shError("tsDumpBarWrite: writing bar");
      return(SH_GENERIC_ERROR);
   }
/*
 * We are ready to start writing. Do the hard ones by hand
 */
   if(shDumpIntWrite(fil,&bar->nfoo) == SH_GENERIC_ERROR) {
      shError("tsDumpBarWrite: reading bar->nfoo");
      return(SH_GENERIC_ERROR);
   }

   for(i = 0;i < bar->nfoo;i++) {
      if(shDumpPtrWrite(fil,bar->foo[i],shTypeGetFromName("FOO")) ==
							    SH_GENERIC_ERROR) {
	 shError("tsDumpBarWrite: writing %dth foo",i);
	 return(SH_GENERIC_ERROR);
      }
   }
/*
 * write the BAR's remaining elements (starting at the third)
 * No more thought is required
 */
   for(i = 2;i < sch->nelem;i++) {
      shDebug(2,"tsDumpBarWrite: writing %s",sch->elems[i].name);

      if(shDumpSchemaElemWrite(fil,"BAR",bar,&sch->elems[i]) != SH_SUCCESS) {
	 shError("tsDumpBarWrite: writing %s",sch->elems[i].name);
      }
   }

   p_shDumpStructWrote(bar);
   return(SH_SUCCESS);
}

/*
 * and now the read code; this is simply the mirror image of the above
 */
RET_CODE
tsDumpBarRead(
	      FILE *fil,		/* input file */
	      void **home		/* where the new object should go */
	      )
{
   int i;
   long id;
   BAR *bar;
   int nfoo;				/* the nfoo element of bar */
   const SCHEMA *sch;
/*
 * find out what we know about BARs
 */
   if((sch = shSchemaGet("BAR")) == NULL) {
      shError("Can't retrieve schema BAR %s");
      return(SH_GENERIC_ERROR);
   }
/*
 * Start by making sure that the current schema agrees with our prejudices
 */
   i = -1;
   if(strcmp(sch->elems[++i].name,"nfoo") != 0 ||
      strcmp(sch->elems[++i].name,"foo") != 0) {
      shError("tsDumpBarRead: structure mismatch at BAR.%s",
	      sch->elems[i].name);
      return(SH_GENERIC_ERROR);
   }
/*
 * read header; more boilerplate
 */
   if(p_shDumpTypeCheck(fil,"BAR") != 0) {
      shError("tsDumpBarRead: checking type");
      return(SH_GENERIC_ERROR);
   }
   if(shDumpLongRead(fil,&id) == SH_GENERIC_ERROR) {
      shError("tsDumpBarRead: reading id");
      return(SH_GENERIC_ERROR);
   }
   if(id == 0) {
      *home = NULL;
      return(SH_SUCCESS);
   }
/*
 * We are ready to start reading. Do the hard ones by hand
 */
   if(shDumpIntRead(fil,&nfoo) == SH_GENERIC_ERROR) {
      shError("tsDumpBarRead: reading bar->nfoo");
      return(SH_GENERIC_ERROR);
   }
   
   *home = bar = tsBarNew(nfoo);
   p_shPtrIdSave(id,bar);
/*
 * That's got our BAR made; read the FOO pointers
 */
   for(i = 0;i < bar->nfoo;i++) {
      if(shDumpPtrRead(fil,(void **)&bar->foo[i]) == SH_GENERIC_ERROR) {
	 shError("tsDumpBarRead: reading %dth foo",i);
	 return(SH_GENERIC_ERROR);
      }
   }
/*
 * OK, home free. We've read the first two elements, and the rest can
 * be handled by the schema
 */
   for(i = 2;i < sch->nelem;i++) {
      shDebug(2,"tsDumpBarRead: reading %s",sch->elems[i].name);
      
      if(shDumpSchemaElemRead(fil,"BAR",bar,&sch->elems[i]) != SH_SUCCESS){
	 shError("tsDumpBarRead: reading %s",sch->elems[i].name);
      }
   }
   return(SH_SUCCESS);
}

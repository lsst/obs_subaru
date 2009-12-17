/*
 * Test the structure I/O package
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "shCUtils.h"
#include "region.h"
#include "shCSchema.h"
#include "diskio_gen.h"
#include "shCDiskio.h"
#include "foo.h"

int
main(int ac, char **av)
{
   FILE *fil;
   int i;
   void *ptr;
   REGION *sub[2];
   TYPE type;
   int verbose;
   
   while(ac > 1 && av[1][0] == '-') {
      switch (av[1][1]) {
       case 'v':
	 if(av[1][2] == '\0') {
	    if(--ac <= 1 || !isdigit(*(++av)[1])) {
	       fprintf(stderr,"You must specify a number with -v\n");
	       continue;
	    }
	    verbose = atoi(av[1]);
	 } else {
	    verbose = atoi(&av[1][2]);
	 }
	 shVerboseSet(verbose);
	 break;
       default:
	 fprintf(stderr,"Unknown flag: %s\n",av[0]);
	 break;
      }
      ac--; av++;
   }
   if(ac <= 2) {
      shError("main: too few arguments. Usage: tstio [-v#] file r|w");
      exit(1);
   }
   
   if(strcmp(av[2],"r") == 0) {
      REGINFO *reginfo;
      REGION *reg;
      SCHEMA *sch;
      
      if((fil = shDumpOpen(av[1],"r")) == NULL) {
         exit(1);
      }
      for(i = 0;i < 2;i++) {
	 shDumpRegRead(fil,&sub[i]);
      }
      if(shDumpClose(fil) == SH_GENERIC_ERROR) {
	 shError("main: failed to resolve all ids");
	 return(SH_GENERIC_ERROR);
      }

      if((sch = shSchemaGet("REGION")) == NULL) {
	 fprintf(stderr,"Can't retrieve schema for REGION\n");
	 exit(1);
      }
      if(shRegInfoGet(sub[1],&reginfo) != SH_SUCCESS) {
	 exit(1);
      }
      if((reg = reginfo->parent) == NULL) {
	 fprintf(stderr,"Region has no parent\n");
	 exit(1);
      }
      for(i = 0;i < sch->nelem;i++) {
	 int j;
	 printf("%s(%s",sch->elems[i].name,sch->elems[i].type);
	 for(j = 0;j < sch->elems[i].nstar;j++) {
	    printf("*");
	 }
	 printf(")");
	 if((ptr = shElemGet(reg,&sch->elems[i],&type)) != NULL) {
	    printf("\t\t%s",shPtrSprint(ptr,type));
	 }
	 printf("\n");
      }
   } else if(strcmp(av[2],"w") == 0) {
      char name[30];
      REGION *reg = shRegNew("Robert",15,20,TYPE_U16);

      shRegClear(reg);
      reg->rows_u16[1][1] = 200;
      sub[0] = shSubRegNew("Hugh",reg,3,3,0,0,READ_ONLY);
      sub[1] = shSubRegNew("Lupton",reg,3,3,2,2,READ_ONLY);
   
      if((fil = shDumpOpen(av[1],"w")) == NULL) {
         exit(1);
      }
      shDumpRegWrite(fil,reg);
      for(i = 0;i < 2;i++) {
	 shDumpRegWrite(fil,sub[i]);
      }
      shDumpClose(fil);
   } else {
      shError("main: Unknown mode: %s",av[2]);
      exit(1);
   }

   return(0);
}

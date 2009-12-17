#include <stdio.h>
#include "shCErrStack.h"

#define MAXLINELENGTH 2048

/******************************************************************************
<AUTO EXTRACT>
 *  ROUTINE: paramIncOpen
 *
 </AUTO> */

FILE *paramIncOpen(
		   char *fileName,         /* name of parameter file to open */
		   long int seekSet,       /* file position to seek to */
		   long int *seekEOF)      /* seek position for end of file - can be used for next calls for faster access */
{
  FILE *file;
  char emsg[MAXLINELENGTH];

  if ( (file = fopen(fileName,"r")) == NULL) { sprintf(emsg,"paramOpen: ERROR %s does not exist\n",fileName);                         goto ERROR; }
  if (fseek(file,-1,SEEK_END) != 0)          { sprintf(emsg,"paramOpen: ERROR can not seek to last byte of %s\n",fileName);           goto ERROR; }
  if (getc(file)!='\n') { sprintf(emsg,"paramOpen: ERROR last byte of paramFile %s is not \\n - probably  being updated\n",fileName); goto ERROR; }
  *seekEOF = ftell(file);
  if (seekSet>*seekEOF)  { sprintf(emsg,"paramOpen: ERROR seekSet=%d > eof at %d for%s\n",  seekSet,*seekEOF,fileName); fclose(file); goto ERROR; }
  if (seekSet==*seekEOF) { sprintf(emsg,"paramOpen: ERROR seekSet=%d at eof. No new records in %s\n",seekSet,fileName); fclose(file); goto ERROR; }
  if (fseek(file,seekSet,SEEK_SET) != 0) { sprintf(emsg,"paramOpen: ERROR cannot seek to %d in %s\n",seekSet,fileName); fclose(file); goto ERROR; }
  return file;

 ERROR:
  if(file!=NULL) fclose(file);
  shErrStackPush(emsg);
  return NULL;
}

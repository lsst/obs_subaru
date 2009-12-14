/*
 * Routines to support the FakeStampCollecting Pipeline, FSC
 */
#include <string.h>
#include "dervish.h"
#include "phFsc.h"
#include "phConsts.h"

/*****************************************************************************/
/*
 * {con,de}structors for postage stamps
 */
SCSTAMP *
phFscStampNew(void)
{
   SCSTAMP *stamp = shMalloc(sizeof(SCSTAMP));
   stamp->type = shTypeGetFromName("SCSTAMP");
   stamp->midRow = -1;
   stamp->midCol = -1;
   stamp->rowc = stamp->colc = -1;
   stamp->peak = 0;
   stamp->status = -9;
   stamp->reg = shRegNew("scStampNew", SC_STAMPNROW, SC_STAMPNCOL, TYPE_U16);

   return(stamp);
}

void
phFscStampDel(SCSTAMP *stamp)
{
   if(stamp != NULL) {
      shRegDel(stamp->reg);
      shFree(stamp);
   }
}

void
phFscStampChainDel(CHAIN *chain)
{
   int i;
   int size;
   SCSTAMP *stamp;

   if(chain == NULL) return;
   shAssert(chain->type == shTypeGetFromName("SCSTAMP"));
   size = chain->nElements;

   for(i = 0; i < size; i++) {
      stamp = shChainElementRemByPos(chain, 0);
      phFscStampDel(stamp);
   }

   shChainDel(chain);
}

SCFSTAMP *
phFscFstampNew(void)
{
   SCFSTAMP *new = (SCFSTAMP *)shMalloc(sizeof(SCFSTAMP));
   
   memset(new->pixelMap, '\0', SC_STAMPNROW*SC_STAMPNCOL);
   shAssert(new->pixelMap[0] == 0);	/* all bits zero == 0 */
   new->midRow = 0;
   new->midCol = 0;
   
   return(new);
}

void
phFscFstampDel(SCFSTAMP *fstamp)
{
   shFree(fstamp);
}

void
phFscFstampChainDel(CHAIN *chain)
{
   int i;
   int size;
   SCFSTAMP *stamp;

   if(chain == NULL) return;
   shAssert(chain->type == shTypeGetFromName("SCFSTAMP"));
   size = chain->nElements;

   for(i = 0; i < size; i++) {
      stamp = shChainElementRemByPos(chain, 0);
      phFscFstampDel(stamp);
   }

   shChainDel(chain);
}

/*****************************************************************************/
/*
 * Given an SCSTAMP, create a new SCFSTAMP and fill its 1-D array with
 * pixel values from the SCSTAMP.  Copy the 'midRow' and 'midCol' fields
 * as well.
 */
SCFSTAMP *
phFscStampToFstamp(const SCSTAMP *stamp)	/* SCSTAMP to be converted */
{
   S16 *fp;
   SCFSTAMP *fstamp = phFscFstampNew();
   int r, c;
   U16 *sp;
   int val;

   shAssert(stamp != NULL);
   shAssert(stamp->reg != NULL && stamp->reg->type == TYPE_U16);

   fp = fstamp->pixelMap;
   for(r = 0; r < stamp->reg->nrow; r++) {
      sp = stamp->reg->rows_u16[r];
      for(c = 0; c < stamp->reg->ncol; c++) {
	 val = (int)sp[c] - SC_INT16VAL; /* convention for SCFSTAMPs */
	 *fp++ = (val < MIN_S16) ? MIN_S16 : (val > MAX_S16) ? MAX_S16 : val;
      }
   }
   fstamp->midRow = stamp->midRow;
   fstamp->midCol = stamp->midCol;

   return(fstamp);
}

/*****************************************************************************/
/*
 * <AUTO EXTRACT>
 *
 * Convert a CHAIN of SCSTAMPs to a CHAIN of SCFSTAMPs.
 * Return a pointer to the new CHAIN.
 */
CHAIN *
phFscStampChainToFstampChain(const CHAIN *stampchain) /* SCSTAMPs to convert */
{
   int i, size;
   CHAIN *fstampchain;
   SCFSTAMP *fstamp;
   SCSTAMP *stamp;
   
   shAssert(shChainTypeGet(stampchain) == shTypeGetFromName("SCSTAMP"));
   
   fstampchain = shChainNew("SCFSTAMP");
   
   size = shChainSize(stampchain);
   for (i = 0; i < size; i++) {
      stamp = shChainElementGetByPos(stampchain, i);
      fstamp = phFscStampToFstamp(stamp);
      shChainElementAddByPos(fstampchain, fstamp, "SCFSTAMP", TAIL, AFTER);
   }
   
   return(fstampchain);
}

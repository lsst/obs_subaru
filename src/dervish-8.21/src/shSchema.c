 
/*************************************************************************
 *  Facility:           dervish                                            *
 *  Synopsis:           definitions for conversions of TBLCOL to schemas *
 *  Environment:        ANSI C                                           *
 *  Author:             Wei Peng                                         *
 *  Creation Date:      Feb 1, 1994                                      *
 *                                                                       *
 *  Last Modification:  July 19, 1994                                     *
 *************************************************************************

 Routines that are called here come from two other supporting files,
 shSchemaSupport.c and shSchemaTrans.c, with the latter dealing with
 translation tables (SCHEMATRANS structure). Tcl routines are in
 tclSchema.c only.


**************************************************************************/
/***************  Author's note *****************************************

 *  modification of the codes will be much easier if full understanding of
 *  the functionalities is achieved. Conversion (either direction) is done
 *  taking a source data pointer and a destination data pointer, and copying
 *  the data. However, extensive checking and pointer-following are done
 *  before copying takes place.

 *  shCnvTblToSchema: Upon on entry, this routine assumes the existence of
    a tblcol structure, an object (container or real object), and a translation
    table. It then will:

       1. Check the arguments for data validity.
       2. For each field (or attribute) of the schema, find all related entries
          from the translation table.
       3. Extract array specification both at FITS side and at schema side. 
          Compare the array sizes and dimensions. 
       4. Copy the data.

    Note: shSpptSchemaObjectStateCheck() is called before this routine to ensure
    objects that passed to it have valid state.

 *  shCnvSchemaToTbl: similar to shCnvTblToSchema, it 

       1. Checks the arguments for data validity.
       2. Analyzes the translation table for redundancy and FITS-side array
          information. Initializes run-time support structure.
       3. Extract array specification both at FITS side and at schema side. 
          Compare the array sizes and dimensions.
       4. Copy data.

 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "libfits.h"
#include "dervish_msg_c.h"
#include "shCErrStack.h"
#include "shCUtils.h"
#include "shCSchema.h"
#include "shCTbl.h"
#include "shCArray.h"
#include "shTclHandle.h"
#include "shCAlign.h"
#include "shChain.h"
#include "shCGarbage.h"


#include "shCSchemaCnv.h"
#include "shCSchemaTrans.h"
#include "shCSchemaSupport.h"

#define size_min(x,y) ((x) >= (y) ? y: x)
#define size_max(x,y) ((x) >= (y) ? x: y)

/* maxium number of runtime independent entries is MAX_RUNTIME_ENTRIES */

#define MAX_IDENTICAL_ENTRIES    10*MAX_SCHEMATRANS_ENTRY_SIZE    

/******************* some runtime support structure are declared *****/
/* private structure for table redundancy checks */

typedef struct _entry_info 
{
     int num[MAX_INDIRECTION];       	/* length in each dimension */
     int len;			    	/* number of dimension */
     int status;			/* status of this entry */
     int fldIdx;			/* which field index in TBLCOL */
     char src[MAX_NAME_LEN];		/* FITS field name */
} ENTRY_INFO; 

typedef struct _redund_info 
{
       int entry;
       struct _entry_info *infoPtr;
} REDUND_ENTRY;


static ENTRY_INFO* redundEntryInfo = NULL;
static REDUND_ENTRY* entryTable = NULL;
static struct _entry_info *currentInfo = NULL;
static int redundEntryInfoCnt = 0, entryTableCnt = 0;


/*********** Some more globals are declared here ************/
/* anything that starts with fits is fits side variable
 * anything that starts with obj is object side variable 
 *
 * When I say "of an entry", that quantity is directly or 
 * indirectly given in a translation table entry. 
 */


static SCHEMA_ELEM*  objElemPtr      = NULL;		/* obj:  element pointer */
static const SCHEMA* rootSchemaPtr   = NULL;		/* top level object schema     */
static char*         objInstancePtr  = NULL;		/* object instance of rootSchema */
static char*         fitsSideDataPtr = NULL;		/* fits: data pointer */
static char*         objSideDataPtr  = NULL;		/* obj: data pointer */

static int           objSideArrayIndex   = 0;		/* obj: index of an entry */
static int           objSideArrayDimen   = 0;		/* obj: dimen of an entry */
static int           objSideDataSize     = 1;		/* obj: data size in bytes */
static int           objSideAddedDimen   = 0;		/* obj: dynamic dimension of an entry */
static int           objSideAddedArraySize   = 1;	/* obj: dynamic array size of an entry */
static int           objSideStaticDimen      = 0;	/* obj: static dimension */
static int           objSideStaticArraySize  = 1;	/* obj: static array size */
static int           objSideTotalDimen       = 0;	/* obj: total array dimen */
static int           objSideTotalArraySize   = 1;	/* obj: total array size */
static int           objSideNumOfBlocks      = 1;       /* obj: num of data blocks per obj */
static DSTTYPE       objSideDataType         = DST_UNKNOWN;	/* obj: data type */
static DSTTYPE       objSideHeapDataType     = DST_UNKNOWN;     /* useful when DataType is heap */

static int           fitsSideArrayIndex      = 0;	/* fits: array index of an entry */
static int           fitsSideArrayDimen      = 0;	/* fits: array dimen of an entry */
static int           fitsSideNumOfBlocks     = 1;	/* fits: number of blocks of an entry */
static int           fitsSideBlocksPerRow    = 1;  	/* fits: num of blocks per row */
static DSTTYPE       fitsSideDataType        = DST_UNKNOWN;     /* fits: data type */

/* some supporting global variables */

static TBLFLD*       curTblfldPtr     = NULL;		/* TBLFLD pointer */
static ARRAY*        curArrayPtr      = NULL;		/* ARRAY  pointer */
static SCHEMATRANS_ENTRY*  transTblEntryPtr = NULL;           /* translation table entries */

/*************************************************************************************************/
/* Some private support routines are defined below */

static int shCnvObjectStore(
	CONTAINER_TYPE  containerType,  	/* container type */
	void* container, 			/* container instance */
	void* instance, 		/* object instance */
        int row,
	int max_objects,			/* maximum number of objects */	
        char* schemaName	
)
{

     ARRAY*  containerArray   = (ARRAY*) container;  /* only one to be used */
     CHAIN*  containerChain   = (CHAIN*) container;

     
     switch(containerType)
     {
         case ARRAY_TYPE: 
            if (containerArray->arrayPtr==NULL) 
            {
               if((containerArray->arrayPtr= 
                         (char**) shMalloc(max_objects*sizeof(char*)))==NULL)
               {  
                       shErrStackPush("Can't alloc mem for containerArray"
                                           "data area");
                       return -1;
                }
             }

                 /* We don't use array->data.dataPtr as this is a 
                    redundant information and would introduce extra 
                    memory burden. Array->data.dataPtr requires memory 
                    to be contiguous */

             *(char**) ((char*) containerArray->arrayPtr+ row * sizeof(char*)) = (char*) instance;
             containerArray->dim[0]++;
             break;

        case  CHAIN_TYPE:

             if(shChainElementAddByPos(containerChain, instance,
                                   schemaName, TAIL, AFTER) != SH_SUCCESS)
             {  shErrStackPush("Can't add instances to the chain");
                 return -1;
             }

             break;

        default:
                    shErrStackPush("Can't identify the type of container.");
                    return -1;

    } 

    return 0;
}


 
static char* shCnvSetTableType(DSTTYPE dataType)

{
     switch(dataType)
     {
         case DST_HEAP:   return "TBLHEAPDSC";
         case DST_ENUM:
         case DST_STR:    return "STR";

         case DST_CHAR:   return "CHAR";
         case DST_SHORT:  return "SHORT";
         case DST_INT:    return "INT";
         case DST_LONG:   return "LONG";

         case DST_SCHAR:   return "SCHAR";
         case DST_UCHAR:   return "UCHAR";
         case DST_USHORT:  return "USHORT";
         case DST_UINT:    return "UINT";
         case DST_ULONG:   return "ULONG";

         case DST_DOUBLE: return "DOUBLE";

         case DST_FLOAT:  return "FLOAT";
         
         default:         return "UNKNOWN";
     }
}     





static int shCnvRuntimeStructInit(int size)
{
    void* ptr = shMalloc((sizeof(REDUND_ENTRY)+sizeof(ENTRY_INFO))*size);

    if(ptr == NULL) return -1;
    redundEntryInfo = (ENTRY_INFO*) ptr;
    entryTable = (REDUND_ENTRY*) ((char*)ptr + size*sizeof(ENTRY_INFO));
    return size;
}


static void shCnvRuntimeStructFree(void)
{
    shFree((void*) redundEntryInfo);
}
    


static int shCnvRedundInfoSet(
          SCHEMATRANS* xtbl_ptr        
)
{
   
      int i, j, k, entry;
      char tmp[MAX_NAME_LEN], *tmpPtr;
      SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;
      
      int tmp_num[MAX_INDIRECTION];
      int src_dimen, working_dimen = 0, *working_num = NULL;
      int entry_stack[MAX_IDENTICAL_ENTRIES];
      int entry_cnt = 0;
      int last_entry = xtbl_ptr->entryNum;
      int hash1, hash2;
   
     /* find all similar entries and put them in entry_stack */
      
     /* if there are no similar entries but entries do have
        brackets, we're seeing them as standalone one-element arrays */

      for(i=0; i < last_entry ;i++)
      {

             if(xptr[i].type==CONVERSION_CONTINUE || 
               xptr[i].type==CONVERSION_IGNORE  )  continue;

            if(shSchemaTransStatusGet(xtbl_ptr, i)!=SCHTR_FRESH) continue;

            shSchemaTransStatusSet(xtbl_ptr,SCHTR_VISITED, i);  /* mark as visited */
            hash1 = shSchemaTransGetHashValue(xptr[i].src);
            
            for(j=i+1; j < last_entry; j++)
             {
                  if(xptr[j].type==CONVERSION_CONTINUE || 
                     xptr[i].type==CONVERSION_IGNORE)  continue;

                 if(shSchemaTransStatusGet(xtbl_ptr, j)!=SCHTR_FRESH) continue;

                  /* screening out hash-different entries, because sttrchr,
                     strtok and strcmp in p_shSpptStringCmp() are expensive */

                 hash2 = shSchemaTransGetHashValue(xptr[j].src);
                 if(hash1 != hash2) continue;

                 if(p_shSpptStringCmp(xptr[i].src, xptr[j].src, TRUE) >= 0) 
                 {
                    if(entry_cnt == 0) entry_stack[entry_cnt++]=i;
                    entry_stack[entry_cnt++]=j;
                    shSchemaTransStatusSet(xtbl_ptr,SCHTR_VISITED, j);
                 }
             }
                 
            if(entry_cnt == 0)
            { 
                 tmpPtr=strcpy(tmp, xptr[i].src);
                 if(strchr(tmpPtr,'<')!=NULL||strchr(tmpPtr,'[')!=NULL)
                         entry_stack[entry_cnt++]=i;
            }

          /* process the same entries and extract their array info */

          for(j=0; j < entry_cnt; j++)
          {
               entry = entry_stack[j];
               if( working_num==NULL|| currentInfo==NULL )
                           
               {  
                   memset((char*) &redundEntryInfo[redundEntryInfoCnt], 0, 
                                    sizeof(struct _entry_info));
                   currentInfo = &redundEntryInfo[redundEntryInfoCnt];
                   working_num = currentInfo->num;
                   entryTable[entryTableCnt].entry=entry;
                   entryTable[entryTableCnt].infoPtr = currentInfo;
                   if((src_dimen=p_shSpptArrayInfoExtract(xptr[entry].src, tmp_num)) < 0) 
                   {
                               shErrStackPush("shCnvRedundInfoSet: Array extraction error\n");
                               return -1;
                    }
                   working_dimen = src_dimen;
                   k=0;
                   while(k < src_dimen) {working_num[k] = tmp_num[k] +1;k++ ; }
                   currentInfo->status=0;
                   currentInfo->len=working_dimen;
                   tmpPtr=strcpy(tmp, xptr[entry].src);
                   if(strchr(tmpPtr,'<')!=NULL||strchr(tmpPtr,'[')!=NULL)
                                tmpPtr=strcpy(tmp,strtok(tmp,"<["));
                   strcpy(currentInfo->src, tmpPtr);
                   entryTableCnt++;
                   redundEntryInfoCnt++;
                   continue;
                }
                
                  entryTable[entryTableCnt].entry=entry;
                  entryTable[entryTableCnt].infoPtr = currentInfo;
                  memset((char*) tmp_num, 0, sizeof(tmp_num));
                  if((src_dimen=p_shSpptArrayInfoExtract(xptr[entry].src, tmp_num)) < 0) 
                  {
                       shErrStackPush("shCnvRedundInfoSet: Array extraction error\n");
                       return -1;
                  }
                  if(src_dimen > working_dimen)  working_dimen=src_dimen;
                  k=0;
                  while(k < working_dimen) 
                  {
                       if(tmp_num[k] +1 > working_num[k]) 
                                 working_num[k]=tmp_num[k] +1;
                       k++;
                  }
                        
                 entryTable[entryTableCnt++].infoPtr = currentInfo;
           }
           working_num =NULL;
           currentInfo=NULL;
           entry_cnt = 0;
     }
          
  
     return 0;
}


static int shCnvRedundNumGet(SCHEMATRANS* xtbl_ptr, int* redund)
{
      int i, j,  same_dst = 0, same_src=0;
      SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;
      int last_entry = xtbl_ptr->entryNum;
      int hash1, hash2;

      shSchemaTransStatusInit(xtbl_ptr);
      redundEntryInfoCnt = entryTableCnt = 0;

     /* don't ever try to save some codes by grouping following
      * codes into one big loop because the two for loops are
      * very similar. The state of the translation table changes
      * in each loop, and thus group the codes together would
      * impose certain execution order, which would result in
      * inaccurate calculation of the redundancies. That's why
      * a shSchemaTransStatusInit() is needed between loops to
      * restore the translation table. Better believe me!
      */

      for(i=0; i < last_entry ;i++)
      {

            if(xptr[i].type==CONVERSION_CONTINUE || 
               xptr[i].type==CONVERSION_IGNORE )  continue;

            if(shSchemaTransStatusGet(xtbl_ptr, i)!=SCHTR_FRESH) continue;

            shSchemaTransStatusSet(xtbl_ptr,SCHTR_VISITED, i);  /* mark as visited */
            hash1 = shSchemaTransGetHashValue(xptr[i].dst);
            
            for(j=i+1; j < last_entry; j++)
             {
                 if(xptr[j].type==CONVERSION_CONTINUE || 
                     xptr[j].type==CONVERSION_IGNORE)  continue;

                 if(shSchemaTransStatusGet(xtbl_ptr, j)!=SCHTR_FRESH) continue;
 
                 if(strcmp(xptr[i].src, xptr[j].src)==0 )
                 {
                     shErrStackPush("Entry %d attempting to overwrite entry %d", j, i);
                     return -1;
                 }

                 hash2 = shSchemaTransGetHashValue(xptr[j].dst);
                 if(hash1 != hash2 ) continue;

                 if(p_shSpptStringCmp(xptr[i].dst, xptr[j].dst, TRUE) >= 0) 
                 {
                       same_dst++;
                       shSchemaTransStatusSet(xtbl_ptr,SCHTR_VISITED, j);
                       
                 }
                 
             }
                 
      }
     shSchemaTransStatusInit(xtbl_ptr);
     for(i=0; i < last_entry ;i++)
      {   
            if(xptr[i].type==CONVERSION_CONTINUE || 
               xptr[i].type==CONVERSION_IGNORE )  continue;
            if(shSchemaTransStatusGet(xtbl_ptr, i)!=SCHTR_FRESH) continue;
            shSchemaTransStatusSet(xtbl_ptr,SCHTR_VISITED, i);  /* mark as visited */
            hash1 = shSchemaTransGetHashValue(xptr[i].src);
            for(j=i+1; j < last_entry ; j++)
             {
                 if(xptr[j].type==CONVERSION_CONTINUE || 
                     xptr[j].type==CONVERSION_IGNORE)  continue;

                 if(shSchemaTransStatusGet(xtbl_ptr, j)!=SCHTR_FRESH) continue;
                 hash2 = shSchemaTransGetHashValue(xptr[j].src);
                 if(hash1 != hash2) continue;
                 if(p_shSpptStringCmp(xptr[i].src, xptr[j].src, TRUE) >= 0) 
                 {
                       same_src++;
                       shSchemaTransStatusSet(xtbl_ptr,SCHTR_VISITED, j);
                 }

                 
             }
                 
      }

      shSchemaTransStatusInit(xtbl_ptr);
      if(shCnvRedundInfoSet(xtbl_ptr) < 0) return -1;
      shSchemaTransStatusInit(xtbl_ptr);
      *redund = same_dst - same_src;
      return 0;

}


/*****************************************************************************/
/****** Following two routines are the conversion routines ***************/ 

/* this is 1st of the two most important routines. it extracts objects from
   TBLCOL and, depending on type of object given in argument 
   list, copy the data to the given object or store the objects in
   to the given container.
*/
 
   
RET_CODE   
shCnvTblToSchema(
   TBLCOL* 	tblCol,
   SCHEMATRANS* xtbl_ptr,
   void*        object,
   char* 	schemaName,
   char** 	addr,             	/* address passed in from outside */
   int    	size,        		/*  Number of addresses available in schemaInstAddr */
   int 		row_index,        	/* row index. if < 0, 
                         		means coverting all of the rows */
   int 		stop_index,
   CONTAINER_TYPE  containerType,
   unsigned short objectReuse   /* true if we reuse the old objects */
)
{
    

     SCHEMA_ELEM *elemptr=     	  NULL;    /* points to a field of rootSchema */
     unsigned convert_all = FALSE;

     int      fldindx    = 0;           /* TBLCOL field index */
     char*    dststr     = NULL;        /* string to map to a FITS field */
    
     char*    upperLevelPtr = NULL;	/* points immediately before a field in an obj */
     char*   objSideDataPtr_save = NULL;
     

     int i, j, k, row;  					/* loop indice */
     int entry_stack[MAX_IDENTICAL_ENTRIES], entry_nums;	/* entry holder */
     
     double convRatio=1.0; 			/* conversion ratio between fits data and 
                 				   object data due to different units */
     double scale;        			/* scale factor */
     double zero;        			/* zero translation */

     CONVTYPE type = CONVERSION_END;

     int entry_fnd;  				/* current entry */
     int entry_save;				/* current entry save */
     
     int start_row =0, stop_row = 0;
     int tmp_int;                          	/* generic tmp integer */

     int heaplen = 0, heap_base_size =0;
     int heap_dimen =0, heap_block_num=1;	/* heap stuff */
     TBLHEAPDSC* heapdsc = NULL;     
           
     char tmp[MAX_NAME_LEN];
     int  tmp_num[MAX_INDIRECTION];		/* to store dimension info */

     if(tblCol==NULL || object==NULL || xtbl_ptr==NULL || schemaName==NULL)
     {
         shErrStackPush("shCnvTblToSchema: invalid arguments");
         return SH_GENERIC_ERROR;
     }

     if(tblCol->rowCnt <= 0 ) return SH_SUCCESS;
   
     rootSchemaPtr = shSchemaGet(schemaName);

     if(containerType == NOT_A_CONTAINER)
     {
           /* case:  one single row at time */
 
         if(row_index < 0) 
         { 
               shErrStackPush("shCnvTblToSchema: bad indexing"); 
               return SH_GENERIC_ERROR;
         }
         start_row = row_index; 
         stop_row = start_row + 1;
         objInstancePtr = (char*) object;
         convert_all = FALSE;
     }
     else if (containerType == ARRAY_TYPE)
     {
         /* case: put the objects in the containerArray array */
      
         ((ARRAY*)object)->data.schemaType
                               =shTypeGetFromName(schemaName);
         convert_all = TRUE;
         if( ((ARRAY*)object)->dimCnt > 1) 
         { 
              shErrStackPush("Multi-D array not allowed. Array is not empty.");
              return SH_MULTI_D_ERR;
         }
     }
     else if (containerType == CHAIN_TYPE)
     {
         convert_all = TRUE;
     }
     else 
     {
         shErrStackPush("Unsupported container"); 
         return SH_BAD_CONTAINER;
     }

     if(rootSchemaPtr==NULL) 
     {
          shErrStackPush("No knowledge about your schema");
          return SH_BAD_SCHEMA;
     }

     if(convert_all)
     {   

           start_row = (row_index >= 0) ? row_index:0;
           stop_row = (stop_index >= 0) ? stop_index:0;
           if(stop_row > tblCol->rowCnt) stop_row = tblCol->rowCnt;

           if(addr==NULL || size < stop_row -start_row)
           {         
              shErrStackPush("Not enough objects supplied");
              return SH_OBJNUM_INVALID;
           }
      }

     transTblEntryPtr      = xtbl_ptr->entryPtr;


     /* Let's do the extraction for each field in the schema */

      for(i=0; i < rootSchemaPtr->nelem; i++)
      {  
            entry_nums=0;
            elemptr = &rootSchemaPtr->elems[i];
            entry_fnd = p_shSpptEntryFind(xtbl_ptr, elemptr->name);
            entry_stack[entry_nums++] = entry_fnd;

            /* translation table may have multiple entry with same name but different
               indice -- we've got to find out all of them, because a field name is
               only searched once 
             */

            while(entry_fnd >= 0)
            {
               entry_fnd = p_shSpptEntryFind(xtbl_ptr, elemptr->name);
               if(entry_fnd >= 0)  entry_stack[entry_nums++] = entry_fnd;
            }

           for(k=0; k < entry_nums; k++)
           {
               entry_fnd = entry_stack[k];
               if(entry_fnd < 0 ) 
               {
                  /* the case that this particular field is not listed in
                     translation field. Let's use the field's orginal
                     name and hopefully TBLCOL has a field by this name.
                  */
/*
                     dststr=elemptr->name;
                     type = CONVERSION_END;
                     convRatio = 1.0;
                     objSideDataType = shSchemaTransDataTypeGet(elemptr->type, 
                                       &objSideDataSize);
*/
/* Let us bypass automated conversion entirely (there is a bug that it
   triggers) - we don't use this much anyway */
		     continue;
                }   
               else 
               {
                   /* the field IS listed in the translation table.
                      we then use the returned name to search in TBLCOL.
                      Before that, we have to strip off array specification
                      that might be present in the string.
                    */
 
                   dststr = strcpy(tmp, transTblEntryPtr[entry_fnd].src);
                   while(*dststr!='\0'&&*dststr!='['&& *dststr!='<') dststr++;
                   *dststr='\0';
                   dststr=tmp;
                   type = transTblEntryPtr[entry_fnd].type;
                   convRatio = transTblEntryPtr[entry_fnd].srcTodst;
                   
               }
  
            
               if(type==CONVERSION_BY_TYPE || type==CONVERSION_END) 
               {
                   if(shTblFldLoc(tblCol, -1, dststr, 0, SH_CASE_INSENSITIVE, &curArrayPtr,
                       &curTblfldPtr, &fldindx)!=SH_SUCCESS) 
                   {
                       shErrStackPush("Failed to find the right field "
                           "by name \"%s\", ignore it\n", dststr);
                       continue;
                   }
               }

               else if(type==CONVERSION_IGNORE) continue;
                
         
             /* fill the data of this curTblfldPtr to that of elemptr */

                
                fitsSideNumOfBlocks=1;
                fitsSideBlocksPerRow=1; 
                fitsSideArrayIndex=0;
                fitsSideArrayDimen=0;

                /* extract array info for fits-side fields */

                if(entry_fnd >= 0) 
                {
                    if(p_shSpptArrayCheck(transTblEntryPtr[entry_fnd].src, (int*)&curArrayPtr->dim[1], 
                          &fitsSideArrayIndex, &fitsSideArrayDimen, NULL)==NULL)
                    {
                        shErrStackPush("Fits-field array info extraction failure"
                              " for entry %d\n", entry_fnd);
                        return  SH_ARRAY_CHECK_ERR;
                    }

                    fitsSideArrayIndex--;
                    
                }


                /* match the dimension */
      
                if(fitsSideArrayDimen > curArrayPtr->dimCnt -1)
                {
                   shErrStackPush("Illegal %s: %s has %d dimension(s).\n",
                         transTblEntryPtr[entry_fnd].src, dststr,curArrayPtr->dimCnt -1);
                   return  SH_ARRAY_CHECK_ERR;
                }
                
                /* fits field might be an array, deal with it */
                /* get the total number of objSideNumOfBlocks from FITS in fitsSideBlocksPerRow */

                if(curArrayPtr->dimCnt > 1) 
                {        
                    for(j=1; j < curArrayPtr->dimCnt; j++) 
                    {
                         if(j > fitsSideArrayDimen) fitsSideNumOfBlocks *= curArrayPtr->dim[j];
                         fitsSideBlocksPerRow *= curArrayPtr->dim[j];
                    }
                     
                }
                entry_save = entry_fnd;  

                if(entry_fnd < 0 ) 
                {
                     if(shSchemaGet(elemptr->type)!=NULL) 
                         objSideDataSize=shSchemaGet(elemptr->type)->size;
                     else 
                     {  
                         shErrStackPush("Field %s has unknown size\n", 
                                             elemptr->name);
                         return SH_BAD_SCHEMA;
                      }
                 }

                /* get the pointer to the actual destination */

                 if(p_shSpptTransTblWalk(xtbl_ptr, elemptr, 
                                &objElemPtr, &entry_fnd)==NULL)
                          return SH_GENERIC_ERROR;

                 /* after traversing, we'd have to find array info
                    given on the last continuation line. Note entry_fnd
                    already points to the line now */

                 if(entry_fnd >=0 ) 
                 {    
                      /* use the type on last continuation line */

                       objSideDataType = shSchemaTransDataTypeGet(transTblEntryPtr[entry_fnd].dsttype, 
                                                    &objSideDataSize);

                      /* objSideDataType = transTblEntryPtr[entry_fnd].dstDataType;  */

                       objSideArrayIndex = 0;
                       objSideAddedDimen = 0;

                    /*  heap dimension information is not extractable thru this routine 
                        we have to use p_shSpptHeapDimenGet later on   */

                      if(objSideDataType!=DST_HEAP) 
                       {
                          p_shSpptArrayCheck(transTblEntryPtr[entry_fnd].dst, transTblEntryPtr[entry_fnd].num, 
                            &objSideArrayIndex, &objSideArrayDimen, NULL);
                          objSideArrayIndex--;

                          p_shSpptArrayCheck(transTblEntryPtr[entry_fnd].size, NULL, &objSideAddedArraySize, 
                                      &objSideAddedDimen, "x");
                      } 
                      else objSideHeapDataType=shSchemaTransDataTypeGet(transTblEntryPtr[entry_fnd].heaptype, 
                                                   &objSideDataSize);
      
           
                  }

   
                  /* actually transTblEntryPtr[entry].size information is stored in num[] 
                        we did it here for consistency anyway */
         
                 p_shSpptArrayCheck(objElemPtr->nelem, NULL,
                         &objSideStaticArraySize, &objSideStaticDimen, " ");
            
               
                if( objSideDataType!=DST_HEAP)
                {
                   /* objSideAddedDimen = objElemPtr->nstar for elementary types */

                   objSideTotalDimen = objSideAddedDimen + objSideStaticDimen - objSideArrayDimen;
                   if(objSideTotalDimen < 0) 
                   {
                        shErrStackPush("Too many dimension on entry: %d\n", 
                                           entry_fnd); 
                        return SH_ARRAY_CHECK_ERR;
                   }
                   objSideTotalArraySize=1;
                   for(j=objSideArrayDimen; j <  objSideAddedDimen + objSideStaticDimen; j++) 
                       objSideTotalArraySize *= transTblEntryPtr[entry_fnd].num[j];
                   if(objSideTotalArraySize <= 0) 
                   {
                        shErrStackPush("Bad array_size on entry: %d\n", 
                                  entry_fnd); 
                        return SH_TABLE_SYNTAX_ERR;
                   }
                }

               

               /* this is a new check -- check if the array dimens are the same */
                 
                 if(objSideDataType == DST_STR)
                 {

                    if(objSideTotalArraySize!=1 && 
                          objSideTotalDimen != curArrayPtr->dimCnt - 1 -fitsSideArrayDimen)
                    { 
                      shErrStackPush("For entry %d's %s: Schema/Fits have different dimension: %d vs %d", 
                             entry_fnd, objElemPtr->name, objSideTotalDimen, 
                             curArrayPtr->dimCnt - 1- fitsSideArrayDimen);
                       return SH_SIZE_UNMATCH;
                     }
                    if((objSideTotalArraySize!=1 && objSideTotalArraySize < fitsSideNumOfBlocks) ||
                        (objSideTotalArraySize==1 && fitsSideNumOfBlocks!=2))
                     {  
                       shErrStackPush("Schema/Fits space not same size: %d vs %d at entry %d",
                            objSideTotalArraySize,fitsSideNumOfBlocks, entry_fnd);
                       return SH_SIZE_UNMATCH;
                     }
                 }

                 else 
                 {
                        /* enum is special. Dimension difference should be 1 
                           since it converts int to array of chars */

                    if(objSideDataType==DST_ENUM)
                    {
                       if(objSideTotalDimen +1 != curArrayPtr->dimCnt -1 - fitsSideArrayDimen)
                       {
                           shErrStackPush("Entry %d: enum has %d dimension, but TBLCOL side "
                                "is %d-D. One enum number should correspond to 1-D array of "
                                "characters.\n", entry_fnd, objSideTotalDimen, 
                                curArrayPtr->dimCnt -1);
                           return SH_SIZE_UNMATCH;
                       }

                    }

                   /* heap is also special, we don't even bother to check it */

                    else if( objSideDataType!=DST_HEAP &&
                         objSideTotalDimen != curArrayPtr->dimCnt -1 - fitsSideArrayDimen)   
                     {
		       if( (curArrayPtr->dimCnt -1 - fitsSideArrayDimen != 1) ||
			    (objSideDataType != DST_CHAR) ) {
			 shErrStackPush("Entry %d: Schema/Fits have different dimension "
					"for %s: %d vs %d", entry_fnd, objElemPtr->name, 
					objSideTotalDimen, curArrayPtr->dimCnt -1-fitsSideArrayDimen);
			 return SH_SIZE_UNMATCH;
		       }
                     }
                    
                 /* check if numbers of elements in the arrays are same */
                 /* again, enum/heap are excluded. Only dimension info is important to enum */
        
                   if(objSideDataType!=DST_ENUM && objSideDataType!=DST_HEAP 
		      && objSideTotalArraySize!=fitsSideNumOfBlocks) {  
		     if (objSideTotalArraySize!=1 || fitsSideNumOfBlocks!=2 || objSideDataType!=DST_CHAR) {
		       printf("Warning: Schema/Fits space not same size: %d vs %d at entry %d. ",
			      objSideTotalArraySize,fitsSideNumOfBlocks, entry_fnd);
		       printf("use size %d \n", size_min(objSideTotalArraySize, fitsSideNumOfBlocks));
		     }
                   }
		 }

               /* figure out the scale/zero stuff */

                scale = 1.0;
                zero  = 0.0;

                if(shInSet(curTblfldPtr->Tpres, SH_TBL_TSCAL)) scale *= curTblfldPtr->TSCAL;
                if(shInSet(curTblfldPtr->Tpres, SH_TBL_TZERO)) zero  += curTblfldPtr->TZERO;

                scale *= convRatio;     /* always true */
                
                fitsSideArrayIndex /= fitsSideNumOfBlocks;

               /* now lets find source/dstination data pointer and copy data */

               for(row = start_row; row < stop_row ; row++)
               {
                   entry_fnd = entry_stack[k];
                   if(convert_all) objInstancePtr = addr[row - start_row];
                   objSideDataPtr=(char*) objInstancePtr + elemptr->offset;
                   objSideDataPtr = p_shSpptTransTblTraverse(xtbl_ptr, elemptr, 
                                 &objElemPtr, objSideDataPtr, &entry_fnd, &upperLevelPtr);
                   if (objSideDataType!=DST_HEAP)
                   { 
                      if(objSideDataPtr==NULL) 
                      {
                         shErrStackPush("shCnvTblToSchema: dst data pointer ="
                            "null for %s\n when processing entry %d", elemptr->name, entry_fnd);
                         return SH_INV_DATA_PTR;
                       }
                   }
                   else objSideDataPtr = upperLevelPtr;
     
                  if(objSideDataType==DST_ENUM)
                  { 
                      /* if enum is specified, FITS side has to be string */

		      if(curArrayPtr->data.schemaType!=shTypeGetFromName("STR"))
                      {
                         shErrStackPush("Can't convert a non-string field to"
                            "enum type. Change entry %d to use int, please\n",
                              entry_fnd);
                         return SH_GENERIC_ERROR;
                      }

                      fitsSideDataPtr = (char*)curArrayPtr->data.dataPtr;
                      fitsSideDataPtr += fitsSideArrayIndex*fitsSideNumOfBlocks;  /* objSideDataSize = 1 */
                      if((tmp_int = p_shSpptGetMaxStrlenOfEnum(objElemPtr->type, NULL)) < 0)
                      {
                          shErrStackPush("Can't get maximum enum member length\n");
                          return SH_GENERIC_ERROR;
                      }
                      tmp_int += strlen(objElemPtr->type) + 2;  /* for 2 tabs */
                      if(tmp_int > MAX_ENUM_LEN )
                      {
                          shErrStackPush("Names in enum %s are too long.\n"
                                "Shorten the name or increase MAX_ENUM_LEN in shSchema.c\n");
                          return SH_GENERIC_ERROR;
                      }
                      fitsSideDataPtr += row * fitsSideBlocksPerRow;   /* objSideDataSize = 1 */
                  }
                  else if(objSideDataType==DST_HEAP)  /* init the heap field */
                  {
                      /* heap number is stored in fitsSideDataPtr for converison in
                         this direction, but if heaplen is given in table 
                         we'll have to check later to see if it is consistent */
                     
                     fitsSideDataPtr = (char*)curArrayPtr->data.dataPtr;
                     if(curArrayPtr->data.schemaType!=shTypeGetFromName("TBLHEAPDSC"))
                     {
                          shErrStackPush("Can't copy heap data to non-heap data\n");
                          return SH_GENERIC_ERROR;
                     }
                     heapdsc = (TBLHEAPDSC*) fitsSideDataPtr;
		     heapdsc += row; 
                     fitsSideDataPtr = (char*) heapdsc->ptr; /* this is where the data should be */   
	             heaplen = heapdsc->cnt; 
                  /*
                     heap_base_size = shSchemaGet(
                               shNameGetFromType(curTblfldPtr->heap.schemaType))->size; 
                   */

                     shSchemaTransDataTypeGet(transTblEntryPtr[entry_fnd].heaptype, &heap_base_size);

                     if(heap_base_size <= 0) 
                     {
                          shErrStackPush("Don't know what heap's base size is\n");
                          return SH_GENERIC_ERROR;
                     }

                     /* malloc memory for heap */
                     
                     if(curTblfldPtr->heap.schemaType!=shTypeGetFromName("STR") || heaplen != 1)
                     {
   
                        if(p_shSpptHeapFldInit((SCHEMA*) rootSchemaPtr, objInstancePtr, 
                               objElemPtr, (char**) objSideDataPtr, xtbl_ptr, entry_fnd, 
                                   heap_base_size*heaplen) <= 0)
                        {  
                              shErrStackPush("shCnvTblToSchema: Heap field initialization "
                                      "error for entry %d", entry_fnd);
			      return SH_MALLOC_ERR;
                        }
                     }

#if 0
                     /* This part is useful for debugging! */
                    /* OK, retraverse, but reset entry_fnd first */

                     entry_fnd = entry_stack[k];

                     objSideDataPtr=(char*) objInstancePtr + elemptr->offset;
                     objSideDataPtr = p_shSpptTransTblTraverse(xtbl_ptr, elemptr, 
                                &objElemPtr, objSideDataPtr, &entry_fnd, &upperLevelPtr);

                     if(objSideDataPtr==NULL) 
                     {
                         shErrStackPush("shCnvTblToSchema: heap data pointer ="
                            "null for %s\n", elemptr->name);
                         return SH_INV_DATA_PTR;
                      }
#endif
                   }

                   else if(objSideDataType==DST_STR)
                   {
                      if(curArrayPtr->data.schemaType!=shTypeGetFromName("STR"))
                      {
                            shErrStackPush("FITS side is not a string type for entry %d",entry_fnd); 
                            return SH_GENERIC_ERROR;
                      }

                      fitsSideDataPtr = (char*)curArrayPtr->data.dataPtr + row*fitsSideBlocksPerRow;
                      fitsSideDataPtr += fitsSideArrayIndex*curArrayPtr->data.size*fitsSideNumOfBlocks;
                   }

                   else /* don't care what type FITS side is, becaus cast and copy will be used */
                   {
                       fitsSideDataPtr = (char*) curArrayPtr->data.dataPtr + 
                          row*fitsSideBlocksPerRow*curArrayPtr->data.size;
                       fitsSideDataPtr += fitsSideArrayIndex*curArrayPtr->data.size*fitsSideNumOfBlocks;
                   }

                 /* compare source type and dest type, print warnings if necessary */

                if(row == start_row)
                {
                    if(objSideDataType!=DST_HEAP) 
                    {
                        fitsSideDataType = shSpptSchemaTypeToDSTType(curArrayPtr->data.schemaType);

                        if(fitsSideDataType!=objSideDataType)
                        {
                              /* filter out certain combinations and print a warning for the rest */

                            if(fitsSideDataType == DST_STR && objSideDataType==DST_ENUM) {}
                            else if(fitsSideDataType == DST_LONG && objSideDataType == DST_INT) {}
                            else if(fitsSideDataType == DST_UINT && objSideDataType == DST_INT) {}
                            else if(fitsSideDataType == DST_FLOAT && objSideDataType==DST_DOUBLE) {}
                            else if(fitsSideDataType == DST_STR && objSideDataType==DST_CHAR) {}

                       /* I'll have to warning if fitsSide is double while objSide is float *
                        * because of precision loss                                         */

                         else shError("Warning: FITS side is %s, but translation table says %s at entry %d (%s)",
                                      shNameGetFromType(curArrayPtr->data.schemaType), 
                                      transTblEntryPtr[entry_fnd].dsttype, entry_fnd, curTblfldPtr->TTYPE);
                        }
                    }

                    else  
                    {
                       fitsSideDataType = shSpptSchemaTypeToDSTType(curTblfldPtr->heap.schemaType);
                       if(fitsSideDataType!=objSideHeapDataType)
                           shError("Warning: FITS side (heap) has %s, but translation table says %s at entry %d (%s)",
                                      shNameGetFromType(curTblfldPtr->heap.schemaType), 
                                      transTblEntryPtr[entry_fnd].heaptype, entry_fnd, curTblfldPtr->TTYPE);
                    }
                    
                 }
                 

                 if(objSideDataType!=DST_HEAP) 
                 {
                    /* Urah, the last thing */
            
                    if(p_shSpptCastAndConvert(fitsSideDataPtr, fitsSideDataType, objSideDataPtr, 
                          (DSTTYPE) objSideDataType, size_min(objSideTotalArraySize,fitsSideNumOfBlocks),
                           scale, zero) < 0) 
                    {
                         shErrStackPush("shCnvTblToSchema: Can't do value conversion\n");
		         return SH_GENERIC_ERROR;
		    }

                 }
                 else
                 {
                        /* the heap stuff */

                 /* idea: get objSideArrayDimen, objSideDataPtr, traverse all the memory
                    areas below objSideDataPtr. We know objSideTotalDimension, obtained
                    from -dimen switch and stored in a tmp_num plus heaplen,
                    The number of dimensions we need to climb down would be
                    objSideTotalDimension(include heaplen) minus objSideArrayDimen. Pass in
                    the num information, and a given block to access (can
                    index from 0 to total_num of objSideNumOfBlocks, calculatable from
                    num), and then directly do memcpy. Use ArrayInstGet.

                 */


                    heap_dimen=p_shSpptHeapDimenGet((SCHEMA*) rootSchemaPtr, objInstancePtr, 
                                    transTblEntryPtr[entry_fnd].size, tmp_num, FALSE);
                  
                  /* push the new tmp_num info into transTblEntryPtr[entry].num */
                  
                    for(j = 0; j < heap_dimen; j++) transTblEntryPtr[entry_fnd].num[j+objSideStaticDimen]=tmp_num[j];
                    
                    p_shSpptArrayCheck(transTblEntryPtr[entry_fnd].dst, transTblEntryPtr[entry_fnd].num, 
                            &objSideArrayIndex, &objSideArrayDimen, NULL);
                    objSideArrayIndex--;

                    heap_block_num =1;
                    for(j=objSideArrayDimen; j < objSideStaticDimen+heap_dimen; j++) 
                                     heap_block_num *= transTblEntryPtr[entry_fnd].num[j];

                    /* heaplength is tricky. If heap_block_num is 1, everything is fine,
                      because heaplength should be consistent with heapdsc->cnt. But the block
                      number is not 1, heapdsc->cnt is larger than the result of
                      p_shSpptHeapDimenGet. If such result is not valid (i.e, tmp_int <=0),
                      we have no way of knowing what's the proper heaplength */

                    if(heap_block_num!=1)
                        tmp_int=p_shSpptHeapDimenGet((SCHEMA*)rootSchemaPtr, objInstancePtr, 
                                    transTblEntryPtr[entry_fnd].heaplen,NULL, TRUE);
                    else tmp_int = heapdsc->cnt;

                    if( heap_block_num <= 0)
                    {
                       shErrStackPush("Heap array specification error: entry=%d", entry_fnd);
                       return SH_GENERIC_ERROR;
                     }

                  /* a heap that exists in an static array C structure would be very weired
                     but we still have to deal with this situation. We first deRef upperLevelPtr,
                     which immediately points to the field before array shift or indirection is
                     taken. First ArrayInstGet tries to get the address indicated by transTblEntryPtr[entry].dst.
                     Second ArrayInstGet goes further down to fetch all the valid pointer that can be
                     found by deref-ing objSideDataPtr. */
                     
                 
                    objSideDataPtr_save = upperLevelPtr;
                    objSideTotalDimen = heap_dimen + objSideStaticDimen;

                    for(j=0; j < heap_block_num; j++)
                    {
                       objSideDataPtr = p_shSpptHeapBlockGet(upperLevelPtr,
                                  objElemPtr->nstar,
                                  objSideStaticDimen,
                                  objSideArrayDimen, 
                                  objSideTotalDimen,
                                  transTblEntryPtr[entry_fnd].num,
                                  objSideArrayIndex,
                                  j);
                       if(objSideHeapDataType != DST_STR)
                       {
                           if(objSideDataPtr == NULL)
                           {
                               shErrStackPush("Heap destination pointer NULL for %dth heap block",j);
                               return SH_GENERIC_ERROR;
                           }

                           /* tmp_int is the block size */

                           memcpy(objSideDataPtr, fitsSideDataPtr + j*tmp_int*objSideDataSize , 
                                                tmp_int*objSideDataSize); 
                       }
                       else
                       {
                            /* objSideDataSize = 1 for this case, tmp_int includes extra \0 */

                           if(objSideDataPtr != NULL) 
                               memcpy(objSideDataPtr, fitsSideDataPtr + j*tmp_int , tmp_int); 
                       }
  
                     }

                 }/* else */

              } /* for-loop over row */

           } /* for-loop of k */

        } /* for-loop over i*/
         
        shSchemaTransStatusInit(xtbl_ptr); 
     
        if(!objectReuse && convert_all)
        { 
           for(row = 0; row < stop_row - start_row; row++)
            {
                  
                if(shCnvObjectStore(containerType, object, 
                   /* objInstancePtr */ addr[row], row, stop_row - start_row, schemaName) < 0)
                {  
                    shErrStackPush("shCnvTblToSchema: Fail to add objects to the container");
                    return SH_CONTAINER_ADD_ERR;
                }
            }
         } 

       return SH_SUCCESS;
}      
 

/* this routine extract the data from object(s) given in 
   the argument list and write the data to a TBLCOL that
   will be created internally. On successful return, it
   attaches the created TBLCOL address to a handle that
   was passed in -- tblcolHandlePtr,
   
*/

RET_CODE
shCnvSchemaToTbl(
   TBLCOL** tblcolPtrPtr,    		/* out: tblcol instance to convert to */
   void*   object,    		/* in: schema or array to convert from */
   SCHEMATRANS* xtbl_ptr,       /* in: translation table structure */
   char*   schemaName,          /* schema name that's in array */
   CONTAINER_TYPE  containerType,
   unsigned want_auto_convert	/* TRUE if one wants auto_convert */
)
{

   SCHEMA* tmpSchema     =   NULL;     	/* tmp schema variable */
   SCHEMA_ELEM * elemptr =   NULL;	/* field of a root schema */
   
   TBLCOL*      tblcol   =   NULL;

   void *walkPtr;			/* link context pointer */
   
    /* multiple objects? redundant entry? want_auto_convert? */

   unsigned convert_all = FALSE;
   unsigned use_redund  = FALSE ;
   unsigned auto_convert = want_auto_convert;  	
                 
   int start_row, stop_row;
   int entry_fnd;			/* current entry */
	
   int i, j, k, l, fldIdx = -1, fldIdx_save, row;		/* loop indices */
   char* dststr  =   NULL;       			/* misnomer for mapped string */

   double convRatio =1.0;
   
   CONVTYPE type;
   
   char tmp1[MAX_NAME_LEN], *tmpPtr1 = NULL;
   char tmp2[MAX_NAME_LEN], *tmpPtr2 = NULL;   	/* to use with strtok, which changes the argument */
   
   char *objSideDataPtr_save = NULL;		/* save a copy of global objSideDataPtr */
   char *upperLevelPtr       = NULL;    	/* points to immediately before a field in an obj */
   
   int fld_num;          			/* current field number in TBLCOL */

   int tmp_num[MAX_INDIRECTION];		/* tmp array information */
   int redund = 0;				/* number of redundancy in a translation table */
   

   int entry_nums;				
   int entry_stack[MAX_IDENTICAL_ENTRIES];  	/* entry holder: hold all related entries */


   unsigned short has_arrow = FALSE;		/* related to a string */
   unsigned has_square = FALSE;
   char half_token[5];       	

   int tmp_int;                       		/* generic int storage */
	
   TBLHEAPDSC *heapdsc=NULL;			/* heap stuff */
   unsigned char* extrCnt=NULL;			
   int heap_dimen, heap_length, heap_block_num;

   
   if(schemaName==NULL|| object==NULL || xtbl_ptr==NULL) 
    {
         shErrStackPush("shCnvSchemaToTbl: invalid arguments");
         return SH_INVARG;
    }
    
   rootSchemaPtr = (SCHEMA*) shSchemaGet(schemaName);

    /* figure out what is passed in: object or container? */

    if(containerType == NOT_A_CONTAINER)
    {
           /* case:  single-row conversion */
         start_row =0; stop_row = 1;
         objInstancePtr = (char*) object;
         convert_all = FALSE;
     }
     else if(containerType == ARRAY_TYPE)
     {
         
         if(((ARRAY*) object)->dimCnt > 1) 
         {
              shErrStackPush("Multi-D array is not allowed.");
              return SH_MULTI_D_ERR;
         }

         if(((ARRAY*)object)->nStar > 1 ) 
         {
              shErrStackPush("Warning: unsupported number of indirections %d\n",
                         ((ARRAY*)object)->nStar );
         }

         start_row = 0; stop_row = ((ARRAY*)object)->dim[0];
         if(stop_row <= 0) return SH_SUCCESS;
         if(((ARRAY*) object)->data.schemaType!=shTypeGetFromName(schemaName))
         {
               shErrStackPush("Containee type and supplied schema mismatch.");
               return SH_TYPE_MISMATCH;
         }
         convert_all = TRUE;
     }
     else if(containerType == CHAIN_TYPE)
     {
         /* case: convert all the objects in the containerChain */

         start_row = 0; stop_row = shChainSize((CHAIN*)object);
       /*  if(stop_row <= 0) return SH_SUCCESS; */
         if(shChainTypeGet((const CHAIN*)object)!=shTypeGetFromName(schemaName))
         {
               shErrStackPush("Containee type and supplied schema mismatch.");
               return SH_TYPE_MISMATCH;
         }
         convert_all = TRUE;
     }
     else
     {
         shErrStackPush("Unsupported container");
         return SH_BAD_CONTAINER;
     }
 
     if(rootSchemaPtr==NULL) 
     {
          shErrStackPush("No knowledge about your schema");
          return SH_BAD_SCHEMA;
     }

     transTblEntryPtr = xtbl_ptr->entryPtr;

    /* find the actual number of fields we need create in TBLCOL */

     fld_num = rootSchemaPtr->nelem;
   
     for(i=0; i < rootSchemaPtr->nelem; i++)
     {
        elemptr = &rootSchemaPtr->elems[i];
        if(transTblEntryPtr!=NULL) 
              entry_fnd = p_shSpptEntryFind(xtbl_ptr, elemptr->name);
        else entry_fnd = -1;
        if(entry_fnd < 0)
        {
            /* this field isn't even listed in the translation table.
               if it is complex type, we simply ignore it. fld_num is
               then decreased by one. Or if auto_convert is FALSE. fld_num
	       is also decremented.
             */
          if(auto_convert)
          {
              objSideDataType = shSchemaTransDataTypeGet(elemptr->type, NULL);
              if(objSideDataType==DST_UNKNOWN || objSideDataType==DST_STRUCT)
              {
                printf("Can't autoconvert non-elementary types"
                          ": %s is a %s. Ignored.\n", elemptr->name, elemptr->type);
                fld_num--;
              }
           }
           else fld_num--;
        }
        else 
        {
          /* the field is listed, but if it is position-oriented we
             still have to ignore it. We don't know the final positions of
             FITS fields yet. We're not even sure how many fields there
             will be.
           */

          type=transTblEntryPtr[entry_fnd].type;
          if(type == CONVERSION_IGNORE) fld_num--;

        }
     }

    /* find the redundancy in the translation table, i.e, 
       names that appear multiple times.  Normally when there is
       no redundancy, different schema fields have different
       corresponding FITS fields. In practice, a same schema 
       field may be written to multiple different FITS fields,
       while a same FITS field may get data from different
       schema fields. The former case increases redundancy, but
       the lattter decreases the redundancy level. */  

    
     if(shCnvRuntimeStructInit(xtbl_ptr->totalNum) < 0)
     {
          shErrStackPush("shCnvSchemaToTbl: runtime structure init error");
          return SH_GENERIC_ERROR;
     }


     redund = 0;
     if(shCnvRedundNumGet(xtbl_ptr, &redund) < 0)
     {
          shErrStackPush("Redundancy check error");
          return SH_GENERIC_ERROR;
     }
     if(fld_num + redund < 1)
     {
         shErrStackPush("Too many redundant entries in translation table\n "
            "Attempt writing multiple schema fields to a same FITS field "
             "without array syntax \n");
         return  SH_BAD_FLD_NUM;
     }

    /* create the tblcol and corresponding TBLFLD here 
    
     if(stop_row == 0) 
     {
          *tblcolPtrPtr = shTblcolNew();
          if(*tblcolPtrPtr==NULL) shErrStackPush("Can't create a TBLCOL");
          return (*tblcolPtrPtr==NULL) ? SH_OBJ_CREATE_ERR:SH_SUCCESS;
     }
      */


     if(shTblColCreate(stop_row, fld_num +redund, 1, &tblcol) 
          != SH_SUCCESS )
      {
          shErrStackPush("Can't create TBLCOL");
          return  SH_OBJ_CREATE_ERR;
      }
    
     fldIdx = -1;     /* index that counts the field in TBLCOL, will start at zero */
     k = 0;           /* index that counts the field in schema */

     for(i=0; i < fld_num; i++, k++)
     {
          entry_nums=0;
          objSideArrayDimen = objSideAddedDimen =0;
          objSideArrayIndex =0; objSideAddedArraySize=1;
          elemptr = &rootSchemaPtr->elems[k];
          if(transTblEntryPtr!=NULL) 
              entry_fnd=p_shSpptEntryFind(xtbl_ptr, elemptr->name);
          else entry_fnd = -1;
          entry_stack[entry_nums++] = entry_fnd;
 
          while(entry_fnd >= 0)
          {
               entry_fnd = p_shSpptEntryFind(xtbl_ptr, elemptr->name);
               if(entry_fnd >= 0)  entry_stack[entry_nums++] = entry_fnd;
          }


          for(j=0; j < entry_nums; j++)
          {
                  
               entry_fnd = entry_stack[j];   
               if(entry_fnd < 0 ) 
                {

                  /* this field is not even listed in the table. Lets
                     use its name directly and hope it is not a complex
                     type (otherwise, we'd ignore it) */
                     
                     if(!auto_convert) {i--; continue;}
                     dststr=strcpy(tmp2, elemptr->name);
                     while(*dststr!='\0') *dststr++ = toupper(*dststr);
                     dststr=tmp2;
                     type = CONVERSION_END;
                     objSideDataType = shSchemaTransDataTypeGet(elemptr->type, 
                                           &objSideDataSize);
                     convRatio = 1.0;
                     objSideAddedArraySize=1;
                     objSideAddedDimen=0;
                     objElemPtr=elemptr;
                     if(objSideDataType==DST_UNKNOWN || 
                           objSideDataType==DST_STRUCT) 
                     {
                       i--;
                       continue;
                     }
 
                }
               else /* entry_fnd >= 0 */
               {
                   /* this field is listed, we get the the mapped string
                      and traverse its possible continuation lines */

                   dststr=strcpy(tmp2,  transTblEntryPtr[entry_fnd].src);
                   while(*dststr!='\0'&& *dststr!='['&& *dststr!='<') dststr++;
                   *dststr='\0';
                   dststr=tmp2;
                   type = transTblEntryPtr[entry_fnd].type;

                   if(type == CONVERSION_IGNORE)
                   {
                          i--; 
                          continue;
                   }
                   convRatio = transTblEntryPtr[entry_fnd].srcTodst;
                   while(transTblEntryPtr[++entry_fnd].type==CONVERSION_CONTINUE) {}
                   entry_fnd--;

                   objSideDataType = shSchemaTransDataTypeGet(
                            transTblEntryPtr[entry_fnd].dsttype, &objSideDataSize);
                   if(objSideDataType==DST_HEAP)
                   {  
                      objSideHeapDataType=shSchemaTransDataTypeGet(transTblEntryPtr[entry_fnd].heaptype, 
                                                       &objSideDataSize);
                   }
                   p_shSpptArrayCheck(transTblEntryPtr[entry_fnd].dst, 
                       transTblEntryPtr[entry_fnd].num, &objSideArrayIndex, &objSideArrayDimen, NULL);
                   objSideArrayIndex--;

                   p_shSpptArrayCheck(transTblEntryPtr[entry_fnd].size, NULL,
                              &objSideAddedArraySize, &objSideAddedDimen, "x");
                   

                   if(entry_fnd!=entry_stack[j]) 
                   {
                      /* so, the entry is followed by continuation lines
                         lets get the ultimate SCHEMA_ELEM  */

                      row = entry_stack[j];        /* row is used as a loop index here! */
                      tmpSchema = (SCHEMA*) rootSchemaPtr;
                      while(row < entry_fnd + 1)
                      {
                        tmpPtr1=strcpy(tmp1,transTblEntryPtr[row++].dst);
                        has_arrow = (strchr(tmp1, '<')!=NULL);
                        has_square = (strchr(tmp1, '[')!=NULL);
                        if(has_square && has_arrow) 
                        {
                             shErrStackPush("Mixed array specification not allowed\n");
                             return SH_ARRAY_SYNTAX_ERR;
                        }
                        if(has_arrow) strcpy(half_token, "<");
                        else strcpy(half_token, "[");
                        tmpPtr1=strtok(tmp1, half_token);
                        objElemPtr = (SCHEMA_ELEM*) shSchemaElemGet(tmpSchema->name, tmpPtr1);
                        if(objElemPtr==NULL) 
                        {
                          shErrStackPush("Schema %s doesn't have field: %s\n", tmpSchema->name, tmpPtr1);
                          return SH_FLD_SRCH_ERR;
                        }
                        tmpSchema = (SCHEMA*) shSchemaGet(objElemPtr->type);
                      
                      } /* while */
                          
                   } /* if(entry_fnd!=entry_stack[j]) */

                   else objElemPtr=elemptr;
           

                } /* else */

              /* get the existing dimension from the last 
                  continuation line  if any */

               p_shSpptArrayCheck(objElemPtr->nelem, NULL, 
                     &objSideStaticArraySize, &objSideStaticDimen, " ");
              
               fldIdx++; 
               fldIdx_save = fldIdx; 
               use_redund = FALSE;
               row=0;
               if(entry_fnd >= 0)
               { 
                   /* check if this is a redundancy entry. If yes,
                      we set use_redund to TRUE, and find what its
                      array info was. The array info has a fldIdx,
                      which is the fldIdx we should use. Row is
                      used as a loop index.  */

                  for(; row < entryTableCnt; row++)
                  {
                     if(entryTable[row].entry!=entry_stack[j]) continue;
                     if(entryTable[row].infoPtr->status < 0) 
                     {  
                        /* yes, a redundancy entry. Go get its infoPtr
                           and work with that fldIdx. fldIdx_save that
                           was just incremented should be decremented 
                           back.                                     */


                        fldIdx = entryTable[row].infoPtr->fldIdx;
                        currentInfo =entryTable[row].infoPtr;
                        fldIdx_save--;
                        use_redund = TRUE;
                        if(shTblFldLoc(tblcol, fldIdx, NULL, 0, 
                          SH_CASE_INSENSITIVE, &curArrayPtr, &curTblfldPtr, NULL)!=SH_SUCCESS)
                        {
                             shErrStackPush("Fail to locate field %d .\n", fldIdx);
                             return SH_FLD_SRCH_ERR;
                        }
                        break;
                      }

                      /* No, not a redundancy yet. Mark the status and
                         proceed on normally */

                      entryTable[row].infoPtr->status = -1;
                      entryTable[row].infoPtr->fldIdx = fldIdx;
                      if(shTblFldLoc(tblcol, fldIdx, NULL, 0, 
                         SH_CASE_INSENSITIVE, &curArrayPtr, &curTblfldPtr, NULL)!=SH_SUCCESS)
                      {
                           shErrStackPush("Fail to locate field %d .\n", fldIdx);
                           return SH_FLD_SRCH_ERR;
                      }

                 /* potentially may have a problem here as dim is type long */

                      memcpy((char*) &curArrayPtr->dim[curArrayPtr->dimCnt], 
                            (char*) entryTable[row].infoPtr->num,
                                entryTable[row].infoPtr->len*sizeof(int));
                      curArrayPtr->dimCnt += entryTable[row].infoPtr->len;
                      currentInfo =entryTable[row].infoPtr;
                      break;
                
                  } /* for(row=0; row < entryTableCnt; row++) */
             
               } /* if entry_fnd > 0 */

              if( entry_fnd < 0 || row==entryTableCnt) 
               {
                 /* the case this field is not listed in the table or
                    simply, not a redundancy entry */

                  if(shTblFldLoc(tblcol, fldIdx, NULL, 0, 
                       SH_CASE_INSENSITIVE, &curArrayPtr, &curTblfldPtr, NULL)!=SH_SUCCESS)
                  {   
                    shErrStackPush("Fail to locate field %d .\n", fldIdx);
                    return SH_FLD_SRCH_ERR;
                  }  
               }

             /* some array information checking */

              if(objSideArrayIndex >= objSideStaticArraySize*objSideAddedArraySize)
              {
                  shErrStackPush("Field %s: index overflow at entry %d.", 
                             objElemPtr->name, entry_fnd);
                  return SH_BAD_INDEX;
               }

              if(objSideDataType!=DST_HEAP && objSideArrayIndex < 0 )
              {
                  shErrStackPush("Field %s: index underflow at entry %d.", 
                             objElemPtr->name, entry_fnd);
                  return SH_BAD_INDEX;
               }


              if(objSideArrayDimen > objSideStaticDimen && transTblEntryPtr[entry_fnd].size==NULL)
              {
                  shErrStackPush("Field %s: dimension info ridiculous.\n", objElemPtr->name);
                  return SH_SIZE_UNMATCH;
              }

              row = curArrayPtr->dimCnt;

              /* lets amend curArrayPtr->dimCnt properly when not redund*/

              if(objSideDataType==DST_ENUM)
              {
                    if((tmp_int=p_shSpptGetMaxStrlenOfEnum(objElemPtr->type, NULL))< 0)
                    {
                        shErrStackPush("Can't get enum \"%s\"\n",objElemPtr->type);
			return SH_GENERIC_ERROR;
		    }

                    tmp_int += strlen(objElemPtr->type) + 2; /* 2 tabs */
                    if(tmp_int > MAX_ENUM_LEN ) 
                    {
                       shErrStackPush("Enum member name too long in %s."
                              "\nEither truncate the name or increase MAX_ENUM_LEN in shSchema.c\n");
                       return SH_GENERIC_ERROR;
                    }
                    if(!use_redund) curArrayPtr->dim[curArrayPtr->dimCnt++] = MAX_ENUM_LEN;
		    objSideNumOfBlocks = MAX_ENUM_LEN;
              }
              else if (objSideDataType == DST_HEAP)
              {
                     /*  do nothing here */
              }

              else
              {
                     objSideTotalDimen = objSideAddedDimen + objSideStaticDimen - objSideArrayDimen;     
                     if(!use_redund) while(row < objSideTotalDimen + curArrayPtr->dimCnt) 
                     {
                          curArrayPtr->dim[row] = 
                               transTblEntryPtr[entry_fnd].num[row -curArrayPtr->dimCnt + objSideArrayDimen];
                          row++;
                         /* curArrayPtr->dimCnt++; */
                         
                     }
                     curArrayPtr->dimCnt = row;
                   
              }
             
             /* counting how many objSideNumOfBlocks are suggested by the entry to copy */

             row = objSideTotalDimen;  /* counting backwards */
             objSideNumOfBlocks = 1;
             while(row > 0 )  objSideNumOfBlocks *=curArrayPtr->dim[curArrayPtr->dimCnt - row--]; 

             if(!use_redund)
             {
                if( curTblfldPtr!=NULL) 
                {
                   strncpy((char*) curTblfldPtr->TTYPE, dststr, shFitsHdrStrSize);
                   shAddSet(curTblfldPtr->Tpres, SH_TBL_TTYPE); 
                 }
              
              
                if(shArrayDataAlloc(curArrayPtr, shCnvSetTableType(objSideDataType)) !=SH_SUCCESS)
                 {
                    shErrStackPush("Memory alloc error for field \"%s\" of type \"%s\".\n", 
                          elemptr->name, tmpPtr1);
                    return SH_MALLOC_ERR;
   
                 }
                 


               if(objSideDataType==DST_HEAP)
                  {
                       /* lets continue to call shTblHeapAlloc
                          after filling the cnt field in heap.
                          give TBLFLD a schema by heaptype */

                     curTblfldPtr->heap.schemaType
                          =shTypeGetFromName(shCnvSetTableType(objSideHeapDataType));
                     heapdsc = (TBLHEAPDSC*) curArrayPtr->data.dataPtr;

                     /* fill in the count */

                      for(row = 0; row < stop_row - start_row; row++)
                      {                     
                          if(convert_all)   /* i.e, not one row converting. Have to find address*/
                          {
                               if(row == 0) walkPtr = NULL;
                               objInstancePtr = shSpptObjGetFromContainer(
                                                      containerType,
                                                      object,
                                                      &walkPtr,
                                                      schemaName,
                                                      row);
                               if(objInstancePtr==NULL) 
                               {
                                  shErrStackPush("Can't get instance from "
                                          "the container\n");
                                  return SH_INV_DATA_PTR;
                               }
                     
                           }
                           
                         
                           if((heap_length=p_shSpptHeapDimenGet((SCHEMA*) rootSchemaPtr, objInstancePtr, 
                                    transTblEntryPtr[entry_fnd].heaplen,NULL, TRUE)) < 0 )
                            {
                               shErrStackPush("Heap length invalid at entry %d: %s = %d\n", 
                                            entry_fnd, transTblEntryPtr[entry_fnd].heaplen, heap_length);
			       return SH_GENERIC_ERROR;
		            }

                            if((heap_dimen=p_shSpptHeapDimenGet((SCHEMA*)rootSchemaPtr, objInstancePtr, 
                                    transTblEntryPtr[entry_fnd].size, tmp_num, FALSE)) < 0 )
                            {
                               shErrStackPush("Heap dimension invalid at entry %d: %s = %d\n", 
                                            entry_fnd, transTblEntryPtr[entry_fnd].size, heap_dimen);
			       return SH_GENERIC_ERROR;
		            }
                            for(l = 0; l < heap_dimen; l++)
                            {
                                transTblEntryPtr[entry_fnd].num[objSideStaticDimen+l] = tmp_num[l];
                            }
      
                           /* Redo objSideArrayIndex as now transTblEntryPtr.num becomes available  */

                            p_shSpptArrayCheck(transTblEntryPtr[entry_fnd].dst, 
                               transTblEntryPtr[entry_fnd].num, &objSideArrayIndex, &objSideArrayDimen, NULL);
                            objSideArrayIndex--;

                            tmp_int = heap_length;
                            objSideTotalDimen = objSideStaticDimen + heap_dimen -objSideArrayDimen;

                             for(l=0; l < objSideTotalDimen ; l++) 
                             {   
                                tmp_int *= transTblEntryPtr[entry_fnd].num[l+objSideArrayDimen];
                             }


                            if(tmp_int <= 0 && heap_length != 0)
                            {
                                 shErrStackPush("shCnvSchemaToTbl: illegal heap array specification "
                                        "at entry %d", entry_fnd);
                                 return SH_GENERIC_ERROR;
                            }
                            heapdsc->cnt = tmp_int;
                            heapdsc++;
                         

                       }  /* for loop over row ends here */

#if 0
               
                       if(shTblFldHeapAlloc(curArrayPtr, 
                          strcmp(transTblEntryPtr[entry_fnd].heaptype,"STRING")==0 ? "STR" : 
                                   transTblEntryPtr[entry_fnd].heaptype , 
                          0, &extrCnt)!=SH_SUCCESS)
                        {
                           shErrStackPush("Fail to allocate heap memory for entry %d\n", entry_fnd);
                           return SH_MALLOC_ERR;
                        }

#endif
                        if(shTblFldHeapAlloc(curArrayPtr, shCnvSetTableType(objSideHeapDataType), 
                                  0, &extrCnt)!=SH_SUCCESS)
                        {
                           shErrStackPush("Fail to allocate heap memory for entry %d\n", entry_fnd);
                           return SH_MALLOC_ERR;
                        }


                 }  /* if objSideDataType==DST_HEAP */

              }  /* if(!use_redund) */

             /*restore fldIdx */

              fldIdx = fldIdx_save; 
              fitsSideDataPtr = (char*) curArrayPtr->data.dataPtr;
              fitsSideArrayIndex =0;
              fitsSideArrayDimen =0;
              
             /* check array index at fits side */

              if(entry_fnd >= 0 && currentInfo!=NULL) 
              {
                  p_shSpptArrayCheck(transTblEntryPtr[entry_stack[j]].src, (int*) &curArrayPtr->dim[1],
                       &fitsSideArrayIndex, &fitsSideArrayDimen, NULL); 
                  fitsSideArrayIndex--;
              }

              row =1;                  		/* row is used as an index */
              fitsSideNumOfBlocks = 1;			/* number of objSideNumOfBlocks left over */
              fitsSideBlocksPerRow = 1;		/* total number of objSideNumOfBlocks per row */
              while(row < curArrayPtr->dimCnt) 
              {
                   if(row > fitsSideArrayDimen) fitsSideNumOfBlocks*=curArrayPtr->dim[row];
                   fitsSideBlocksPerRow *=curArrayPtr->dim[row];
                   row++;
              } 
              
              if(fitsSideArrayIndex > 0)   
              {
                    /* convert src array index to be in units of fitsSideNumOfBlocks */

                  /*  fitsSideArrayIndex = (fitsSideArrayIndex + 1)/fitsSideNumOfBlocks;
                   fitsSideArrayIndex--;  */

                  fitsSideArrayIndex /= fitsSideNumOfBlocks;


              }
              if(fitsSideArrayDimen!=0 && 
                    fitsSideNumOfBlocks!=objSideNumOfBlocks 
                       && objSideDataType!=DST_ENUM) 
              {
                  shErrStackPush("shCnvSchemaToTbl: Fits field has block size %d "
                    "but schema field has %d when processing entry %d\n", 
                     fitsSideNumOfBlocks, objSideNumOfBlocks, entry_fnd);
                  return SH_SIZE_UNMATCH;
              }
               
              if(fitsSideDataPtr==NULL && stop_row - start_row > 0) 
              {
                  shErrStackPush("shCnvSchemaToTbl: %s in FITS has NULL memory\n", dststr);
                  return SH_INV_DATA_PTR;
              }

              fitsSideDataPtr += fitsSideArrayIndex*objSideDataSize*fitsSideNumOfBlocks;

                                
              /***************   ready to copy data *************************/

	      walkPtr = NULL; 
             
              for(row=0; row < stop_row - start_row; row++)
              {
                  entry_fnd = entry_stack[j];
                  if(convert_all)   /* i.e, not one row converting. Have to find address*/
                  {

                      if (row == 0)  {
                          if (containerType == CHAIN_TYPE)
                              walkPtr = (char *) shChainElementGetByPos((CHAIN *)object, start_row);
                          else
                              shFatal("shCnvSchemaToTbl: unknown container encountered!!");
                              
		      }
                
                       objInstancePtr = shSpptObjGetFromContainer(
                                              containerType,
                                              object,
                                              &walkPtr,
                                              schemaName,
                                              row);
                      if(objInstancePtr==NULL) 
                      {
                            shErrStackPush("Can't get instance from "
                                          "the container; possible bad type\n");
                            return SH_INV_DATA_PTR;
                      }
                     
                   }
                  
                   objSideDataPtr = (char*) objInstancePtr + elemptr->offset ;
                   objSideDataPtr = p_shSpptTransTblTraverse(xtbl_ptr, elemptr, 
                                  &objElemPtr, objSideDataPtr , &entry_fnd, &upperLevelPtr);

                   /* if obj side is string, but has NULL pointer, let it pass and we'll
                      make a 0-length string at fits side later on                   */


                   if(objSideDataPtr==NULL && objSideDataType!=DST_STR &&
                              objSideDataType!=DST_HEAP) 
                   {
                     /* elementary pointer type, but points to nothing */

                         if(row==start_row) printf( 
                             "Entry %d's \"%s\" (type \"%s\") \thas no data (NULL pointer). Ignored. \n", 
                              entry_fnd, objElemPtr->name, objElemPtr->type);
                         continue;
                   }
                   
                   if(objSideDataType==DST_HEAP)
                   {
                        /* deal with heap */

                      if((heap_dimen=p_shSpptHeapDimenGet((SCHEMA*)rootSchemaPtr, objInstancePtr, 
                                    transTblEntryPtr[entry_fnd].size, tmp_num, FALSE)) < 0 )
                      {
                               shErrStackPush("Heap dimension invalid at entry %d: %s = %d\n", 
                                            entry_fnd, transTblEntryPtr[entry_fnd].size, heap_dimen);
			       return SH_GENERIC_ERROR;
		      }

                      if((heap_length=p_shSpptHeapDimenGet((SCHEMA*)rootSchemaPtr, objInstancePtr, 
                                    transTblEntryPtr[entry_fnd].heaplen, NULL, TRUE)) < 0 )
                      {
                               shErrStackPush("Heap length invalid at entry %d: %s = %d\n", 
                                            entry_fnd, transTblEntryPtr[entry_fnd].heaplen, heap_length);
			       return SH_GENERIC_ERROR;
		      }


                      heap_block_num=1;
                      for(l = 0; l < heap_dimen; l++)
                      {
                            transTblEntryPtr[entry_fnd].num[objSideStaticDimen+l] = tmp_num[l];
                      }
      

                      /* Redo objSideArrayIndex as now transTblEntryPtr.num becomes available  */

                      p_shSpptArrayCheck(transTblEntryPtr[entry_fnd].dst, 
                       transTblEntryPtr[entry_fnd].num, &objSideArrayIndex, &objSideArrayDimen, NULL);
                      objSideArrayIndex--;

                      objSideTotalDimen = objSideStaticDimen + objSideAddedDimen;
 		      if( objSideTotalDimen < 0 )
                      {
			  shErrStackPush("Array over-specified at entry %d", entry_fnd);
			  return SH_GENERIC_ERROR;
		      }
                     
                      for(l=objSideArrayDimen; l < objSideTotalDimen; l++)
                      {   
                          heap_block_num *=transTblEntryPtr[entry_fnd].num[l];
                      }
                      objSideDataPtr_save = upperLevelPtr;
                      heapdsc = (TBLHEAPDSC *) fitsSideDataPtr;
                      heapdsc += row;
                      for(l=0; l < heap_block_num; l++)
                      {
                          objSideDataPtr = p_shSpptHeapBlockGet(objSideDataPtr_save,
                                     objElemPtr->nstar,
                                     objSideStaticDimen,
                                     objSideArrayDimen, 
                                     objSideTotalDimen,
                                     transTblEntryPtr[entry_fnd].num,
                                     objSideArrayIndex,
                                      l);
                          if(heap_length == 0 ) continue;
                          if(objSideDataPtr!=NULL)
                             memcpy((void*)((char*)heapdsc->ptr + l*heap_length*objSideDataSize), 
                                             objSideDataPtr, heap_length * objSideDataSize);
                          else if(objSideHeapDataType==DST_STR)
                          {
                             *((char*)heapdsc->ptr + l*heap_length*objSideDataSize)='\0';
                          }
                          else
                          {
                              shErrStackPush("SchemaToTbl: schema heap data pointer NULL for entry %d",
           				entry_fnd);
 			      return SH_GENERIC_ERROR;
                          }

                      }
                   }
                   else if (objSideDataType==DST_STR)
                   {
                        /* treats string types differently. If NULL is seen, make a
                           0-length string                                       */

                       if(objSideDataPtr!=NULL) 
                       {
                          tmp_int = strlen(objSideDataPtr);
                          strcpy((char*) fitsSideDataPtr + row * fitsSideBlocksPerRow*objSideDataSize,
                                objSideDataPtr);
                       }
                       else tmp_int = 0;
                       memset((char*) fitsSideDataPtr + row * fitsSideBlocksPerRow*objSideDataSize 
                          + tmp_int, '\0', objSideNumOfBlocks*objSideDataSize - tmp_int);

                   }

                   else if(objSideDataType!=DST_ENUM) 
                   {
                       /* most other elementary types here */

                      memcpy((char*) fitsSideDataPtr + row *fitsSideBlocksPerRow*objSideDataSize , 
                               objSideDataPtr, objSideNumOfBlocks*objSideDataSize); 

#if 0
                       fitsSideDataType = objSideDataType;

                       if(p_shSpptCastAndConvert(objSideDataPtr, objSideDataType,
                             fitsSideDataPtr + row *fitsSideBlocksPerRow*objSideDataSize, 
                             fitsSideDataType, objSideNumOfBlocks,
                           1.0 /* scale = 1.0 */, 0.0 /* zero = 0.0 */) < 0) 
                       {
                            shErrStackPush("shCnvSchemaToTbl: Can't do value conversion\n");
                            return SH_GENERIC_ERROR;
                       }
#endif 

                   }
                   else
                   {
                      /* deal with portable enums */

                      /* use tmp1, tmp2 as tmp storage */
                      /* get next sizeof(int) bytes into an integer */

                      memcpy((char*) &tmp_int, objSideDataPtr, sizeof(tmp_int));
                     if((tmpPtr2=shEnumNameGetFromValue(objElemPtr->type,
                           tmp_int))==NULL)
                      {
                         shErrStackPush("Can't find enum string from the given "
                            "value %d in enum %s\n", tmp_int, objElemPtr->type);
                         return SH_GENERIC_ERROR;
                      }
                      
                     
                      sprintf(tmp1, "%s\t%s\t", objElemPtr->type, tmpPtr2);

                      /* appending space to fill MAX_ENUM_LEN */

                      tmp_int =  strlen(tmp1);
                      memset(tmp1 + strlen(tmp1), ' ',MAX_ENUM_LEN - tmp_int);
                      tmp1[MAX_ENUM_LEN-1]='\0';
                      strcpy((char*) fitsSideDataPtr + row *fitsSideBlocksPerRow, tmp1);

                   }
              }
   
              currentInfo=NULL;

          }  /* for(j=0; j < entry_nums; j++) */

      }  /* loop over rootSchemaPtr->nelem */

      shCnvRuntimeStructFree();
      *tblcolPtrPtr = tblcol;
      return SH_SUCCESS;
}


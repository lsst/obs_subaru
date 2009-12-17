
/*************************************************************************
 *  Facility:           dervish                                            *
 *  Synopsis:           definitions for SCHEMATRANS routines 		 *
 *  Environment:        ANSI C                                           *
 *  Author:             Wei Peng                                         *
 *  Creation Date:      Feb 1, 1994                                      *
 *                                                                       *
 *  Last Modification:  June 29, 1994                                    *
 *************************************************************************
 
 

*************************************************************************/
/************** Author's note *******************************************
 *
 * Most of the routines here are straightforward. However, special attention
 * should be paid to EntryAdd and EntryDel routines. Currently, I implemented
 * a local heap for each entry, so that each entry only needs one malloc call.
 * This heap is pointed by src of a SCHEMATRANS structure. Attempt to free
 * memories for strings of an entry other than src will be disasterous.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "libfits.h"
#include "dervish_msg_c.h"
#include "shCErrStack.h"
#include "shCUtils.h"
#include "shCGarbage.h"
#include "shCSchema.h"
#include "shCTbl.h"
#include "shCArray.h"
#include "shCSchemaTrans.h"
#include "shTclHandle.h"
#include "shCAlign.h"


#define MAX_NAME_LEN 100
#define MAX_ENUM_LEN 64     /* MAX_NAME_LEN must be larger than MAX_ENUM_LEN */
#define SCHTRS_HASH_BLOCK 20

/* hash table routines first */

int shSchemaTransGetHashValue(char* str)
{
      /* implement a way to turn string into number */
     int i=0, rvalue = 0;
     char c;

    while(i < 3)
    {
        if((c=str[i++]) == '[' || c == '<' || c == '\0' ) break;
         rvalue += c -'A';
    }


	if (rvalue < 0) rvalue=1;

    return rvalue%SCHTRS_HASH_TBLLEN;
}

/* expand a hash table */

static int hashExpand(SCHEMATRANS* xtbl_ptr, int entry)
{
     int * origHashPtr = NULL, size = 0;
     int* newHashPtr = NULL;

     origHashPtr = xtbl_ptr->hash[entry].entries;
     size  = xtbl_ptr->hash[entry].totalNum + SCHTRS_HASH_BLOCK;
     /* when used with gcc or acc, realloc() behaves differently
        than with Ansi C. So, we'd have to be better prepared */

     
     if(origHashPtr!=NULL)
           newHashPtr = (int*) shRealloc(origHashPtr, size*sizeof(int)); 
     else newHashPtr = (int*) shMalloc(size*sizeof(int));
     
     xtbl_ptr->hash[entry].totalNum = size;
     xtbl_ptr->hash[entry].entries = newHashPtr;
     return 0;
}

static void hashEntryAdd(SCHEMATRANS* xtbl_ptr, char* str, int entry)
{
       int hash_value = shSchemaTransGetHashValue(str);
       int curNum, totalNum;
    
       curNum = xtbl_ptr->hash[hash_value].curNum;
       totalNum = xtbl_ptr->hash[hash_value].totalNum;
       if(curNum >= totalNum && hashExpand(xtbl_ptr, hash_value) < 0) 
              shFatal("Can't expand hash table in SCHEMATRANS");
       xtbl_ptr->hash[hash_value].entries[curNum++] = entry;
       xtbl_ptr->hash[hash_value].curNum++; 
       return;
}
      

/* this is not something you do very often */

static void hashEntryDel(SCHEMATRANS* xtbl_ptr, char* str, int entry)
{
       int hash_value = shSchemaTransGetHashValue(str);
       int curNum, i = 0, j=0;
    
       curNum = xtbl_ptr->hash[hash_value].curNum;
       while (i < curNum)
        {
             if(xtbl_ptr->hash[hash_value].entries[i]==entry) break;
             i++;
        }

       if ( i == curNum ) {shFatal("bad args in hashEentryDel\n");} /* this case is really absurd */
       i++;
       for(;i < curNum; i++)  
          xtbl_ptr->hash[hash_value].entries[i-1]=xtbl_ptr->hash[hash_value].entries[i];
       xtbl_ptr->hash[hash_value].curNum--;

      /* all entry values in hash tables that are greater than entry
         should be decremented by 1                                         */

       for(i=0; i < SCHTRS_HASH_TBLLEN;i++)
       {
           for(j=0; j < xtbl_ptr->hash[i].curNum; j++)
             if(xtbl_ptr->hash[i].entries[j] > entry) xtbl_ptr->hash[i].entries[j]--;
       }
       return;
}



 
/* destroy the hash table */

static void hashDestroy(SCHEMATRANS* xtbl_ptr)
{
     int i = 0;
     for(i = 0; i < SCHTRS_HASH_TBLLEN; i++)
     {
          if(xtbl_ptr->hash[i].entries != NULL) 
                              shFree(xtbl_ptr->hash[i].entries);
          xtbl_ptr->hash[i].entries = NULL;
     }
     return ;
}


/* return address of mallocked memory */

SCHEMATRANS* shSchemaTransNew(void)
{
    SCHEMATRANS *xptr = (SCHEMATRANS*) shMalloc(sizeof(SCHEMATRANS));
    memset((void*)xptr, 0, sizeof(SCHEMATRANS));
    xptr->totalNum = MAX_SCHEMATRANS_ENTRY_SIZE;
    xptr->entryNum = 0;
    xptr->entryPtr = (SCHEMATRANS_ENTRY*)
              shMalloc(sizeof(SCHEMATRANS_ENTRY)*xptr->totalNum);
    xptr->status = (SCHTR_STATUS*) shMalloc(sizeof(SCHTR_STATUS)*xptr->totalNum);
    memset((void*)xptr->entryPtr, 0, sizeof(SCHEMATRANS_ENTRY)* xptr->totalNum);
    memset((void*)xptr->status, 0, sizeof(SCHTR_STATUS)* xptr->totalNum);
    return xptr;
}


/* expand the SCHEMATRASN_ENTRY in SCHEMATRANS */
/* MAX_SCHEMATRANS_ENTRY_SIZE is added at a time. */
/* Returns orignal if successful. */

static
SCHEMATRANS* shSchemaTransExpand(SCHEMATRANS* original)
{
    SCHEMATRANS_ENTRY *new_entry = NULL;
    SCHTR_STATUS      *new_status = NULL;
    int size = MAX_SCHEMATRANS_ENTRY_SIZE;
    
    if(original->totalNum==0 || original->entryPtr == NULL) return NULL;
    size += original->totalNum;
    new_entry = (SCHEMATRANS_ENTRY*)shRealloc(original->entryPtr,
                                        size*sizeof(SCHEMATRANS_ENTRY));
    new_status = (SCHTR_STATUS *)shRealloc(original->status,
                                     size*sizeof(SCHTR_STATUS));

    memset((void*)&new_entry[original->totalNum], 0, 
                    MAX_SCHEMATRANS_ENTRY_SIZE*sizeof(SCHEMATRANS_ENTRY));
    memset((void*)&new_status[original->totalNum], 0, 
                    MAX_SCHEMATRANS_ENTRY_SIZE*sizeof(SCHTR_STATUS));
 
    original->totalNum = size; 
    original->entryPtr = new_entry;
    original->status = new_status;             
    return original;
}


/* delete a table structure. All memory associated with entries
   are free-ed first. */

void shSchemaTransDel(SCHEMATRANS* xptr)
{
    shSchemaTransClearEntries(xptr);  /* release entries */
    hashDestroy(xptr);		   /* destroy hash table */
    shFree(xptr->entryPtr);
    shFree(xptr->status);
    shFree(xptr);
}


/* initialize an entry. No free() is called! 
   All memories have to be free-ed before calling
   this routine */

void
shSchemaTransInit(SCHEMATRANS* xtbl_ptr, unsigned int i)
{
     SCHEMATRANS_ENTRY* ptr = xtbl_ptr->entryPtr;

     if ( i > xtbl_ptr->entryNum ) {
         shErrStackPush("shSchemaTransInit: bad index %d", i);
         return;
     }
    
       ptr[i].type = CONVERSION_END;
       ptr[i].src = ptr[i].dst = NULL;
       ptr[i].dsttype  = NULL;
       ptr[i].dstDataType=DST_UNKNOWN;
       ptr[i].srcTodst = 1.0 ;
       ptr[i].proc = NULL;
       ptr[i].size = NULL;
       ptr[i].heaptype=NULL;
       ptr[i].heaplen=NULL;
       xtbl_ptr->status[i] = SCHTR_FRESH;      
       (void) memset((char*) ptr[i].num, 0, MAX_INDIRECTION*sizeof(int)); 
    
}  


/* restore the status of each entry to SCHTR_FRESH */

void
shSchemaTransStatusInit(SCHEMATRANS* xtbl_ptr)
{
       
      memset((void*) xtbl_ptr->status, 0, 
              xtbl_ptr->totalNum*sizeof(SCHTR_STATUS));
      return;
}

void           
shSchemaTransStatusSet(SCHEMATRANS *xtbl_ptr, SCHTR_STATUS s, int entry)
{
     
     xtbl_ptr->status[entry] = s;
     return;
}



SCHTR_STATUS   shSchemaTransStatusGet(SCHEMATRANS *xtbl_ptr, int entry)
{
     return xtbl_ptr->status[entry];
}
  
     
/* Clear the entries and free all memory associated 
   with the entries. But the memory for table structure
   remains. The table is still usable, i.e, entries
   can be added without calling shSchemaTransNew(). */

void
shSchemaTransClearEntries(SCHEMATRANS* xtbl_ptr)      /* base pointer passed */
{
     int i;
     SCHEMATRANS_ENTRY* ptr = xtbl_ptr->entryPtr;
     int last_entry = xtbl_ptr->entryNum;

     for(i = 0; i < last_entry; i++)
      {
         shFree(ptr[i].src);      /* free memory previously allocated */
         
      }
     for(i=0; i < SCHTRS_HASH_TBLLEN; i++) 
      {
         xtbl_ptr->hash[i].curNum = 0;  /* reset hash entries without freeing */
     }
     
     xtbl_ptr->entryNum = 0;
     shSchemaTransInit(xtbl_ptr,0);  

}




/* routine that converts a string to enumerated type 
   the str passed in should be something like, "int",
   "unsigned char", etc. Otherwise, it'll return
   DST_UNKNOWN.
   
   size, if passed, will be filled with size of the
   data type.
 */


DSTTYPE shSchemaTransDataTypeGet(char* str, int* size)
{
   char  dsttype[MAX_NAME_LEN];
   char* dststr;
   unsigned UFLAG=0;   /* 1 if str has unsigned */
   DSTTYPE dtype;
   const SCHEMA * schema;
   int tmp_size=0;

   if(str==NULL ) {if(size!=NULL) *size=0; return DST_UNKNOWN;}
   else    strcpy(dsttype, str);

   if((dststr = strstr(dsttype, "unsigned"))!=NULL) UFLAG=1;
   else if((dststr = strstr(dsttype, "UNSIGNED"))!=NULL) UFLAG=1;

   if((dststr = strstr(dsttype, "char"))!=NULL ||
      (dststr = strstr(dsttype, "CHAR"))!=NULL ) 
   {
        tmp_size=sizeof(char);

        if(dststr > dsttype) {
	   if(dststr[-1] == 'u' || dststr[-1] == 'U') {
	      dtype = DST_UCHAR;
	   } else if(dststr[-1] == 's' || dststr[-1] == 'S') {
	      if(UFLAG) {
		 shError("shSchemaTransDataTypeGet: "
			 "type %s was previously declared unsigned",dsttype);
	      }
	      dtype = DST_SCHAR;	
	   } else if(dststr[-1] == ' ') {
	     	   dtype= UFLAG ? DST_UCHAR: DST_CHAR;
	   } else {			/* something other than [sSuU]CHAR */
	      {if(size!=NULL) *size=0; return DST_UNKNOWN;}
	   }
	} else {
	   dtype= UFLAG ? DST_UCHAR: DST_CHAR;
	}
   }
   else if((dststr = strstr(dsttype, "short"))!=NULL ||
           (dststr = strstr(dsttype, "SHORT"))!=NULL ) 
   {
           dtype= UFLAG ? DST_USHORT: DST_SHORT;
           tmp_size=sizeof(short);
   }
   else if((dststr = strstr(dsttype, "int"))!=NULL ||
       (dststr = strstr(dsttype, "INT"))!=NULL ) 
   {
           dtype= UFLAG ? DST_UINT: DST_INT;
           tmp_size = sizeof(int);
   }
   else if((dststr = strstr(dsttype, "long"))!=NULL||
        (dststr = strstr(dsttype, "LONG"))!=NULL)  
  {
           dtype= UFLAG ? DST_ULONG: DST_LONG;
           tmp_size=sizeof(long);
   }
   else if(strcmp(dsttype,"struct")==0)
   {
         dtype = DST_STRUCT;
         tmp_size = 0;
   }
   else if(strncmp(dsttype, "string",3)== 0 || strncmp(dsttype, "STRING",3)==0) 
   {
        dtype= DST_STR;
        tmp_size=sizeof(char);
   }
   else if(strcmp(dsttype, "float")==0||strcmp(dsttype, "FLOAT")==0) 
  { 
            dtype= DST_FLOAT;
            tmp_size=sizeof(float);
   }
   else if(strcmp(dsttype, "double")==0||strcmp(dsttype, "DOUBLE")==0) 
  {
            dtype= DST_DOUBLE;
            tmp_size=sizeof(double);
   }
   else if(strcmp(dsttype, "enum")==0)
   {
            dtype= DST_ENUM;
            tmp_size=sizeof(char);
   }
   else if(strcmp(dsttype, "heap")==0)
   {
            dtype= DST_HEAP;  /* heap size is not known from here */
            tmp_size = 0;
   }
   
   else 
   {
      schema=shSchemaGet(dsttype);
      if(schema!=NULL && schema->type==STRUCT)
      { 
          dtype=DST_STRUCT;
          tmp_size = schema->size;
       }
      else 
      {   dtype=DST_UNKNOWN;
          tmp_size=0;
      }
   }

   if(size!=NULL) *size=tmp_size;
   return dtype;
}


/* this is private! Purpose of this routine is to
 * strip off array specification and trailing spaces 
 */


static char* getRealString(char* dst, char* src)
{
    char* tmp;
    char  tmp_space[MAX_NAME_LEN];
    unsigned short has_arrow, has_square;
   
    tmp = strcpy(tmp_space, src);
    has_arrow=(strchr(tmp_space, '<')!=NULL);
    has_square=(strchr(tmp_space, '[')!=NULL);
    if(has_square && has_arrow) 
    {
       shErrStackPush("Mixed array specification not allowed.\n");
       return NULL;
    }
    while(*tmp==' ') tmp++;
    if (tmp!=tmp_space) strcpy(tmp_space, &src[tmp - tmp_space]);
    tmp=tmp_space + strlen(tmp_space)-1;
    while(*tmp==' ') tmp--;
    if (tmp!=tmp_space + strlen(tmp_space) -1) *++tmp = '\0';

    strcpy(dst, tmp_space);
    return dst;    
}

#if 0
static char* nameFromConvType(CONVTYPE type)
{
    switch(type)
    {
        case CONVERSION_BY_TYPE:    	return "name";
        case CONVERSION_CONTINUE:   	return "continue";
        case CONVERSION_IGNORE:         return "ignore";
        default:       			return "unkown";
    }
}
#endif


CONVTYPE convTypeFromName(char* name)
{
     if(strcmp(name, "name")==0) return CONVERSION_BY_TYPE;
     if(strcmp(name, "cont")==0) return CONVERSION_CONTINUE;
     if(strcmp(name, "ignore")==0) return CONVERSION_IGNORE;
     return CONVERSION_UNKNOWN;
}


/* add an entry to the translation table 
   src of each entry will always be made uppercase. 
   Any space before the string or after the string
   will be ignored. */

RET_CODE
shSchemaTransEntryAdd(
	SCHEMATRANS*   xtbl_ptr,         /* points to a translation table */
 	CONVTYPE       convtype,        /* input: Conversion Type */
 	char*          src,         /* input: source name not to be NULL*/
 	char*          dst,         /* input: destination name not to be NULL*/
 	char*          dsttype,     /* input: destination type not to be NULL*/
 	char*          heaptype,    /* input: heap type */
 	char*          heaplen,    /* input: dynamical heap size */
 	char*          proc,        /* input: cmd string to use in malloc */
 	char*          size,        /* input: size string */
 	double         r,           /* input: units ration */
 	int            pos         /* position to force the entry to be added at */
                             /* if < 0, just ignore */

)
{
   unsigned int i = 0 , j=0;  /* loop index */
   DSTTYPE tmp_type;
   SCHEMATRANS_ENTRY* ptr;
   int last_entry = xtbl_ptr->entryNum;
   char* tmp;
   char  tmp_src[MAX_NAME_LEN];
   char  tmp_dst[MAX_NAME_LEN];

   int total_strlen = 0;        /* calculate the total string length of the entry */
                                    /* also count in null termination of each field */

#if 0
   if(strcmp("name", type)==0 || strcmp("type", type)==0)  
    			convtype=CONVERSION_BY_TYPE;
   else if(strncmp("continue", type, 4)==0) 	 
			convtype=CONVERSION_CONTINUE;
   else if(strcmp("ignore", type)==0)       
			convtype=CONVERSION_IGNORE;
   else {
      shErrStackPush("Unsupported conversion type: %s", type);
      return SH_GENERIC_ERROR;
   }
#endif

   if(pos >= 0 ) i = pos;
   else i = last_entry;

   if(i >= xtbl_ptr->totalNum-1 && shSchemaTransExpand(xtbl_ptr)==NULL)
   {
       shErrStackPush("Table full. Expansion failed");
       return SH_GENERIC_ERROR;
   }

   xtbl_ptr->status[i] = SCHTR_FRESH;
   ptr = xtbl_ptr->entryPtr;
   ptr[i].type=convtype;
   

   if(getRealString(tmp_src, src)==NULL) return SH_GENERIC_ERROR; 
   if(getRealString(tmp_dst, dst)==NULL) return SH_GENERIC_ERROR;  

   total_strlen += strlen(tmp_src) + strlen(tmp_dst) + strlen(dsttype) + 3;
   if(proc != NULL ) total_strlen += strlen(proc) + 1;
   if(size != NULL ) total_strlen += strlen(size) + 1;
   if(heaplen != NULL) total_strlen += strlen(heaplen) + 1;
   if(heaptype!= NULL) total_strlen += strlen(heaptype) + 1;

   /* call malloc once for the entry, use ptr[i].src to store the address */

   ptr[i].src=(char*) shMalloc(total_strlen);
   (void) strcpy(ptr[i].src, tmp_src);

/* VKG: Commented to in order to make Tblcol case insensitive
   tmp = ptr[i].src;
   while(*tmp!='\0') *tmp++=toupper(*tmp);
*/
   tmp = ptr[i].src + strlen(ptr[i].src);
   *tmp++ = '\0';
  
   ptr[i].dst = strcpy(tmp, tmp_dst);
   tmp += strlen(tmp_dst);
   *tmp++='\0';

   /* store dst into hash table */

   if(convtype==CONVERSION_BY_TYPE)
    {
             hashEntryAdd(xtbl_ptr, tmp_dst, i);  
    }

   ptr[i].dsttype = strcpy(tmp, dsttype);
   tmp += strlen(dsttype);
   *tmp++='\0';


   if(proc!=NULL) 
   {
        ptr[i].proc = strcpy(tmp, proc);
        tmp += strlen(proc);
        *tmp++='\0';
    }
   
    if(size!=NULL) 
    {
        ptr[i].size = strcpy(tmp, size);
        tmp += strlen(size);
        *tmp++='\0';
     }

    ptr[i].srcTodst= r;
    ptr[i].dstDataType = shSchemaTransDataTypeGet(dsttype, NULL);


   /* analyze the size string */
     
    if(size==NULL) 
    {
            ptr[i].num[0]=1;
            ptr[i].num[1]=0;
    }
    else 
      if(ptr[i].dstDataType!=DST_HEAP) 
	for(j=0; j < MAX_INDIRECTION; j++) 
	  {
            if(j == 0) tmp=strtok(size, "x");
            else      tmp=strtok(NULL, "x");
            if(tmp!=NULL) {ptr[i].num[j]=atoi(tmp);}
            else if(j==0) {ptr[i].num[j]=atoi(size); break;}
            else {  ptr[i].num[j] = 0; break;}
	  }
    
   if(ptr[i].dstDataType==DST_HEAP)
     {
       if(heaptype!=NULL) tmp_type=shSchemaTransDataTypeGet(heaptype, NULL);
       else 
	 {
           shErrStackPush("Must know heaptype");
           return SH_GENERIC_ERROR;
	 }
       if(tmp_type==DST_UNKNOWN)
	 {
	   shErrStackPush("%s is not a known type\n", heaptype);
	   return SH_GENERIC_ERROR;
	 }
       if(/* tmp_type !=DST_STR && */ heaplen == NULL)
	 {
           shErrStackPush("Must know heaplength");
           return SH_GENERIC_ERROR;
	 }

       ptr[i].heaptype = strcpy(tmp, heaptype);
       j = 0;
/* VKG: Commented to in order to make Tblcol case insensitive 
      while( j < strlen(heaptype)) {*tmp = toupper(*tmp); tmp++; j++;}
      *tmp++='\0';
*/
       tmp = ptr[i].heaptype + strlen(ptr[i].heaptype);
       *tmp++ = '\0';       

       if(heaplen!=NULL) ptr[i].heaplen = strcpy(tmp, heaplen);
       tmp += strlen(heaplen);
       *tmp++='\0';
      
     }
   
   i++;
   xtbl_ptr->entryNum = i;
   /* always initialize the last entry to proper values */

   shSchemaTransInit(xtbl_ptr, i);
   return SH_SUCCESS;
}



/* this code optionally deletes all lines that 
   are associated with the entry for multi-line
   entries. Memory associated with the entries
   is also free-ed. */

RET_CODE
shSchemaTransEntryDel(SCHEMATRANS* xtbl_ptr, 
      unsigned int entry, 
      unsigned short delete_related
)
{
   int j = (int) entry;
   unsigned int entry_start=entry;
   unsigned int num = 1;    /* number of entries actually to be deleted */
   SCHEMATRANS_ENTRY* ptr = xtbl_ptr->entryPtr;
  
   if( entry > MAX_SCHEMATRANS_ENTRY_SIZE - 1) 
   {
        shErrStackPush("illegal entry number %d", entry);
        return SH_TABLE_FULL;
   }

   if(delete_related) 
   {  
      if(ptr[entry].type==CONVERSION_CONTINUE) 
      {
      /* find the continue lines before the entry */

        while(ptr[j].type==CONVERSION_CONTINUE) { j--; num++;if (j < 0) break; }
        entry_start=j;
        if ( j < 0 ) 
         {
          shErrStackPush("First line can't be continue line. Delete failed.");
          return SH_TABLE_SYNTAX_ERR;
         }
      }

    /* find the continue lines after the entry */

      j=entry+1;
      while(ptr[j].type==CONVERSION_CONTINUE) 
      { 
        j++; num++; 
        if (j >MAX_SCHEMATRANS_ENTRY_SIZE - 1) break; 
      }
      if ( j >MAX_SCHEMATRANS_ENTRY_SIZE - 1 ) 
      {
          shErrStackPush("Continue line beyond the table size. "\
                  "Delete failed.");
          return SH_TABLE_SYNTAX_ERR;
      }
   }
   
   for(j=entry_start; j < entry_start + num; j++ ) 
   {

      if(ptr[j].type==CONVERSION_BY_TYPE)
               hashEntryDel(xtbl_ptr, ptr[j].dst, j);
      shFree(ptr[j].src);             /* free the memory mallocked */
      (void) memset((char*) ptr[j].num, 0, MAX_INDIRECTION*sizeof(int));
   }

   for(j=entry_start; ptr[j].type!=CONVERSION_END; j++)
         ptr[j] = ptr[j+num];
         
   xtbl_ptr->entryNum -= num;
   shSchemaTransInit(xtbl_ptr,j-1);
   return SH_SUCCESS;
   
}



/* imports multiple entries from a source table to
   destination table. Useful for copying tables. */

RET_CODE
shSchemaTransEntryImport(
	 SCHEMATRANS* xtbl_src,          /* source table */
         SCHEMATRANS* xtbl_dst,          /* destination table */
         int src_start,                  /* beginning in src */
         int src_end,                    /* ending in src */
         int dst_pos                     /* begining in dst */
)
{
    int i, num, dst_start, dst_end, k;

    SCHEMATRANS_ENTRY* src = xtbl_src->entryPtr;
    SCHEMATRANS_ENTRY* dst = xtbl_dst->entryPtr;

    if(dst == NULL || src == NULL)
    {
         shErrStackPush("No source or destination table\n");
         return SH_INVARG;
    }
    
    if(src_end > xtbl_src->entryNum) src_end = xtbl_src->entryNum;
    
    if( src_end < src_start || src_end < 0 || src_start < 0) 
    {
         shErrStackPush("Illegal entry range specification\n");
         return SH_INVARG;
    }
    
    num= src_end - src_start;
    if(num == 0) return SH_SUCCESS;
    dst_end = xtbl_dst->entryNum;
    if(dst_pos < 0) dst_start = dst_end;
    else  dst_start = dst_pos;

    if(dst_end + num > xtbl_dst->totalNum - 1)
    {
         shErrStackPush("Not enough entry spaces in the destination table."\
              " Fill in as many as I can.\n");
         num = MAX_SCHEMATRANS_ENTRY_SIZE -1 - dst_end;
    }

    for(i=dst_end; i >= dst_start; i--) dst[i+num ]= dst[i];
    for(i=dst_start; i < dst_start + num; i++) 
    {
         k =src_start + i - dst_start;
         shSchemaTransEntryAdd(xtbl_dst, src[k].type, src[k].src, src[k].dst, 
                  src[k].dsttype, src[k].heaptype, src[k].heaplen, 
                  src[k].proc, src[k].size, 
                  src[k].srcTodst,  i);
    }

    shSchemaTransInit(xtbl_dst, dst_end + num +1);
    return SH_SUCCESS;
}


/* print out the table on stdout */

void shSchemaTransEntryShow(
      SCHEMATRANS* base,   /* base pointer to array */
      int start,	   /* start entry */
      int end		   /* stop entry */
)		
{
      unsigned int i;
      SCHEMATRANS_ENTRY* ptr;

      if(start < 0) start = 0;
      if(end >= 0 && start > end ) return;
      else if ( start == end ) end++;
      if(end < 0 ) end = base->entryNum;

      if(base->entryPtr[0].type == CONVERSION_END) return;
      printf("Entry ConvType FITS-side\tSCHEMA-side\tFieldType\t"\
         "Ratio\tConstructor\tSize\tHeapType\tHeapNum\n");
      for(i = start; i < end ; i++)
       {
           ptr = &base->entryPtr[i];
           switch (ptr->type) 
            {
               case CONVERSION_BY_TYPE:
                    printf("\n%d     name     %s\t%s\t%s\t%f",
                          i, ptr->src, ptr->dst, ptr->dsttype, ptr->srcTodst);
                    if(ptr->proc!=NULL) printf("\t%s", ptr->proc);
                    else                printf("\tnone");
                    if(ptr->size!=NULL) printf("\t%s", ptr->size);
                    else                printf("\tnone");
                    if(ptr->heaptype!=NULL) printf("\t%s\t%s",ptr->heaptype, ptr->heaplen);
                    else                printf("\t-- N/A --");
                    break;

               case CONVERSION_IGNORE:
                    printf("%d         ignore          %s  ",  i, ptr->dst);
                    break;

               case CONVERSION_CONTINUE:
                    printf("%d     cont     %s\t%s\t%s",
                          i, ptr->src, ptr->dst, ptr->dsttype);
                    if(ptr->proc!=NULL) printf("\t%s", ptr->proc);
                    else                printf("\tnone");
                    if(ptr->size!=NULL) printf("\t%s", ptr->size);
                    else                printf("\tnone");
                    if(ptr->heaptype!=NULL) printf("\t%s\t%s",ptr->heaptype, ptr->heaplen);
                    else                printf("\t-- N/A --");
                    break;

               case CONVERSION_END:
               default:  break;
           }
          printf("\n");
       }
     
     return ;
}


/* dump the contents of a translation table to a file */


RET_CODE shSchemaTransWriteToFile(
      SCHEMATRANS* base,   /* base pointer to array */
      char*        fileName, /* file name to dump to */
      int start,	   /* start entry */
      int end		   /* stop entry */
)		
{
      unsigned int i;
      SCHEMATRANS_ENTRY* ptr;
      FILE *fp = NULL;

      /* open the file */

      fp = fopen(fileName, "w");
      if(fp == NULL) 
      {
          shErrStackPush("Fail to create/append file %s", fileName);
          return SH_GENERIC_ERROR;
      }

      /* write some header information */

      fprintf(fp, "#\n# following entries are machine-generated. "\
                  "Editing is welcome. \n"\
                  "#\n# All comments start with pound sign.\n"\
                  "#\n# Entries are in following order: \n"\
                  "# ConversionType   FitsSideName    ObjSideName  "\
                  "ObjDataType   others   ;\n"\
                  "#\n# An entry always ends with a semicolon ';'\n");


      if(start < 0) start = 0;
      if(end >= 0 && start > end ) { fclose(fp); return SH_SUCCESS;}
      else if ( start == end ) end++;
      if(end < 0 ) end = base->entryNum;

      if(base->entryPtr[0].type == CONVERSION_END) { fclose(fp); return SH_SUCCESS;}
      for(i = start; i < end ; i++)
       {
           ptr = &base->entryPtr[i];
           switch (ptr->type) 
            {
               case CONVERSION_BY_TYPE:
                    fprintf(fp,"\nname  %s  %s  %s\t",ptr->src, ptr->dst, ptr->dsttype);
                    if((int)(ptr->srcTodst+0.0001)!=1) fprintf(fp, " -ratio %f", ptr->srcTodst);
                    break;
               
               case CONVERSION_IGNORE:
                    fprintf(fp, "ignore  \t\t%s\t",  ptr->dst);
                    break;

               case CONVERSION_CONTINUE:
                    fprintf(fp, "cont  %s  %s  %s\t", ptr->src, ptr->dst, ptr->dsttype);
                    break;

               case CONVERSION_END:
               default:  break;
            }

            if(ptr->proc!=NULL) 
            {
                 if(strchr(ptr->proc, ' ')==NULL) fprintf(fp," -proc %s", ptr->proc);
                 else fprintf(fp," -proc=\"%s\"", ptr->proc);
            }
            if(ptr->size!=NULL) 
            {
                if(strchr(ptr->size, ' ')==NULL) fprintf(fp," -dimen %s", ptr->size);
                else fprintf(fp," -dimen=\"%s\"", ptr->size);
            }
            if(ptr->heaptype!=NULL)
            {
                 fprintf(fp," -heaptype %s",ptr->heaptype);
                 if(strchr(ptr->heaplen, ' ')==NULL) 
                       fprintf(fp, "  -heaplength %s" , ptr->heaplen);
                 else fprintf(fp, "  -heaplength \"%s\"" , ptr->heaplen);
             }
               
          fprintf(fp,"  ;\n");
       }
     
      fprintf(fp, "\n\n#\n# end of table\n#\n");
      fclose(fp);
      return SH_SUCCESS;
}



/*   creating a table from a file ****/

RET_CODE shSchemaTransCreate(
      SCHEMATRANS* base,     /* base pointer to array */
      char*        fileName  /* file name to dump to */
)		
{
      FILE *fp = NULL;

      /* open the file */

      fp = fopen(fileName, "r");
      if(fp == NULL) 
      {
          shErrStackPush("Fail to read file %s", fileName);
          return SH_GENERIC_ERROR;
      }

      /*  some parsing here */

      fclose(fp);
      return SH_SUCCESS;
}
     

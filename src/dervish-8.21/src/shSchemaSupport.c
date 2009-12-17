/*****************************************************************************
  This file contains the supporting routines to use in shSchema.c and tclSchema.c
  for conversion between TBLCOL and schema. 

  Environment: ANSI C
  Author:      Wei Peng
  
  Creation date: June 29, 1994

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* #include <fp_class.h> */

#include "libfits.h"
#include "dervish_msg_c.h"
#include "shCErrStack.h"
#include "shChain.h"
#include "shCUtils.h"
#include "shCGarbage.h"
#include "shCSchema.h"
#include "shCTbl.h"
#include "shCArray.h"
#include "shTclHandle.h"
#include "shCAlign.h"


#include "shCSchemaSupport.h"
#include "shCSchemaTrans.h"


static
char * shSpptPointerDeRef(char *, int, int*, int, int);
static void initDataTypes(void);

/* these are the globals to use in shSpptSchemaTypeToDSTType */

static unsigned initDataTypeAlreadyCalled = FALSE;
static TYPE sh_uchar, sh_schar, sh_char, sh_string;
static TYPE sh_short, sh_ushort;
static TYPE sh_int,   sh_uint;
static TYPE sh_long, sh_ulong;
static TYPE sh_float, sh_double;
static TYPE sh_unknown;


static void initDataTypes(void)
{
#define setType(ctype, shtype) ctype = shTypeGetFromName(#shtype)

    setType(sh_schar,SCHAR);
    setType(sh_uchar,UCHAR);
    setType(sh_char,CHAR);
    setType(sh_string,STR);

    setType(sh_short,SHORT);
    setType(sh_ushort,USHORT);

    setType(sh_int,INT);
    setType(sh_uint,UINT);

    setType(sh_long,LONG);
    setType(sh_ulong,ULONG);

    setType(sh_float,FLOAT);
    setType(sh_double,DOUBLE);
    setType(sh_unknown,UNKNOWN);

    initDataTypeAlreadyCalled = TRUE;
}


TYPE        shSpptDSTTypeToSchemaType(DSTTYPE dstType)
{
       TYPE type;
       
       if(!initDataTypeAlreadyCalled) initDataTypes();
       switch(dstType)
       {
          case DST_CHAR  :   type =  sh_char; break;
          case DST_UCHAR :   type =  sh_uchar; break;
          case DST_SCHAR :   type =  sh_schar; break;

          case DST_SHORT :   type =  sh_short; break;
          case DST_USHORT:   type =  sh_ushort; break;
         
          case DST_INT   :   type =  sh_int; break;
          case DST_UINT  :   type =  sh_uint; break;

          case DST_LONG  :   type =  sh_long; break;
          case DST_ULONG :   type =  sh_ulong; break;
         
          case DST_FLOAT  :  type =  sh_float; break;
          case DST_DOUBLE :  type =  sh_double; break;

          case DST_ENUM  :
          case DST_STR   :   type = sh_string; break;

          case DST_STRUCT:
          case DST_UNKNOWN:  
          default         :  type = sh_unknown; break;
       }

       return type;
}


    
DSTTYPE   shSpptSchemaTypeToDSTType(TYPE type)
{
       DSTTYPE dstType;

       if(!initDataTypeAlreadyCalled) initDataTypes();

       if(type == sh_uchar) 	dstType = DST_UCHAR;
       else if(type==sh_schar) 	dstType = DST_SCHAR;
       else if(type==sh_char) 	dstType = DST_CHAR;
       else if(type==sh_string)	dstType = DST_STR;
       else if(type==sh_short)  dstType = DST_SHORT;
       else if(type==sh_ushort) dstType = DST_USHORT;
       else if(type==sh_int)   	dstType = DST_INT;
       else if(type==sh_uint)  	dstType = DST_UINT;
       else if(type==sh_long)   dstType = DST_LONG;
       else if(type==sh_ulong)  dstType = DST_ULONG;
       else if(type==sh_float)  dstType = DST_FLOAT;
       else if(type==sh_double) dstType = DST_DOUBLE;
       else       	        dstType = DST_UNKNOWN;
       
       return dstType;
}

      



/* extract the array dimension info specified in str and store
   the info into num. For example, if str is "abcd[2][3][4]",
   num will be {2, 3, 4}. num should at least have MAX_INDIRECTION
   spaces. */


 int
p_shSpptArrayInfoExtract(char* str, int *num)
{
     char tmp[MAX_NAME_LEN];
     char* ltoken="[]<>";
     char* cptr=NULL;

     int  dimen=0, tmp_int=0;

     if(str==NULL || num == NULL) return -1;
     strcpy(tmp, str);
     if(strchr(tmp, '[') == NULL 
                && strchr(tmp, '<')==NULL) return 0;

     else   /* only one set of notation is allowed */
       if(strchr(tmp, '[')!=NULL 
            && strchr(tmp, '<')!=NULL)  return -1;

     cptr=strpbrk(tmp, ltoken);
     cptr++;
     cptr=strtok(cptr, ltoken); 
     while(cptr!=NULL)
      {  
          if(dimen > MAX_INDIRECTION-1)
          {
                 shErrStackPush("Too many dimensions: %s\n", str);
                 return -1;
           }

          tmp_int = atoi(cptr);
          if(tmp_int < 0) 
          {
                    shErrStackPush("For %s: illegal array "\
                          "specification %d.\n", str, tmp_int);
                    return -1;
          }
          num[dimen++]=tmp_int;
          cptr=strtok(NULL, ltoken); 
           
      }
      return dimen;
}


/* 
   Check str for array info. Dimension info is stored in
   num (if not NULL). Output array_size in accordance of
   dimension information. Output the dimension specified
   in str. Token is to specify what kind of token in
   str to look for. If NULL, token is [], i.e, array
   represented by brackets. if not NULL, they must be
   "x" or " ", the former is used in size for each entry
   in a table, and the latter, used in schema. 

   Returned array_size if 1-based. 

   Example:  

     1. str = abce[2][3][4], while num={5,6,7}, token=NULL. 
        On return, array_size = 2x6x7 + 3x7 + 4, dimen=3, 
        Most likely found in the src or dst field of an entry.

     2. str = 2x3x4, num=NULL, token="x". 
        On return, array_size = 2+3+4, dimen=3.
        Most likely found in the size field of an entry.

     3. str = "2 3 4", num=NULL, token=" ". 
        On return, array_size = 2+3+4, dimen=3.
        Most likely found in the nelem field of SCHEMA_ELEM.
    
 */
             


char* 
p_shSpptArrayCheck(
     char* str, int* num, /* num is array of MAX_INDIRECTION */
     int* array_size, 
     int *dimen,
     char* token /* if NULL, check array syntax [] */
)
{
       
    char tmp[MAX_NAME_LEN];
    char ltoken[MAX_NAME_LEN];   /* local token */
    char half_token='[';
    char* cptr = NULL;
    int i=0;
    int tmp_dimen = 0;
    int tmp_int   =0;
    int tmp_array_size = 0;
    int block=1;
    unsigned has_arrow=FALSE;
    unsigned has_square=FALSE;

    if(str!=NULL) (void)strcpy(tmp, str); 
    else          
    {
       if(array_size != NULL) *array_size = 1;
       if(dimen != NULL ) *dimen = 0;
       return NULL;
    }

    if(token!=NULL) strcpy(ltoken, token);
    else            strcpy(ltoken, "[]<>");

    has_arrow = (strchr(tmp,'<')!=NULL);
    has_square = (strchr(tmp, '[')!=NULL);

    /* determine which half_token to use */

    if( has_arrow && has_square) return NULL;
    if(has_arrow) half_token = '<';
    else if(has_square) half_token = '[';

    /* check for array syntax if token is NULL */
    
    if(token==NULL && (!has_arrow) && (!has_square)) 
    {
       if(array_size != NULL) *array_size = 1;
       if(dimen != NULL ) *dimen = 0;
        return str;
    }
   
    if(token==NULL) 
    {
       /* check if brackets are balanced */

       cptr=tmp;
       while((cptr=strchr(cptr+1, half_token))!=NULL) tmp_dimen +=1;
       
       cptr=tmp;
 
           while((cptr=strchr(cptr+1, half_token))!=NULL) tmp_int +=1;

       if( tmp_dimen != tmp_int)
       {
          shErrStackPush("Unbalanced brackets\n");
          return NULL; 
       }

      tmp_dimen=0;
      tmp_int =0;
    }

   i=0;
   block=1;
   tmp_array_size=1;
   if(num!=NULL) while(i < MAX_INDIRECTION && num[i]!=0) {block *=num[i++];}
   
  /* if token==NULL will search for ltoken the default behavior */

   cptr=tmp;
   if(token==NULL) 
   {  
      cptr=strpbrk(tmp, ltoken);
      cptr++;
   }
   cptr=strtok(cptr, ltoken); 
   while(cptr!=NULL)
   {  
          if( tmp_dimen > MAX_INDIRECTION-1 )
          {
                  shErrStackPush("%s: Too many dimensions\n", str);
                 return NULL;
           }

           if(num!=NULL)  
           {
                 if(num[tmp_dimen]!=0) block /=  num[tmp_dimen];
                 tmp_int = atoi(cptr);
                 if(tmp_int >= (num[tmp_dimen]==0 ? 1: num[tmp_dimen])) 
                 {
                    shErrStackPush("For %s: array out of bound. "\
                            "Index %d asked out of %d\n", 
                            str, tmp_int, num[tmp_dimen]);
                    return NULL;
                 }
                 /* when fastest dimension encountered, block=0 or 1 */

                 if(block==0)  tmp_array_size += atoi(cptr);  
                 else tmp_array_size  += atoi(cptr) * block;
           }
                 
           else  
           {
              tmp_int = atoi(cptr);
              if(tmp_int <= 0) 
              {
                    shErrStackPush("For %s: illegal array "\
                          "specification index %d.\n", str, tmp_int);
                    return NULL;
               }
              tmp_array_size *=atoi(cptr);
           }
           tmp_dimen       += 1;
           cptr=strtok(NULL, ltoken); 
           
    }
   
    if(array_size != NULL) *array_size = tmp_array_size;
    if(dimen != NULL ) *dimen = tmp_dimen;
    return str;


     
}

static 
SCHEMA_ELEM* shSpptNextEntrySchemaElem(
         SCHEMATRANS * xtbl_ptr,
         char*         type,
         int           cur_entry
)
{
     char tmp[MAX_NAME_LEN], *tmpPtr=NULL, *str;
     char string[MAX_NAME_LEN];
     SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;

     if(cur_entry < 0 ) cur_entry = 0;
     str = xptr[cur_entry].dst;
     if(type==NULL || str==NULL || xptr==NULL) return NULL;
     str=strcpy(string, xptr[cur_entry].dst);
     if(strchr(str, '[') != NULL || strchr(str, '<')!=NULL)
        tmpPtr = strcpy(tmp, strtok(str, "<["));
     else tmpPtr = strcpy(tmp, str);

     return (SCHEMA_ELEM*) shSchemaElemGet(type, tmpPtr);
}


/* compare two string that may have array specification */
/* return -1 if different, 0, if same except array, 1 otherwise */

int p_shSpptStringCmp(char* s1, char* s2, unsigned short check_array)
{
     char tmp1[MAX_NAME_LEN], *tmpPtr1 = NULL;
     char tmp2[MAX_NAME_LEN], *tmpPtr2 = NULL;
     unsigned has_array1 = FALSE;
     unsigned has_array2 = FALSE;

     if(s1 == NULL || s2 == NULL) return -1;
     tmpPtr1=strcpy(tmp1, s1);
     tmpPtr2=strcpy(tmp2, s2);
     if(check_array)
     {
        has_array1 = strchr(tmp1,'[') != NULL || strchr(tmp1,'<')!=NULL;
        if(has_array1) tmpPtr1=strcpy(tmp1,strtok(tmp1,"<["));
        has_array2 = strchr(tmp2,'[') != NULL || strchr(tmp2,'<')!=NULL;
        if(has_array2) tmpPtr2=strcpy(tmp2,strtok(tmp2,"<["));
     }
     if(strcmp(tmpPtr1, tmpPtr2)!=0) return -1;
     if(check_array &&(has_array1 || has_array2)) return 0;
     return 1;
}



/* given a string, find the entry in a translation table
   whose dst field matches the string. 
 */

int                              
p_shSpptEntryFind(
    SCHEMATRANS*    xtbl_ptr,       /* input: translation table */
    char*           str           /* input: str to map from */
 )
{
    int i, j, value;
    SCHTR_STATUS istat;

    SCHEMATRANS_ENTRY* ptr = xtbl_ptr->entryPtr;

    int last_entry = xtbl_ptr->entryNum;

    if(ptr==NULL || str==NULL || strlen(str) < 1) 
    {
#if 0
        shErrStackPush("Illegal values passed to the entry find routine.\n");
        return -1;
#endif
        shFatal("Illegal values passed to the entry find routine.\n");
    }
#if 0
    is_digit = isdigit(str[0]);   /* if a digit, first character should be a digit */
    if(is_digit) 
    {
            shErrStackPush("Can't do search by number");
             return -1;
    }
#endif
    value = shSchemaTransGetHashValue(str);
    last_entry = xtbl_ptr->hash[value].curNum;
    for(j = 0; j < last_entry; j++)
    {
        i =xtbl_ptr->hash[value].entries[j];

        if (ptr[i].type== CONVERSION_END ) break ;
        istat = shSchemaTransStatusGet(xtbl_ptr, i);
        if(istat ==SCHTR_VISITED || istat == SCHTR_VGOOD ) 
                    continue; 
        if(ptr[i].type==CONVERSION_CONTINUE || 
                  ptr[i].type==CONVERSION_IGNORE) continue;
        if(p_shSpptStringCmp(str, ptr[i].dst, TRUE) >= 0) 
        {
             if(istat == SCHTR_FRESH)     shSchemaTransStatusSet(xtbl_ptr,SCHTR_VISITED, i);
             else if(istat == SCHTR_GOOD) shSchemaTransStatusSet(xtbl_ptr,SCHTR_VGOOD, i);
             return i;   
        }         
       
     }
    
    return -1;
}
 



/* given an entry in translation table, check dst 
   (reprsents schema-side field name) agains required 
   array syntax. This routine is not to be called
   by others, but only called by shSpptGeneralSyntaxCheck().

 */


static RET_CODE 
shSpptArraySyntaxCheck(SCHEMATRANS* xtbl_ptr, int entry, SCHEMA_ELEM* elemptr)
{           
    int req_array_size=1, existing_array_size=1;
    int dst_array_index=0, total_array_size=1 ;
    int req_dimen=0, existing_dimen=0, dst_dimen=0, total_dimen=0;
    SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;
    SCHTR_STATUS entry_status;

    char tmp[MAX_NAME_LEN], *tmpPtr;
    int j, k;
    int schema_nelem[MAX_INDIRECTION];
    DSTTYPE dstDataType;

    if(xptr[entry].size!=NULL && xptr[entry].dstDataType!=DST_HEAP) 
    {
       if(p_shSpptArrayCheck(xptr[entry].size, NULL, &req_array_size, 
          &req_dimen, "x")==NULL) 
       {
           shErrStackPush("Fail to check the array \n");
           return SH_ARRAY_CHECK_ERR ;
       }
    }
    else
    {
        req_array_size = 1;
        req_dimen=0;
    }

    /* if the entry is already checked, return immediately. */

    entry_status = shSchemaTransStatusGet(xtbl_ptr, entry);
    if(entry_status == SCHTR_GOOD || entry_status == SCHTR_VGOOD)
             return SH_SUCCESS;

    /* push the schema array information into the table */

    if(elemptr->nelem!=NULL)
     {
          strcpy(tmp, elemptr->nelem);
          j=0; 
          memset((char*)schema_nelem, 0, MAX_INDIRECTION*sizeof(int));
          tmpPtr=strtok(tmp, " ");
          while(tmpPtr!=NULL) 
          { 
              if(j >= MAX_INDIRECTION ) 
              {   /*  an unlikely case */
                  shErrStackPush("Array dimension large for %s."\
                     "Current limit is %d\n", elemptr->name, MAX_INDIRECTION);
                  return SH_TOO_MANY_INDIRECTIONS;
              }
              schema_nelem[j]=atoi(tmpPtr);
              existing_array_size *= schema_nelem[j++];
              tmpPtr=strtok(NULL, " ");
           }
           existing_dimen =j;
           if( j + req_dimen >=  MAX_INDIRECTION ) 
           {  
                shErrStackPush("Array dimension large for %s."\
                    " Current limit is %d\n", elemptr->name, MAX_INDIRECTION);
                return SH_TOO_MANY_INDIRECTIONS;
           }

           for(k=j; k < MAX_INDIRECTION; k++) 
           {
               schema_nelem[k]=xptr[entry].num[k - j];
           }
           memcpy((char*) xptr[entry].num, (char*) schema_nelem, 
                     MAX_INDIRECTION*sizeof(int));
      }


     /* don't check syntax for heap entry */

     if(xptr[entry].dstDataType==DST_HEAP) return SH_SUCCESS;

     total_dimen = existing_dimen + req_dimen;
     total_array_size = existing_array_size * req_array_size;
            
     if(p_shSpptArrayCheck(xptr[entry].dst, xptr[entry].num, 
           &dst_array_index, &dst_dimen, NULL)==NULL)
                    return SH_ARRAY_CHECK_ERR; 
     dst_array_index--;

     /* check array requested: 
        for complex type:      req_dimen must be nstar - 1. leave one star.
        for elementary type:   req_dimen must be nstar to fully resolve the 
                               indirections 

        check array access:
        for complex type:      dst_dimen must equal to total dimension 
                                = req + existing
        for elementary type:   dst_dimen can be anything between 
                           existing_dimen and total dimension, or 0

        See documentation for why this is so required. 
     */

    dstDataType = xptr[entry].dstDataType;

    if(dstDataType==DST_STR && total_dimen - dst_dimen!= 1)
    {
          shErrStackPush("Can't make a string from entry %d\n", entry);
          return SH_ARRAY_SYNTAX_ERR;
    }
    if(dstDataType!=DST_HEAP && dstDataType!=DST_STRUCT && 
             dstDataType!=DST_UNKNOWN) 
    {
          /* elementary type */
       if(req_dimen !=elemptr->nstar)
       {
            shErrStackPush("Wrong dimension specification at entry %d ", 
                         entry);
            shErrStackPush("Requested dimension must equal the number"\
                " of indirections for elementary types\n");
            return SH_ARRAY_SYNTAX_ERR;
       }
     
      if( dst_dimen > total_dimen || 
           (dst_dimen!=0 && dst_dimen < existing_dimen))
      {  
            shErrStackPush("Elementary type array: wrong dimension specification at entry %d.\n", entry);
            return SH_ARRAY_SYNTAX_ERR;
      }


    }
   else  if( xptr[entry].dstDataType!=DST_HEAP) /* complex types */
   {

      if((elemptr->nstar > 0) && req_dimen !=elemptr->nstar -1)
       {
            shErrStackPush("Wrong dimension specification at entry %d "\
                 "for %s ",  entry, elemptr->name);
            shErrStackPush("Requested dimension %d vs. the number of"\
               " indirections %d\n", req_dimen, elemptr->nstar);
            return SH_ARRAY_SYNTAX_ERR;
       }
      if( elemptr->nstar == 0 && req_dimen !=0 )
       {
            shErrStackPush("Wrong dimension specification at entry %d. ", entry);
            shErrStackPush("No dimension needed.\n");
            return SH_ARRAY_SYNTAX_ERR;
       }

      if(((dst_dimen!=0 && total_dimen!=0)&& dst_dimen != total_dimen )
           || (total_dimen==0 && dst_dimen!=0))
      {  
            shErrStackPush("Complex type array: wrong dimension specification at entry %d.\n", entry);
            return SH_ARRAY_SYNTAX_ERR;
      }
      
   }

   if(dst_array_index!=0 && dst_array_index > total_array_size -1)
   {
       shErrStackPush("Bad index at entry %d: total requested %d "\
         "but only %d specified.\n", entry, dst_array_index, total_array_size);
       return SH_ARRAY_SYNTAX_ERR;
    } 
    return SH_SUCCESS;
}




/* check the syntax of the translation table against given schema */


RET_CODE
shSpptGeneralSyntaxCheck(SCHEMATRANS *xtbl_ptr, SCHEMA* schema, unsigned to_table)
{
     int i, j;
     SCHEMA_ELEM * elemptr=NULL, *child_elemptr=NULL;
     SCHEMA_ELEM*  parent=NULL;
     SCHEMA*  child_schema=NULL;
     SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;
     int entry, entry_save, existing_dimen=0;   
     char tmp[MAX_NAME_LEN], *tmpPtr=NULL;
     int last_entry = xtbl_ptr->entryNum;
    
     if(xptr==NULL || schema==NULL) return SH_INVARG;
     for(i=0; i < schema->nelem; i++)
     {
         elemptr = &schema->elems[i];
         entry=0;
         existing_dimen=0;
         while((entry = p_shSpptEntryFind(xtbl_ptr, elemptr->name)) >= 0)
         {
             /* don't check ignored fields */

            if(xptr[entry].type == CONVERSION_IGNORE) continue;

             /* check array syntax */

             if(shSpptArraySyntaxCheck(xtbl_ptr, entry, elemptr)!=SH_SUCCESS)
                                       return  SH_ARRAY_SYNTAX_ERR;
             parent = elemptr;
             shSchemaTransStatusSet(xtbl_ptr,SCHTR_VGOOD, entry);

             entry_save = entry++;
             
             /* deal with continuation lines */

             while(xptr[entry].type==CONVERSION_CONTINUE)
             {
                child_schema = (SCHEMA*) shSchemaGet(parent->type);
                child_elemptr = shSpptNextEntrySchemaElem(xtbl_ptr, child_schema->name,entry);
                if(child_elemptr==NULL)
                {
                    shErrStackPush("shSpptGeneralSyntaxCheck: bad continuation line at %d", entry);
                    return  SH_TABLE_SYNTAX_ERR;
                }
                
                if(shSpptArraySyntaxCheck(xtbl_ptr, 
                          entry, child_elemptr)!=SH_SUCCESS) return  SH_ARRAY_SYNTAX_ERR;
                parent = child_elemptr;
                shSchemaTransStatusSet(xtbl_ptr,SCHTR_VGOOD, entry++);

              }/* while */
              entry = entry_save;

           }/* while */

      }/* for-i */

      /* other checks */
       
      for(i=0; i < last_entry ; i++)
      {
          if(shSchemaTransStatusGet(xtbl_ptr, i)== SCHTR_FRESH) 
          {  

              if(xptr[i].type!=CONVERSION_CONTINUE 
                     && xptr[i].type!=CONVERSION_IGNORE)
                 printf("Warning: entry %d is not used by schema %s.\n",
                                       i, schema->name);
          }
          else shSchemaTransStatusSet(xtbl_ptr, SCHTR_FRESH, i);
         
          /* table syntax -- check agains continuation */

         if(xptr[i].dstDataType==DST_STRUCT)
         {
             if(xptr[i+1].type!=CONVERSION_CONTINUE)
             {
                 shErrStackPush("Continuation line missing at entry %d\n",i+1);
                 return  SH_TABLE_SYNTAX_ERR;
             }
             tmpPtr=strcpy(tmp, xptr[i].dst);
/* VKG: Commented to in order to make Tblcol case insensitive
             while(*tmpPtr!='\0') *tmpPtr++=toupper(*tmpPtr);
*/
             tmpPtr=tmp;
             if(shStrcmp(tmpPtr, xptr[i+1].src, 0)!=0)
             {
                 shErrStackPush("Entry %d has bad field name. Should be %s not %s\n",
                      i+1, tmpPtr,  xptr[i+1].src );
                 return  SH_TABLE_SYNTAX_ERR;
              }
         }
         if(xptr[i].dstDataType==DST_HEAP)
         {
            j=i;
            while(xptr[j].type==CONVERSION_CONTINUE) j--;
            if(strchr(xptr[j].src, '[')!=NULL || strchr(xptr[j].src, '<')!=NULL)
            {
                shErrStackPush("FITS heap field can't be an array: %s at entry %d\n",
                     xptr[j].src, j);
                return SH_TABLE_SYNTAX_ERR;
            }
           if(xptr[i].heaptype==NULL)
           {
                shErrStackPush("You must specify heap type at entry %d\n", i);
                return SH_TABLE_SYNTAX_ERR;
            }
           if(to_table && xptr[i].heaplen==NULL)
           {
               shErrStackPush("schemaToTbl requires a priori knowledge about heap length: entry %d\n", i);
                return SH_TABLE_SYNTAX_ERR;
           }

         } /* if ... == DST_HEAP */

          /* enforce the rule that two entries should 
             not have identical components, for schemaToTbl 
             this is a pain for large tables, as the loop time
             goes as n^2.

           */

         if(to_table)
           for(j=i+1; j < last_entry ; j++)
           {
               if(strcmp(xptr[i].src, xptr[j].src) 
                 || strcmp(xptr[i].dst, xptr[j].dst)  ||
                  xptr[j].type==CONVERSION_CONTINUE
                  || xptr[j].type==CONVERSION_IGNORE) continue;
             
               shErrStackPush("Entry %d and %d have identical components: %s "\
                 "and %s\n Overwriting a same field is not allowed.\n",
                   i, j, xptr[i].src, xptr[i].dst);
               return SH_TABLE_SYNTAX_ERR;
            }


       } /* for loop */

   return SH_SUCCESS;
}       


/* this routine enforces the object construction order. Whenever *new_memory is
 * set to true, a newly allocated address holder is returned by the routine.
 * Caller is responsible to free this memory. 
 */

static char** shSpptCreateObjects(
       char** addr, 
       int*  size,
       DSTTYPE dstDataType,
       const SCHEMA* schema,
       int    obj_num,
       unsigned short *new_memory     /* new_memory should never be NULL */
)
{
       /* this routine enforces the malloc strategy */

    int i=0, obj_size = 0;
    char** tmp_addr = addr;

    *new_memory = FALSE;
    if(dstDataType == DST_HEAP) return NULL;
    if( addr != NULL && *size > 0) return addr;
    if( *size < obj_num || tmp_addr == NULL) 
    {
         if(dstDataType==DST_STRUCT) 
         {
             tmp_addr = (char**) shMalloc(obj_num*sizeof(char*));
             *size = obj_num;
         }
         else  
         {   
             tmp_addr = (char**) shMalloc(sizeof(char*));
             *size = 1;
         }
         if(tmp_addr==NULL) return NULL;
         *new_memory = TRUE;
         
    }
         
        
    if(schema!=NULL && schema->construct!=NULL)
    {
        for(i=0; i < obj_num; i++)
        {
            tmp_addr[i] = (char*) (*schema->construct)();
            if(tmp_addr[i] == NULL ) return NULL;
        }
        return tmp_addr;
    }

    /* for elementary type we'd have to malloc consecutive memory */

    if(dstDataType!=DST_STRUCT)
    {
        if(schema!=NULL) obj_size = schema->size;
        else  
        {
          switch(dstDataType)
          {   
              case DST_CHAR:
              case DST_SCHAR:
              case DST_UCHAR:    obj_size = sizeof(char);  break;
              case DST_SHORT:
              case DST_USHORT:   obj_size = sizeof(short); break;
              case DST_INT:
              case DST_UINT:     obj_size = sizeof(int);   break;
              case DST_LONG:
              case DST_ULONG:    obj_size = sizeof(long);   break;
              case DST_FLOAT:    obj_size = sizeof(float); break;
              case DST_DOUBLE:   obj_size = sizeof(double);break;
              default:           return NULL;
           }
         }
              
        tmp_addr[0] = (char*) shMalloc(obj_num * obj_size);
        if(tmp_addr[0]==NULL) return NULL;
        memset(tmp_addr[0], 0, obj_num * obj_size);
        *size = 1;
        return tmp_addr;
    }


    for(i=0; i < obj_num; i++)
    {   
          tmp_addr[i] = (char*) shMalloc(schema->size);
          if(tmp_addr[i] == NULL) return NULL;
          memset(tmp_addr[i], 0, schema->size);
    }

    return tmp_addr;
}





/* This routine checks/validates the state of a given field in
   a schema (referred by SCHEMA_ELEM) against a translation
   table. It expects to be given an array of valid memory addresses, 
   otherwise it'll call default constructor in the field's schema,
   or optionally malloc memory.

   The routine only deals with the given field. It doesn't traverse
   pointers to validate all indirected fields. Such validation, if
   necessary, is to be done outside by repeatedly calling this routine.
  (see shTclTblToSchema()). For example, if the given field is
   REGION* regionPtr, this routine only ensures upon successful return
   a valid memory is pointed by regionPtr. Fields that are pointers
   inside this REGION, however, is not taken care of.

 */

static RET_CODE
shSpptSchemaObjectStateCheck(
      SCHEMATRANS*    xtbl_ptr,     	/* points to a translation table */
      int             entry,           	/* entry in the table */
      SCHEMA_ELEM *   schemaElemPtr,  	/* current schema elem */
      char*           schemaElemInst,   /* points to a schema elem instance */
      char*           addr[],         	/* address the elem instance attaches to */
      int             size            	/* size of the memory area addr points to, 
                                           units = sizeof(addr) = 4 */
)                                    
{
     const SCHEMA* child_schema;
     char**        genericPtrPtr;         /* generic pointer to pointer */
     char*         stack[MAX_INDIRECTION];        /* stack */
     char**        tmp_addr = NULL;
     SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;
     int           existing_array_size=1; /* defined in schema */
     int           existing_dimen = 0;
     int           req_dimen =0;

     int           i, j, k, block_num=1, block_save=1;
     unsigned short has_new_memory = FALSE;
  
#if 0

    if(entry >  last_entry -1) 
    {
          shErrStackPush("Array index too large");
          return SH_INVARG;
    }

    else 
#endif
    if(entry < 0) return SH_SUCCESS;


    if(xptr[entry].type==CONVERSION_IGNORE) return SH_SUCCESS;
    if(schemaElemPtr->nstar == 0 ) return SH_SUCCESS;

    if(xptr[entry].dstDataType==DST_HEAP) return SH_SUCCESS;

    /* figure out if the field is an array */

    p_shSpptArrayCheck(schemaElemPtr->nelem, NULL, &existing_array_size, 
                          &existing_dimen, " ");
    if( schemaElemPtr->nstar > MAX_INDIRECTION ) 
    {
               shErrStackPush("Too many indirections");
               return SH_TOO_MANY_INDIRECTIONS;
     }

    /* figure out how many additional array dimensions to add */

     p_shSpptArrayCheck(xptr[entry].size, NULL, &block_num,&req_dimen, "x");

    
    if (addr!=NULL && size > 0 && size!=block_num) 
    {
         shErrStackPush("Incorrect number of objects supplied");
         return SH_OBJNUM_INVALID;
    }
     
    block_save = block_num;
    child_schema = shSchemaGet(schemaElemPtr->type);
    
    /* 05/06/94 I think I'd have to return SH_GENERIC_ERROR if
       child_schema is NULL. Some users may forget to put
       pragma SCHEMA, and hence make_io doesn't generate any
       thing about the structure. */

    if(xptr[entry].type==DST_STRUCT && child_schema==NULL) 
    {
        shErrStackPush("Sorry, no knowledge of schema %s",
               schemaElemPtr->type);
        return SH_GENERIC_ERROR;
    }

#if 0
    if(!force_malloc && child_schema!=NULL) 
          for(i=0; i < child_schema->nelem; i++) 
              nstar_sum += child_schema->elems->nstar; 
#endif

    /* 2 cases where non-contiguous memory is encountered */

    if((addr!=NULL && size > 0) || 
         (child_schema!=NULL && child_schema->construct!=NULL ))
    {
          /* this type consists of pointer fields */

       if(xptr[entry].num[schemaElemPtr->nstar -1] > 1) 
       {
              shErrStackPush("\nCan't use non-contiguous blocks: "\
                   "size of the fastest-varying dimension has to be 1\n");
              return SH_ARRAY_CHECK_ERR;
       }
       
    }
   
    /* clear the stack, which is just an address holder */

    memset((char*) stack, 0, MAX_INDIRECTION*sizeof(char*));

    for(i=0; i < existing_array_size; i++)
     {
     
         if(shSpptPointerDeRef(schemaElemInst+i * schemaElemPtr->size,
             schemaElemPtr->nstar, NULL, 0,0) != NULL) continue;
         block_num = 1;
         for(j=1; j < schemaElemPtr->nstar  ; j++)
         {
             if(entry >=0) block_num *= xptr[entry].num[j -1 + existing_dimen];
             stack[j] = (char*) shMalloc( block_num * sizeof(stack));

           /* note: this memory should get free-ed when the user frees 
              all the objects s/he gets from TBLCOL It is somewhat hidden 
              from view by user, though. This is the overhead of dynamic 
              memory -- you've got to bear the burdern.
           */
         }
         
        
        block_num = block_save;
        if(xptr[entry].dstDataType == DST_HEAP) continue;

        if((tmp_addr=shSpptCreateObjects(addr, &size, xptr[entry].dstDataType,
              child_schema, block_num, &has_new_memory))==NULL)
        {
             shErrStackPush("StateInit: Can't get memory");
             return SH_MALLOC_ERR;
        }

        if(schemaElemPtr->nstar == 1) stack[0] = tmp_addr[0];
        else  (void) memcpy((char*) stack[schemaElemPtr->nstar - 1], 
                (char*) tmp_addr, size*sizeof(char*));

       /* 
          now form the hierachy of pointers. This step is crucial
          when field has multiple indirections (more than 1 stars).

        */

        block_num = 1;

        for(j=0; j < schemaElemPtr->nstar - 1; j++)
        {   
            if(j > 0 && entry >=0) block_num *= xptr[entry].num[j -1 + existing_dimen];
            for(k=0; k < block_num; k++)
            {
              if(j==0 ) genericPtrPtr = (char**) &stack[j];
              else  genericPtrPtr =(char**)(stack[j] + k*sizeof(stack));
              *genericPtrPtr = stack[j+1];
              if(entry >= 0) *genericPtrPtr += k*xptr[entry].num[j + existing_dimen]*sizeof(stack); 
  
             /* if entry < 0, block_num will be one */
            }
        
        }

     /* now attach the memory to the given field */

      if(i > 0) schemaElemInst += schemaElemPtr->size;
      *(char**)schemaElemInst = stack[0];

      if (has_new_memory)  {
          /*
           * The individual items of tmp_addr cannot be freed here. They are
           * referenced by other pointers elsewhere.
           */
          shFree(tmp_addr); 

      }

      }

      return SH_SUCCESS;   

}


/* This routine deals with multiple indirection. It traverse
   across the indirections to return a pointer to the
   ultimate data. Dimension information num has to at least
   have MAX_INDIRECTION spaces. Shift is the total shift.
   Elem_size is the size in bytes for each unit shift.
  
   Private routine.
*/

      
static
char * shSpptPointerDeRef(char * ptr, int n, int* num, int shift, int elem_size)
{
      char* tmp=ptr;
      int i=0;
      int block=1, mod =0;

      if(n < 0) {
         shErrStackPush("\nNegative indirection number");
         return NULL;
      }

      if(shift!=0 && elem_size <=0) {
          shErrStackPush("\nSize info unknown when dereferencing pointers\n");
          return NULL;
      }

      if(n == 0) return tmp;
      if(*(char**) tmp!= NULL) tmp = *(char**) tmp;
      else return NULL;
      if(n==1) return tmp + shift*elem_size;
      
      if(num!=NULL) 
         while(i < MAX_INDIRECTION && num[i] > 0) block *= num[i++];

      for(i=1; i < n; i++)   
      {  
         if(num!=NULL && shift > 0) 
         {
           if(num[i-1]!=0) 
           {
             /* get the shift in each dimension */

               mod = (shift - mod*block)/(block/num[i-1]);
               block /= num[i-1];
              
           }
           else
           {
               shErrStackPush("\nBad dimension information when "\
                     "deref-ing pointer");
               return NULL;
            }
          }

         if(*(char**) (tmp + mod*elem_size) != NULL) 
         {
            tmp = *(char**) (tmp + mod*elem_size);
         } 

         else  return NULL;
      }
      if(num==NULL) tmp += shift*elem_size;
      return tmp;
}



/* get the maximum string length of enum member */


int p_shSpptGetMaxStrlenOfEnum(char *enumName, int* number)
{
    const SCHEMA* schema;
    int i;
    int max_len=0;
    int tmp_len=0;

    schema=shSchemaGet(enumName);
    if(schema==NULL || schema->type!=ENUM)
    {
       shErrStackPush("Can't get enum type by \"%s\"\n", enumName);
       return -1;
    }
    if(number!=NULL) *number = schema->nelem;
    for(i=0; i < schema->nelem; i++)
    {
       if((tmp_len=strlen(schema->elems[i].name)) > max_len)
          max_len = tmp_len;
     }

    return max_len;
} 
   

/* determine if a string consists of digits */

static unsigned short 
shSpptTermIsNumber(char* str, int n)
{
    char * tmpPtr = str, *start, *end;
    int str_end;
   
    if(str==NULL || n < 0) return FALSE;
    str_end = (n == 0 || n > strlen(str) ) ? strlen(str) : n;

    /* strip off trailing and leading white spaces */

    start = str;
    while(isspace(*start)) start++;
    end = str + str_end -1;
    while(isspace(*end)) end--;
    if((long) end < (long) start) return FALSE;
    tmpPtr = start;
    /* 
      while(isdigit(*tmpPtr) && (long) tmpPtr <= (long) end ) tmpPtr++;
      if((long)tmpPtr > (long)end) return TRUE;
      return FALSE;
     */
   /* if first char is digit then return true */
    if(*tmpPtr == '.') tmpPtr++;
    if(isdigit(*tmpPtr)) return TRUE;
    return FALSE;
}
    

/* Extract an arithmetic term for use in p_shSpptHeapDimenGet */

static
unsigned int shSpptTermExtract(
          char* in_str, 
          char** mark, 
          char* out_str,
          unsigned short * is_number_ptr
 ) 
{
     char internal[MAX_NAME_LEN];
     char *tmpPtr = NULL, *tmpPtr_save = NULL;
     unsigned short is_number = FALSE;

     if(in_str == NULL || strlen(in_str) < 1
         || out_str == NULL) return FALSE;

     tmpPtr=tmpPtr_save = strcpy(internal, in_str);
     while(1)
     {
        tmpPtr = strstr(tmpPtr, "x");
        if(tmpPtr==NULL)
        {
           strcpy(out_str, internal);
           if(is_number_ptr!=NULL) *is_number_ptr = shSpptTermIsNumber(internal, 0);
           if(mark!=NULL) *mark = NULL;
           return TRUE;
         }

         /* case "x" does exists. Have to determine if the char before
        "    x" is a space char or the term is a number */
 
         is_number = shSpptTermIsNumber(internal, 
             (int)((long) tmpPtr - (long) tmpPtr_save));

         if(isspace(*(tmpPtr -1)) || is_number )
         {  
             strcpy(out_str, internal);
             if(is_number_ptr!=NULL) 
              *is_number_ptr = is_number;
             if(mark!=NULL) *mark = tmpPtr + 1;
             return TRUE;
          }

        /* if the char before x is not a space 
            we have to continue to search. Skip the x */

         tmpPtr++;
    }
     
}
     
/* extract number information from heap specification */ 

int p_shSpptHeapDimenGet(SCHEMA* schema,char* inst, char* str, 
     int* num, unsigned get_total_length)
{

    char tmp_str[MAX_NAME_LEN], *tmpPtr=NULL;
    char term[MAX_NAME_LEN], *termPtr=NULL;
    char *addr = NULL;
    double tmp_num[MAX_INDIRECTION]; /* heap dimension should be integral
                                        but result of evaluation string
                                        may be double. Declare it to be
                                        double for generality. */
    int i, dimen, type;
    double total_length=1.0;
    int rvalue;

    char* usePtr;
    unsigned short is_number = FALSE;
    unsigned short expect_string = FALSE;

    if(str==NULL) return 0;
    tmpPtr=strcpy(tmp_str, str);
    if(strchr(tmpPtr, '+')!=NULL ||
          strchr(tmpPtr, '/')!=NULL)
    {
        shErrStackPush("Non-supported arithmetic operation exists: %s",
                 str);
        return -1;
    }
    while(tmpPtr=strchr(tmpPtr, '-'), tmpPtr++!=NULL)
    {
       if(*tmpPtr!='>')
       {
          shErrStackPush("Subtraction is not supported: %s", str);
          return -1;
       }
    }
       
    tmpPtr = tmp_str;
    
    memset((char*)tmp_num, 0, sizeof(tmp_num));
    i=0;
    while(shSpptTermExtract(tmpPtr, &tmpPtr, term, &is_number)) 
    {
       /* strip off leading and trailing white spaces */

       termPtr = term + strlen(term) -1;
       while(isspace(*termPtr)) *termPtr--='\0';
       termPtr = term;
       while(isspace(*termPtr)) termPtr++;
       if(i >= MAX_INDIRECTION)
       {
            shErrStackPush("Maxium %d terms on one line exceeded",i);
            return -1;
       }
       if(is_number) 
       {
          tmp_num[i] = atof(termPtr);
          i++;
          continue;
       }
       /* check if termPtr now is comprised of strlen() */

       if(strncmp(termPtr, "strlen",6)==0)
       {
           /* usePtr is used as a tmp storage here */

           usePtr = termPtr;
           termPtr += 6;
           /* shExprEval doesn't the remaining (), do something here */

           while(isspace(*termPtr)) termPtr++;
           if(*termPtr!='(') termPtr = usePtr;
           else
           {
               expect_string = TRUE;
               termPtr += 1;
               *(termPtr + strlen(termPtr) -1) = '\0';
            }
           
       }

       if(shExprEval(schema, termPtr, inst, &addr, &type, &usePtr)!=SH_SUCCESS) return -1;
       strcpy(term, shNameGetFromType(type));
       if(expect_string)
       {
           /* I'm relying on the user to make sure the type is indeed char* or
              unsigned char*. Otherwise I'd have to check for number of indirections
              but at this level, it is difficult to do so           */

          if(strcmp(term, "CHAR")!=0 && 
               strcmp(term, "UCHAR")!=0 && strcmp(term, "STR")!=0 )
           {
              shErrStackPush("Heap size error: strlen() used on non-string field \"%s\"",
                                  termPtr);
               return -1;
           }
#if 0
           if(*(char**)addr == NULL)
           {
               shErrStackPush("Heap size error: \"%s\" point to NULL",
                                  termPtr);
               return 0;
           }

           tmp_num[i] = (double)(strlen(*(char**)addr) + 1);  /* extra space for '\0' */
#endif
           tmp_num[i] = (double)((*(char**)addr==NULL)?1:strlen(*(char**)addr)+1);
       }

       else if(strcmp(term, "ULONG")==0)
       {
           tmp_num[i] =  (int) *(unsigned long*) addr;
       }
       else if(strcmp(term, "LONG")==0)
       {
           tmp_num[i] =  (int) *(long*) addr;
       } 
       else if (strcmp(term, "INT")==0)
       {
           tmp_num[i] =  *(int*) addr;
       }
       else if( strcmp(term, "UINT")==0)
       {
           tmp_num[i]  = (int) *(unsigned int *) addr; 
       }
       else if(strcmp(term, "SHORT")==0)
       {
           tmp_num[i] =  (int) *(short*) addr;
       }
       else if(strcmp(term, "USHORT")==0)
       {   
           tmp_num[i] = (int) *(unsigned short *) addr; 
       }
       else 
       { 
            shErrStackPush("Heap size evaluation: %s gives a %s, not an integer.",
                 str, term);
            return -1;
       }
       i++;
    }

    dimen = i;
    if(get_total_length)
    {
      for(i=0; i < dimen; i++) 
      {
         total_length  *= tmp_num[i];
         if(num!=NULL) num[i] = (int)tmp_num[i];
      }
      rvalue = (int)total_length;
      return rvalue;
    }

    if(num!=NULL)
      for(i=0; i < MAX_INDIRECTION; i++) num[i] = (int)tmp_num[i];
      
   /*  if(num!=NULL) memcpy((void*) num, (void*) tmp_num, sizeof(tmp_num)); */
    return dimen;
}

/* p_shHSppteapFldInit is used in shCnvTblToSchema only. It
   initialize a heap pointer regardless of how
   many indirections this pointer may have. All
   indirections are properly taken care of.
 */

int p_shSpptHeapFldInit(SCHEMA* rootSchema,
      char* inst, 
      SCHEMA_ELEM * elemptr,
      char** addr, 
      SCHEMATRANS* xtbl_ptr, 
      int entry,
      int heap_size_in_bytes
)
{
    int i, j, k, nstar, dimen, array_index =0, static_dimen = 0, static_size;
    int num[2*MAX_INDIRECTION];  
    int length_fits, length_xtbl=1, heaplength, unit_size=0, block=1, block_save=1;
    int array_dimen = 0;
    DSTTYPE type;
    char* stack[MAX_INDIRECTION], *tmpPtr, **tmpPtrPtr = NULL;
    SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;


    if(*addr != NULL ) return elemptr->nstar;  /* already initialized */
    if(rootSchema==NULL || elemptr==NULL || 
        addr==NULL || xptr==NULL || entry < 0 
        || heap_size_in_bytes <= 0) return -1;
    nstar = elemptr->nstar;
    p_shSpptArrayCheck(elemptr->nelem, NULL, &static_size, &static_dimen," ");
    if(xptr[entry].dstDataType!=DST_HEAP || nstar < 1)
    {
       shErrStackPush("p_shSpptHeapFldInit: Can't initialize non-heap field");
       return -1;
    }
    
    if((dimen = p_shSpptHeapDimenGet(rootSchema, inst, xptr[entry].size, 
          (int*) &num[static_dimen], FALSE)) < 0)
     {
         shErrStackPush("Bad heap dimension info: entry %d.\n", entry);
         return -1;
     }

    if(dimen != nstar -1) 
    {
       shErrStackPush("Heap dimension info incompatible "\
            "with indirection: entry %d", entry);
       return -1;
    }

    type=shSchemaTransDataTypeGet(xptr[entry].heaptype, &unit_size);

    /* push the array info from xptr[entry].num into num */

    for(i=0; i < static_dimen; i++) num[i] = xptr[entry].num[i];


    p_shSpptArrayCheck(xptr[entry].dst, num, &array_index, &array_dimen,NULL);

    block = 1;
    for(i=static_dimen + dimen -1; i >= static_dimen; i--) 
    {
       if(num[i] <= 0) 
       {
           shErrStackPush("Heap size info gives %d blocks: entry %d.\n", num[i],entry);
           return -1;
       }
       block *= num[i];
    }

    /* heaplength is the length of fastest-changing dimension! It should
       go into dimen, but for historical reasons stays separate. */

     
    memset((void*) stack, 0, sizeof(stack));
    
    if(nstar == 1) /* Well, I really like this case */
    {
       /* this case requires dimen = 0 */

       heaplength = heap_size_in_bytes/unit_size/static_size;

       for(i=0; i < static_size; i++)
       {
           if((tmpPtr = (char*) shMalloc(block*unit_size*heaplength))==NULL)
            {
                shErrStackPush("p_shHSppteapFldInit: memory alloc error\n");
                return -1;
            }
            memset(tmpPtr, 0, block*unit_size*heaplength);
           *(addr + i*sizeof(char*)) = tmpPtr;
       }
       return 1;
    }

     heaplength = p_shSpptHeapDimenGet(rootSchema, inst, xptr[entry].heaplen, 
         NULL, TRUE);

     if(heaplength < 0)
     {
         shErrStackPush("Bad heap length info: entry %d.\n", entry);
         return -1;
     }

     if(heaplength > 0) length_xtbl *= heaplength;

     if(array_dimen > dimen + static_dimen)
     {
        shErrStackPush("Illegal array specification for heap: entry %d", entry);
        return -1;
     }

  /*  for(i= static_dimen + dimen - array_dimen -1; i >= 0; i--) length_xtbl *= num[i]; */

    for(i= static_dimen + dimen -1; i >= array_dimen; i--) length_xtbl *= num[i];
    if(type==DST_STRUCT||type==DST_ENUM || 
       type==DST_UNKNOWN || type == DST_HEAP)
    {
       shErrStackPush("Non-supported heap type %s at entry %d",
                  xptr[entry].heaptype, entry);
       return -1;
    }

     /* this is total number of blocks available from FITS */

    length_fits=heap_size_in_bytes/unit_size;
   
    if(heaplength == 0) 
    {
        heaplength = length_fits / length_xtbl;
        if( length_fits%length_xtbl != 0)
        {
           shErrStackPush("Total heap length from FITS is %d, not"\
               "divisible by %d from dimension info", length_fits,
                length_xtbl);
           return -1;
        }
       length_xtbl = length_fits;
    }

    if(length_xtbl!=length_fits)
    {
       /* trouble is here! */

       shErrStackPush("Incompatible heap size: FITS has %d units while "\
               "table specifies %d units at entry %d", length_fits,
               length_xtbl, entry);
       return -1;
    }

   
   block_save = block;
   for(j =0; j< static_size; j++)
   {  
      if((tmpPtr = (char*) shMalloc(block_save*unit_size*heaplength))==NULL)
      {
          shErrStackPush("p_shHSppteapFldInit: memory alloc error\n");
          return -1;
      }
      memset(tmpPtr, 0, block_save*unit_size*heaplength);
      for(i=0; i < nstar -1; i++)
      {  
         if((stack[i] = (char*)shMalloc(num[i+static_dimen]*sizeof(char*)))==NULL)
         {
           shErrStackPush("p_shHSppteapFldInit: memory alloc error\n");
           return -1;
          }
          memset(stack[i], 0, num[i+static_dimen]*sizeof(char*));
       }

       /*form hierachy */
   
      block = block_save;
    
      for(i=0; i < nstar -1; i++)
      {
          block /= num[i+static_dimen];
          for(k=0; k < num[i+static_dimen]; k++) 
          {
              tmpPtrPtr= (char**) (stack[i] + k*sizeof(char*));
            /*  *tmpPtrPtr = tmpPtr + (j* block_save +k) *heaplength*unit_size ; */

              if(i!=nstar-2) *tmpPtrPtr = stack[i+1]+(i*block + k)*sizeof(char*);
              else *tmpPtrPtr = tmpPtr + k*heaplength*unit_size;
          }
       }

      *(char**)(addr + j ) = stack[0];
    }
    return nstar;
}
          
  

char* p_shSpptHeapBlockGet(char* inst,
        int nstar,
        int existing_dimen,
        int dst_dimen, 
        int total_dimen,
        int *num,
        int dst_array_index,
        int desired_block)
       
{
      char* root = NULL;
      int lnstar = nstar, block=1,  block_remain=1;
      int    static_index, static_units =1, i,  tmp_int, total_index, added_index;

      if(inst == NULL || nstar <= 0) return NULL;

      /* calculate the part that belongs to existing_dimen */

      total_index = dst_array_index + desired_block;
      for(i=0; i < total_dimen; i++)
      {
          block *= num[i];
          if(i >=dst_dimen) block_remain *= num[i];
          if(i >= existing_dimen) static_units *= num[i];
      }
      i=existing_dimen;
      static_index = 0;
      while(i < total_dimen )
      {
         if(total_index >= static_units ) 
         {
            tmp_int = total_index / static_units;
            total_index -= tmp_int * static_units;
            if(i!=total_dimen -1) static_index += tmp_int*static_units/num[i+1];
            else static_index += tmp_int;
         }
         static_units /= num[i];
         i++;
      }
      
      added_index = total_index;
      root = inst + static_index * sizeof(char*);
      if(root == NULL || static_index < 0 || added_index < 0)
      {
          shErrStackPush("Can't extract heap address");
          return NULL;
      }

      /* OK, we've got a good address, go down further */

       return shSpptPointerDeRef(root, lnstar, &num[existing_dimen],
                  added_index, sizeof(char*));

}
      
  
   

/* given source data pointed to by src, this routine
   copies it to the memory pointed by dst and does
   other necessary operations, e.g, shifting, scaling,
   and translation. But first, let's define a private
   routine to deal with data type issues.
 */

/* for floating point numbers */

static double 
toDoubleNaNCheck(DSTTYPE type, void* pointer, int index, int scalevalue, double scale, double zero)
{
 /* changing float to double without losing precision */

   double rvalue = 0.;

   if(type == DST_DOUBLE) rvalue = *((double*)pointer + index);

   if(type == DST_DOUBLE && shNaNDoubleTest(rvalue))
     return shNaNDouble();
   if(type == DST_DOUBLE && shDenormDoubleTest(rvalue) ) 
     return 0.0;

   rvalue = ((scalevalue == 1) ? 1: scale)*rvalue+zero; 
   return rvalue;
}

static float 
toFloatNaNCheck(DSTTYPE type, void* pointer, int index, int scalevalue, double scale, double zero)
{
 /* changing float to double without losing precision */

   float rvalue = 0.;

   if(type == DST_FLOAT) rvalue = *((float*) pointer + index);

   if( shNaNFloatTest(rvalue) )
     return shNaNFloat();
   if( shDenormFloatTest(rvalue) ) 
     return 0.0;

   rvalue = ((scalevalue == 1) ? 1: scale)*rvalue+zero; 
   return rvalue;
}

/* for integral types */

static unsigned long
toULong(DSTTYPE type, void* pointer, int index)
{
   unsigned long rvalue = 0;

/* In the following macro the break in NOT followed by a ';' on
   purpose! The ';' has to be put after the call to the macro.
   If both are present Irix Compiler will complain about 
   redondant statement and say 'control flow cannot reach this statement'
*/

#define CASE(label, type) case label: \
      rvalue = (unsigned long) (*((type *) pointer + index)); \
      break

/* We create this second macro to deal with unsigned number.
   Irix warned us that we has unreacheable code in this case. */

#define CASE_U(label,type) case label: \
      rvalue = (unsigned long) (*((type *) pointer + index)); \
      break

   switch(type)
   {
        CASE  (DST_SCHAR,   signed char    );
        CASE_U(DST_UCHAR,   unsigned char  );
        CASE  (DST_CHAR,    char           );
        CASE_U(DST_USHORT,  unsigned short );
	CASE  (DST_SHORT,   short 	   );
        CASE  (DST_INT,     int		   );
        CASE_U(DST_UINT,    unsigned int   );
        CASE  (DST_LONG,    long	   );
        CASE_U(DST_ULONG,   unsigned long  );
        case DST_UNKNOWN:
              shFatal("toULong: This should never occur! type == DST_UNKNOWN");
	      break;
        default:
              shFatal("toULong: This should never occur! unexpected type %d",
									 type);
	      break;
   }
   return rvalue;
}


int p_shSpptCastAndConvert(
   char* src, 
   DSTTYPE srcDataType,
   char* dst, 
   DSTTYPE dstDataType, 
   int array_size,      /* length of the dst side */
   double scale,   	/* scale factor, applied before zero*/
   double zero    	/* zero translation, applied after scale */
)
{
   short* spt = (short *) dst; 
   int* ipt   = (int *)   dst; 
   long* lpt  = (long *)  dst;
   char* chpt = (char *)  dst;
   unsigned short *uspt = (unsigned short *) dst;
   unsigned int   *uipt = (unsigned int   *) dst;
   unsigned long  *ulpt = (unsigned long  *) dst;
   unsigned char  *uchpt= (unsigned char  *) dst;
   double* dpt= (double*) dst; 
   float *fpt = (float *) dst;

   int i=0,j=0;
   int scalevalue = (int)scale;

  /* when scale is 1.0, scale value should be 1 , a case when don't want to
    scale at all */
 
   char type[MAX_NAME_LEN];
   char member[MAX_NAME_LEN];
   char *charPtr=NULL;
   const SCHEMA_ELEM* schema_elem=NULL;

   unsigned src_is_unsigned = FALSE;
   unsigned src_is_float    = FALSE;
   unsigned src_is_double   = FALSE;


   /* if destination is a string, source can only be char, uchar or string */

   if(dstDataType == DST_STR)
   {
       switch (srcDataType)
       {
           case DST_STR:
                  if(array_size > 1) 
                  {
                     /* strip off trailing spaces in src */
                     i=strlen(src)-1;
                     while(isspace(*(src + i)) && i >=0 ) {*(src+i)='\0'; i--;}
                     strncpy(dst, src, array_size -1);
                     i=strlen(src);
                     dst[(i > array_size-1) ? array_size -1: i]='\0'; 
                  }

                  /* single character treated as string, extremely rare */

                 else  *dst = *(char*)src;
                 break;

           case DST_CHAR:
           case DST_SCHAR:
           case DST_UCHAR:

                /* copy exact array-size chars into the dst and null terminate the dst */
                /* caller of this routine has made sure array_size is the minimum of   */
                /* destination and source. Direct copying is then meaningful.          */
                  
                 strncpy(dst, src, array_size);
                 dst[array_size] = '\0'; /* be safe */
                 break;
          default:
                  shErrStackPush("Can't convert non-string, non-char types to string");
                  return -1;
        }
      
        return 0;
   }
  
   /* I only allow dstDataType to be string, which already is dealt with above,
       char, uchar or enum, if srcDataType ==  DST_STR */

  if(srcDataType==DST_STR)
   {
       switch(dstDataType)
       {
           case DST_ENUM:  /* no op */ break;
           case DST_CHAR:
           case DST_SCHAR:
           case DST_UCHAR:
                 
                 /* caller of this routine has made sure array_size is the minimum of   */
                /* destination and source. Direct copying is then meaningful.          */

                 strncpy(dst, src, array_size);
                 return 0;

           default:
                  shErrStackPush("Can't convert string to types "\
                    "other than string, char, or unsigned char");
                  return -1;
        }

    }

  if(dstDataType==DST_ENUM)
  {
     /* Idea is that enum is a string of two parts separated by a tab,
        first part being the type name, and second part the value.
        array_size is useless here. Assume srcPtr points to the
        top of the string for this row. Clearly, the first part
        which we have to write for each row, is an overhead. */
    
    for(i=0; i < array_size; i++)
    {
        j=0;
        while(*(src+j)!='\t') {j++; if(j > MAX_ENUM_LEN) break;}
        if(j==0 || j >MAX_ENUM_LEN)
        {
            shErrStackPush("Can't read the enum type.\n");
            return -1;
        }
        strncpy(type, src, j);
        type[j++]='\0';
        charPtr = type + strlen(type)-1;
        while(isspace(*charPtr)) *charPtr--='\0';
        while(*(src+j)!='\t') {j++; if(j - strlen(type) > MAX_ENUM_LEN) break;}
        if(j==strlen(type)+1 || j -strlen(type) > MAX_ENUM_LEN)
        {
            shErrStackPush("Can't read the enum value.\n");
            return -1;
        }
        strncpy(member, src + strlen(type)+1, j - strlen(type)-1);
        member[j - strlen(type)-1]='\0';
        charPtr = member + strlen(member) -1;
        while(isspace(*charPtr)) *charPtr--='\0';
        schema_elem = shSchemaElemGet(type, member);
        if(schema_elem==NULL)
        {
           shErrStackPush("No such member in %s: %s\n", type, member);
           return -1;
        }
      
        *ipt = schema_elem->value;
        src +=  MAX_ENUM_LEN;
     }
     return 0;

  }

  /*  Integral values in C are inter-castable. Trouble comes when types are
   *  floating point numbers, which are not castable. I'd have to do
   *  assignment by value.
   */
    
  src_is_unsigned = (srcDataType == DST_UCHAR || srcDataType ==DST_USHORT || 
                        srcDataType == DST_UINT || srcDataType == DST_ULONG );

  src_is_float = (srcDataType == DST_FLOAT);
  src_is_double = (srcDataType == DST_DOUBLE);

#define DATACONV(pt, cast_type) \
   pt += i;     /* initialize the pointer */ \
   if(src_is_float) \
      *pt = (cast_type)(toFloatNaNCheck(srcDataType,src,i,scalevalue,scale,zero)); \
   else if(src_is_double) \
      *pt = (cast_type)(toDoubleNaNCheck(srcDataType,src,i,scalevalue,scale,zero)); \
   else if(src_is_unsigned) \
	*pt = (cast_type) (((scalevalue==1)?1:scale)* toULong(srcDataType, src, i)+zero);\
   else *pt = (cast_type) (((scalevalue==1)?1:scale)*((long)toULong(srcDataType, src, i))+zero); \
   pt = (cast_type *) dst;  /* reset the pointer */ 

  for(i=0; i < array_size; i++)
   switch(dstDataType)
   { 
       case DST_UCHAR:   DATACONV(uchpt, unsigned char);break;
       case DST_SCHAR:
       case DST_CHAR:    DATACONV(chpt, char);	   	break;                 
       case DST_SHORT:   DATACONV(spt, short);   	break;
       case DST_USHORT:  DATACONV(uspt, unsigned short);break;
       case DST_INT:     DATACONV(ipt, int);		break;
       case DST_UINT:    DATACONV(uipt, unsigned int);  break;
       case DST_LONG:    DATACONV(lpt, long); 		break;
       case DST_ULONG:   DATACONV(ulpt, unsigned long); break;
       case DST_FLOAT:   DATACONV(fpt, float);		break;
       case DST_DOUBLE:  DATACONV(dpt, double); 	break;
       default:     shErrStackPush("can't do unknown types: dstDataType=%d", dstDataType);
                    return -1;
     }
    return 0;
  
}
 
/* this routine fetches a single element from
   a field that is any combination of array and
   pointers (memory allocated for the pointer).

   Input: 1) number of indirections ;2) absolute 
   array index (including the part that's in indirection)
   3) array dimension.

   Return: a pointer to the desired element.

   Example:  given  char  ***c[20][10][5] and the
   3 stars are mallocked as [4][3][2], we want to find
   the address to the element c[2][3][4][1][2]. This 
   routine will return the pointer to the element if:

      inst = address of c;
      nstar = 3;
      array_dimen = 3;
      total_dimen =5;
      num={20,10,5,4,3,2};
      shift=2x20+3x10+4x5+1x4+2x3;
      elem_size =sizeof(char);
*/

static
char* shSpptPointerArrayInstGet (
        char* inst,             /* instance pointer */
        int   nstar,            /* number of indirections */
        int   array_dimen,      /* array dimension -- static memory part */
        int   total_dimen,      /* total_dimension */
        int*  num,              /* array info, at least size MAX_INDIRECTION */
        int   shift,            /* total absolute shift */
        int   elem_size         /* size in bytes of unit shift */
)
{
    int existing_dimen, existing_index;
    int dst_array_index=0, dst_array_dimen;
    int i=0,block=1, tmp_int, lnstar=nstar;
    char* genericPtr;

   /* extracting the index that belongs to existing array */

    existing_dimen = array_dimen;
    dst_array_dimen=total_dimen;
    dst_array_index = shift;
    genericPtr = inst;
    if(inst==NULL) return NULL;
    existing_index=0;
    if(num!=NULL) 
    {  if(dst_array_dimen > existing_dimen) 
       {
          i=0;
          block =1;
          while(i < existing_dimen) block *= num[i++];
          if(block==0) 
          {
            shErrStackPush("Array information is not compatible\n"); 
            return NULL;
          }
          i=0;
          while(i < existing_dimen) 
          {   
             block /=  num[i];     /* block can't be zero */
             tmp_int = dst_array_index/block;   /* assignment gets rid of the mod */
             existing_index += tmp_int * num[i++];
          }
           
        }
       else  
       {  
         existing_index =dst_array_index;
       }
       dst_array_index -= existing_index;
       genericPtr += existing_index*elem_size;
       lnstar = nstar;
       if(dst_array_index==0) 
           genericPtr =shSpptPointerDeRef(genericPtr, lnstar, NULL,  0,  1);
       else genericPtr =shSpptPointerDeRef(genericPtr, lnstar, 
              &num[existing_dimen], dst_array_index, elem_size);
     }
    else genericPtr = shSpptPointerDeRef(genericPtr, nstar, NULL, shift, elem_size);
   return genericPtr;
}


/* upon entry, entry_fnd points to the non-continuation line */

SCHEMA_ELEM*
p_shSpptTransTblWalk(
       SCHEMATRANS* xtbl_ptr, const SCHEMA_ELEM* elemptr,
       SCHEMA_ELEM** tmp_elemptr, int* entry_fnd)
{
     
    int entry = *entry_fnd;
    SCHEMA_ELEM* sch_elem = (SCHEMA_ELEM* )elemptr;
    SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;
  
    while(xptr[entry+1].type==CONVERSION_CONTINUE)
    {
        entry++;
        sch_elem = shSpptNextEntrySchemaElem(xtbl_ptr,
                      sch_elem->type, entry);
        if(sch_elem==NULL)
        {
           shErrStackPush("EntryTraverse: at entry %d, %s is not a member of %s",
                 entry, xptr[entry].dst, sch_elem->type);
           return NULL;
        }
    }

    *tmp_elemptr = sch_elem;
    *entry_fnd = entry;
    return sch_elem;
}
    




/* p_shSpptTransTblTraverse(), as its name suggests, traverse
   a multi-line entry in a tranlation table to get to
   the pointer to the data field.
  
   Absolute requirement:  on entry "field" points to the top 
   if the field is an array. No indirection has taken. 
   I.e, nstar remains valid.

   Example: if the field to traverse is REGION* region[3];
   On entry, the argument "field" has &region[0].

 */


char*
p_shSpptTransTblTraverse(
    SCHEMATRANS* xtbl_ptr,            /* translation table */
    SCHEMA_ELEM * current_elem,       /* in: current, top level schema elem */
    SCHEMA_ELEM ** last,              /* out: schema elem at the bottom */
    char*   field,                    /* field instance address to start */
    int*    entry_start,              /* in: entry gives the start up entry number */
                                      /* out: entry gives the bottom continuation entry number */
    char**   upperPtr                 /* pointer to memory of next to last level */              
)
{
   const SCHEMA* tmp_schema =NULL;
   SCHEMA_ELEM *tmp_schema_elem = current_elem;
   SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;
   char*   genericPtr= field;
   int i;
   int dst_array_index = 0, dst_array_dimen=0;
   int existing_dimen =0;
   int entry = *entry_start;
   char tmp[MAX_NAME_LEN];

   if(xptr == NULL) {shErrStackPush("No translation table\n"); return NULL;}
   if(upperPtr!=NULL) *upperPtr = field;
   if(entry < 0 || xptr[entry+1].type != CONVERSION_CONTINUE) 
   {
     if(last!=NULL) *last = current_elem;
     return shSpptPointerDeRef(field, tmp_schema_elem->nstar, NULL, 0, 1);
   }

   genericPtr=field;

  /* takes care of indirect addressing and array element fetch */

   p_shSpptArrayCheck(xptr[entry].dst, 
          xptr[entry].num, &dst_array_index, &dst_array_dimen, NULL);
   dst_array_index--;
   p_shSpptArrayCheck(tmp_schema_elem->nelem, NULL, NULL, &existing_dimen, " ");
   genericPtr=shSpptPointerArrayInstGet(genericPtr, tmp_schema_elem->nstar, 
                 existing_dimen, dst_array_dimen, xptr[entry].num,
                    dst_array_index, tmp_schema_elem->size);


   if(genericPtr==NULL)
   {
      if(xptr[entry].dstDataType!=DST_HEAP)
          shErrStackPush("%s points to NULL\n", tmp_schema_elem->name);
      return NULL;
   }
   entry++;

   /* OK, deals with continuation line */
   
   while(xptr[entry].type == CONVERSION_CONTINUE)
   {

       tmp_schema = shSchemaGet(tmp_schema_elem->type);
       if(tmp_schema==NULL) /* undocumented types encountered */
       {
           shErrStackPush("Don't know what kind schema this is: %s\n", tmp_schema_elem->type);
           return NULL;
        }

      /* find the offset of xptr[entry].dst in the new schema */

       strcpy(tmp, xptr[entry].dst);
       for(i = 0;i < tmp_schema->nelem; i++) 
       {
           if(strcmp(strtok(tmp, "<["),tmp_schema->elems[i].name) == 0) break;
            
        }

       if(i >= tmp_schema->nelem) 
        {
              shErrStackPush("%s is not found in schema %s", 
                    xptr[entry].dst, tmp_schema->name);
              return NULL;
        }

       tmp_schema_elem = &tmp_schema->elems[i];

      /* takes care of indirection and array element fetch again */
     
       genericPtr = genericPtr + tmp_schema_elem->offset;
       p_shSpptArrayCheck(xptr[entry].dst, xptr[entry].num, &dst_array_index, &dst_array_dimen, NULL);
       dst_array_index--;
       p_shSpptArrayCheck(tmp_schema_elem->nelem, NULL, NULL, &existing_dimen, " ");
       if(upperPtr!=NULL) *upperPtr=genericPtr;
       genericPtr=shSpptPointerArrayInstGet(genericPtr, tmp_schema_elem->nstar, 
                       existing_dimen, dst_array_dimen, xptr[entry].num,
                             dst_array_index, tmp_schema_elem->size);

       /* only heap is initialized at conversion time */

       if(genericPtr==NULL && xptr[entry].dstDataType!=DST_HEAP)
       {
          shErrStackPush("%s points to NULL\n", tmp_schema_elem->name);
          return NULL;
        }

       entry++;
   }

   entry--;
   if(last!=NULL) *last = tmp_schema_elem;
   *entry_start =entry;  
   return genericPtr;
}


char* 
shSpptObjGetFromContainer(
          CONTAINER_TYPE type,
          void* container, 
          void** walkPtr,          /* for use with list */
          char* schemaName,
          int index
)
{
     char* obj = NULL;
     ARRAY* array= (ARRAY*)container;
     CHAIN* chain = (CHAIN*)container;
    
     if(container == NULL || index < 0 ) return NULL;
     switch(type)
     {
        case ARRAY_TYPE:  

              if(index >= array->dim[0]) obj = NULL;
              else obj = *(char**)((char*)array->arrayPtr 
                         + index *sizeof(char*));
              break;

        case CHAIN_TYPE:   
                            

               obj = (char*) shChainElementGetByPos(chain, index);
               break;

        default:  obj = NULL;
     }

     return obj;
}
                                


/************************************************************************************/
/**** used by Tcl routines *******************************/

#if 0

RET_CODE shSpptTclObjectInit(
            Tcl_Interp*  a_interp, 
            SCHEMATRANS* xtbl_ptr,
            void* instPtr,
            SCHEMA* schemaPtr,
            unsigned short handleRetain
)
{

          
     int entry_nums, entry_stack[MAX_RUNTIME_ENTRIES]; 	
     int entry_save, current_entry, entry_fnd;		/* ints related to table entry */

     int dst_dimen, dst_array_index;			/* ints related to array */
     int existing_dimen, reqst_array_size, reqst_dimen;
     int req_dimen, req_array_size;

     const SCHEMA*      child_schema ;           	/* schema stuff */
     SCHEMA_ELEM* 	schemaElemPtr, *child_schema_elem;
     char*        	schemaElemInst ;

     char* parent_inst, *child_inst;			/* instances */

     HANDLE genericHdl;
     SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;

     int 	size = 0, i, j, k, l;
     int	fld_addr_size = MAX_ADDRESS_SIZE;	/* misc variables */
     int 	status;
     char**  	fldAddrPtr;
     char*	fldAddr[MAX_ADDRESS_SIZE];
     unsigned 	fld_addr_malloc = FALSE;
     char 	result[MAX_NAME_LEN];

     fldAddrPtr = fldAddr;
     for(i=0; i < schemaPtr->nelem; i++)
       {
           schemaElemPtr = &schemaPtr->elems[i];
           schemaElemInst = (char*) instPtr + schemaElemPtr->offset;
           entry_nums = 0;
           entry_fnd = p_shSpptEntryFind(xtbl_ptr, schemaElemPtr->name);  
           entry_stack[entry_nums++] = entry_fnd;
        
          /* Same field may have appear more than once in a table.
             Let's find all of them. Put their entry numbers in
             entry_stack.
           */
 
           while(entry_fnd >= 0)
           {
               entry_fnd = p_shSpptEntryFind(xtbl_ptr, schemaElemPtr->name); 
               if(entry_fnd >= 0) entry_stack[entry_nums++] = entry_fnd;
           }
   
          /* process all the entries of the same field */
 
          for(l=0; l < entry_nums; l++)
          {
           	size=0;
           	entry_fnd = entry_stack[l];
           	entry_save = entry_fnd;  /* useful in search on schemaTrans's continuation line */
           	reqst_array_size = 1;  
           	if(entry_fnd >= 0) 
           	{
                     /*field is found in the table, let's grab the array info
                            specified in the entry */
 
              	     p_shSpptArrayCheck(xptr[entry_fnd].dst, xptr[entry_fnd].num,
                           &dst_array_index,  &dst_dimen, NULL);
                     dst_array_index--;
 
                      /* we also grab the array info from the schema */
 
                     p_shSpptArrayCheck(schemaElemPtr->nelem, NULL, NULL,  
                      				&existing_dimen, " ");
 
                 /* if proc is given in the table, we'll call it with Tcl_Eval.
                    Then put the addresses into fldAddrPtr to pass to a C routine,
                    shSpptSchemaObjectStateCheck(). If no proc, the C routine
                    will call the default constructor given by the schema of
                    that field or manually allocate memory, in that order.
                  */ 
 
                   if(schemaElemPtr->nstar > 0 && *(char**) schemaElemInst==NULL
                        && xptr[entry_fnd].proc!=NULL) 
                   {
 
                	/* find how many times we'd call Tcl_Eval */
 
                 	p_shSpptArrayCheck(xptr[entry_fnd].size,NULL,
                           	&reqst_array_size,  &reqst_dimen, "x"); 
                 	if(reqst_array_size > fld_addr_size) 
                 	{
			        if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                    		fld_addr_size = reqst_array_size;
                    		if((fldAddrPtr=(char**)shMalloc(reqst_array_size*
					sizeof(char**)))==NULL)
                    		{   
                      		      shErrStackPush("Can't shMalloc mem for address holder"); 
                      		      return SH_GENERIC_ERROR;
                    		}
                    		fld_addr_malloc=TRUE;
                 	}
 
                 	for(j=0; j < reqst_array_size; j++)
                 	{
                      		if((status=Tcl_Eval(a_interp,
					xptr[entry_fnd].proc))
					== TCL_OK)
                      		{
                          		strcpy(result, a_interp->result);
                          		status=shTclHandleGetFromName(a_interp, 
							result, &genericHdl);
                          		fldAddrPtr[size] = (char*) genericHdl.ptr;
                          		size++;
                                  
                      		}
 
                     		if(status != TCL_OK ||genericHdl.ptr==NULL )
                     		{
				  if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                          		shErrStackPush("Cmd execution failure");
                          		return SH_GENERIC_ERROR;
                     		}

                     		/* delete the handle  */

                     		if(!handleRetain)
                     		{
                            		if(p_shTclHandleDel(a_interp, result) != TCL_OK) 
                           		{
					  if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                                		shErrStackPush("Handle delete failure");
                                		return SH_GENERIC_ERROR;
                           		}
                     		}
 
                 	}/*  for(j=0; j < reqst_array_size; j++) */
 
                    } /* if(... xptr[entry_fnd].proc!=NULL ...) */
 
	       }/* if(entry_fnd >= 0) */
 
 
            /* now initialize the state of this particular field. If
              no address is passed in from outside, shSpptSchemaObjectStateCheck
              will take other means to validate the memory. If 
              this field doesn't need additional memory, the routine
              will simply return SH_SUCCESS. 
 
              Note, StateCheck doesn't allocate heap memory. For tbl to schema,
              theres is no way to find out heap length before a table is read.
            */
 
 
           if(shSpptSchemaObjectStateCheck(xtbl_ptr, entry_fnd, schemaElemPtr,   
                         schemaElemInst, fldAddrPtr, size)!=SH_SUCCESS)
            {
	        if(fld_addr_malloc) shFree((char*) fldAddrPtr);
               	shErrStackPush("Field \"%s\" is uninitialized entry=%d\n", 
                              schemaElemPtr->name, entry_fnd);  
                return SH_GENERIC_ERROR;
               
            }
              
           if (entry_fnd < 0 || 
              xptr[entry_fnd+1].type!=CONVERSION_CONTINUE) continue;
 
           /* now deal with continuation line. Treat the current
              field as a parent and proceed on. Each level we go
              down, we StateCheck it. */
 
           
           parent_inst = shSpptPointerArrayInstGet(schemaElemInst, 
				schemaElemPtr->nstar, 
                         	existing_dimen, 
				dst_dimen,
                         	xptr[entry_fnd].num, 
				dst_array_index, 
				schemaElemPtr->size);
 
           if(parent_inst == NULL ) 
           {
	         if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                 shErrStackPush("shTclTblToSchema: %s of %s for entry %d is NULL",
                      schemaElemPtr->name, schemaElemPtr->type, entry_fnd);
                 return SH_GENERIC_ERROR;
           }
 
           current_entry = entry_save;
           child_schema = shSchemaGet(schemaElemPtr->type);
           entry_fnd++;
           while(xptr[entry_fnd].type==CONVERSION_CONTINUE)
           {
                  child_schema_elem = shSpptNextEntrySchemaElem(
					xtbl_ptr,
                            		(char*) child_schema->name,
					entry_fnd);

                  if(child_schema_elem==NULL) 
                  {
		      if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                      shErrStackPush("Bad field name at entry %d for schema %s\n", 
					entry_fnd, child_schema->name);
                      return SH_GENERIC_ERROR;
                  }

                  size = 0;
                  child_inst = parent_inst + child_schema_elem->offset;
                  
                  /* just like above, we call Tcl_Eval if neccessary. */
 
                  p_shSpptArrayCheck(xptr[entry_fnd].size, 
                               		NULL, 
					&req_array_size, 
					&req_dimen, "x");

                  if(reqst_array_size > fld_addr_size) 
                  {
                      if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                      fld_addr_size = reqst_array_size;
                      if((fldAddrPtr=(char**)shMalloc(
                                reqst_array_size*sizeof(char**)))==NULL)
                      {   
                              shErrStackPush("Can't shMalloc mem for address holder");
                              return SH_GENERIC_ERROR;
                      }
                      fld_addr_malloc=TRUE;
                  }

                  if(child_schema_elem->nstar > 0 && *(char**)child_inst==NULL
                        && xptr[entry_fnd].proc!=NULL)
                  {  
                       for(k=0; k < req_array_size; k++)
                       {
                          if((status=Tcl_Eval(a_interp,
				xptr[entry_fnd].procL)) == TCL_OK)
                          {
                             strcpy(result, a_interp->result);
                             status =shTclHandleGetFromName(a_interp, result,&genericHdl);
                             fldAddrPtr[size] = (char*) genericHdl.ptr;
                             size++;
                              
                          }
 
                          if(status != TCL_OK || genericHdl.ptr== NULL)
                          {
			      if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                              shErrStackPush("Cmd execution failure");
                              return SH_GENERIC_ERROR;
                              
                          }
                          /* delete the handle */
                          if(!handleRetain)
                          {
                             if(p_shTclHandleDel(a_interp, result) != TCL_OK) 
                             {
			        if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                                shErrStackPush("Handle delete failure");
                                return SH_GENERIC_ERROR;
                              
                             }
                         
                           }   
 
                        } /* for-loop over k */
 
                  } /* if nstar > 0 and child_inst != NULL and proc!= NULL */
               
                 /* StateCheck this field */
 
                  if(shSpptSchemaObjectStateCheck(xtbl_ptr, 
				entry_fnd, 
                     		child_schema_elem, 
				child_inst, 
				fldAddrPtr, 
				size)!=SH_SUCCESS) 
                  {
                      if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                      shErrStackPush("Field \"%s\" in \"%s\"is uninitialized\n",
 
                                 child_schema_elem->name, child_schema->name);
                      return SH_GENERIC_ERROR;
 
                  }
                  
                   /* check if given entry is specified as array and extract 
                        the array info */
 
                  p_shSpptArrayCheck(xptr[entry_fnd].dst, 
                      xptr[current_entry].num, &dst_array_index, &dst_dimen,NULL);
                  dst_array_index--;
 
                   /* check the schema for the field's array info */
 
                  p_shSpptArrayCheck(child_schema_elem->nelem, NULL, NULL, 
                         &existing_dimen, " ");
 
                  /* ready to go down another level */
 
                  parent_inst = shSpptPointerArrayInstGet(child_inst, 
					child_schema_elem->nstar,
                            		existing_dimen, 
					dst_dimen,  
                            		xptr[current_entry].num, 
					dst_array_index, 
					child_schema_elem->size);

                  child_schema = shSchemaGet(child_schema_elem->type);
                  if(child_schema == NULL && 
                     xptr[entry_fnd].dstDataType==DST_STRUCT)
                  {  
		        if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                        shErrStackPush("Undefined schema encountered");
                        return SH_GENERIC_ERROR;
                  }
                  entry_fnd++;
          
          }/* while (xptr[entry_fnd++].type!= CONVERSION_CONTINUE) */
 
       }  /* for-loop over l */
 
     } /* for(i=0; i < schemaPtr->nelem; i++) */
 
 
    /* EntryFind marked the entry status as visited. 
       Restore it here. */
 
     shSchemaTransStatusInit(xtbl_ptr);  
     if(fld_addr_malloc) shFree((char*) fldAddrPtr);

     return SH_SUCCESS;
 
} 


#endif

/***********************************************************************/
/*** new definition! *************************************************/

RET_CODE shSpptTclObjectInit(
            Tcl_Interp*  a_interp, 
            SCHEMATRANS* xtbl_ptr,
            void** objPtrs,
            int    objCnt,
            SCHEMA* schemaPtr,
            unsigned short handleRetain
)
{


#define MAX_OBJCNT        1000
    
     int entry_nums, entry_stack[MAX_RUNTIME_ENTRIES]; 	
     int entry_save, current_entry, entry_fnd;		/* ints related to table entry */

     int dst_dimen, dst_array_index;			/* ints related to array */
     int existing_dimen, reqst_array_size, reqst_dimen;
     int req_dimen, req_array_size;

     const SCHEMA*      child_schema ;           	/* schema stuff */
     SCHEMA_ELEM* 	schemaElemPtr, *child_schema_elem;
     char*        	schemaElemInst ;

     char*   parent[MAX_OBJCNT], *child[MAX_OBJCNT], *child_inst;	/* instances */
     char**  parentPtr, **childPtr;
     unsigned par_chld_malloc = FALSE;
     int      max_obj_cnt = MAX_OBJCNT;

     char** 	fldAddrPtr;

     /* Make the following static because if it is not, it will cause dervish
	running on OSF1	to core dump when this routine is called.  This will
	happen with any	routine that declares a variable greater than 2M. */
     static char*	fldAddr[MAX_OBJCNT][MAX_ADDRESS_SIZE];

     unsigned 	fld_addr_malloc = FALSE;
     int	fld_addr_size = MAX_ADDRESS_SIZE;
     
     HANDLE genericHdl;
     SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;

     int 	size = 0, i, j, k, l, objIndx;             /* misc variables */
     int 	status;
     char 	result[MAX_NAME_LEN];




     fldAddrPtr = (char**)fldAddr;
     parentPtr  = parent;
     childPtr   = child;

     /* determine if our MAX_OBJCNT is big enough */

     if(objCnt > max_obj_cnt)
     {
           par_chld_malloc = TRUE;
           fld_addr_malloc = TRUE;
            
           fldAddrPtr = (char**) shMalloc(objCnt * MAX_ADDRESS_SIZE *sizeof(char**));
           parentPtr  = (char**) shMalloc(2*objCnt*sizeof(char*));
           childPtr = (char**)((char*) parentPtr + objCnt);
           max_obj_cnt = objCnt;
     }

     for(i=0; i < schemaPtr->nelem; i++)
       {
           size = 0;
           schemaElemPtr = &schemaPtr->elems[i];
           schemaElemInst = (char*) objPtrs[0] + schemaElemPtr->offset;  
           entry_nums = 0;
           entry_fnd = p_shSpptEntryFind(xtbl_ptr, schemaElemPtr->name);  
           entry_stack[entry_nums++] = entry_fnd;
        
          /* Same field may have appear more than once in a table.
             Let's find all of them. Put their entry numbers in
             entry_stack.
           */
 
           while(entry_fnd >= 0)
           {
               entry_fnd = p_shSpptEntryFind(xtbl_ptr, schemaElemPtr->name); 
               if(entry_fnd >= 0) entry_stack[entry_nums++] = entry_fnd;
           }
   
          /* process all the entries of the same field */
 
          for(l=0; l < entry_nums; l++)
          {
           	entry_fnd = entry_stack[l];
           	entry_save = entry_fnd;  /* useful in search on schemaTrans' continuation line */
           	reqst_array_size = 1;  
           	if(entry_fnd >= 0) 
           	{
                     /*field is found in the table, let's grab the array info
                            specified in the entry */
 
              	     p_shSpptArrayCheck(xptr[entry_fnd].dst, xptr[entry_fnd].num,
                           &dst_array_index,  &dst_dimen, NULL);
                     dst_array_index--;
 
                      /* we also grab the array info from the schema */
 
                     p_shSpptArrayCheck(schemaElemPtr->nelem, NULL, NULL,  
                      				&existing_dimen, " ");
 
                 /* if proc is given in the table, we'll call it with Tcl_Eval.
                    Then put the addresses into fldAddrPtr to pass to a C routine,
                    shSpptSchemaObjectStateCheck(). If no proc, the C routine
                    will call the default constructor given by the schema of
                    that field or manually allocate memory, in that order.
                  */ 
 
                   if(schemaElemPtr->nstar > 0 && *(char**) schemaElemInst==NULL
                        && xptr[entry_fnd].proc!=NULL) 
                   {
 
                	/* find how many times we'd call Tcl_Eval */
 
                 	p_shSpptArrayCheck(xptr[entry_fnd].size,NULL,
                           	&reqst_array_size,  &reqst_dimen, "x"); 
                 	if(reqst_array_size > fld_addr_size) 
                 	{
                    		if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                    		fld_addr_size = reqst_array_size;
                    		fld_addr_malloc=TRUE;
                    		fldAddrPtr = (char**) shMalloc(reqst_array_size*max_obj_cnt*
					sizeof(char**));
                 	}
                        for(objIndx = 0; objIndx < objCnt; objIndx++)
                        {
                           size=0;
                 	   for(j=0; j < reqst_array_size; j++)
                 	   {
                      		if((status=Tcl_Eval(a_interp,
					xptr[entry_fnd].proc))
					== TCL_OK)
                      		{
                          		strcpy(result, a_interp->result);
                          		status=shTclHandleGetFromName(a_interp, 
							result, &genericHdl);
                          		*(char**)((char*)fldAddrPtr+(objIndx*reqst_array_size + size)*sizeof(char*))
                                                =  (char*) genericHdl.ptr;
                          		size++;
                                  
                      		}
 
                     		if(status != TCL_OK ||genericHdl.ptr==NULL )
                     		{
                          		shErrStackPush("Cmd execution failure");
                          		return SH_GENERIC_ERROR;
                     		}

                     		/* delete the handle  */

                     		if(!handleRetain)
                     		{
                            		if(p_shTclHandleDel(a_interp, result) != TCL_OK) 
                           		{
                                		shErrStackPush("Handle delete failure");
                                		return SH_GENERIC_ERROR;
                           		}
                     		}
 
                 	     }/*  for(j=0; j < reqst_array_size; j++) */

                        } /* for(objIndx = 0; objIndx < objCnt; objIndx++) */
 
                    } /* if(... xptr[entry_fnd].proc!=NULL) */
 
	       }/* if(entry_fnd >= 0) */
 
 
            /* now initialize the state of this particular field. If
              no address is passed in from outside, shSpptSchemaObjectStateCheck
              will take other means to validate the memory. If 
              this field doesn't need additional memory, the routine
              will simply return SH_SUCCESS. 
 
              Note, StateCheck doesn't allocate heap memory. For tbl to schema,
              theres is no way to find out heap length before a table is read.
            */
 
           for(objIndx = 0; objIndx < objCnt; objIndx++)
           {  
               schemaElemInst = (char*) objPtrs[objIndx] + schemaElemPtr->offset;
               if(shSpptSchemaObjectStateCheck(xtbl_ptr, entry_fnd, schemaElemPtr, schemaElemInst, 
                    (char**)((char*)fldAddrPtr+objIndx*reqst_array_size*sizeof(char*)), size)!=SH_SUCCESS)
               {
               	   shErrStackPush("Field \"%s\" is uninitialized entry=%d", 
                                     schemaElemPtr->name, entry_fnd);  
                    return SH_GENERIC_ERROR;
               
               }
           }
              
           if (entry_fnd < 0 || xptr[entry_fnd+1].type!=CONVERSION_CONTINUE) continue;
 
           /* now deal with continuation line. Treat the current
              field as a parent and proceed on. Each level we go
              down, we StateCheck it.                          */
 
           for(objIndx = 0; objIndx < objCnt; objIndx++)
           { 
               schemaElemInst = (char*) objPtrs[objIndx] + schemaElemPtr->offset;
               parentPtr[objIndx] = shSpptPointerArrayInstGet(schemaElemInst, 
				schemaElemPtr->nstar, 
                         	existing_dimen, 
				dst_dimen,
                         	xptr[entry_fnd].num, 
				dst_array_index, 
				schemaElemPtr->size);
 
              if(parentPtr[objIndx] == NULL ) 
              {
                    shErrStackPush("shTclObjStateInit: %s of %s for entry %d is NULL",
                         schemaElemPtr->name, schemaElemPtr->type, entry_fnd);
                    return SH_GENERIC_ERROR;
               }
           }

           current_entry = entry_save;
           child_schema = shSchemaGet(schemaElemPtr->type);
           entry_fnd++;
           while(xptr[entry_fnd].type==CONVERSION_CONTINUE)
           {
                  child_schema_elem = shSpptNextEntrySchemaElem(
					xtbl_ptr,
                            		(char*) child_schema->name,
					entry_fnd);

                  if(child_schema_elem==NULL) 
                  {
                      shErrStackPush("Bad field name at entry %d for schema %s\n", 
					entry_fnd, child_schema->name);
                      return SH_GENERIC_ERROR;
                  }

                  size = 0;
                  child_inst = (char*) parentPtr[0] + child_schema_elem->offset;
                  
                  /* just like above, we call Tcl_Eval if neccessary. */
 
                  p_shSpptArrayCheck(xptr[entry_fnd].size, 
                               		NULL, 
					&req_array_size, 
					&req_dimen, "x");

                  if(reqst_array_size > fld_addr_size) 
                  {
                      if(fld_addr_malloc) shFree((char*) fldAddrPtr);
                      fld_addr_size = reqst_array_size;
                      fld_addr_malloc=TRUE;
                      fldAddrPtr = (char**) 
                          shMalloc(max_obj_cnt*reqst_array_size*sizeof(char**));
                  }

                  if(child_schema_elem->nstar > 0 && *(char**)child_inst==NULL
                        && xptr[entry_fnd].proc!=NULL)
                  {  

                    for(objIndx = 0; objIndx < objCnt; objIndx++)
                    {
                       size = 0;

                       for(k=0; k < req_array_size; k++)
                       {
                          if((status=Tcl_Eval(a_interp,
				xptr[entry_fnd].proc)) == TCL_OK)
                          {
                             strcpy(result, a_interp->result);
                             status =shTclHandleGetFromName(a_interp, result,&genericHdl);
                             *(char**)((char*)fldAddrPtr+(objIndx*reqst_array_size + size)*sizeof(char*))
                                      = (char*) genericHdl.ptr;
                             size++;
                              
                          }
 
                          if(status != TCL_OK || genericHdl.ptr== NULL)
                          {
                              shErrStackPush("Cmd execution failure");
                              return SH_GENERIC_ERROR;
                              
                          }
                          /* delete the handle */
                          if(!handleRetain)
                          {
                             if(p_shTclHandleDel(a_interp, result) != TCL_OK) 
                             {
                                shErrStackPush("Handle delete failure");
                                return SH_GENERIC_ERROR;
                              
                             }
                         
                           }   
 
                        } /* for-loop over k */

                     } /* for(objIndx = 0; objIndx < objCnt; objIndx++) */

                  } /* if nstar > 0 and child_inst != NULL and proc!= NULL */
              
                  /* check if given entry is specified as array and extract 
                        the array info */
 
                  p_shSpptArrayCheck(xptr[entry_fnd].dst, 
                      xptr[current_entry].num, &dst_array_index, &dst_dimen,NULL);
                  dst_array_index--;
 
                   /* check the schema for the field's array info */
 
                  p_shSpptArrayCheck(child_schema_elem->nelem, NULL, NULL, 
                         &existing_dimen, " ");
               
                 /* StateCheck this field */
                
                  for(objIndx = 0; objIndx < objCnt; objIndx++)
                    {   
                       child_inst = (char*) parentPtr[objIndx] + child_schema_elem->offset;
                       if(shSpptSchemaObjectStateCheck(xtbl_ptr, 
				entry_fnd, 
                     		child_schema_elem,
				child_inst, 
				(char**)((char*)fldAddrPtr+objIndx*reqst_array_size*sizeof(char*)), 
				size)!=SH_SUCCESS) 
                       {
                           shErrStackPush("Field \"%s\" in \"%s\"is uninitialized",
                                 child_schema_elem->name, child_schema->name);
                           return SH_GENERIC_ERROR;
 
                       }
                       parentPtr[objIndx] = shSpptPointerArrayInstGet(child_inst, 
					child_schema_elem->nstar,
                            		existing_dimen, 
					dst_dimen,  
                            		xptr[current_entry].num, 
					dst_array_index, 
					child_schema_elem->size);
                   }

                   
                  /* ready to go down another level */
 
                  child_schema = shSchemaGet(child_schema_elem->type);
                  if(child_schema == NULL && 
                     xptr[entry_fnd].dstDataType==DST_STRUCT)
                  {  
                        shErrStackPush("Undefined schema encountered");
                        return SH_GENERIC_ERROR;
                  }
                  entry_fnd++;
          
          }/* while (xptr[entry_fnd++].type!= CONVERSION_CONTINUE) */
 
       }  /* for-loop over l */
 
     } /* for(i=0; i < schemaPtr->nelem; i++) */
 
 
    /* EntryFind marked the entry status as visited. 
       Restore it here. */
 
     shSchemaTransStatusInit(xtbl_ptr);  
     if(fld_addr_malloc) shFree((void*) fldAddrPtr);
     if(par_chld_malloc) shFree((void*) parentPtr);

     return SH_SUCCESS;
 
} 




RET_CODE shSpptObjectStateInit(
            SCHEMATRANS* xtbl_ptr,
            void** objPtrs,
            int    objCnt,
            SCHEMA* schemaPtr
)
{


#define MAX_OBJCNT        1000
    
     int entry_nums, entry_stack[MAX_RUNTIME_ENTRIES]; 	
     int entry_save, current_entry, entry_fnd;		/* ints related to table entry */

     int dst_dimen, dst_array_index;			/* ints related to array */
     int existing_dimen, reqst_array_size;
     int req_dimen, req_array_size;

     const SCHEMA*      child_schema ;           	/* schema stuff */
     SCHEMA_ELEM* 	schemaElemPtr, *child_schema_elem;
     char*        	schemaElemInst ;

     char*   parent[MAX_OBJCNT], *child[MAX_OBJCNT], *child_inst;	/* instances */
     char**  parentPtr, **childPtr;
     unsigned par_chld_malloc = FALSE;
     int      max_obj_cnt = MAX_OBJCNT;

     char** 	fldAddrPtr;

     /* Make the following static because if it is not, it will cause dervish
	running on OSF1	to core dump when this routine is called.  This will
	happen with any	routine that declares a variable greater than 2M. */
     static char*	fldAddr[MAX_OBJCNT][MAX_ADDRESS_SIZE];

     unsigned 	fld_addr_malloc = FALSE;
     
     SCHEMATRANS_ENTRY* xptr = xtbl_ptr->entryPtr;

     int 	size = 0, i, l, objIndx;             /* misc variables */

     fldAddrPtr = (char**)fldAddr;
     parentPtr  = parent;
     childPtr   = child;

     /* determine if our MAX_OBJCNT is big enough */

     if(objCnt > max_obj_cnt)
     {
           par_chld_malloc = TRUE;
           fld_addr_malloc = TRUE;
            
           fldAddrPtr = (char**) shMalloc(objCnt * MAX_ADDRESS_SIZE *sizeof(char**));
           parentPtr  = (char**) shMalloc(2*objCnt*sizeof(char*));
           childPtr = (char**)((char*) parentPtr + objCnt);
           max_obj_cnt = objCnt;
     }

     for(i=0; i < schemaPtr->nelem; i++)
       {
           size = 0;
           schemaElemPtr = &schemaPtr->elems[i];
           schemaElemInst = (char*) objPtrs[0] + schemaElemPtr->offset;  
           entry_nums = 0;
           entry_fnd = p_shSpptEntryFind(xtbl_ptr, schemaElemPtr->name);  
           entry_stack[entry_nums++] = entry_fnd;
        
          /* Same field may have appear more than once in a table.
             Let's find all of them. Put their entry numbers in
             entry_stack.
           */
 
           while(entry_fnd >= 0)
           {
               entry_fnd = p_shSpptEntryFind(xtbl_ptr, schemaElemPtr->name); 
               if(entry_fnd >= 0) entry_stack[entry_nums++] = entry_fnd;
           }
   
          /* process all the entries of the same field */
 
          for(l=0; l < entry_nums; l++)
          {
           	entry_fnd = entry_stack[l];
           	entry_save = entry_fnd;  /* useful in search on schemaTrans' continuation line */
           	reqst_array_size = 1;  
           	if(entry_fnd >= 0) 
           	{
                     /*field is found in the table, let's grab the array info
                            specified in the entry */
 
              	     p_shSpptArrayCheck(xptr[entry_fnd].dst, xptr[entry_fnd].num,
                           &dst_array_index,  &dst_dimen, NULL);
                     dst_array_index--;
 
                      /* we also grab the array info from the schema */
 
                     p_shSpptArrayCheck(schemaElemPtr->nelem, NULL, NULL,  
                      				&existing_dimen, " ");
 
 
	       }/* if(entry_fnd >= 0) */
 
 
            /* now initialize the state of this particular field. If
              no address is passed in from outside, shSpptSchemaObjectStateCheck
              will take other means to validate the memory. If 
              this field doesn't need additional memory, the routine
              will simply return SH_SUCCESS. 
 
              Note, StateCheck doesn't allocate heap memory. For tbl to schema,
              theres is no way to find out heap length before a table is read.
            */
 
           for(objIndx = 0; objIndx < objCnt; objIndx++)
           {  
               schemaElemInst = (char*) objPtrs[objIndx] + schemaElemPtr->offset;
               if(shSpptSchemaObjectStateCheck(xtbl_ptr, entry_fnd, schemaElemPtr, schemaElemInst, 
                    NULL, 0)!=SH_SUCCESS)
               {
               	   shErrStackPush("Field \"%s\" is uninitialized entry=%d", 
                                     schemaElemPtr->name, entry_fnd);  
                    return SH_GENERIC_ERROR;
               
               }
           }
              
           if (entry_fnd < 0 || xptr[entry_fnd+1].type!=CONVERSION_CONTINUE) continue;
 
           /* now deal with continuation line. Treat the current
              field as a parent and proceed on. Each level we go
              down, we StateCheck it.                          */
 
           for(objIndx = 0; objIndx < objCnt; objIndx++)
           { 
               schemaElemInst = (char*) objPtrs[objIndx] + schemaElemPtr->offset;
               parentPtr[objIndx] = shSpptPointerArrayInstGet(schemaElemInst, 
				schemaElemPtr->nstar, 
                         	existing_dimen, 
				dst_dimen,
                         	xptr[entry_fnd].num, 
				dst_array_index, 
				schemaElemPtr->size);
 
              if(parentPtr[objIndx] == NULL ) 
              {
                    shErrStackPush("shTclObjStateInit: %s of %s for entry %d is NULL",
                         schemaElemPtr->name, schemaElemPtr->type, entry_fnd);
                    return SH_GENERIC_ERROR;
               }
           }

           current_entry = entry_save;
           child_schema = shSchemaGet(schemaElemPtr->type);
           entry_fnd++;
           while(xptr[entry_fnd].type==CONVERSION_CONTINUE)
           {
                  child_schema_elem = shSpptNextEntrySchemaElem(
					xtbl_ptr,
                            		(char*) child_schema->name,
					entry_fnd);

                  if(child_schema_elem==NULL) 
                  {
                      shErrStackPush("Bad field name at entry %d for schema %s\n", 
					entry_fnd, child_schema->name);
                      return SH_GENERIC_ERROR;
                  }

                  size = 0;
                  child_inst = (char*) parentPtr[0] + child_schema_elem->offset;
                  
                  /* just like above, we call Tcl_Eval if neccessary. */
 
                  p_shSpptArrayCheck(xptr[entry_fnd].size, 
                               		NULL, 
					&req_array_size, 
					&req_dimen, "x");

                  /* check if given entry is specified as array and extract 
                        the array info */
 
                  p_shSpptArrayCheck(xptr[entry_fnd].dst, 
                      xptr[current_entry].num, &dst_array_index, &dst_dimen,NULL);
                  dst_array_index--;
 
                   /* check the schema for the field's array info */
 
                  p_shSpptArrayCheck(child_schema_elem->nelem, NULL, NULL, 
                         &existing_dimen, " ");
               
                 /* StateCheck this field */
                
                  for(objIndx = 0; objIndx < objCnt; objIndx++)
                    {   
                       child_inst = (char*) parentPtr[objIndx] + child_schema_elem->offset;
                       if(shSpptSchemaObjectStateCheck(xtbl_ptr, 
				entry_fnd, 
                     		child_schema_elem, 
				child_inst, 
				NULL, 
				0)!=SH_SUCCESS) 
                       {
                           shErrStackPush("Field \"%s\" in \"%s\"is uninitialized",
                                 child_schema_elem->name, child_schema->name);
                           return SH_GENERIC_ERROR;
 
                       }
                       parentPtr[objIndx] = shSpptPointerArrayInstGet(child_inst, 
					child_schema_elem->nstar,
                            		existing_dimen, 
					dst_dimen,  
                            		xptr[current_entry].num, 
					dst_array_index, 
					child_schema_elem->size);
                   }

                   
                  /* ready to go down another level */
 
                  child_schema = shSchemaGet(child_schema_elem->type);
                  if(child_schema == NULL && 
                     xptr[entry_fnd].dstDataType==DST_STRUCT)
                  {  
                        shErrStackPush("Undefined schema encountered");
                        return SH_GENERIC_ERROR;
                  }
                  entry_fnd++;
          
          }/* while (xptr[entry_fnd++].type!= CONVERSION_CONTINUE) */
 
       }  /* for-loop over l */
 
     } /* for(i=0; i < schemaPtr->nelem; i++) */
 
 
    /* EntryFind marked the entry status as visited. 
       Restore it here. */
 
     shSchemaTransStatusInit(xtbl_ptr);  
     if(fld_addr_malloc) shFree((void*) fldAddrPtr);
     if(par_chld_malloc) shFree((void*) parentPtr);

     return SH_SUCCESS;
 
} 

/*****************************************************************************
******************************************************************************
**
** FILE:
**      errStack.c
**
** ABSTRACT:
**      This file contains the Error Stack routines. These routines make up
** 	the public interface to Error Stack class. The Error Stack class is
**      a simple error handling facility. Users can push errors encountered
**    	during code development on the stack and review them at a later 
**	time. This package maintains a circular stack which retains MAX_DEPTH
**	most recently added error strings. In order to display the full
**	list, a user must first call the method shErrStackGetEarliest() to
**	set the stack pointer to the most recently pushed error message. Then
**      the iterator shErrStackGetNext() must be called to fetch each item 
**      from the stack, fetching a NULL at the end of the stack. Since this 
**      is a circular stack, the (earliest) oldest error string is deleted 
**      to make space for the newest one. This information is captured in 
**      shErrStackOverflow(), which returns the number of all error strings 
**      deleted to make space for new ones.
**
** ENTRY POINT          SCOPE   DESCRIPTION
** -------------------------------------------------------------------------
** shErrStackClear        public  Nuke the error stack and reset pointers
** shErrStackPush         public  Pushes error strings on the stack
** shErrStackGetNext      public  Get next error string from the stack
** shErrStackGetEarliest  public  Get the most recently pushed error message
** shErrStackOverflow     public  Did we experience a stack overflow?
**
** ENVIRONMENT:
**      ANSI C.
**
** REQUIRED PRODUCTS:
**	None
**
** AUTHORS:
**      Creation date:  May 11, 1993
**      Vijay Gurbani
**
******************************************************************************
******************************************************************************
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define   ERRSTACK_IMP
#include "shCErrStack.h"
#include "prvt/errStackP.h"
#undef    ERRSTACK_IMP

static int g_position = 0;
static int m_returnedNullMessage = 0;
static int stackDepth = MAX_DEPTH - 1;

void shErrStackClear(void)
{
   memset((char *)errStack.stack, 0, sizeof(errStack.stack));

   errStack.numItems = 0;
   g_position = 0;
   m_returnedNullMessage = 0;
   stackDepth = MAX_DEPTH - 1;
   errStack.flags &= ~shErrStack_ClearOnPush;	/* Don't waste time letting   */
						/* ... another clear occur.   */
}

void shErrStackPush (const char *a_format, ...)
{
   char buf[BUFSIZ];
   int  insertPosition;
   va_list args;

   /*
    * Should the stack be cleared (first push)?
    */

   if ((errStack.flags & shErrStack_ClearOnPush) != 0)
      {
      shErrStackClear ();
      }

   errStack.flags &= ~shErrStack_ClearOnPush;	/* Already cleared out        */

   /*
    * Maintain a LIFO order. Newly added items are pushed to the bottom of
    * the stack. If the stack is full, get rid of the oldest error message 
    * (which is at the bottom) and shift the stack down one slot, thus 
    * making room at the top for the latest error message.
    */
   if (errStack.numItems >= MAX_DEPTH)  {
       int i;

       /* 
        * Stack is full; shift it down. Get rid of oldest error message
        */
       for (i = MAX_DEPTH - 1; i >= 1; i--)
            strcpy(errStack.stack[i], errStack.stack[i - 1]);

       insertPosition = 0;
   }
   else
       insertPosition = stackDepth--;

   va_start(args, a_format);
   vsprintf(buf, a_format, args);
   va_end(args);

   /*
    * Make sure that buf fits within errStack.stack[errStack.insertPosition]
    */
   if (strlen(buf) < MAX_COLS)  {
       strcpy(errStack.stack[insertPosition], buf);
       strcat(errStack.stack[insertPosition], "\n");
   }
   else
       sprintf(errStack.stack[insertPosition], "%-*.*s\n", MAX_COLS-3,
               MAX_COLS-3,  buf);

   errStack.numItems++;
}

void
shErrStackPushPerror(const char *format, ...)
{
   char buf[BUFSIZ];
   int oerrno = errno;
   va_list args;

   va_start(args, format);
   vsprintf(buf, format, args);
   va_end(args);

   shErrStackPush("%s:\n\t%s",buf,strerror(oerrno));
}

const char *shErrStackGetNext(void)
{
   if (errStack.numItems == 0)
      return NULL;                            /* no messages exist */

   if (++g_position == MAX_DEPTH) {           /* do not walk off end */
      if(errStack.numItems >= MAX_DEPTH) { /* we started at g_position == 0 */
	 g_position = -1;		/* it'll be ++'d to 0 next time */
	 return(NULL);
      } else {
	 g_position = 0;
      }
   }

   if (errStack.stack[g_position][0] != 0)    /* Found a message */
     {
     m_returnedNullMessage = 0;
     return errStack.stack[g_position];
     }
   else
     {
     if (m_returnedNullMessage == 0)          /* Return only 1st NULL msg */
       {
       ++m_returnedNullMessage;
       return NULL;
       }
     else
       return(shErrStackGetNext());           /* Get next non-NULL message */
     }

}

const char *shErrStackGetEarliest(void)
{
   if (errStack.numItems == 0)
       return NULL;

   g_position = (errStack.numItems > MAX_DEPTH ? 0 : 
                                                 MAX_DEPTH-errStack.numItems);
   m_returnedNullMessage = 0;
   return errStack.stack[g_position];
}

int shErrStackOverflow(void)
{
   int timesOverflow,      /* How many times did we overflow? */
       numItemsOverflow;   /* By how many items did we overflow? */

   timesOverflow = numItemsOverflow = 0;

   if (errStack.numItems <= MAX_DEPTH)
       return 0;

   timesOverflow = errStack.numItems / MAX_DEPTH;
   numItemsOverflow = ((timesOverflow - 1) * MAX_DEPTH) + 
                       (errStack.numItems % MAX_DEPTH);

   return numItemsOverflow;
}

/*****************************************************************************/
/*
 * Print the Error Stack
 */
void
shErrStackPrint(FILE *fil)
{
   int nlost;				/* no. of messages lost to overflow */
   int i;
   int last_pos;			/* position of last message */

   if(errStack.numItems == 0) {
      return;
   }

   last_pos = (errStack.numItems > MAX_DEPTH ? 0 : 
                                                 MAX_DEPTH-errStack.numItems);

   if((nlost = shErrStackOverflow()) > 0) {
      fprintf(fil,"\t(%d messages lost)\n",nlost);
   }
/*
 * find the earliest message
 */
   for(i = last_pos;i < MAX_DEPTH;i++) {
      if(*errStack.stack[i] == '\0') {
	 i++;				/* it'll be --'ed */
	 break;
      }
   }
/*
 * and print them
 */
   while(--i >= last_pos) {
      fprintf(fil,"%s",errStack.stack[i]);
   }
}



#ifdef TEST_MODE

int main(int argc, char *argv[])
{
   const char *sp;
   int   overFlow;

   shErrStackPush("This is error message 1");
   shErrStackPush("%s", "This is error message 2");
   shErrStackPush("%s %d", "This is error message", 3);
   shErrStackPush("This is error message %d", 4);
   shErrStackPush("%s %s %d", "This is error", "message", 5);
   shErrStackPush("This is error message 6");
   shErrStackPush("This is error message 7");
   shErrStackPush("This is error message 8");
   shErrStackPush("This is error message 9");
   shErrStackPush("This is error message 10");
   shErrStackPush("This is error message 11. This is a very long %s %s",
                  "message. It is more then 132 characters. But it should be",
                  "chopped to 132 characters. Buffer flow should not occur!!");
   shErrStackPush("This is error message 12");
   shErrStackPush("This is error message 13");
   shErrStackPush("This is error message 14");

   for (sp = shErrStackGetEarliest(); sp != NULL; sp = shErrStackGetNext())
        printf("--> %s", sp);
     

   if ((overFlow = shErrStackOverflow()))
       printf("Error stack overflowed by %d items\n", overFlow);
   else
       printf("No stack overflow occurred\n");
   
   shErrStackClear();

   sp = shErrStackGetEarliest();
   if (sp == NULL)
       fprintf(stdout, "1. No error messages\n");
   else
       fprintf(stdout, "1. %s: fatal situation, sp = %s", *argv, sp);

   sp = shErrStackGetNext();    
   if (sp == NULL)
       fprintf(stdout, "2. No error messages\n");
   else
       fprintf(stdout, "2. %s: fatal situation, sp = %s", *argv, sp);

   return 0;
}

#endif

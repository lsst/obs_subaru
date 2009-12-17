/*
 utilsF77Bind.c -- Bindings for f77 callbacks to the dervish kernel.
 Minimal runtime support for f77. Only intended to be called by 
 f77, so no public headers for these routines.
*/

#include "shCUtils.h"

void dervish_fatal_ (char *msg)
{
   shFatal ("%s",msg);
}

void dervish_error_ (char *msg)
{
   shError ("%s",msg);
}

void dervish_debug_ (int *level, char *msg)
{
   shDebug (*level, "%s",msg);
}


#ifndef _SHC_H
#define _SHC_H

/*****************************************************************************
******************************************************************************
**
** FILE:
**      shC.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the C routines used by dervish
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: May 07, 1993
**
******************************************************************************
******************************************************************************
*/

/*----------------------------------------------------------------------------
**
** DEFINITIONS
*/

/*----------------------------------------------------------------------------
**
** STRUCTURE DEFINITIONS
*/

#ifdef linux
#  define PTR_FMT "0x%lx"
#elif defined(DARWIN)
#  define PTR_FMT "%p"
#else
#  define PTR_FMT "0x%p"
#endif 

#endif


#ifndef SHTK_H
#define SHTK_H

/*
 * All dervish TK prototypes
 */
/*****************************************************************************
******************************************************************************
**
** FILE:
**      shTk.h
**
** ABSTRACT:
**      This file contains all necessary definitions, macros, etc., for
**      the routines to implement dervish Tk functions.
**
** ENVIRONMENT:
**      ANSI C.
**
** AUTHOR:
**      Creation date: Sept 07, 1993
**
******************************************************************************
******************************************************************************
*/

/*----------------------------------------------------------------------------
**
** prototypes
*/

#ifdef __cplusplus
extern "C"
{
#endif  /* ifdef cpluplus */

void shTk_MainLoop(CMD_HANDLE *);
void p_shTk_CreateFileHandlerOnly(void);
void p_shTk_DeleteFileHandler(void);

#ifdef __cplusplus
}
#endif  /* ifdef cpluplus */

#endif

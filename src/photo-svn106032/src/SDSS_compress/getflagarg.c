/* Copyright (c) 1993 Association of Universities for Research 
 * in Astronomy. All rights reserved. Produced under National   
 * Aeronautics and Space Administration Contract No. NAS5-26555.
 */
/* EXTRACT BINARY FLAG PARAMETERS FROM COMMAND LINE ARGUMENTS */

#include <string.h>

/* returns 1 if flag is present, else 0 */

extern int
getflagarg(argc,argv,flag)
int argc; 
char *argv[];
char flag[];								/* flag (e.g. c for "-c")		*/
{
int arg;
char *p;

	for (arg = 1; arg<argc; arg++) {
		/*
		 * look for -flag
		 */
		p = argv[arg];
		if (*p++ == '-' && (strcmp(p,flag) == 0)) return(1);
	}
	return(0);
}

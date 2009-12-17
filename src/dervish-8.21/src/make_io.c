/*
 * This file reads a header file, and writes the code to dump data objects
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "shCUtils.h"
#include "region.h"
#include "shCDiskio.h"

#define LSIZE 500			/* size of input line */
#define NARG 20				/* max number of args to a function */
#define NELEM 500			/* max number of elements in a struct*/
#define NTYPE 500			/* max number of typedefs */

/*
 * #define, enum, function pointer or struct?
 */
typedef enum { IS_DEFINE, IS_ENUM, IS_FUNC, IS_STRUCT } ENUMORSTRUCT;

typedef struct {
   char name[SIZE];			/* name of member */
   char type[SIZE];			/* type of member */
   int nstar;				/* number of indirections */
   char *nelem;				/* number of elements if an array */
} MEMBER;				/* members of a type */

typedef enum { READ, WRITE } FUNC;	/* desired operation */

enum {
   MKIO_AUTO = 01,			/* generate automatic dump code */
   MKIO_CONSTRUCTOR = 02,		/* assume vanilla typeNew/typeDel */
   MKIO_IGNORE = 04,			/* ignore this type entirely */
   MKIO_SCHEMA = 010,			/* only schema for this type */
   MKIO_USER = 020,			/* user will provide dump functions */
   MKIO_LOCK = 040,			/* No over-writing of this schema allowed */
   MKIO_FUNCTION = 0100		/* a function pointer */
};					/* what is desired for a type */

static char funcname[SIZE];		/* name of function being generated */
static FILE *infil;			/* input file stream */
static char *input_file;		/* current input file */
static char line[LSIZE];		/* input line */
static int lineno;			/* line number in file */
static char module_prefix[3];		/* prefix for functions, e.g. "sh" */
static int nstruct = 0;			/* number of structs we've found */
static int ntype = 0;			/* number of types we've found */

static struct {
   char file[SIZE];			/* file where type is defined */
   int lineno;				/* line number */
   ENUMORSTRUCT type;			/* struct or enum? */
   char name[SIZE];			/* name of type */
   int nelem;				/* number of elements */
   int desires;				/* what code is required for type */
   MEMBER *elems;			/* list of members */
   char constructor[SIZE];		/* name of constructor */
   char destructor[SIZE];		/* name of destructor */
   char reader[SIZE];			/* name of read dump function */
   char writer[SIZE];			/* name of write dump function */
   char lock;
} types[NTYPE + 1];			/* 1 extra for TYPE */

static int verbose = 0;			/* should I be chatty? */
static long cur_pos = 0;                /* current position in infil , WP 04/10/94 */

static char *canonical(char *name);
static void do_define(void);
static void do_enum(void);
static int do_generic(char *);
static void do_struct(void);
static char *find_name(char *ptr, char *delim, char *delim_char);
static char *find_string(char *ptr, 
         char *delim,   char* ignore_str, char *delim_char,
         unsigned short initialize_static ); /* WP, 04/10/94 */
static void free_tokens(char **tokens);
static char *lcase(char *name);
static int process_pragmas(char *lptr);
static char *save_str(char *str, int n);
static char *skip_comment(char *lptr, char **comment);
static char **tokenise(void);
static void usage(void);
static void write_file_header(FILE *fil, int ac, char **av);
static void write_member_schema(char *name, MEMBER *elems, int nelem, int num,
		FILE *fil, ENUMORSTRUCT parent /* WP, 04/10/94 */);
static void write_schema(FILE *fil, char *module_name);

int
main(int ac,char **av)
{
   int i;
   char *lptr;				/* pointer to line */
   char module_name[SIZE];		/* name of this module; used to name
					   the function to initialise schema */
   FILE *outfil;			/* output file stream */

   *module_name = '\0';
   *module_prefix = '\0';
   while(ac > 1 && av[1][0] == '-') {
      switch (av[1][1]) {
       case 'm':			/* module name */
	 if(av++,--ac <= 1) {
	    fprintf(stderr,"You must specify a module name with -m\n");
	    continue;
	 }
	 strcpy(module_name,canonical(av[1]));
	 break;
       case 'p':			/* module prefix */
	 if(av++,--ac <= 1) {
	    fprintf(stderr,"You must specify a module prefix with -p\n");
	    continue;
	 }
	 if(strlen(av[1]) > 2) {
	    fprintf(stderr,
		    "Prefix names should be two or less chars; truncating\n");
	 }
	 strncpy(module_prefix,av[1],2);
	 module_prefix[2] = '\0';
	 break;
       case 'v':			/* set verbosity */
	 if(av[1][2] == '\0') {
	    if(--ac <= 1 || !isdigit(*(++av)[1])) {
	       fprintf(stderr,"You must specify a number with -v\n");
	       continue;
	    }
	    verbose = atoi(av[1]);
	 } else {
	    verbose = atoi(&av[1][2]);
	 }
	 break;
       default:
	 fprintf(stderr,"Unknown flag: %s\n",av[0]);
	 break;
      }
      ac--; av++;
   }
   if(ac < 3 || *module_name == '\0') {
      usage();
      exit(1);
   }
   if(*module_prefix == '\0') {
      strncpy(module_prefix,lcase(module_name),2);
      module_prefix[2] = '\0';
   }   

   if(strcmp(av[1],"-") == 0) {
      outfil = stdout;
   } else if((outfil = fopen(av[1],"w")) == NULL) {
      fprintf(stderr,"Can't open %s\n",av[1]);
      exit(1);
   }
   ac--; av++;

   write_file_header(outfil,ac,av);
/*
 * Now read the files and generate code
 */
   for(i = 1;i < ac;i++) {
      input_file = av[i];
      if((infil = fopen(input_file,"r")) == NULL) {
	 fprintf(stderr,"Can't open %s\n",input_file);
	 exit(1);
      }
      lineno = 0;
      
       /* remember the current position before typedef */
      while(lineno++,cur_pos=ftell(infil),(lptr = fgets(line,LSIZE,infil)) != NULL) {
	 while(isspace(*lptr)) lptr++;
	 if(strncmp(lptr,"typedef",7) == 0) {
	    lptr += 7;
	    while(isspace(*lptr)) lptr++;
	    if(strncmp(lptr,"enum",4) == 0 && isspace(lptr[4])) {
	       do_enum();
	    } else if(strncmp(lptr,"struct",6) == 0 && isspace(lptr[6])) {
	       do_struct();
	    } else {
	       if(do_generic(lptr) < 0) { /* processed OK */
		  if(verbose) {
		     fprintf(stderr,"Ignoring typedef: %s\n",lptr);
		  }
	       }
	    }
	 } else if(strncmp(lptr,"#define",7) == 0 && isspace(lptr[7])) {
	    do_define();
	 }
      }
      fclose(infil);
   }

   write_schema(outfil,module_name);
   
   return(0);
}

/*****************************************************************************/
/*
 * Print information about make_io
 */
static void
usage(void)
{
   fprintf(stderr,"%s%s\n","\
Usage: make_io options -m name outfile headerfiles\n\
<name> is the name used to refer to this set of schema,\n\
<outfile> is the name of the generated C file,\n\
<headerfiles> are one or more .h files containing type definitions.\n","\
Options are:\n\
	-p prefix	A 2-character function prefix. This defaults to the\n\
			first 2 characters of <name>\n\
	-v#		Set the verbosity to #");
}

/*****************************************************************************/
#if 0					/* comment has embedded star-slash */
 *
 * Process a #define "typedef"
 *
 * Not all #defines are associated with TYPES; the ones that are must be of
 * the form:
 *
#define NORMAL_GALAXY 0x00001   /* pragma typedef { */
#define RED_GALAXY    0x00002
#define QSO           0x00004
#define HI_Z_QSO      0x00008   /* pragma } GALTYPE */
 *
 * In other words, the defined value must have no embedded spaces, and the
 * pragma comments are important. There must be no close-brace before the
 * one in the second pragma comment
 *
 * Let us start by looking for the "pragma typedef {" string
 *
#endif

static void
do_define(void)
{
   char *comment;			/* contents of comment */
   char *cptr;				/* pointer to comment */
   char *lptr = line;			/* pointer to line */
   char *name;				/* name of "type" */
   int nelem =0;			/* number of elements in the "type" */
   MEMBER elems[NELEM];                 /* tmp MEMBER structures */
   int lineno_save;                     /* to save lineno */
   
   lptr += 7;				/* skip "#define" */
   while(isspace(*lptr)) lptr++;
   while(*lptr != '\0' && !isspace(*lptr)) lptr++; /* skip "NORMAL_GALAXY" */
   while(isspace(*lptr)) lptr++;
   while(*lptr != '\0' && !isspace(*lptr)) lptr++; /* skip "0x0001" */
   while(isspace(*lptr)) lptr++;

   if(strncmp(lptr,"/*",2) != 0) {
      return;
   }

   lptr = skip_comment(lptr,&comment);
   if((cptr = strchr(comment,'{')) != NULL) {
      while(--cptr >= comment && isspace(*cptr)) continue;
      cptr++;			/* move to first whitespace char */
      cptr -= strlen("pragma typedef");
      if(strncmp(cptr,"pragma typedef",14) != 0) {
	 cptr = NULL;			/* not a pragma typedef */
      }
   }
   free(comment);

   if(cptr == NULL) {
      return;
   }
/*
 * OK, we have a #define "typedef", so look for the closing "pragma }".
 * When we find it, we'll leave lptr at the 'p' of "pragma"
 */
   lineno_save = lineno;

   for(;;) {
      if((lptr = strchr(line,'}')) != NULL) {
	 if(lptr > line) lptr--;	/* backup to space before "}" */
	 while(lptr >= line && isspace(*lptr)) lptr--;
	 while(lptr >= line && !isspace(*lptr)) lptr--;
	 if(lptr != line) lptr++;		/* one too far */

	 if(strncmp(lptr,"pragma",6) == 0) {
	    break;
	 }
      }
      if(lineno++,(lptr = fgets(line,LSIZE,infil)) == NULL) {
	 fprintf(stderr,"Premature EOF in %s at line %d\n",input_file,lineno);
	 exit(1);
      }
   }
/*
 * we have the right line, so backup to "pragma }"
 */
   lptr = strchr(lptr,'}') + 1;
   while(isspace(*lptr)) lptr++;

   name = find_name(lptr,";\t\n *",NULL);
/*
 * We have found our typename, so setup the schema entry
 */
   if(ntype >= NTYPE) {
      fprintf(stderr,"Too many type names\n");
      fprintf(stderr,"Extra type: %s\n",name);
      return;
   }
   strcpy(types[ntype].file,input_file);
   types[ntype].lineno = lineno;
   strcpy(types[ntype].name,name);
   types[ntype].type = IS_DEFINE;
   types[ntype].nelem = 0;
   types[ntype].desires = MKIO_SCHEMA;
   strcpy(types[ntype].constructor,"NULL");
   strcpy(types[ntype].destructor,"NULL");
   strcpy(types[ntype].reader,"NULL");
   strcpy(types[ntype].writer,"NULL");
/*
 * and go back to read the values
 */
   fseek(infil, cur_pos, 0);		/* back to the start of the "type" */
   lineno = lineno_save - 1;		/* we are about to re-read it */

   for(;;) {
      if(lineno++,(lptr = fgets(line,LSIZE,infil)) == NULL) {
	 fprintf(stderr,"Failed to re-read line %d of %s\n",lineno,input_file);
	 exit(1);
      }

      while(isspace(*lptr)) lptr++;
      lptr += 7;				/* skip "#define" */
      while(isspace(*lptr)) lptr++;
      name = lptr;
      while(*lptr != '\0' && !isspace(*lptr)) lptr++;
      if(isspace(*lptr)) *lptr++ = '\0'; /* teminate name */
      while(isspace(*lptr)) lptr++;
      while(*lptr != '\0' && !isspace(*lptr)) lptr++;
      while(isspace(*lptr)) lptr++;
/*
 * got the name, so add it to the schema
 */
      strcpy(elems[nelem].name, name);
      strcpy(elems[nelem].type, types[ntype].name);
      elems[nelem].nstar=0;
      elems[nelem].nelem=NULL;
      nelem++;

      if(strncmp("/*",lptr,2) == 0) {   /* look in comment for "pragma }" */
	 lptr = skip_comment(lptr,&comment);
	 if((cptr = strchr(comment,'}')) != NULL) {
	    while(--cptr >= comment && isspace(*cptr)) continue;
	    cptr++;			/* move to first whitespace char */
	    cptr -= strlen("pragma");
	    if(strncmp(cptr,"pragma",6) == 0) {
	       free(comment);
	       break;
	    }
	 }
	 free(comment);
      }

      if(nelem > NELEM - 1) {
	 fprintf(stderr, "Too many members of a \"#define type\". "
		 "Please increase NELEM\n");
	 exit(1);
      }
   }
/*
 * malloc space for members and fill in
 */
   if((types[ntype].elems = (MEMBER *)malloc(nelem*sizeof(MEMBER))) == NULL) {
      fprintf(stderr,"Can't malloc space for %s's members; %s at line %d\n",
	      types[ntype].name,input_file,lineno);
      return;
   }

   types[ntype].nelem=nelem;
   memcpy((char*) types[ntype].elems, (char*) elems, nelem*sizeof(MEMBER));

   ntype++;
}

/*****************************************************************************/
/*
 * Process an enum typedef. The elements of the enum are skipped, and
 * the name remembered in types.
 */
/* Modified by S. Kent to add a 1-line element to the schema just as for
 * primitive types. */

/* Latter half of do_enum() was added by Wei Peng to take care of enums
   as valid schema types with details about their "members" just like struct.

   Wei Peng, 04/10/1994
 */

static void
do_enum(void)
{
   char *lptr;				/* pointer to line */
   char *name;				/* name of enum */
   char dc = '\0';          		/* delim char returned by find_string, WP */
   int nelem =0;			/* number of elements in the enum */
   MEMBER elems[NELEM];                 /* tmp MEMBER structures, WP */
   unsigned long pos_save_end;          /* remember file position at end of typedef, WP */
   int lineno_save;                     /* to save lineno, WP */
   char lptr_save[LSIZE];               /* to save lptr, WP */
   
/*
 * Look for the typedef name
 */
   while((lptr = strchr(line,'}')) == NULL) {
      if(lineno++,(lptr = fgets(line,LSIZE,infil)) == NULL) {
	 fprintf(stderr,"Premature EOF in %s at line %d\n",input_file,lineno);
	 exit(1);
      }
   }
   while(isspace(*lptr)) lptr++;
   name = find_name(lptr,";",NULL);
   if(ntype >= NTYPE) {
      fprintf(stderr,"Too many type names\n");
      fprintf(stderr,"Extra type: %s\n",name);
      return;
   }
   strcpy(types[ntype].file,input_file);
   types[ntype].lineno = lineno;
   strcpy(types[ntype].name,name);
   types[ntype].type = IS_ENUM;
   types[ntype].nelem = 0;	/* Changed to have a 1 member schema def */
                                /* NOT true any more. I modified it to list
                                   all "members" of the enum type -- WP */
   types[ntype].desires = MKIO_SCHEMA;
   strcpy(types[ntype].constructor,"NULL");
   strcpy(types[ntype].destructor,"NULL");
   strcpy(types[ntype].reader,"NULL");
   strcpy(types[ntype].writer,"NULL");

   types[ntype].lock = SCHEMA_NOLOCK; /* There is no PRAGMA for enum so .. no lock */

  /* Following codes are added by Wei Peng, 04/10/94 */
  /* process the "members" of this enum, set the default values */
   
   pos_save_end =ftell(infil);       /* remember the current position again, Wei Peng */
   lineno_save = lineno;
   memcpy(lptr_save, lptr, LSIZE);

  /* rewind to the previous position */
   
   fseek(infil, cur_pos, 0);
   lptr = fgets(line, LSIZE, infil);

  /* lptr has typedef stuff, get next element of enum, first search for { */

   name=find_string(lptr, "{}", NULL, &dc, 1);

   while(dc!='{') /* to remember comments */
   { 
        if(name==NULL)
        {
          if((lptr = fgets(line, LSIZE, infil))==NULL) 
           fprintf(stderr, "prematured ending of enum definition.\n"),
           exit(1);
          name=find_string(lptr, "{}", NULL, &dc, 0);  
        }
        else name=find_string(NULL,"{}",NULL, &dc, 0); 

   }

   name=find_string(NULL, "{}=",NULL, &dc, 0);
   while(1)
   {
      if(name==NULL || strlen(name) < 1 )
      {
         /* this breaks down only in the case a string
            is extremely long and is using continuation
            line                                     */
          lptr=fgets(line, LSIZE, infil);
          while(isspace(*lptr)) lptr++;
          name=find_string(lptr, "{}=",NULL, &dc, 0);
          
         
      }  
      
      if(dc==',' || dc=='\n' || dc=='=' ||isspace(dc)|| dc=='}' )
      {
        if(name!=NULL && strlen(name) > 1)
        {
            strcpy(elems[nelem].name, name);
            strcpy(elems[nelem].type, types[ntype].name);
            elems[nelem].nstar=0;
            elems[nelem].nelem=NULL;
            nelem++;
        }
        if(dc=='}') break;
        if(dc=='=') name = find_string(NULL, "{}=", NULL,&dc, 0);
        if(dc=='}') break;
      }
      
      name=find_string(NULL, "{}=", NULL,&dc, 0);  
      if(name!=NULL && strlen(name) <1 && dc=='}') break; 
      if(nelem > NELEM -1 )
      {
           fprintf(stderr, "Too many enum members. Increase NELEM, please\n");
           exit(1);
      }
      
   }
   
  /* restore infil status and others */

   fseek(infil, pos_save_end, 0);
   lineno = lineno_save;
   memcpy(lptr, lptr_save, LSIZE);

   types[ntype].nelem=nelem;

  /* malloc space for nelem + 1 member and fill in */
   if((types[ntype].elems = (MEMBER *)malloc(nelem*sizeof(MEMBER))) == NULL) {
      fprintf(stderr,"Can't malloc space for %s's members; %s at line %d\n",
	      types[ntype].name,input_file,lineno);
      return;
   }

  /* copy over the info in elems */

   memcpy((char*) types[ntype].elems, (char*) elems, nelem*sizeof(MEMBER));

   nelem = 0;
   ntype++;


}

/*****************************************************************************/
/*
 * process a typedef that isn't a struct or enum
 *
 * Currently function pointers are the only case considered
 */
static int
do_generic(char *lptr)
{
   char c;
   int pragma=0;			/* a possible pragma */
   char *name;				/* name of type */
   
   while(isspace(*lptr)) lptr++;
   while(*lptr != '\0' && !isspace(*lptr)) lptr++; /* skip type */
   while(isspace(*lptr)) lptr++;

   if(strncmp(lptr,"(*",2) != 0) {	/* not a function pointer */
      return(-1);
   }
   lptr += 2;

   name = find_name(lptr,")",&c);
   lptr += strlen(name) + 1;
   
   while(*lptr != ';') {
      if(*lptr == '\0') {
	 if(lineno++,(lptr = fgets(line,LSIZE,infil)) == NULL) {
	    fprintf(stderr,"Premature EOF in %s at line %d\n",
		    input_file,lineno);
	    exit(1);
	 }
      } else {
	 lptr++;
      }
   }
   lptr++;

   while(isspace(*lptr)) lptr++;
   if(strncmp(lptr,"/*",2) == 0) {	/* a comment to process */
      pragma = process_pragmas(lptr);
   }
   if(pragma & MKIO_FUNCTION) {		/* it's a function pointer */
/*
 * Setup the schema entry
 */
      if(ntype >= NTYPE) {
	 fprintf(stderr,"Too many type names\n");
	 fprintf(stderr,"Extra type: %s\n",name);
	 return(-1);
      }
      strcpy(types[ntype].file,input_file);
      types[ntype].lineno = lineno;
      strcpy(types[ntype].name,name);
      types[ntype].type = IS_FUNC;
      types[ntype].nelem = 0;
      types[ntype].desires = MKIO_SCHEMA;
      strcpy(types[ntype].constructor,"NULL");
      strcpy(types[ntype].destructor,"NULL");
      strcpy(types[ntype].reader,"NULL");
      strcpy(types[ntype].writer,"NULL");
      ntype++;

      return(0);
   }
   
   return(-1);
}

/*****************************************************************************/
/*
 * Process a struct typedef. This consists of reading the entire definition,
 * finding elements and their types. When the entire struct is read we'll
 * see its new name and record that in types[].
 */
static void
do_struct(void)
{

   int desires;				/* what do we want to do with type? */
   MEMBER elems[NELEM];			/* elements of the structure */
   int i;
   char *lptr;				/* pointer to line */
   char name[SIZE];			/* module name */
   int nelem;				/* number of elements */
   int nesting;				/* nesting level of {} */
   char *nstring;			/* Pointer to nelem string */
   char tag[SIZE];			/* structure tag */
   char **token_list;			/* list of tokens in the struct */
   char **token;			/* pointer to tokens */
   char type_str[SIZE];			/* string for type info */

   token = token_list = tokenise();

   if(strcmp(*token++,"typedef") != 0 || strcmp(*token++,"struct") != 0) {
      fprintf(stderr,"Expected `typedef struct' in %s at line %d, saw %s\n",
	      input_file,lineno,token_list[0]);
      exit(1);
   }
   if(strcmp(*token,"{") == 0) {
      *tag = '\0';
   } else {
      sprintf(tag,"struct %s",*token++);
   }
   
   if(strcmp(*token++,"{") == 0) {
      nesting = 1;
   } else {
      fprintf(stderr,"Expected `typedef struct [tag] {' in %s at line %d\n",
	      input_file,lineno);
      exit(1);
   }
/*
 * read the elements
 */
   for(nelem = 0;nelem < NELEM;nelem++, token++) {
      if(strcmp(*token,"{") == 0) {
	 nesting++;
      } else if(strcmp(*token,"}") == 0) {
	 nesting--;
      }
      if(nesting == 0) {
	 token++;
	 strcpy(name,*token++);
	 if(strcmp(";",*token++) != 0) {
	    fprintf(stderr,"Expected `NAME ;' in %s at line %d\n",
		    input_file,lineno);
	    exit(1);
	 }
	 break;
      }
/*
 * we have a list of elements on a line, preceeded by type information
 */
      *type_str = '\0';
      while(strcmp("const",*token) == 0 ||
	    strcmp("signed",*token) == 0 ||
	    strcmp("unsigned",*token) == 0) {
	 sprintf(&type_str[strlen(type_str)],"%s ",*token);
	 token++;
      }
      if(strcmp("struct",*token) == 0) {
	 token++;
	 sprintf(&type_str[strlen(type_str)],"struct %s",*token);
      } else if(strcmp("union",*token) == 0) {
	 int nest = 0;
	 fprintf(stderr,"I can't yet read/write unions; in %s line %d\n",
		 input_file,lineno);
	 nelem--;
	 for(;;) {
	    if(strcmp(*token,"{") == 0) {
	       nest++;
	    } else if(strcmp(*token,"}") == 0) {
	       nest--;
	    }
	    if(nest == 0) {
	       break;
	    }
	 }
	 continue;
      } else {
	 sprintf(&type_str[strlen(type_str)],"%s",*token);
      }
      strcpy(elems[nelem].type,type_str);
      token++;				/* skip type */

      for(;;) {				/* many elements on the same line */
/*
 * first deal with indirection/array indices
 */
	 for(elems[nelem].nstar = 0;strcmp(*token,"*") == 0;token++) {
	    elems[nelem].nstar++;
	 }

	 strcpy(elems[nelem].name,*token++);
	 
         if((nstring = elems[nelem].nelem = (char *)malloc(1000)) == NULL) {
	    fprintf(stderr,"Can't allocate memory for array sizes; %s at %d\n",
		    input_file,lineno);
	 }
	 *nstring = '\0';

	 while(strcmp(*token,"[") == 0) {
	    token++;
	    if(nstring > elems[nelem].nelem) {
	       sprintf(nstring," \" \" ");
	       nstring += strlen(nstring);
	    }
	    do {
	       sprintf(nstring,"STRING(%s)",*token);
	       nstring += strlen(nstring);
	       if(++token == NULL) {
		  fprintf(stderr,
			  "found end of definition while looking for ] for %s;"
			  "%s at line %d\n",
			  elems[nelem].name,input_file,lineno);
		  exit(1);
	       }
	    } while (strcmp(*token,"]") != 0);
	    token++;
	 }
/*
 * recover space that we didn't use
 */
	 if(*elems[nelem].nelem == '\0') {
	    free(elems[nelem].nelem);
	    elems[nelem].nelem = NULL;
	 } else {
	    if((elems[nelem].nelem =
		(char *)realloc(elems[nelem].nelem,
				strlen(elems[nelem].nelem) +1)) == NULL) {
	       fprintf(stderr,"Can't realloc for array dimens; %s at %d\n",
		       input_file,lineno);
	    }
	 }
	 if(strcmp(*token,";") == 0) {
	    break;
	 } else if(strcmp(*token,",") == 0) {
	    if(++nelem >= NELEM) {
	       fprintf(stderr,
		       "Too many elements in %s (line %d); increase NELEM\n",
		       input_file,lineno);
	       exit(1);
	    }
	    token++;
	    strcpy(elems[nelem].type,elems[nelem - 1].type);
	 }
      }
   }

   if(*token != NULL) {			/* look for pragmas */
      lptr = *token++;
      if(strncmp(lptr,"/*",2) != 0) {
	 fprintf(stderr,"Expected a comment; found %s; %s at line %d\n",
		 lptr,input_file,lineno);
	 desires = MKIO_IGNORE;
      } else {
	 desires = process_pragmas(lptr);
      }
   } else {
      char null_comment[5] = "/**/";	/* may be modified by process_pragmas*/
      desires = process_pragmas(null_comment);
   }
/*
 * parsing is complete
 */
   free_tokens(token_list);

   if(desires & MKIO_IGNORE) {
      if(verbose) {
	 fprintf(stderr,"Ignoring %s\n",name);
      }
      return;
   }
   if(verbose && (desires & MKIO_USER) != 0) {
      fprintf(stderr,
	      "You must provide code/prototypes for dumping %ss\n",name);
   }
/*
 * Replace any self-references (e.g. struct mask) by the typedef name
 * (e.g. MASK)
 */
   for(i = 0;i < nelem;i++) {
      if(strcmp(tag,elems[i].type) == 0) {
	 sprintf(elems[i].type,"%s",name);
      }
   }
/*
 * remember the name and save what we need to know about the type
 */
   if(ntype >= NTYPE) {
      fprintf(stderr,"Too many type names\n");
      fprintf(stderr,"Extra type: %s\n",name);
      return;
   }
   strcpy(types[ntype].file,input_file);
   types[ntype].lineno = lineno;
   strcpy(types[ntype].name,name);
   types[ntype].type = IS_STRUCT;
   types[ntype].desires = desires;
   types[ntype].nelem = nelem;
/*
 * set up the function pointers
 */
   if((types[ntype].desires & MKIO_AUTO) != 0 ||
      (types[ntype].desires & MKIO_CONSTRUCTOR) != 0) {
      sprintf(types[ntype].constructor,"%s%sNew",
	      module_prefix,canonical(types[ntype].name));
      sprintf(types[ntype].destructor,"%s%sDel",
	      module_prefix,canonical(types[ntype].name));
   } else {
      strcpy(types[ntype].constructor,"NULL");
      strcpy(types[ntype].destructor,"NULL");
   }
   
   if((types[ntype].desires & MKIO_AUTO) != 0) {
      strcpy(types[ntype].reader,"shDumpGenericRead");
      strcpy(types[ntype].writer,"shDumpGenericWrite");
   } else if((types[ntype].desires & MKIO_USER) != 0) {
      sprintf(types[ntype].reader,"%sDump%sRead",
	      module_prefix,canonical(types[ntype].name));
      sprintf(types[ntype].writer,"%sDump%sWrite",
	      module_prefix,canonical(types[ntype].name));
   } else {
      strcpy(types[ntype].reader,"NULL");
      strcpy(types[ntype].writer,"NULL");
   }
/*
 * Set up the lock.
 */
   if ((types[ntype].desires & MKIO_LOCK) != 0) {
     types[ntype].lock = SCHEMA_LOCK;
   } else {
     types[ntype].lock = SCHEMA_NOLOCK;
   }

/*
 * save the list of members
 */
   if((types[ntype].elems = (MEMBER *)malloc(nelem*sizeof(MEMBER))) == NULL) {
      fprintf(stderr,"Can't malloc space for %s's members; %s at line %d\n",
	      types[ntype].name,input_file,lineno);
      exit(1);
   }
   for(i = 0;i < nelem;i++) {
      types[ntype].elems[i] = elems[i];
   }
   
   nstruct++; ntype++;
}

/*****************************************************************************/
/*
 * process a set of dump/schema pragmas in a comment.
 *
 * Assumes that you are looking at slash-star
 */
static int
process_pragmas(char *lptr)
{
   int desires;				/* what we want to do with this type */
   int i;
   int no;				/* pragma starts "NO" */
   char *pragma;
   char *ptr;
   struct {
      char *str;			/* name of pragma */
      int flags;			/* and corresponding variable */
   } pragmas[] = {
      { "AUTO", MKIO_AUTO },		/* use generic i/o code */
      { "CONSTRUCTOR", MKIO_CONSTRUCTOR }, /* make handles automatically */
      { "IGNORE", MKIO_IGNORE },	/* ignore this type completely */
      { "SCHEMA", MKIO_SCHEMA },	/* generate schema for this type */
      { "USER", MKIO_USER },		/* user will provide read/write code */
      { "LOCK", MKIO_LOCK },            /* No over-writing of this schema allowed */
      { "FUNCTION", MKIO_FUNCTION | MKIO_USER }, /* a function pointer */
      { NULL, 0 },
   };
   char *start = lptr;
/*
 * strip trailing whitespace
 */
   ptr = lptr + strlen(lptr) - 1;
   while(ptr > lptr && isspace(*ptr)) ptr--;
   *(ptr + 1) = '\0';
/*
 * process comment
 */
   desires = MKIO_CONSTRUCTOR;

   lptr += 2;
   pragma = strtok(lptr," ");
   while(pragma != NULL) {
     if(strcmp(pragma,"*/") == 0) {
       ;				/* end of comment */
     } else if(strcmp(pragma,"pragma") != 0) { /* deal with legacy code */
       if(strcmp(pragma,"NODUMP") == 0) {
	 desires = MKIO_IGNORE;
	 if(verbose > 0) {
	   fprintf(stderr,
		   "Old-style pragma NODUMP; "
		   "please change to `pragma IGNORE';\n"
		   "file %s (line %d)\n",
		   input_file,lineno);
	 }
       } else if(strcmp(pragma,"DUMP-SCHEMA") == 0) {
	 desires = MKIO_SCHEMA;
	 if(verbose > 0) {
	   fprintf(stderr,
		   "Old-style pragma DUMP-SCHEMA; "
		   "please change to `pragma SCHEMA';\n"
		   "file %s (line %d)\n",input_file,lineno);
	 }
       } else if(strcmp(pragma,"NODUMP-R/W") == 0) {
	 desires = MKIO_CONSTRUCTOR | MKIO_SCHEMA;
	 if(verbose > 0) {
	   fprintf(stderr,
		   "Old-style pragma NODUMP-R/W; "
		   "please change to `pragma SCHEMA' or `pragma USER';\n"
		   "file %s (line %d)\n",
		   input_file,lineno);
	 }
       } else {
	 fprintf(stderr,"Unknown struct pragma: %s (%s; line %d)\n",
		 pragma,input_file,lineno);
       }
     } else {
       if((pragma = strtok(NULL," ")) == NULL) {
	 fprintf(stderr,
		 "Missing keyword after pragma; %s (line %d)\n",
		 input_file,lineno);
	 break;
       }
	 
       if(strncmp(pragma,"NO",2) == 0) {
	 no = 1;
	 pragma += 2;
       } else {
	 no = 0;
       }
       for(i = 0;pragmas[i].str != NULL;i++) {
	 if(strcmp(pragma,pragmas[i].str) == 0) {
	   if(no) {
	     desires &= ~pragmas[i].flags;
	   } else {
	     desires |= pragmas[i].flags;
	   }
	   break;
	 }
       }
       if(pragmas[i].str == NULL) {
	 fprintf(stderr,"Unknown struct pragma: %s (%s; line %d)\n",
		 pragma,input_file,lineno);
       }
     }
     pragma = strtok(NULL," ");
   }
   
   if((desires & MKIO_USER) == 0) {
     desires |= MKIO_AUTO;
   }
   if((desires & MKIO_IGNORE) != 0) {
     desires = MKIO_IGNORE;		/* turn off everything else */
   }
   if((desires & MKIO_SCHEMA) != 0) {
     desires = (desires & MKIO_LOCK) | MKIO_SCHEMA;
                                        /* turn off everything else 
					   except LOCK */
   }
   
   *start = '\0';			/* we devoured the comment */
   return(desires);
}

/*****************************************************************************/
/*
 * Tokenise a definition, stripping white space and comments. This is used
 * as a preliminary to dealing with structs and enums.
 *
 * The last `token' will be any comment following the ; after the typedef
 * name
 */
static char **
tokenise(void)
{
   int nalloc = 100;			/* number of tokens allocated */
   int nesting;				/* nesting level of braces */
   int ntoken;				/* number of tokens seen */
   char *ptr;
   int seen_body = 0;			/* have we seen all of the body of
					   this typedef? */
   char *start;				/* the start of a token */
   char **tokens;			/* the list of tokens to be returned */

   if((tokens = (char **)malloc((nalloc + 2)*sizeof(char *))) == NULL) {
      fprintf(stderr,"Can't allocate storage for tokens\n");
      exit(1);
   }
   ntoken = 0;

   nesting = 0;
   ptr = line;
   for(;;) {
      while(isspace(*ptr)) ptr++;
      if(*ptr == '\0') {
	 if(lineno++, fgets(line,LSIZE,infil) == NULL) {
	    fprintf(stderr,"Premature EOF in %s at line %d\n",
		    input_file,lineno);
	    exit(1);
	 }
	 ptr = line;
	 continue;
      }
/*
 * We could be looking at a comment
 */
      if(strncmp("/*",ptr,2) == 0) {
	 ptr = skip_comment(ptr,NULL);
	 continue;
      }
/*
 * we are at the start of the token
 */
      start = ptr;
      if(*ptr == '_' || isalnum(*ptr)) { /* a number or identifier */
	 ptr++;
	 while(isalnum(*ptr) || *ptr == '_') ptr++;
      } else {				/* a single character */
	 ptr++;
      }
/*
 * OK, we have the token. Put it on the list
 */
      tokens[ntoken++] = save_str(start,ptr - start);

      if(ntoken == nalloc) {
	 nalloc *= 2;
	 if((tokens = (char **)realloc((char *)tokens,
				       (nalloc + 2)*sizeof(char *)))
								     == NULL) {
	    fprintf(stderr,"Can't reallocate storage for tokens\n");
	    exit(1);
	 }
      }
/*
 * do we have to do something?
 */
      start = tokens[ntoken - 1];
      if(strcmp(start,"{") == 0) {
	 nesting++;
      } else if(strcmp(start,"}") == 0) {
	 nesting--;
	 if(nesting == 0) {
	    seen_body = 1;
	 } else if(nesting < 0) {
	    fprintf(stderr,"Unmatched } in %s at line %d\n",input_file,lineno);
	    exit(1);
	 }
      } else if(strcmp(start,";") == 0 && seen_body) {
	 while(isspace(*ptr)) ptr++;
	 if(strncmp("/*",ptr,2) == 0) {		/* Read the closing comment */
	    ptr = skip_comment(ptr,&tokens[ntoken++]);
	 }
	 tokens[ntoken] = NULL;
	 return(tokens);
      }
    }

   return(NULL);			/* NOTREACHED */
}

/*****************************************************************************/
/*
 * free a token list
 */
static void
free_tokens(char **tokens)
{
   char **tptr = tokens;
   
   for(tptr = tokens;*tptr != NULL;tptr++) {
      free(*tptr);
   }
   free((char *)tokens);
}

/*****************************************************************************/
/*
 * return a malloced copy of the first n characters of a string
 */
static char *
save_str(char *str, int n)
{
   char *saved;
   
   if((saved = (char *)malloc(n + 1)) == NULL) {
      fprintf(stderr,"Can't allocate storage for saved\n");
      exit(1);
   }
   strncpy(saved,str,n);
   saved[n] = '\0';

   return(saved);
}

/*****************************************************************************/
/* I can't get find_name to work properly. I'm creating a working find_name here.
 * 
 * Wei Peng, 04/10/94.
 *
 * char* find_string(char* ptr, char* delim_str, char* ignore_str, 
 *              char* return_delim, unsigned short initialize_static)
 *
 * searches string pointed by ptr, use characters in delim_str as delimit
 * characters and return the used delimiter in return_delim. Return
 * the string found, NULL (zero string) if none found or if
 * this line has been exhausted. Comment lines are skipped.
 *
 * if ignore_str is not NULL, the routine will not search the characters
 * trailing the first character that is found present in the searched string
 * from the set.
 * 
 * if initialize_static is true, all the static variables in the routine
 * will be initialized.
 *
 ************************************************************************/

static char* 
find_string(char* ptr, char* delim_str, char* ignore_str, 
 char* dc,unsigned short initialize_static )
{
   static unsigned int in_comment=0;
   static char save_string[LSIZE];
   static char *save_ptr=NULL;
   char* C_sep_token=", \t\n;()";       /* official C separation tokens */
   char  sep[LSIZE];
   char* search_base;
   char* search_mark;
   char* tmp=NULL;
   static char tmp_string[LSIZE];


   if(initialize_static)
   {
      in_comment=0;
      memset(save_string, 0, LSIZE);
      memset( tmp_string, 0, LSIZE);
      save_ptr=NULL;
   }
      
   /* establish seach base */
  
   strcpy(sep, C_sep_token);
   if(delim_str!=NULL) strcat(sep, delim_str);

   if(ptr==NULL) 
   {
       if(save_ptr==NULL) { if(dc!=NULL) *dc='\0'; return NULL;}
       save_ptr=strcpy(save_string, tmp_string);
       search_base = save_ptr;
   }
         
   else
   {  
       
       memset(save_string, 0, LSIZE);
       while(isspace(*ptr)) ptr++;
       save_ptr=strcpy(save_string, ptr);
       search_base=save_ptr;
    }

   /* strip off comment lines */

   search_mark = search_base;
   if(strlen(search_base) < 1)
   {
     if(dc!=NULL) *dc='\0'; 
     return NULL;
   }
   if(in_comment)
   {
      /* search for the end of comment */

      if((tmp=strchr(search_base, '*'))!=NULL && *(tmp+1)=='/')
      {
         tmp++;
         while(tmp >= search_base) *tmp--=' ';
         in_comment = 0;
      }
      else { if(dc!=NULL) *dc='\0'; return NULL;}  /* case the whole line is in comment */
   }
        
   if(!in_comment)  /* not in comment */
   {

      while(search_mark < search_base + strlen(search_base))
      {   
          if((tmp=strchr(search_base, '/'))!=NULL && *(tmp+1)=='*')
          {
             *tmp++=' '; *tmp++=' ';
             search_mark=tmp;
             in_comment=1;
          
            if((tmp=strchr(search_base, '*'))!=NULL && *(tmp+1)=='/')
            {
               while(tmp >= search_mark) *tmp--=' ';
               in_comment = 0;
               search_mark =strchr(search_base, '/');
               *search_mark++=' ';
          
             }
            else 
            {   /*  line is partially in comment */

               tmp = search_mark;
               while(*tmp!='\n') *tmp++=' ';
               break;

             } 
          }
          else break;
       }
    }

   /* now search_base is clean, all comments are stripped off */

    while(isspace(*search_base)) search_base++;   /* suppress leading white space */
    tmp=strpbrk(search_base, sep);

    if(tmp==NULL) 
     { if(dc!=NULL) *dc='\0'; 
              return NULL;
     }

    if(dc!=NULL) *dc = *tmp;
    search_mark=tmp;            /* save a copy of tmp pointing to the char */

    if(ignore_str!=NULL) tmp=strpbrk(search_base, ignore_str);
    if(tmp!=NULL && tmp < search_mark)
    {
           /* too bad, we'd have to ignore the whole string */
       if(dc!=NULL) *dc = *tmp;
       return NULL;
    }

    /* save the string to save_ptr for next pass */

    tmp = search_mark +1;
    while(isspace(*tmp)) tmp++;
    save_ptr = tmp;
    strcpy(tmp_string, save_ptr);

     /* suppress trailing white space */
     /* last char is always '\n', we don't use it */

    tmp = save_ptr + strlen(save_ptr) -2;   
    while(tmp > save_ptr && isspace(*tmp)) *tmp--='\0';
    *(tmp+1)='\n';          /* compensate the '\n' character */

    if(strlen(search_base) < 1)
    {
       memset(save_string, 0, LSIZE);
       save_ptr=NULL;
       return NULL;
    }
   
    /* null-terminate the string before we return */

    tmp = search_mark;
    *tmp='\0';             
    return search_base;
   
}

    
     
  

/*****************************************************************************/
/*
 * Find a name that is either at the end of the line, or immediately
 * preceeding the delim character; comments are assumed to extend to
 * the end of the line
 *
 * If ptr is NULL, resume at end of previous search
 *
 * The delim string may be a list of characters, e.g. " \t". If it is
 * NULL, return the next non-space character. Note that if the delim
 * string contains a space you may want to skip white space before
 * calling this function.
 *
 * If del_char isn't NULL, the actual delimiting character is returned.
 */
static char *
find_name(char *ptr,char *delim_str, char *delim_char)
{
   char delim[128];			/* characters to delimit search */
   char dc = '\0';			/* char that delimited the search */
   static char *save_ptr = NULL;
   char *start;

   memset(delim,'\0',128);
   if(ptr == NULL) {
      if(save_ptr == NULL) {
	 dc = '\0';
	 return("");
      }
      ptr = save_ptr;
   }
   start = ptr;

   if(delim_str == NULL) {
      while(isspace(*ptr)) ptr++;
      
      if(delim_char != NULL) *delim_char = *ptr;
      return(*ptr == '\0' ? NULL : ptr);
   } else {
      do {
	 delim[(int)*delim_str] = 1;
      } while(*delim_str++ != '\0');
   }

   save_ptr = NULL;			/* we may not find another field */
   for(;*ptr != '\0';ptr++) {
      if(*ptr == '/' && *(ptr + 1) == '*') {
	 if(ptr == start) {
	    dc = *ptr;
	    *ptr = '\0';
	    save_ptr = ptr + 1;
	 } else {
	    ptr--;
	    while(isspace(*ptr)) ptr--;
	    dc = *(ptr + 1);
	    *(ptr + 1) = '\0';
	    save_ptr = ptr + 2;
	 }
	 break;
      } else if(delim[(int)*ptr]) {
	 dc = *ptr;
	 save_ptr = ptr + 1;
	 if(ptr == start) {
	    *ptr = '\0';
	 } else {
	    *ptr-- = '\0';
	 }
	 break;
      }
   }
   while(ptr > start && !isspace(*ptr)) ptr--;
   if(isspace(*ptr)) ptr++;
   if(delim_char != NULL) *delim_char = dc;

   return(ptr);
}
   
/*****************************************************************************/
/*
 * skip a comment that starts at lptr; return a pointer to just after
 * the closing star-slash. The comment may extend over many lines
 *
 * If comment is not NULL, return the comment (in a malloced string). Any
 * sequence of whitespace characters is replaced by a single space
 */
static char *
skip_comment(char *lptr, char **comment)
{
   int len=0;				/* length of comment so far */
   
   if(comment != NULL) {
      len = strlen(lptr) + 1;
      if((*comment = (char *)malloc(len)) == NULL) {
	 fprintf(stderr,"Can't get storage for comment in %s at line %d\n",
		 input_file,lineno);
	 exit(1);
      }
      strcpy(*comment,lptr);
   }
   for(lptr += 2;;lptr++) {
      if(*lptr == '\0') {
	 if(lineno++,(lptr = fgets(line,LSIZE,infil)) == NULL) {
	    fprintf(stderr,"Premature EOF in %s at line %d\n",
		    input_file,lineno);
	    exit(1);
	 }
	 if(comment != NULL) {
	    if((*comment = (char *)realloc(*comment,len + strlen(lptr)))
                                                                == NULL) {
	       fprintf(stderr,"Can't realloc comment in %s at line %d\n",
		 input_file,lineno);
	       exit(1);
	    }
	    strcpy(&(*comment)[len - 1],lptr);
	    len += strlen(lptr);
	 }
      }
      if(*lptr == '\\') {
	 lptr += 2;
      }
      if(*lptr == '*' && *(lptr + 1) == '/') {
	 lptr += 2;
/*
 * We promised to compress whitespace
 */
	 if(comment != NULL) {
	    char *iptr,*optr;
	    for(iptr = optr = *comment;;) {
	       if(*iptr == '\0') {
		  if(*(optr - 1) == ' ') { /* trailing white space */
		     optr--;
		  }
		  *optr = '\0';		/* we could realloc some space, but
					   let's not bother */
		  break;
	       } else if(isspace(*iptr)) {
		  *optr++ = ' ';
		  while(isspace(*iptr)) iptr++;
	       } else {
		  *optr++ = *iptr++;
	       }
	    }
	 }
	 return(lptr);
       }
    }

   return(NULL);			/* NOTREACHED */
}

/*****************************************************************************/
/*
 * convert a string to be all lower case
 *
 * Note that the return value is in a static string, so no more than one
 * call may be active at a time
 */
static char *
lcase(char *name)
{
   int i;
   static char str[SIZE];

   for(i = 0;name[i] != '\0';i++) {
      str[i] = tolower(name[i]);
   }
   str[i] = '\0';
   return(str);
}

/*****************************************************************************/
/*
 * convert a type name to canonical form; in most cases this means make the
 * first letter uppercase and all others lowercase, but sometimes more
 * is required, for example REGION --> Reg
 *
 * Note that the return value is in a static string, so no more than one
 * call may be active at a time
 */
static char *
canonical(char *name)
{
   static char str[SIZE];

   if(strcmp(name,"REGION") == 0) {
      strcpy(str,"Reg");
   } else {
      strcpy(str,lcase(name));
      str[0] = toupper(str[0]);
   }

   return(str);
}

/*****************************************************************************/
/*
 * Here are the functions that generate C
 *
 * Write the file header. The files that are read are passed in as the
 * argument list; not only are they recorded for posterity in a comment
 * but they are #included in the generated code (as they presumably
 * contain needed struct definitions)
 */
static void
write_file_header(FILE *fil, int ac, char **av)
{
   char *basename;			/* header files with path stripped */
   int i;
   time_t tim = time(NULL);

   if(fil == NULL) {
      return;
   }
   fprintf(fil,"/*\n");
   fprintf(fil," * This file was machine generated from ");
   for(i = 1;i < ac;i++) {
      fprintf(fil,"%s ",av[i]);
   }
   fprintf(fil,"\n");
   fprintf(fil," * on %s",ctime(&tim));
   fprintf(fil," *\n");
   fprintf(fil," * Don't even think about editing it by hand\n");
   fprintf(fil," */\n");
   fprintf(fil,"#include <stdio.h>\n");
   fprintf(fil,"#include <string.h>\n");
   fprintf(fil,"#include \"dervish_msg_c.h\"\n");
   for(i = 1;i < ac;i++) {
      if((basename = strrchr(av[i],'/')) != NULL) {
	 basename++;
      } else {
	 basename = av[i];
      }
      fprintf(fil,"#include \"%s\"\n",basename);
   }
   fprintf(fil,"#include \"shCSchema.h\"\n");
   fprintf(fil,"#include \"shCUtils.h\"\n");
   fprintf(fil,"#include \"shCDiskio.h\"\n");
   fprintf(fil,"#include \"prvt/utils_p.h\"\n");
   fprintf(fil,"\n");
   fprintf(fil,"#define NSIZE 100\t\t/* size to read strings*/\n");
   fprintf(fil,"\n");
   fprintf(fil,"#define _STRING(S) #S\n");
   fprintf(fil,"#define STRING(S) _STRING(S)\t\t"
	   "/* convert a cpp value to a string*/\n");
}

/*****************************************************************************/
/*
 * Generate code to preserve the type's schema
 *
 * First the members of a struct; this is written as a static array with
 * a unique name; it will be associated with the name of its type later
 * (when all the types have been seen) in write_schema(). This is the
 * num'th initialisation (i.e. schema_elems{num})
 */

/* WP modified the function header to include ENUMORSTRUCT parent, 04/10/94 */
/* parent designates what type (struct or enum) of parent this member is in */

static void
write_member_schema(char *name, MEMBER *elems, int nelem, int num, FILE *fil, 
  ENUMORSTRUCT parent)
{
   int i;
   
   fprintf(fil,"\n");
   fprintf(fil,"static SCHEMA_ELEM schema_elems%d[] = { /* %s */\n",num,name);
   for(i = 0;i < nelem;i++) {

      if(parent == IS_ENUM || parent == IS_DEFINE)
      fprintf(fil,"   { \"%s\", \"%s\", (int) %s, %d, 0, %s, 0, ",
	      elems[i].name,elems[i].type, elems[i].name,  elems[i].nstar,
	      (elems[i].nelem == NULL ? "NULL" : elems[i].nelem));
     else
       fprintf(fil,"   { \"%s\", \"%s\", 0, %d, 0, %s, 0, ",
	      elems[i].name,elems[i].type, elems[i].nstar,
	      (elems[i].nelem == NULL ? "NULL" : elems[i].nelem));
      if(*elems[i].name == '\0') {
	 fprintf(fil,"0");
      } else {
#if !defined(IRIX_OFFSETOF_BUG)		/* present in iris 5.1 and 5.2 cc */
	 if(parent == IS_ENUM || parent == IS_DEFINE) {
	    fprintf(fil,"0");
	 } else {
	    fprintf(fil,"offsetof(%s,%s)",name,elems[i].name);
	 }
#else
/*
 * work around an iris 5.1 compiler bug. offsetof of array elements
 * in structures doesn't work, so get the offset of the previous element
 * and add sizeof(prev)
 */
	 if(elems[i].nelem == NULL) {	/* no problem */
	    if(parent == IS_ENUM || parent == IS_DEFINE) {
	       fprintf(fil,"0");
	    } else {
	       fprintf(fil,"offsetof(%s,%s)",name,elems[i].name);
	    }
	 } else {
	    int j;

	    for(j = i - 1;j >= 0;j--) {	/* look for a usable offsetof */
	       if(elems[j].nelem == NULL) { /* got one */
		  break;
	       }
	    }
	    if(j == -1) {		/* one too far */
	       j = 0;
	    }
	    if(j == 0) {
	       fprintf(fil,"0");
	    } else {
	       fprintf(fil,"\n\toffsetof(%s,%s)",name,elems[j].name);
	    }
	    for(;j < i;j++) {
	       fprintf(fil," + sizeof(%s",elems[j].type);
	       {
		  int k;
		  for(k = 0;k < elems[j].nstar;k++) {
		     fprintf(fil,"*");
		  }
	       }
	       fprintf(fil,")");
	       if(elems[j].nelem != NULL) { /* an array type */
		  char buff[40];
		  char *ptr;
		  int len;
		  char *sp;		/* pointer to " " in nelem */

		  sp = strcpy(buff,elems[j].nelem);
		  do {
		     ptr = sp;

		     if((sp = strchr(ptr,' ')) != NULL) {
			*sp++ = '\0';
			while(isspace(*sp) || *sp == '\"') sp++;
		     }
		     if(*ptr == '\"' || *ptr == ')') {
			ptr++;
		     } else if(strncmp(ptr,"STRING(",7) == 0) {
			ptr += strlen("STRING(");
		     }
		     len = strlen(ptr);
		     if(ptr[len - 1] == '\"'  || ptr[len - 1] == ')') {
			ptr[len - 1] = '\0';
		     }
					     
		     fprintf(fil,"*%s",ptr);
		  } while(sp != NULL);
	       }
	    }
	 }
#endif
      }
#if SCHEMA_ELEMS_HAS_TTYPE
      fprintf(fil,", UNKNOWN_SCHEMA");
#endif
      fprintf(fil," },\n");
   }
   fprintf(fil,"   { \"\", \"\", -1, 0, 0, NULL, 0 , -1");
#if SCHEMA_ELEMS_HAS_TTYPE
      fprintf(fil,", UNKNOWN_SCHEMA");
#endif
   fprintf(fil," },\n");
   fprintf(fil,"};\n");
}

/*****************************************************************************/
/*
 * Generate code to write the schema for a type. The array describing the
 * members of each type (schema_elem?) were written previously by
 * write_member_schema().
 *
 * Also generate code to read/write all known schema to/from a file
 */
static void
write_schema(FILE *fil, char *module_name)
{
   MEMBER elems;
   int s_e_num;				/* number of this schema_elem */
   int func_s_e_num = -1;		/* schema_elem# for FUNCTION */
   int i;
   int nprims;
   char *prims[] = {
      "TYPE", "CHAR", "UCHAR", "INT", "UINT", "SHORT", "USHORT", "LONG",
      "ULONG", "FLOAT", "DOUBLE",
      "LOGICAL", "STR", "PTR", "FUNCTION", "SCHAR"};
   char *type;

   nprims = sizeof(prims)/sizeof(prims[0]);
/*
 * Write schema_elems for derived types
 */
   for(i = 0;i < ntype;i++) {
      if((types[i].desires) != 0) {
	 write_member_schema(types[i].name,types[i].elems,types[i].nelem,
			     i,fil, types[i].type /* WP, 04/10/94 */);
      }
   }
/*
 * and now for the primitive types
 */
   for(i = 0;i < nprims;i++) {
      strcpy(elems.name,"");
      if(strcmp(prims[i],"STR") == 0) {
	 strcpy(elems.type,"char");
	 elems.nstar = 1;
      } else if(strcmp(prims[i],"PTR") == 0) {
	 strcpy(elems.type,"ptr");
	 elems.nstar = 1;
      } else if(strcmp(prims[i],"LOGICAL") == 0) {
         strcpy(elems.type,"logical");
         elems.nstar = 0;
      } else if( strcmp(prims[i],"UCHAR") == 0 ) {
         strcpy(elems.type,"unsigned char");
	 elems.nstar = 0;
      } else if( strcmp(prims[i], "SCHAR") == 0 )  {
         strcpy(elems.type, "signed char");
         elems.nstar = 0;
      } else if(strcmp(prims[i],"UINT") == 0) {
         strcpy(elems.type,"unsigned int");
	 elems.nstar = 0;
      } else if( strcmp(prims[i],"USHORT") == 0 ) {
         strcpy(elems.type,"unsigned short");
	 elems.nstar = 0;
      } else if( strcmp(prims[i],"ULONG") == 0) {
         strcpy(elems.type,"unsigned long");
	 elems.nstar = 0;
      } else if(strcmp(prims[i], "FUNCTION") == 0) {
         strcpy(elems.type, "function");
	 func_s_e_num = ntype + i;
         elems.nstar = 0;
      } else if(strcmp(prims[i], "TYPE") == 0) {
         strcpy(elems.type, "TYPE");
         elems.nstar = 0;
      } else {
	 strcpy(elems.type,lcase(prims[i]));
	 elems.nstar = 0;
      }

      elems.nelem = NULL;
      write_member_schema(prims[i], &elems, 1, ntype + i, fil, IS_STRUCT);
   }
/*
 * generate the code to define the schema for all desired types; this
 * is in terms of the already initialised schema_elems#
 *
 * Note that types that are actually IS_DEFINE are written out as ENUMs,
 * as that's how dervish treats them
 */
   fprintf(fil,"\n");
   fprintf(fil,"static SCHEMA schema[] = {\n");
   for(i = 0;i < ntype;i++) {
      s_e_num = i;
      switch(types[i].type) {
       case IS_DEFINE:
	 type = "ENUM"; break;
       case IS_FUNC:
	 if(func_s_e_num < 0) {
	    fprintf(stderr,"I don't see a schema_elem for FUNCTIONs\n");
	    exit(1);
	 }
	 s_e_num = func_s_e_num;
	 type = "PRIM"; break;
       case IS_ENUM:
	 type = "ENUM"; break;
       case IS_STRUCT:
	 type = "STRUCT"; break;
       default:
	 type = "UNKNOWN"; break;
      }
      
      fprintf(fil,"   { \"%s\", %s, sizeof(%s), %d, schema_elems%d,\n",
	      types[i].name, type,
	      (types[i].type == IS_DEFINE ? "int" : types[i].name),
	      types[i].nelem,s_e_num);
      
      fprintf(fil,"     (void *(*)(void))%s,\n",types[i].constructor);
      fprintf(fil,"     (void *(*)(void *))%s,\n",types[i].destructor);
      fprintf(fil,"     (RET_CODE(*)(FILE *,void *))%s,\n",types[i].reader);
      fprintf(fil,"     (RET_CODE(*)(FILE *,void *))%s,\n",types[i].writer);
      fprintf(fil,"     %d,\n",types[i].lock);

      fprintf(fil,"   },\n");
   }
/*
 * now for the primitive types
 */
   for(i = 0;i < nprims;i++) {
      if(strcmp(prims[i],"FUNCTION") == 0) {
	 sprintf(types[i].reader,"NULL");
	 sprintf(types[i].writer,"NULL");
      } else {
	 sprintf(types[i].reader,"shDump%sRead",canonical(prims[i]));
	 sprintf(types[i].writer,"shDump%sWrite",canonical(prims[i]));
      }
      fprintf(fil,"   { \"%s\", PRIM, ",prims[i]);
      if(strcmp(prims[i],"FUNCTION") == 0) {
	 fprintf(fil,"0, ");
      } else if(strcmp(prims[i],"PTR") == 0) {
	 fprintf(fil,"sizeof(void *), ");
      } else if(strcmp(prims[i],"STR") == 0) {
	 fprintf(fil,"sizeof(char *), ");
      } else if(strcmp(prims[i],"LOGICAL") == 0) {
         fprintf(fil,"sizeof(unsigned char), ");
      } else if(strcmp(prims[i],"UCHAR") == 0) {
	 fprintf(fil,"sizeof(unsigned char), ");
      } else if (strcmp(prims[i], "SCHAR") == 0)  {
         fprintf(fil, "sizeof(signed char), ");
      } else if(strcmp(prims[i],"UINT") == 0) {
	 fprintf(fil,"sizeof(unsigned int), ");
      } else if(strcmp(prims[i],"USHORT") == 0) {
	 fprintf(fil,"sizeof(unsigned short), ");
      } else if(strcmp(prims[i],"ULONG") == 0) {
	 fprintf(fil,"sizeof(unsigned long), ");
      } else if(strcmp(prims[i], "TYPE") == 0 ) {
         fprintf(fil, "sizeof(TYPE), ");
      } else {
	 fprintf(fil,"sizeof(%s), ",lcase(prims[i]));
      }

      fprintf(fil,"1, schema_elems%d,\n",ntype + i);
      fprintf(fil,"     NULL, NULL,\n");
      fprintf(fil,"     (RET_CODE(*)(FILE *,void *))%s,\n",types[i].reader);
      fprintf(fil,"     (RET_CODE(*)(FILE *,void *))%s,\n",types[i].writer);
      fprintf(fil,"     %d,\n",SCHEMA_LOCK | SCHEMA_LOCK_NOWARN); /* We expect those types to be redefined
								     for each product, let's ignore this
								     redefinitions */
      fprintf(fil,"   },\n");
   }
/*
 * and finally for UNKNOWN
 */
   fprintf(fil,"   { \"\", UNKNOWN, 0, 0,  NULL, NULL, NULL, NULL, NULL, SCHEMA_LOCK},\n");
   fprintf(fil,"};\n");
/*
 * write the function to load the schema from this file into the global
 * schema array
 */
   sprintf(funcname,"%sSchemaLoadFrom%s",module_prefix,module_name);

   fprintf(fil,"\n");
   fprintf(fil,"RET_CODE\n");
   fprintf(fil,"%s(void)\n",funcname);
   fprintf(fil,"{\n");
   fprintf(fil,"   return(p_shSchemaLoad(schema));\n");
   fprintf(fil,"}\n");
   fprintf(fil,"\n");
}

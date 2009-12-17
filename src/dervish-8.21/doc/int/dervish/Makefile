#=============================================================================
# 
# This is a template makefile for dervish_template.c, the Dervish template main
# program.  
#
# As a minimum, you must copy this makefile, premake.template, and 
# dervish_template.c to a directory of your choice and modify the USERxxxx
# variables as directed below.
#
# WARNING: This makefile must be invoked through the premake.template file,
#          as it relies on the way premake sets up the compilation environment
# 
#=============================================================================

#
# If you modified your copy of dervish_template.c and/or renamed it, modify
# this variable accordingly. If you are adding a small amount of c code,
# append the name of the source files here as well.
#
USERSOURCEFILE	= dervish_template.c privateTclVerbs.c

#
# Change the name of the executable if desired
#
USERMAINPGM	= dervish_template

#
# If you have any additional '#include' files, library directories, and
# libraries, add them to these variables.
#
USERINCDIRS	= -I.
USERLIBFLAGS	= -L.

# You should not have to edit below this line to merely re-build the 
# template main program. you will have to edit below this line if
# you are adding substantial C source to dervish.
#=============================================================================
# Environment:
# Environmental variables
#	$LIBFITS_DIR The Libfits package, libfits having been setup by ups
#	$FTCL_DIR    The FTCL Package, ftcl having ben setup by ups
#	$PGPLOT_DIR  The PGPLOT package pgplot having been setup by ups
# Command-line macro selecting switches to support the three compilation
# environments:
#	SGI C Compiler under IRIX
#	SUN ACC Compilers under SunOS
#	GNU C Compiler under SunOS
#
# Expected variables on the command line:
# CC=        The name of the appropriate compiler, with suitable flags
# CCCHK=     A set of compiler flags which check ANSI C strictly
# LLIBS=     A set of default libraries set by premake

INCDIRS	= $(USERINCDIRS) -I$(FTCL_DIR)/include -I$(DERVISH_DIR)/include
LDFLAGS	= $(USERLIBFLAGS) $(LIBSHV) $(LIBFITS) $(PGPLOTLIB) \
	  -L$(FTCL_DIR)/lib -ltcl $(LLIBS)
CFLAGS  = $(INCDIRS) $(CCCHK)
#
# Build everything
#
all:	$(USERMAINPGM)

#
# Build Dervish based main program
#
$(USERMAINPGM):	$(USERSOURCEFILE)
	@echo "=== Building main program : " $@ " ==="
	$(CC) -o $@ $(CFLAGS) $(CCCHK) $(USERSOURCEFILE) $(LDFLAGS)
	@echo "=== Main program : " $@ " built ==="
	@echo
#
# Now standard targets
#
install :;

clean :
	@(SDSSTOOLS_DIR)/bin/clean
	rm -f *.o $(USERMAINPGM)
#
make :
	@if [ "$(CCENV)" = "" ]; then ccenv=`uname`; else ccenv=$(CCENV); fi; \
	echo \
	"$(DERVISH_DIR)/bin/make_make -cc '$(CC) $(CFLAGS)' -nostd" \
					"-file Makefile -env $$ccenv *.c"; \
	$(DERVISH_DIR)/bin/make_make -cc '$(CC) $(CFLAGS)' -nostd \
					 -file Makefile -env $$ccenv *.c; \
#
# include file dependencies.
# All lines below START_DEPEND are machine generated; Do Not Edit
# 
#START_DEPEND

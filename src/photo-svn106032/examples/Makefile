SHELL = /bin/sh
#
#  WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING 
#  		!!!!!  THIS MAKEFILE DOESN'T WORK YET  !!!!!
#		USE MAKEFILE IN ../src TO MAKE PHOTO
#
#=============================================================================
# 
# This is a template makefile for photo_template.c, the PHOTO template main
# program.
#
# As a minimum, you must copy this makefile, and 
# photo_template.c to a directory of your choice and modify the USERxxxx
# variables as directed below.
#
#=============================================================================

#
# If you modified your copy of photo_template.c and/or renamed it, modify
# this variable accordingly. If you are adding a small amount of c code,
# append the name of the source files here as well.
#
USERSOURCEFILE	= photo_template.c

#
# Change the name of the executable if desired
#
USERMAINPGM	= photo

#
# If you have any additional '#include' files, library directories, and
# libraries, add them to these variables.
#
USERINCDIRS	= -I. -I../include
USERLIBFLAGS	= -L. ../lib/libphoto.a

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
CFLAGS  = $(SDSS_CFLAGS) $(INCDIRS) $(CCCHK)
#
# Build everything
#
all:
#
# Now standard targets
#
install :
	@if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi
	@rm -rf $(PHOTO_DIR)/examples
	@mkdir  $(PHOTO_DIR)/examples
	cp photo_template.c $(PHOTO_DIR)/examples;
	cp Makefile $(PHOTO_DIR)/examples
clean :
	rm -f *~ core *.bak *.orig *.old .#* #*# 
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

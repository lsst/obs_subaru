SHELL = /bin/sh
#
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
# If you renamed your copy of dervish_template.c modify
# this variable accordingly. If you are adding a small amount of c code,
# append the name of the object files here as well.
#
USEROBJECTFILE	= dervish_template.o privateTclVerbs.c

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
# template main program. You may have to edit below this line if
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
LDFLAGS	= $(USERLIBFLAGS) $(LIBSHV) $(LIBFITS) $(PGPLOTLIB) $(ELIBS) $(PGLIB) \
	  $(CONTRIBLIB) -L$(FLIBS) \
	  -L$(FTCL_DIR)/lib -ltcl -L$(TK_DIR)/lib -ltk $(LLIBS)
CFLAGS  = $(SDSS_CFLAGS) $(INCDIRS) $(CCCHK)
#
# Build everything
#
all:	$(USERMAINPGM)

#
# Build Dervish based main program
#
$(USERMAINPGM):	$(USEROBJECTFILE)
	@echo "=== Building main program : " $@ " ==="
	$(CC) -o $@ $(CFLAGS) $(CCCHK) $(USEROBJECTFILE) $(LDFLAGS)
	@echo "=== Main program : " $@ " built ==="
	@echo
#
# These targets are used if you want to invent your own data type,
# and be able to dump it to disk
#
# Here's a dervish executable that knows about the type FOO
#
FOO_OBJS = dervish_foo.o foo.o tclFoo.o diskio_gen.o
dervish_foo : $(FOO_OBJS)
	$(CC) -o $@ $(CFLAGS) $(CCCHK) $(FOO_OBJS) $(LDFLAGS)
#
# These are the include files used to generate schema and dump code
#
DISKIO_FILES =  $(DERVISH_DIR)/include/types.h \
		$(DERVISH_DIR)/include/lists.h \
		$(DERVISH_DIR)/include/region.h foo.h
#
diskio_gen.h : diskio_gen.c
diskio_gen.c : $(DISKIO_FILES)
	rm -f diskio_gen.[ch]
	$(DERVISH_DIR)/bin/make_io -v1 diskio_gen.c $(DISKIO_FILES)
	chmod 444 diskio_gen.[ch]
#
# List the state of a dump file
#
LIST_OBJS = dump_list.o diskio_gen.o foo.o
dump_list : $(LIST_OBJS)
	$(CC) -o $@ $(CFLAGS) $(CCCHK) $(LIST_OBJS) $(LDFLAGS)
#
# Now standard targets
#
install :
	if [ "$(DERVISH_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi
	@rm -rf $(DERVISH_DIR)/examples
	@mkdir $(DERVISH_DIR)/examples
	@cp -r * $(DERVISH_DIR)/examples

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
foo.o: foo.c 
foo.o: $(DERVISH_DIR)/include/shAssert.h 
foo.o: $(DERVISH_DIR)/include/utils.h 
foo.o: $(DERVISH_DIR)/include/dervish_msg_c.h 
foo.o: $(DERVISH_DIR)/include/types.h 
foo.o: $(DERVISH_DIR)/include/lists.h 
foo.o: foo.h 
hdrTemplate.o: hdrTemplate.c 
hdrTemplate.o: ccc.h 
dervish_foo.o: dervish_foo.c 
dervish_foo.o: $(DERVISH_DIR)/include/dervish.h 
dervish_foo.o: $(FTCL_DIR)/include/tclExtend.h 
dervish_foo.o: $(FTCL_DIR)/include/tcl.h 
dervish_foo.o: $(FTCL_DIR)/include/ftcl.h 
dervish_foo.o: $(DERVISH_DIR)/include/dervish_msg_c.h 
dervish_foo.o: $(DERVISH_DIR)/include/region.h 
dervish_foo.o: $(DERVISH_DIR)/include/shC.h 
dervish_foo.o: $(DERVISH_DIR)/include/utils.h 
dervish_foo.o: $(DERVISH_DIR)/include/shCHdr.h 
dervish_foo.o: $(DERVISH_DIR)/include/shCFitsIo.h 
dervish_foo.o: $(DERVISH_DIR)/include/errStack.h 
dervish_foo.o: $(DERVISH_DIR)/include/shCRegUtils.h 
dervish_foo.o: $(DERVISH_DIR)/include/shAssert.h 
dervish_foo.o: $(DERVISH_DIR)/include/shEnvscan.h 
dervish_foo.o: $(DERVISH_DIR)/include/shTclRegSupport.h 
dervish_foo.o: $(DERVISH_DIR)/include/shTclVerbs.h 
dervish_foo.o: $(DERVISH_DIR)/include/imtool.h 
dervish_foo.o: $(DERVISH_DIR)/include/shSao.h 
dervish_template.o: dervish_template.c 
dervish_template.o: $(DERVISH_DIR)/include/dervish.h 
dervish_template.o: $(FTCL_DIR)/include/tclExtend.h 
dervish_template.o: $(FTCL_DIR)/include/tcl.h 
dervish_template.o: $(FTCL_DIR)/include/ftcl.h 
dervish_template.o: $(DERVISH_DIR)/include/dervish_msg_c.h 
dervish_template.o: $(DERVISH_DIR)/include/region.h 
dervish_template.o: $(DERVISH_DIR)/include/shC.h 
dervish_template.o: $(DERVISH_DIR)/include/utils.h 
dervish_template.o: $(DERVISH_DIR)/include/shCHdr.h 
dervish_template.o: $(DERVISH_DIR)/include/shCFitsIo.h 
dervish_template.o: $(DERVISH_DIR)/include/errStack.h 
dervish_template.o: $(DERVISH_DIR)/include/shCRegUtils.h 
dervish_template.o: $(DERVISH_DIR)/include/shAssert.h 
dervish_template.o: $(DERVISH_DIR)/include/shEnvscan.h 
dervish_template.o: $(DERVISH_DIR)/include/shTclRegSupport.h 
dervish_template.o: $(DERVISH_DIR)/include/shTclVerbs.h 
dervish_template.o: $(DERVISH_DIR)/include/imtool.h 
dervish_template.o: $(DERVISH_DIR)/include/shSao.h 
tclFoo.o: tclFoo.c 
tclFoo.o: $(FTCL_DIR)/include/tcl.h 
tclFoo.o: $(DERVISH_DIR)/include/utils.h 
tclFoo.o: $(DERVISH_DIR)/include/dervish_msg_c.h 
tclFoo.o: foo.h 
tclFoo.o: $(DERVISH_DIR)/include/lists.h 
tclFoo.o: $(DERVISH_DIR)/include/types.h 
tclFoo.o: $(DERVISH_DIR)/include/errStack.h 
tclFoo.o: $(DERVISH_DIR)/include/shTclHandle.h 
tclFoo.o: $(DERVISH_DIR)/include/shTclRegSupport.h 
tclFoo.o: $(FTCL_DIR)/include/ftcl.h 
tclFoo.o: $(DERVISH_DIR)/include/region.h 

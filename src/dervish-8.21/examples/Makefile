SHELL = /bin/sh
#
#=============================================================================
# 
# This is a template makefile for dervish_template.c, the Dervish template main
# program.  
#
# As a minimum, you must copy this makefile and dervish_template.c to a
# directory of your choice and modify the USERxxxx variables as directed below.
#
# WARNING: This makefile must be invoked through sdssmake as it relies on the 
#	   way sdssmake sets up the compilation environment
# 
#=============================================================================

# handle FORTRAN with embedded cpp.
.SUFFIXES:  .o .F 
.F.o : 
	$(F77) -c  $(FFLAGS) $*.F

#
# If you renamed your copy of dervish_template.c modify
# this variable accordingly. If you are adding a small amount of c code,
# append the name of the object files here as well.
#
USEROBJECTFILE	= dervish_template.o

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
#
#=============================================================================
#
INCDIRS	= $(USERINCDIRS) -I$(DERVISH_DIR)/include -I$(DERVISH_DIR)/contrib \
	   -I$(FTCL_DIR)/include -I$(FTCL_DIR)/src \
	   -I$(LIBFITS_DIR)/src -I$(FPGPLOT_DIR)/include -I$(PGPLOT_DIR)/dst \
	   -I$(TK_DIR)/include -I$(TK_DIR)/src \
	   -I$(TCLX_DIR)/src/tksrc -I$(TCLX_DIR)/include -I$(TCLX_DIR)/src/src \
	   -I$(TCL_DIR)/include -I$(TCL_DIR)/src

LDFLAGS	= $(USERLIBFLAGS) $(LIBSHV) $(LIBFITS) $(PGPLOTLIB) \
	  $(CONTRIBLIB) $(LIBFTCL) $(LLIBS) $(PGLIB) \
	  $(FLIBS) $(ELIBS)

CFLAGS  = $(SDSS_CFLAGS) $(INCDIRS)
CFLAGS_DEFGP = $(SDSS_CFLAGS_DEFGP) $(INCDIRS)
# NOTE: CFLAGS_DEFGP is provided because some IRIX systems do not handle -G 0
#       well when loading (ld) images.  The 'DEFGP' stands for default global
#       pointer processing (that is, no -G n option).

#
# Build everything
#
all:	$(USERMAINPGM)

#
# Build Dervish based main program
#
$(USERMAINPGM):	$(USEROBJECTFILE)
	@echo "=== Building main program : " $@ " ==="
	$(CC) -o $@ $(CFLAGS_DEFGP) $(CCCHK) $(USEROBJECTFILE) $(LDFLAGS)
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
DISKIO_FILES = foo.h
#
diskio_gen.c : $(DISKIO_FILES)
	rm -f diskio_gen.c
	$(DERVISH_DIR)/bin/make_io -v1 -m test -p ts diskio_gen.c $(DISKIO_FILES)
	chmod 444 diskio_gen.c

#
# List the state of a dump file
#
LIST_OBJS = dump_list.o diskio_gen.o foo.o
dump_list : $(LIST_OBJS)
	$(CC) -o $@ $(CFLAGS) $(CCCHK) $(LIST_OBJS) $(LDFLAGS)

#
# Example f77 package 
#
EXAMPLE_OBJS = operate.o operateCBind.o  operateTest.o 
f77_example : $(EXAMPLE_OBJS)
	$(CC) -o $@ $(CFLAGS) $(CCCHK) $(EXAMPLE_OBJS) $(LDFLAGS) \
	$(DERVISH_DIR)/lib/dummyF77Main.o 

#
# Now standard targets
#
install :
	@if [ "$(DERVISH_INSTALL_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi
	@rm -rf   $(DERVISH_INSTALL_DIR)/examples
	@mkdir    $(DERVISH_INSTALL_DIR)/examples
	@cp -pr * $(DERVISH_INSTALL_DIR)/examples

clean :
	@ $(SDSSTOOLS_DIR)/bin/clean
	rm -f *.o $(USERMAINPGM) f77_example
#
make :
	@if [ "$(CCENV)" = "" ]; then ccenv=`uname`; else ccenv=$(CCENV); fi; \
	echo \
	"$(SDSSTOOLS_DIR)/bin/make_make -cc '$(CC) $(CFLAGS)' -nostd" \
					"-file Makefile -env $$ccenv *.c"; \
	$(SDSSTOOLS_DIR)/bin/make_make -cc '$(CC) $(CFLAGS)' -nostd \
					 -file Makefile -env $$ccenv *.c; \
#
# include file dependencies.
# All lines below START_DEPEND are machine generated; Do Not Edit
# 
#START_DEPEND
foo.o:	foo.c
foo.o:	$(DERVISH_DIR)/include/shCAssert.h
foo.o:	$(DERVISH_DIR)/include/shCUtils.h
foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
foo.o:	$(DERVISH_DIR)/include/shCSchema.h
foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
foo.o:	$(DERVISH_DIR)/include/shCUtils.h
foo.o:	$(DERVISH_DIR)/include/shCSchema.h
foo.o:	$(DERVISH_DIR)/include/shCList.h
foo.o:	$(DERVISH_DIR)/include/shCSchema.h
foo.o:	$(DERVISH_DIR)/include/shCDiskio.h
foo.o:	$(DERVISH_DIR)/include/shCSchema.h
foo.o:	$(DERVISH_DIR)/include/shCList.h
foo.o:	$(DERVISH_DIR)/include/region.h
foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
foo.o:	$(DERVISH_DIR)/include/shCAlign.h
foo.o:	$(DERVISH_DIR)/include/shCSchema.h
foo.o:	$(DERVISH_DIR)/include/shCHdr.h
foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
foo.o:	$(DERVISH_DIR)/include/shChain.h
foo.o:	$(DERVISH_DIR)/include/shCArray.h
foo.o:	$(DERVISH_DIR)/include/shCSchema.h
foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
foo.o:	./foo.h
foo.o:	$(DERVISH_DIR)/include/shCList.h
hdrTemplate.o:	hdrTemplate.c
operateCBind.o:	operateCBind.c
operateCBind.o:	./operate.h
operateTest.o:	operateTest.c
operateTest.o:	./operate.h
dervish_foo.o:	dervish_foo.c
dervish_foo.o:	$(DERVISH_DIR)/include/dervish.h
dervish_foo.o:	$(TCLX_DIR)/include/tclExtend.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(FTCL_DIR)/include/ftcl.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(FTCL_DIR)/include/ftclParseArgv.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCAlign.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCHdr.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shC.h
dervish_foo.o:	$(DERVISH_DIR)/include/shChain.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCArray.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCUtils.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCErrStack.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclErrStack.h
dervish_foo.o:	$(FTCL_DIR)/include/ftcl.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCArray.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchemaCnv.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_foo.o:	$(FTCL_DIR)/include/ftcl.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCList.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shChain.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_foo.o:	$(LIBFITS_DIR)/src/libfits.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCAlign.h
dervish_foo.o:	$(DERVISH_DIR)/include/shChain.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCArray.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCHdr.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCUtils.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCHdr.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCFits.h
dervish_foo.o:	$(LIBFITS_DIR)/src/libfits.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCMaskUtils.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCRegUtils.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCRegPrint.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCHash.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCAssert.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCUtils.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCEnvscan.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
dervish_foo.o:	$(FTCL_DIR)/include/ftcl.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSao.h
dervish_foo.o:	$(DERVISH_DIR)/include/imtool.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclParseArgv.h
dervish_foo.o:	$(FTCL_DIR)/include/ftcl.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclVerbs.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclSao.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(FTCL_DIR)/include/ftclCmdLine.h
dervish_foo.o:	$(DERVISH_DIR)/include/imtool.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSao.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclUtils.h
dervish_foo.o:	$(FTCL_DIR)/include/ftcl.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTk.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCGarbage.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCUtils.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCHg.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCErrStack.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCRegUtils.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCAssert.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclVerbs.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCLink.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCList.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCVecExpr.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCLut.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCVecExpr.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCVecChain.h
dervish_foo.o:	$(DERVISH_DIR)/include/shChain.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCVecExpr.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCHg.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/imtool.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCAlign.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCDemo.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCDiskio.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCList.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shChain.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCHist.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCLink.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCList.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCMask.h
dervish_foo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCMemory.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCRegPhysical.h
dervish_foo.o:	$(DERVISH_DIR)/include/region.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclAlign.h
dervish_foo.o:	$(FTCL_DIR)/include/ftcl.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCAlign.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclArray.h
dervish_foo.o:	$(FTCL_DIR)/include/ftcl.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCArray.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclFits.h
dervish_foo.o:	$(FTCL_DIR)/include/ftcl.h
dervish_foo.o:	$(TCL_DIR)/include/tcl.h
dervish_foo.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclHdr.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclSao.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclSupport.h
dervish_foo.o:	$(DERVISH_DIR)/include/shTclTree.h
dervish_template.o:	dervish_template.c
dervish_template.o:	$(DERVISH_DIR)/include/dervish.h
dervish_template.o:	$(TCLX_DIR)/include/tclExtend.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(FTCL_DIR)/include/ftcl.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(FTCL_DIR)/include/ftclParseArgv.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCAlign.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCHdr.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shC.h
dervish_template.o:	$(DERVISH_DIR)/include/shChain.h
dervish_template.o:	$(DERVISH_DIR)/include/shCArray.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCUtils.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shCErrStack.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclErrStack.h
dervish_template.o:	$(FTCL_DIR)/include/ftcl.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(DERVISH_DIR)/include/shCArray.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchemaCnv.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_template.o:	$(FTCL_DIR)/include/ftcl.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shCList.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shChain.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_template.o:	$(LIBFITS_DIR)/src/libfits.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shCAlign.h
dervish_template.o:	$(DERVISH_DIR)/include/shChain.h
dervish_template.o:	$(DERVISH_DIR)/include/shCArray.h
dervish_template.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shCHdr.h
dervish_template.o:	$(DERVISH_DIR)/include/shCUtils.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
dervish_template.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchemaTrans.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchemaSupport.h
dervish_template.o:	$(DERVISH_DIR)/include/shCHdr.h
dervish_template.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_template.o:	$(DERVISH_DIR)/include/shCFits.h
dervish_template.o:	$(LIBFITS_DIR)/src/libfits.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_template.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_template.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_template.o:	$(DERVISH_DIR)/include/shCTbl.h
dervish_template.o:	$(DERVISH_DIR)/include/shCMaskUtils.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shCRegUtils.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shCRegPrint.h
dervish_template.o:	$(DERVISH_DIR)/include/shCHash.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCAssert.h
dervish_template.o:	$(DERVISH_DIR)/include/shCUtils.h
dervish_template.o:	$(DERVISH_DIR)/include/shCEnvscan.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
dervish_template.o:	$(FTCL_DIR)/include/ftcl.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSao.h
dervish_template.o:	$(DERVISH_DIR)/include/imtool.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclParseArgv.h
dervish_template.o:	$(FTCL_DIR)/include/ftcl.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclVerbs.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclSao.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(FTCL_DIR)/include/ftclCmdLine.h
dervish_template.o:	$(DERVISH_DIR)/include/imtool.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSao.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclUtils.h
dervish_template.o:	$(FTCL_DIR)/include/ftcl.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(DERVISH_DIR)/include/shTk.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_template.o:	$(DERVISH_DIR)/include/shCGarbage.h
dervish_template.o:	$(DERVISH_DIR)/include/shCUtils.h
dervish_template.o:	$(DERVISH_DIR)/include/shCHg.h
dervish_template.o:	$(DERVISH_DIR)/include/shCErrStack.h
dervish_template.o:	$(DERVISH_DIR)/include/shCRegUtils.h
dervish_template.o:	$(DERVISH_DIR)/include/shCAssert.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclVerbs.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclHandle.h
dervish_template.o:	$(DERVISH_DIR)/include/shCLink.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCList.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shCVecExpr.h
dervish_template.o:	$(DERVISH_DIR)/include/shCLut.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
dervish_template.o:	$(DERVISH_DIR)/include/shCVecExpr.h
dervish_template.o:	$(DERVISH_DIR)/include/shCVecChain.h
dervish_template.o:	$(DERVISH_DIR)/include/shChain.h
dervish_template.o:	$(DERVISH_DIR)/include/shCVecExpr.h
dervish_template.o:	$(DERVISH_DIR)/include/shCHg.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/imtool.h
dervish_template.o:	$(DERVISH_DIR)/include/shCAlign.h
dervish_template.o:	$(DERVISH_DIR)/include/shCDemo.h
dervish_template.o:	$(DERVISH_DIR)/include/shCDiskio.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shCList.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shChain.h
dervish_template.o:	$(DERVISH_DIR)/include/shCHist.h
dervish_template.o:	$(DERVISH_DIR)/include/shCLink.h
dervish_template.o:	$(DERVISH_DIR)/include/shCList.h
dervish_template.o:	$(DERVISH_DIR)/include/shCMask.h
dervish_template.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
dervish_template.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shCMemory.h
dervish_template.o:	$(DERVISH_DIR)/include/shCRegPhysical.h
dervish_template.o:	$(DERVISH_DIR)/include/region.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclAlign.h
dervish_template.o:	$(FTCL_DIR)/include/ftcl.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(DERVISH_DIR)/include/shCSchema.h
dervish_template.o:	$(DERVISH_DIR)/include/shCAlign.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclArray.h
dervish_template.o:	$(FTCL_DIR)/include/ftcl.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(DERVISH_DIR)/include/shCArray.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclFits.h
dervish_template.o:	$(FTCL_DIR)/include/ftcl.h
dervish_template.o:	$(TCL_DIR)/include/tcl.h
dervish_template.o:	$(DERVISH_DIR)/include/shCFitsIo.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclHdr.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclSao.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclSupport.h
dervish_template.o:	$(DERVISH_DIR)/include/shTclTree.h
tclFoo.o:	tclFoo.c
tclFoo.o:	$(TCL_DIR)/include/tcl.h
tclFoo.o:	$(DERVISH_DIR)/include/shCUtils.h
tclFoo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tclFoo.o:	$(DERVISH_DIR)/include/shCSchema.h
tclFoo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tclFoo.o:	./foo.h
tclFoo.o:	$(DERVISH_DIR)/include/shCList.h
tclFoo.o:	$(DERVISH_DIR)/include/shCSchema.h
tclFoo.o:	$(DERVISH_DIR)/include/shCErrStack.h
tclFoo.o:	$(DERVISH_DIR)/include/shTclErrStack.h
tclFoo.o:	$(FTCL_DIR)/include/ftcl.h
tclFoo.o:	$(TCL_DIR)/include/tcl.h
tclFoo.o:	$(FTCL_DIR)/include/ftclParseArgv.h
tclFoo.o:	$(DERVISH_DIR)/include/shTclHandle.h
tclFoo.o:	$(FTCL_DIR)/include/ftcl.h
tclFoo.o:	$(TCL_DIR)/include/tcl.h
tclFoo.o:	$(DERVISH_DIR)/include/shCSchema.h
tclFoo.o:	$(DERVISH_DIR)/include/shCList.h
tclFoo.o:	$(DERVISH_DIR)/include/shChain.h
tclFoo.o:	$(DERVISH_DIR)/include/shCArray.h
tclFoo.o:	$(DERVISH_DIR)/include/shCSchema.h
tclFoo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tclFoo.o:	$(DERVISH_DIR)/include/shTclParseArgv.h
tclFoo.o:	$(FTCL_DIR)/include/ftcl.h
tclFoo.o:	$(TCL_DIR)/include/tcl.h
tclFoo.o:	$(DERVISH_DIR)/include/shTclRegSupport.h
tclFoo.o:	$(FTCL_DIR)/include/ftcl.h
tclFoo.o:	$(TCL_DIR)/include/tcl.h
tclFoo.o:	$(DERVISH_DIR)/include/region.h
tclFoo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h
tclFoo.o:	$(DERVISH_DIR)/include/shCAlign.h
tclFoo.o:	$(DERVISH_DIR)/include/shCSchema.h
tclFoo.o:	$(DERVISH_DIR)/include/shCHdr.h
tclFoo.o:	$(DERVISH_DIR)/include/dervish_msg_c.h

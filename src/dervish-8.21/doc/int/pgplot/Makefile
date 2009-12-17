#=============================================================================
#
# DEFINITION:
#	This makefile builds the simple_c program. It should only be invoked
#	through premake.
#
# PRE-REQUISITES:
#	- The following environmental variables must be set (via sdssmake):
#		PGPLOTLIB, PGLIB	Link libraries
#
#	- The following products must be setup:
#		pgplot		(Unix only)
#
#=============================================================================

# Use 'sh' so that 'if' statement can be used
SHELL   = /bin/sh
LDFLAGS	= $(PGPLOTLIB) $(PGLIB)
#

#=============================================================================
# TARGETS
#=============================================================================

simple_c: simple_c.o
	cc -o simple_c simple_c.o $(LDFLAGS) $(LLIBS)

simple_f: simple_f.o
	f77 -o simple_f simple_f.o -L$(PGPLOT_DIR)/lib -lpgplot -lX11 

clean:
	@(SDSSTOOLS_DIR)/bin/clean
	rm -f pgplot.ps simple_f simple_c


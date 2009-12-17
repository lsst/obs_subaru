#++
# FACILITY:
#
# ABSTRACT:	Overall make file for building a product.  Each subdirectory of
#		the product's root directory is built.
#
# ENVIRONMENT:	make file.  Bourne shell.
#		Makefile
#--
#*******************************************************************************
#
# Define the environment.
#

SHELL		= /bin/sh

#
# Define some macros:
#
#   PROD	The product name used only for cosmetics (so case it properly).
#   PRODDIRNAME	The name of the root directory for the product (and the version
#		in use).
#   PRODDIR	The value of the environment value name $(PRODDIRNAME).
#

PROD		=   DERVISH
PRODDIRNAME	=   DERVISH_INSTALL_DIR
PRODDIR		= $(DERVISH_INSTALL_DIR)

#*******************************************************************************

.DEFAULT:
	@ echo ''
	@ echo '*** Finished updating target $@ ***'
	@ echo ''

#*******************************************************************************
#
# Build all targets in all subdirectories.
#
#   o	Check whether we're building on the same platform.
#

all :
	@ if [ ! -f ".uname~" ]; then \
		uname > .uname~; \
	else \
		if [ `uname` != `cat .uname~` ]; then \
			echo ""; \
			echo "This copy of $(PROD) was built for the `cat .uname~` platform.";\
			echo "Execute \"make clean\" first if you want to build on a `uname` platform."; \
			echo ""; \
			exit 1; \
		fi; \
	fi
	@ echo ""
	@ echo "Updating $(PROD) libraries and subdirectories."
	@ (cd src; echo ""; echo "make make_io in ./src"; echo ""; $(MAKE) $(MFLAGS) make_io);
	@ for subdir in contrib src lib test doc; do \
		if [ -d $$subdir ]; then \
			(cd $$subdir; echo ""; echo "make all in ./$$subdir"; echo ""; $(MAKE) $(MFLAGS) all); \
		else \
			echo ""; \
			echo "Subdirectory does not exist: ./$$subdir"; \
			echo ""; \
		fi; \
	done

dervish :
	@ (cd src; $(MAKE) $(MFLAGS) make_io);
	@ for subdir in contrib src lib; do \
		(cd $$subdir; $(MAKE) $(MFLAGS) all); \
	done
	@ (cd src; $(MAKE) $(MFLAGS) ../bin/dervish)

tests :
	@ (cd test; $(MAKE) $(MFLAGS) all)

tags :
	@ /bin/rm -f TAGS
	etags -a -t -o TAGS src/*.c include/*.h
	$$SDSSTOOLS_DIR/bin/tcl_tags -a -f TAGS etc/*.tcl src/tcl*.c

#*******************************************************************************
#
# Create Makefiles in each subdirectory.
#
#

make :
	@ echo ""
	@ echo "Creating Makefiles in subdirectories."
	@ for subdir in contrib src examples test doc; do \
		if [ -d $$subdir ]; then \
			(cd $$subdir; echo ""; echo "make make in ./$$subdir"; echo ""; $(MAKE) $(MFLAGS) make); \
		else \
			echo ""; \
			echo "Subdirectory does not exist: ./$$subdir"; \
			echo ""; \
		fi; \
	done



#*******************************************************************************
#
# Install things from the current directories to their proper places in
# $(PRODDIR).
#
#   o	A "decision" period is provided ("I'll give you n seconds ...").  The
#	length of the echo string (all spaces) is the maximum time that can be
#	waited, since the following sed does replacements on that input string.
#
# NOTE:	contrib   M U S T   last, as other make install delete directories.
#       ups MUST be installed before the doc directory as the doc directory
#       tries to copy things to the toman directory.
#

install :
	@ if $(CVSCHECK); then \
		echo > /dev/null ; \
	else \
	 	echo "This product did not passed the cvs check."; \
		echo "Please correct the above error before installing."; \
		exit 1; \
	fi
	@ echo ""
	@ echo "Make sure the current $(PROD) directories under"
	@ echo ""
	@ echo "     `pwd`"
	@ echo ""
	@ echo "have the latest versions of the files.  These will be copied to the"
	@ echo "the destination during the install of $(PROD)."
	@ echo ""
	@ if [ "$(PRODDIR)" = "" ]; then \
		echo "The destination directory has not been specified.  Set the environment"	>&2; \
		echo "variable $(PRODDIRNAME)"							>&2; \
		echo ""; \
		exit 1; \
	fi
	@ if [ `(cd $(PRODDIR); pwd)` = `pwd` ]; then \
		echo "The destination directory is the same as the current directory."		>&2; \
		echo "The install will be aborted."						>&2; \
		echo ""; \
		exit 1; \
	fi
	@ echo "You will be installing in"
	@ echo ""
	@ echo "   $(PRODDIRNAME) = $(PRODDIR)"
	@ echo ""
	@ echo "I'll give you 5 seconds to think about it (control-C to abort) ..."
	@ for pos in          5 4 3 2 1; do \
	   echo "                              " | sed -e 's/ /'$$pos'/'$$pos; \
	   sleep 1; \
	done
	@ for subdir in ups doc etc examples include lib src test contrib bin; do \
		if [ -d $$subdir ]; then \
			if [ ! -r $(PRODDIR)/$$subdir ]; then \
				mkdir $(PRODDIR)/$$subdir; \
			fi; \
			(cd $$subdir; echo ""; echo "make install in ./$$subdir"; echo ""; $(MAKE) $(MFLAGS) install); \
		else \
			echo "Subdirectory does not exist: ./$$subdir"; \
		fi; \
	done
	@cp -p Makefile $(DERVISH_INSTALL_DIR)
#
# The examples directory cannot install itself because it is meant to
# be copied out by the user. So, we install from the root directory.
#
	rm -rf $(DERVISH_INSTALL_DIR)/examples
	mkdir  $(DERVISH_INSTALL_DIR)/examples
	cp -p examples/Makefile 	$(DERVISH_INSTALL_DIR)/examples
	cp -p examples/*emplate* 	$(DERVISH_INSTALL_DIR)/examples
	cp -p examples/*foo.*      $(DERVISH_INSTALL_DIR)/examples
	cp -p examples/*Foo.*      $(DERVISH_INSTALL_DIR)/examples

#*******************************************************************************
#
# Clean up.
#
#   o	.uname~ is removed last.  This protects against future improper builds
#	(wrong platform) in case the clean fails for some reason.
#

clean :
	@ echo ""
	@ echo "Cleaning in . = `pwd`"
	@ echo ""
	@ $(SDSSTOOLS_DIR)/bin/clean
	@ for subdir in bin contrib demos doc etc examples include lib src test ups; do \
		if [ -d $$subdir ]; then \
			(cd $$subdir; echo ""; echo "Cleaning in ./$$subdir"; echo ""; $(MAKE) $(MFLAGS) clean); \
		else \
			echo "Subdirectory does not exist: ./$$subdir"; \
		fi; \
	done
	- rm -f .uname~

#*******************************************************************************

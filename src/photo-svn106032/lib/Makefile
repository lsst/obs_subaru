SHELL = /bin/sh
#
BIN	= ../bin

default :
	echo "Please invoke this makefile using sdssmake"

all : cellprof.dat rgalprof.dat
	@ :;

cellprof.dat rgalprof.dat:
	@unset PHOTO_USER; export PHOTO_USER; \
	unset DERVISH_USER; export DERVISH_USER; \
	$(BIN)/photo -file make_profdata.tcl

install : cellprof.dat rgalprof.dat
	@if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi 
	@rm -rf $(PHOTO_DIR)/lib
	@mkdir  $(PHOTO_DIR)/lib
	cp Makefile $(PHOTO_DIR)/lib
	cp *.a $(PHOTO_DIR)/lib
	-@if [ "$(RANLIB)" != "" -a -x "$(RANLIB)" ]; then \
		for f in $(PHOTO_DIR)/lib/lib*.a; do \
		   $(RANLIB) $$f; \
		done; \
	fi
	@chmod 444 $(PHOTO_DIR)/lib/*.a
	cp make_profdata.tcl cellprof.dat rgalprof.dat $(PHOTO_DIR)/lib
#
clean :
	rm -f *.a *~ core *.bak *.orig *.old *.\# cellprof.dat rgalprof.dat


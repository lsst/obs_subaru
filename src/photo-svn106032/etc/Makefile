SHELL = /bin/sh
#
default :
	echo "Please invoke this makefile using sdssmake"
all:
	;
install :
	@if [ "$(PHOTO_DIR)" = "" ]; then \
		echo You have not specified a destination directory >&2; \
		exit 1; \
	fi 
	@rm -rf $(PHOTO_DIR)/etc
	@mkdir  $(PHOTO_DIR)/etc
	cp *.tcl $(PHOTO_DIR)/etc
	cp *.par $(PHOTO_DIR)/etc
	cp Makefile $(PHOTO_DIR)/etc
tags :
	etags ../src/*.c ../src/*/*.c ../include/*.h > TAGS
	$(PHOTO_DIR)/bin/tcl_tags -a -f TAGS *.tcl ../src/tcl*.c
clean :
	rm -f *~ core *.bak *.orig *.old .#* #*#

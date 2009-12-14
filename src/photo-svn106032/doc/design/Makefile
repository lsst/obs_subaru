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
	@rm -rf $(PHOTO_DIR)/doc/design
	@mkdir  $(PHOTO_DIR)/doc/design
	cp levelzero.ps $(PHOTO_DIR)/doc/design
	cp Makefile $(PHOTO_DIR)/doc/design
clean :
	rm -f *~ core *.bak *.orig *.old .#* #*#


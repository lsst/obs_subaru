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
	@rm -rf $(PHOTO_DIR)/ups
	@mkdir  $(PHOTO_DIR)/ups
	cp Makefile $(PHOTO_DIR)/ups
	$(SDSSTOOLS_DIR)/bin/install_ups_table photo.table $(PHOTO_DIR)/ups
clean :
	rm -f *~ core *.bak *.orig *.old .#* #*#

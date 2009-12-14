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
	@rm -rf $(PHOTO_DIR)/bin
	@mkdir  $(PHOTO_DIR)/bin
	cp photo $(PHOTO_DIR)/bin
	cp Makefile $(PHOTO_DIR)/bin
	cp *.sh $(PHOTO_DIR)/bin
	cp *.cron $(PHOTO_DIR)/bin
clean :
	rm -f find_iso_bias make_interp make_photom_coeffs photo read_atlas
	rm -f *~ core *.bak *.orig *.old .#* #*#
